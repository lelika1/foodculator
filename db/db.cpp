#include "db.h"

#include <sqlite3.h>

#include <iostream>
#include <sstream>

namespace foodculator {

std::unique_ptr<DB> DB::Create(const std::string& path) {
    sqlite3* db;

    int st = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    if (st != SQLITE_OK) {
        std::cout << "SQL config error: " << st << std::endl;
    }

    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << path << std::endl;
        return nullptr;
    }

    const char sql[] =
        R"*(CREATE TABLE IF NOT EXISTS INGREDIENTS(
            ID           INTEGER   PRIMARY KEY   AUTOINCREMENT NOT NULL,
            NAME         TEXT                                  NOT NULL,
            KCAL         INTEGER   DEFAULT 0,
            UNIQUE (NAME, KCAL)
        );
        CREATE TABLE IF NOT EXISTS TABLEWARE(
            ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT NULL,
            WEIGHT INTEGER NOT NULL,
            UNIQUE (NAME, WEIGHT)
        );
    )*";

    char* err_msg = nullptr;
    auto cb = [](void*, int, char**, char**) -> int { return 0; };
    if (sqlite3_exec(db, sql, cb, 0, &err_msg) != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
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

DB::Result DB::AddProduct(const Ingredient& ingr) {
    std::stringstream ss;
    ss << "INSERT INTO INGREDIENTS (NAME,KCAL) VALUES ('" << ingr.name_ << "', "
       << ingr.kcal_ << ");";
    std::string sql = ss.str();

    return Insert(sql.c_str());
}

DB::Result DB::AddTableware(const Tableware& tw) {
    std::stringstream ss;
    ss << "INSERT INTO TABLEWARE (NAME,WEIGHT) VALUES ('" << tw.name_ << "', "
       << tw.weight_ << ");";
    std::string sql = ss.str();

    return Insert(sql.c_str());
}

DB::Result DB::Insert(const char* sql) {
    char* err_msg = nullptr;
    auto cb = [](void*, int, char**, char**) -> int { return 0; };
    auto st = sqlite3_exec(db_, sql, cb, 0, &err_msg);
    if (st != SQLITE_OK) {
        std::cerr << "SQL error : " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return Result{(st == SQLITE_CONSTRAINT) ? Result::DUPLICATE
                                                : Result::ERROR};
    }

    size_t last_id = sqlite3_last_insert_rowid(db_);
    return Result{Result::OK, last_id};
}

std::vector<Ingredient> DB::GetIngredients() {
    const auto& rows = Select("SELECT ID, NAME, KCAL from INGREDIENTS");

    std::vector<Ingredient> result;
    for (const auto& row : rows) {
        result.emplace_back(row.at("NAME"), std::stoi(row.at("KCAL")),
                            std::stoi(row.at("ID")));
    }
    return result;
}

std::vector<Tableware> DB::GetTableware() {
    const auto& rows = Select("SELECT ID, NAME, WEIGHT from TABLEWARE");

    std::vector<Tableware> result;
    for (const auto& row : rows) {
        result.emplace_back(row.at("NAME"), std::stoi(row.at("WEIGHT")),
                            std::stoi(row.at("ID")));
    }
    return result;
}

std::vector<DB::DBRow> DB::Select(const char* sql) {
    auto cb = [](void* data, int argc, char** argv, char** columns) {
        DB::DBRow row;
        for (size_t i = 0; i < argc; ++i) {
            row[std::string(columns[i])] = std::string(argv[i]);
        }

        auto* result = static_cast<std::vector<DB::DBRow>*>(data);
        result->push_back(row);
        return 0;
    };

    char* err_msg = nullptr;
    std::vector<DB::DBRow> result;
    if (sqlite3_exec(db_, sql, cb, &result, &err_msg) != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }
    return result;
}

bool DB::DeleteProduct(size_t id) {
    std::stringstream ss;
    ss << "DELETE from INGREDIENTS where ID = " << id << ";";
    std::string sql = ss.str();

    return Delete(sql.c_str());
}

bool DB::DeleteTableware(size_t id) {
    std::stringstream ss;
    ss << "DELETE from TABLEWARE where ID = " << id << ";";
    std::string sql = ss.str();

    return Delete(sql.c_str());
}

bool DB::Delete(const char* sql) {
    char* err_msg = nullptr;
    auto cb = [](void*, int, char**, char**) -> int { return 0; };
    auto st = sqlite3_exec(db_, sql, cb, 0, &err_msg);
    if (st != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}
}  // namespace foodculator
