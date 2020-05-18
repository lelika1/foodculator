#include "db.h"

#include <sqlite3.h>

#include <iostream>
#include <sstream>
#include <string_view>

namespace foodculator {

std::unique_ptr<DB> DB::Create(std::string_view path) {
    sqlite3* db = nullptr;

    if (path != ":memory:") {
        if (int st = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
            st != SQLITE_OK) {
            std::cerr << "SQL config error: " << st << std::endl;
        }
    }

    if (sqlite3_open(path.data(), &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << path << std::endl;
        return nullptr;
    }

    const char sql[] =
        R"*(
        CREATE TABLE IF NOT EXISTS INGREDIENTS(
            ID           INTEGER   PRIMARY KEY   AUTOINCREMENT NOT NULL,
            NAME         TEXT                                  NOT NULL,
            KCAL         INTEGER   DEFAULT 0                   NOT NULL,
            UNIQUE (NAME, KCAL)
        );
        CREATE TABLE IF NOT EXISTS TABLEWARE(
            ID           INTEGER   PRIMARY KEY   AUTOINCREMENT NOT NULL,
            NAME         TEXT                                  NOT NULL,
            WEIGHT       INTEGER                               NOT NULL,
            UNIQUE (NAME, WEIGHT)
        );
    )*";

    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, 0, &err) != SQLITE_OK) {
        std::cerr << "SQL error: " << err << std::endl;
        sqlite3_free(err);
        sqlite3_close(db);
        return nullptr;
    }

    return std::unique_ptr<DB>(new DB(db));
}

DB::~DB() {
    if (db_) {
        sqlite3_close(db_);
    }
}

DB::Result DB::AddProduct(std::string name, uint32_t kcal) {
    std::vector<BindParameter> params = {BindParameter(std::move(name)),
                                         BindParameter(kcal)};
    return Insert("INGREDIENTS", {"NAME", "KCAL"}, "ID", params);
}

DB::Result DB::AddTableware(std::string name, uint32_t weight) {
    std::vector<BindParameter> params = {BindParameter(std::move(name)),
                                         BindParameter(weight)};

    return Insert("TABLEWARE", {"NAME", "WEIGHT"}, "ID", params);
}

DB::Result DB::Insert(std::string_view table,
                      const std::vector<std::string_view>& fields,
                      std::string_view id_field,
                      const std::vector<BindParameter>& params) {
    if (fields.size() != params.size()) {
        std::cerr << "fields.size() != params.size(): " << fields.size() << " "
                  << params.size() << std::endl;
        exit(1);
    }

    std::stringstream names;
    std::stringstream binds;
    std::stringstream conds;
    for (size_t idx = 0; idx < fields.size(); idx++) {
        if (idx > 0) {
            names << ", ";
            binds << ", ";
            conds << " AND ";
        }
        names << fields[idx];
        binds << "?";
        conds << fields[idx] << " = ?";
    }
    std::stringstream insert;
    insert << "INSERT INTO " << table << "(" << names.str() << ") VALUES ("
           << binds.str() << ");";

    switch (Exec(insert.str(), params).status) {
        case SQLITE_OK:
            break;
        case SQLITE_CONSTRAINT:
            return Result{Result::DUPLICATE};
        default:
            return Result{Result::ERROR};
    }

    std::stringstream select_id;
    select_id << "SELECT " << id_field << " FROM " << table << " WHERE "
              << conds.str() << ";";
    const auto& res = Exec(select_id.str(), params);
    if ((res.rows.size() != 1) || (res.rows[0].size() != 1)) {
        return {.code = Result::ERROR};
    }

    return {.code = Result::OK, .id = std::stoull(res.rows[0][0])};
}

std::vector<Ingredient> DB::GetProducts() {
    std::vector<Ingredient> ret;
    const auto& res = Exec("SELECT NAME, KCAL, ID from INGREDIENTS", {});
    for (auto& row : res.rows) {
        ret.emplace_back(std::move(row[0]), std::stoul(row[1]),
                         std::stoull(row[2]));
    }
    return ret;
}

std::vector<Tableware> DB::GetTableware() {
    std::vector<Tableware> ret;
    const auto& res = Exec("SELECT NAME, WEIGHT, ID from TABLEWARE", {});
    for (auto& row : res.rows) {
        ret.emplace_back(std::move(row[0]), std::stoul(row[1]),
                         std::stoull(row[2]));
    }
    return ret;
}

bool DB::DeleteProduct(size_t id) {
    const auto& st = Exec("DELETE from INGREDIENTS where ID = ?1;", {{id}});
    return st.status == SQLITE_OK;
}

bool DB::DeleteTableware(size_t id) {
    const auto& st = Exec("DELETE FROM TABLEWARE WHERE ID=?1", {{id}});
    return st.status == SQLITE_OK;
}

DB::ExecResult DB::Exec(std::string_view sql,
                        const std::vector<BindParameter>& params) {
    sqlite3_stmt* stmt = nullptr;
    int st = sqlite3_prepare_v2(db_, sql.data(), -1, &stmt, nullptr);
    if (st != SQLITE_OK || stmt == nullptr) {
        std::cerr << "Prepare failed: " << st << " SQL: " << sql << std::endl;
        return {.status = st};
    }

    for (auto i = 0; i < params.size(); ++i) {
        if (params[i].index() == 0) {
            st = sqlite3_bind_int(stmt, i + 1, std::get<uint32_t>(params[i]));
        } else {
            st = sqlite3_bind_text(stmt, i + 1,
                                   std::get<std::string>(params[i]).c_str(), -1,
                                   SQLITE_TRANSIENT);
        }

        if (st != SQLITE_OK) {
            std::cerr << "Bind failed: " << st << " SQL: " << sql << std::endl;
            return {.status = st};
        }
    }

    std::lock_guard<std::mutex> lock(mu_);

    std::vector<DBRow> rows;
    for (st = sqlite3_step(stmt); st == SQLITE_ROW; st = sqlite3_step(stmt)) {
        rows.emplace_back();
        auto& row = rows.back();
        const int column_count = sqlite3_column_count(stmt);
        for (size_t i = 0; i < column_count; ++i) {
            row.emplace_back((char*)sqlite3_column_text(stmt, i));
        }
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    if (st == SQLITE_OK || st == SQLITE_DONE) {
        return {.status = SQLITE_OK, .rows = std::move(rows)};
    }
    return {.status = st};
}

}  // namespace foodculator
