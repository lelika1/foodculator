#include "db.h"

#include <sqlite3.h>

#include <iostream>

std::unique_ptr<DB> DB::Create(const std::string& path) {
    sqlite3* db;

    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << path << std::endl;
        return nullptr;
    }

    char* err_msg = 0;
    auto cb = [](void*, int, char**, char**) -> int { return 0; };

    const char sql[] =
        R"*(CREATE TABLE IF NOT EXISTS INGREDIENTS(
        ID           INTEGER   PRIMARY KEY   AUTOINCREMENT NOT NULL,
        NAME         TEXT                                  NOT NULL,
        KCAL         INTEGER   DEFAULT 0);
        CREATE TABLE IF NOT EXISTS TABLEWARES(
        ID           INTEGER   PRIMARY KEY   AUTOINCREMENT NOT NULL,
        NAME         TEXT                                  NOT NULL,
        WEIGHT       INTEGER                               NOT NULL);
    )*";

    if (sqlite3_exec(db, sql, cb, 0, &err_msg) != SQLITE_OK) {
        std::cout << "Error: " << err_msg << std::endl;
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

void DB::AddNewProduct(const Ingredient& ingr) {}
