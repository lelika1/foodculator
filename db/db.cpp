#include "db.h"

#include <sqlite3.h>

#include <iostream>
#include <sstream>

std::unique_ptr<DB> DB::Create(const std::string& path) {
    sqlite3* db;

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

    char* err_msg = 0;
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

DB::Result DB::InsertProduct(const Ingredient& ingr) {
    std::stringstream ss;
    ss << "INSERT INTO INGREDIENTS (NAME,KCAL) VALUES ('" << ingr.name_ << "', "
       << ingr.kcal_ << ");";
    std::string sql = ss.str();

    return Insert(sql.c_str());
}

DB::Result DB::InsertTableware(const Tableware& tw) {
    std::stringstream ss;
    ss << "INSERT INTO TABLEWARE (NAME,WEIGHT) VALUES ('" << tw.name_ << "', "
       << tw.weight_ << ");";
    std::string sql = ss.str();

    return Insert(sql.c_str());
}

DB::Result DB::Insert(const char* sql) {
    char* err_msg = 0;
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
    return SelectAll<Ingredient>("SELECT NAME, KCAL, ID from INGREDIENTS");
}

std::vector<Tableware> DB::GetTableware() {
    return SelectAll<Tableware>("SELECT NAME, WEIGHT, ID from TABLEWARE");
}

template <class T>
std::vector<T> DB::SelectAll(const char* sql) {
    auto cb = [](void* data, int argc, char** argv, char** columns) -> int {
        if (argc != 3) {
            return 1;
        }

        std::vector<T>* results = static_cast<std::vector<T>*>(data);
        results->emplace_back(argv[0], std::stoi(argv[1]), std::stoi(argv[2]));
        return 0;
    };

    char* err_msg = 0;
    std::vector<T> results;
    if (sqlite3_exec(db_, sql, cb, &results, &err_msg) != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }
    return results;
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
    char* err_msg = 0;
    auto cb = [](void*, int, char**, char**) -> int { return 0; };
    auto st = sqlite3_exec(db_, sql, cb, 0, &err_msg);
    if (st != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}
