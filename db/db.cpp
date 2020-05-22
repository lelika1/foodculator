#include "db.h"

#include <sqlite3.h>

#include <iostream>
#include <sstream>
#include <string_view>

namespace foodculator {

std::unique_ptr<DB> DB::Create(std::string_view path) {
    sqlite3* db = nullptr;

    if (path != ":memory:") {
        if (int st = sqlite3_config(SQLITE_CONFIG_SERIALIZED); st != SQLITE_OK) {
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
            ID              INTEGER   PRIMARY KEY   AUTOINCREMENT NOT NULL,
            NAME            TEXT                                  NOT NULL,
            KCAL            INTEGER   DEFAULT 0                   NOT NULL,
            UNIQUE (NAME, KCAL)
        );
        CREATE TABLE IF NOT EXISTS TABLEWARE(
            ID              INTEGER   PRIMARY KEY   AUTOINCREMENT NOT NULL,
            NAME            TEXT                                  NOT NULL,
            WEIGHT          INTEGER                               NOT NULL,
            UNIQUE (NAME, WEIGHT)
        );
        CREATE TABLE IF NOT EXISTS RECIPE(
            ID              INTEGER   PRIMARY KEY   AUTOINCREMENT NOT NULL,
            NAME            TEXT                                  NOT NULL,
            DESC            TEXT                                  NOT NULL,
            UNIQUE  (NAME)
        );
        CREATE TABLE IF NOT EXISTS RECIPE_INGREDIENTS(
            RECIPE_ID       INTEGER                               NOT NULL,
            INGR_ID         INTEGER                               NOT NULL,
            WEIGHT          INTEGER                               NOT NULL,
            FOREIGN KEY(RECIPE_ID) REFERENCES RECIPE(ID) ON DELETE CASCADE,
            FOREIGN KEY(INGR_ID)   REFERENCES INGREDIENTS(ID)
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
    std::vector<BindParameter> params = {{std::move(name)}, {kcal}};
    if (auto st = Insert("INGREDIENTS", {"NAME", "KCAL"}, params); st != Result::OK) {
        return {.code = st};
    }
    // TODO remove the product from DB if SelectId returns error
    return SelectId("INGREDIENTS", {"NAME", "KCAL"}, params, "ID");
}

DB::Result DB::AddTableware(std::string name, uint32_t weight) {
    std::vector<BindParameter> params = {{std::move(name)}, {weight}};
    if (auto st = Insert("TABLEWARE", {"NAME", "WEIGHT"}, params); st != Result::OK) {
        return {.code = st};
    }
    // TODO remove the tableware from DB if SelectId returns error
    return SelectId("TABLEWARE", {"NAME", "WEIGHT"}, params, "ID");
}

DB::Result::Code DB::Insert(std::string_view table, const std::vector<std::string_view>& fields,
                            const std::vector<BindParameter>& params) {
    if (params.size() % fields.size() != 0) {
        std::cerr << "params.size() % fields.size() != 0: " << fields.size() << " " << params.size()
                  << std::endl;
        exit(1);
    }

    std::stringstream names;
    std::stringstream params_set;
    params_set << "(";
    for (size_t idx = 0; idx < fields.size(); idx++) {
        if (idx > 0) {
            names << ", ";
            params_set << ", ";
        }
        names << fields[idx];
        params_set << "?";
    }
    params_set << ")";
    std::string params_set_str = params_set.str();

    std::stringstream binds;
    for (size_t line = 0; line < params.size() / fields.size(); ++line) {
        if (line > 0) {
            binds << ",";
        }
        binds << params_set_str;
    }

    std::stringstream insert;
    insert << "INSERT INTO " << table << "(" << names.str() << ") VALUES " << binds.str() << ";";
    switch (Exec(insert.str(), params).status) {
        case SQLITE_OK:
            return Result::OK;
        case SQLITE_CONSTRAINT:
            return Result::DUPLICATE;
        default:
            return Result::ERROR;
    }
}

DB::Result DB::SelectId(std::string_view table, const std::vector<std::string_view>& fields,
                        const std::vector<BindParameter>& params, std::string_view id_field) {
    if (params.size() != fields.size()) {
        std::cerr << "params.size() != fields.size(): " << fields.size() << " " << params.size()
                  << std::endl;
        exit(1);
    }

    std::stringstream conds;
    for (size_t idx = 0; idx < fields.size(); idx++) {
        if (idx > 0) {
            conds << " AND ";
        }
        conds << fields[idx] << " = ?";
    }

    std::stringstream select_id;
    select_id << "SELECT " << id_field << " FROM " << table << " WHERE " << conds.str() << ";";
    const auto& res = Exec(select_id.str(), params);
    if (res.rows.size() != 1 || res.rows[0].size() != 1) {
        return {.code = Result::ERROR};
    }

    return {.id = std::stoull(res.rows[0][0])};
}

std::vector<Ingredient> DB::GetProducts() {
    std::vector<Ingredient> ret;
    const auto& res = Exec("SELECT NAME, KCAL, ID from INGREDIENTS", {});
    for (auto& row : res.rows) {
        ret.emplace_back(std::move(row.at(0)), std::stoul(row.at(1)), std::stoull(row.at(2)));
    }
    return ret;
}

std::vector<Tableware> DB::GetTableware() {
    std::vector<Tableware> ret;
    const auto& res = Exec("SELECT NAME, WEIGHT, ID from TABLEWARE", {});
    for (auto& row : res.rows) {
        ret.emplace_back(std::move(row.at(0)), std::stoul(row.at(1)), std::stoull(row.at(2)));
    }
    return ret;
}

bool DB::DeleteProduct(size_t id) {
    // TODO return an error if there are recipes with this ingredient
    const auto& st = Exec("DELETE from INGREDIENTS where ID = ?1;", {{id}});
    return st.status == SQLITE_OK;
}

bool DB::DeleteTableware(size_t id) {
    const auto& st = Exec("DELETE FROM TABLEWARE WHERE ID = ?1", {{id}});
    return st.status == SQLITE_OK;
}

DB::Result DB::CreateRecipe(const std::string& name, const std::string& description,
                            const std::map<size_t, uint32_t>& ingredients) {
    if (ingredients.empty() || name.empty()) {
        // TODO add new status
        return {.code = Result::ERROR};
    }

    if (auto code = Insert("RECIPE", {"NAME", "DESC"}, {{name}, {description}});
        code != Result::OK) {
        return {.code = code};
    }

    Result st = SelectId("RECIPE", {"NAME", "DESC"}, {{name}, {description}}, "ID");
    if (st.code != Result::OK) {
        // TODO remove the recipe from DB?
        return st;
    }

    const size_t recipe_id = st.id;

    std::vector<BindParameter> params;
    params.reserve(ingredients.size() * 3);
    for (const auto& [id, weight] : ingredients) {
        if (weight == 0) {
            continue;
        }
        params.emplace_back(recipe_id);
        params.emplace_back(id);
        params.emplace_back(weight);
    }

    if (auto code = Insert("RECIPE_INGREDIENTS", {"RECIPE_ID", "INGR_ID", "WEIGHT"}, params);
        code != Result::OK) {
        // TODO check an error and remove the recipe from DB add new code
        return {.code = code};
    }

    return {.id = recipe_id};
}

std::vector<RecipeHeader> DB::GetRecipes() {
    std::vector<RecipeHeader> ret;
    for (auto& row : Exec("SELECT NAME, ID FROM RECIPE;", {}).rows) {
        ret.emplace_back(std::move(row.at(0)), std::stoull(row.at(1)));
    }
    return ret;
}

FullRecipe DB::GetRecipeInfo(size_t recipe_id) {
    FullRecipe recipe;
    recipe.header.id = recipe_id;
    const auto& desc = Exec("SELECT NAME, DESC FROM RECIPE WHERE ID=?1;", {{recipe_id}});
    if ((desc.status != SQLITE_OK) || (desc.rows.empty())) {
        return recipe;
    }

    auto& header = desc.rows[0];
    recipe.header.name = std::move(header.at(0));
    recipe.description = std::move(header.at(1));

    const auto& res =
        Exec("SELECT INGR_ID, WEIGHT FROM RECIPE_INGREDIENTS WHERE RECIPE_ID=?1;", {{recipe_id}});
    if (res.status != SQLITE_OK) {
        return recipe;
    }

    for (const auto& row : res.rows) {
        recipe.ingredients.emplace_back(std::stoull(row.at(0)), std::stoul(row.at(1)));
    }
    return recipe;
}

bool DB::DeleteRecipe(size_t id) {
    const auto& st = Exec("DELETE FROM RECIPE WHERE ID=?1", {{id}});
    return st.status == SQLITE_OK;
}

DB::ExecResult DB::Exec(std::string_view sql, const std::vector<BindParameter>& params) {
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
            st = sqlite3_bind_text(stmt, i + 1, std::get<std::string>(params[i]).c_str(), -1,
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
