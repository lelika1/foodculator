#include "db.h"

#include <sqlite3.h>

#include <iostream>
#include <sstream>

namespace foodculator {

std::unique_ptr<DB> DB::Create(std::string_view path) {
    sqlite3* db = nullptr;

    if (int st = sqlite3_config(SQLITE_CONFIG_SERIALIZED); st != SQLITE_OK) {
        std::cerr << "SQL config error: " << st << std::endl;
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
            KCAL         INTEGER   DEFAULT 0,
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

DB::Result DB::AddProduct(const Ingredient& ingr) {
    std::stringstream ss;
    ss << "INSERT INTO INGREDIENTS (NAME,KCAL) VALUES ('" << ingr.name_ << "', "
       << ingr.kcal_ << ");";
    return Insert(ss.str());
}

DB::Result DB::AddTableware(const Tableware& tw) {
    std::stringstream ss;
    ss << "INSERT INTO TABLEWARE (NAME,WEIGHT) VALUES ('" << tw.name_ << "', "
       << tw.weight_ << ");";
    return Insert(ss.str());
}

DB::Result DB::Insert(std::string_view sql) {
    char* err = nullptr;
    auto st = sqlite3_exec(db_, sql.data(), nullptr, 0, &err);
    if (st != SQLITE_OK) {
        std::cerr << "SQL error : " << err << std::endl;
        sqlite3_free(err);
        return Result{(st == SQLITE_CONSTRAINT) ? Result::DUPLICATE
                                                : Result::ERROR};
    }

    size_t last_id = sqlite3_last_insert_rowid(db_);
    return Result{Result::OK, last_id};
}

std::vector<Ingredient> DB::GetIngredients() {
    std::vector<Ingredient> result;
    for (auto& row : Select("SELECT NAME, KCAL, ID from INGREDIENTS")) {
        result.emplace_back(std::move(row[0]), std::stoi(row[1]),
                            std::stoi(row[2]));
    }
    return result;
}

std::vector<Tableware> DB::GetTableware() {
    std::vector<Tableware> result;
    for (auto& row : Select("SELECT NAME, WEIGHT, ID from TABLEWARE")) {
        result.emplace_back(std::move(row[0]), std::stoi(row[1]),
                            std::stoi(row[2]));
    }
    return result;
}

std::vector<DB::DBRow> DB::Select(std::string_view sql) {
    auto cb = [](void* data, int argc, char** argv, char** columns) {
        DB::DBRow row;
        row.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            row.emplace_back(argv[i]);
        }

        auto* result = static_cast<std::vector<DB::DBRow>*>(data);
        result->emplace_back(std::move(row));
        return 0;
    };

    char* err = nullptr;
    std::vector<DB::DBRow> result;
    if (sqlite3_exec(db_, sql.data(), cb, &result, &err) != SQLITE_OK) {
        std::cerr << "SQL error: " << err << std::endl;
        sqlite3_free(err);
    }
    return result;
}

bool DB::DeleteProduct(size_t id) {
    std::stringstream ss;
    ss << "DELETE from INGREDIENTS where ID = " << id << ";";
    return Delete(ss.str());
}

bool DB::DeleteTableware(size_t id) {
    std::stringstream ss;
    ss << "DELETE from TABLEWARE where ID = " << id << ";";
    return Delete(ss.str());
}

bool DB::Delete(std::string_view sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.data(), nullptr, 0, &err) != SQLITE_OK) {
        std::cerr << "SQL error: " << err << std::endl;
        sqlite3_free(err);
        return false;
    }

    return true;
}
}  // namespace foodculator
