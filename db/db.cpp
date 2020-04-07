#include "db.h"

Ingredient::Ingredient(const std::string& name, uint32_t kcal)
    : name_(name), kcal_(kcal) {}

Tableware::Tableware(const std::string& name, uint32_t weight)
    : name_(name), weight_(weight) {}

DB::DB(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db)) {
        throw DBCreateException();
    }

    // const char createIngredients[] = "CREATE TABLE IF NOT EXISTS
    // INGREDIENTS(" "ID           INTEGER   PRIMARY KEY   AUTOINCREMENT NOT
    // NULL,"
    //     "NAME         TEXT                                  NOT NULL,"
    //     "KCAL         INTEGER   DEFAULT 0);";

    // "CREATE TABLE IF NOT EXISTS INGREDIENTS("
    //     "ID           INTEGER   PRIMARY KEY   AUTOINCREMENT NOT NULL,"
    //     "NAME         TEXT                                  NOT NULL,"
    //     "KCAL         INTEGER   DEFAULT 0);";
}

void DB::AddNewProduct(const Ingredient& ingr) {}

DB::~DB() { sqlite3_close(db); }
