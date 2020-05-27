#include "db.h"

#include <sqlite3.h>

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace foodculator {

namespace {

StatusCode ConvertSqliteToStatus(int status) {
    switch (status) {
        case SQLITE_OK:
        case SQLITE_DONE:
            return StatusCode::OK;
        case SQLITE_CONSTRAINT:
            return StatusCode::INVALID_ARGUMENT;
        default:
            return StatusCode::INTERNAL_ERROR;
    }
}

}  // namespace

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
        PRAGMA foreign_keys = ON;
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

StatusOr<size_t> DB::AddProduct(std::string name, uint32_t kcal) {
    std::vector<BindParameter> params = {{std::move(name)}, {kcal}};
    switch (Insert("INGREDIENTS", {"NAME", "KCAL"}, params)) {
        case StatusCode::OK:
            break;
        case StatusCode::INVALID_ARGUMENT:
            return {StatusCode::INVALID_ARGUMENT,
                    "This ingredient already exists in the database."};
        default:
            return {StatusCode::INTERNAL_ERROR, "DB request failed.Try again later."};
    }

    auto st = SelectId("INGREDIENTS", {"NAME", "KCAL"}, params, "ID");
    if (!st.Ok()) {
        Exec("DELETE FROM INGREDIENTS WHERE NAME=?1 AND KCAL=?2;", params);
    }
    return st;
}

StatusOr<size_t> DB::AddTableware(std::string name, uint32_t weight) {
    std::vector<BindParameter> params = {{std::move(name)}, {weight}};
    switch (Insert("TABLEWARE", {"NAME", "WEIGHT"}, params)) {
        case StatusCode::OK:
            break;
        case StatusCode::INVALID_ARGUMENT:
            return {StatusCode::INVALID_ARGUMENT, "This pot already exists in the database."};
        default:
            return {StatusCode::INTERNAL_ERROR, "DB request failed.Try again later."};
    }

    auto st = SelectId("TABLEWARE", {"NAME", "WEIGHT"}, params, "ID");
    if (!st.Ok()) {
        Exec("DELETE FROM TABLEWARE WHERE NAME=?1 AND WEIGHT=?2;", params);
    }
    return st;
}

StatusCode DB::Insert(std::string_view table, const std::vector<std::string_view>& fields,
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
    return Exec(insert.str(), params).Code();
}

StatusOr<size_t> DB::SelectId(std::string_view table, const std::vector<std::string_view>& fields,
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
    auto res = Exec(select_id.str(), params);
    if (!res.Ok()) {
        return {res.Code(), std::move(res.Error())};
    }

    if (res.Value().empty()) {
        return {StatusCode::NOT_FOUND, "No id for this element was found."};
    }

    if (res.Value()[0].size() != 1) {
        std::cerr << "'" << select_id.str() << "' returned " << res.Value()[0].size()
                  << " columns.";
        exit(2);
    }

    return StatusOr{std::stoul(res.Value()[0][0])};
}

StatusOr<Ingredient> DB::GetProduct(size_t id) {
    std::string_view sql = "SELECT NAME, KCAL from INGREDIENTS WHERE ID=?1;";
    auto res = Exec(sql, {{id}});
    if (!res.Ok()) {
        return {res.Code(), std::move(res.Error())};
    }

    if (res.Value().empty()) {
        return {StatusCode::NOT_FOUND, "Product with id=" + std::to_string(id) + " wasn't found."};
    }

    auto& row = res.Value()[0];
    if (row.size() != 2) {
        std::cerr << "'" << sql << "' returned " << row.size() << " columns.";
        exit(2);
    }

    std::string name = std::move(row[0]);
    auto kcal = static_cast<uint32_t>(std::stoul(row[1]));
    return StatusOr{Ingredient{std::move(name), kcal, id}};
}

StatusOr<std::vector<Ingredient>> DB::GetProducts() {
    std::vector<Ingredient> ret;
    std::string_view sql = "SELECT NAME, KCAL, ID from INGREDIENTS;";
    auto res = Exec(sql, {});
    if (!res.Ok()) {
        return {res.Code(), std::move(res.Error())};
    }

    for (auto& row : res.Value()) {
        if (row.size() != 3) {
            std::cerr << "'" << sql << "' returned " << row.size() << " columns.";
            exit(2);
        }

        std::string name = std::move(row[0]);
        uint32_t kcal = static_cast<uint32_t>(std::stoul(row[1]));
        size_t id = static_cast<size_t>(std::stoull(row[2]));
        ret.emplace_back(std::move(name), kcal, id);
    }
    return StatusOr{std::move(ret)};
}

StatusOr<std::vector<Tableware>> DB::GetTableware() {
    std::vector<Tableware> ret;
    std::string_view sql = "SELECT NAME, WEIGHT, ID from TABLEWARE;";
    auto res = Exec(sql, {});
    if (!res.Ok()) {
        return {res.Code(), std::move(res.Error())};
    }

    for (auto& row : res.Value()) {
        if (row.size() != 3) {
            std::cerr << "'" << sql << "' returned " << row.size() << " columns.";
            exit(2);
        }

        std::string name = std::move(row[0]);
        uint32_t weight = static_cast<uint32_t>(std::stoul(row[1]));
        size_t id = static_cast<size_t>(std::stoull(row[2]));
        ret.emplace_back(std::move(name), weight, id);
    }
    return StatusOr{std::move(ret)};
}

bool DB::DeleteProduct(size_t id) {
    return Exec("DELETE from INGREDIENTS where ID = ?1;", {{id}}).Ok();
}

bool DB::DeleteTableware(size_t id) {
    return Exec("DELETE FROM TABLEWARE WHERE ID = ?1;", {{id}}).Ok();
}

StatusOr<size_t> DB::CreateRecipe(const std::string& name, const std::string& description,
                                  const std::map<size_t, uint32_t>& ingredients) {
    if (name.empty()) {
        return {StatusCode::INVALID_ARGUMENT, "Name of the recipe has to be non-empty."};
    }

    switch (Insert("RECIPE", {"NAME", "DESC"}, {{name}, {description}})) {
        case StatusCode::OK:
            break;
        case StatusCode::INVALID_ARGUMENT:
            return {StatusCode::INVALID_ARGUMENT,
                    "A recipe with this name already exists in the database."};
        default:
            return {StatusCode::INTERNAL_ERROR, "DB request failed.Try again later."};
    }

    auto st = SelectId("RECIPE", {"NAME", "DESC"}, {{name}, {description}}, "ID");
    if (!st.Ok()) {
        Exec("DELETE FROM RECIPE WHERE NAME=?1;", {{name}});
        return st;
    }

    const size_t recipe_id = st.Value();

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
        code != StatusCode::OK) {
        Exec("DELETE FROM RECIPE_INGREDIENTS WHERE RECIPE_ID=?1;", {{recipe_id}});
        Exec("DELETE FROM RECIPE WHERE ID=?1;", {{recipe_id}});

        if (code == StatusCode::INVALID_ARGUMENT) {
            return {code, "Some of the ingredients don't exist in the database."};
        }

        return {code, "DB request failed.Try again later."};
    }

    return StatusOr{recipe_id};
}

StatusOr<std::vector<RecipeHeader>> DB::GetRecipes() {
    std::string_view sql = "SELECT NAME, ID FROM RECIPE;";
    auto res = Exec(sql, {});
    if (!res.Ok()) {
        return {res.Code(), std::move(res.Error())};
    }

    std::vector<RecipeHeader> ret;
    for (auto& row : res.Value()) {
        if (row.size() != 2) {
            std::cerr << "'" << sql << "' returned " << row.size() << " columns.";
            exit(2);
        }

        std::string name = std::move(row[0]);
        size_t id = static_cast<size_t>(std::stoull(row[1]));
        ret.emplace_back(std::move(name), id);
    }
    return StatusOr{std::move(ret)};
}

StatusOr<FullRecipe> DB::GetRecipeInfo(size_t recipe_id) {
    std::string_view sql = "SELECT NAME, DESC FROM RECIPE WHERE ID=?1;";
    auto desc = Exec(sql, {{recipe_id}});
    if (!desc.Ok()) {
        return {desc.Code(), std::move(desc.Error())};
    }

    if (desc.Value().empty()) {
        return {StatusCode::NOT_FOUND,
                "No recipe with id=" + std::to_string(recipe_id) + " exists in the database."};
    }

    auto& header_data = desc.Value()[0];
    if (header_data.size() != 2) {
        std::cerr << "'" << sql << "' returned " << header_data.size() << " columns.";
        exit(2);
    }

    sql = "SELECT INGR_ID, WEIGHT FROM RECIPE_INGREDIENTS WHERE RECIPE_ID=?1;";
    auto ingredients = Exec(sql, {{recipe_id}});
    if (!ingredients.Ok()) {
        return {ingredients.Code(), std::move(ingredients.Error())};
    }

    FullRecipe recipe;
    recipe.header.id = recipe_id;
    recipe.header.name = std::move(header_data[0]);
    recipe.description = std::move(header_data[1]);

    for (const auto& row : ingredients.Value()) {
        if (row.size() != 2) {
            std::cerr << "'" << sql << "' returned " << row.size() << " columns.";
            exit(2);
        }

        size_t id = static_cast<size_t>(std::stoull(row[0]));
        uint32_t weight = static_cast<uint32_t>(std::stoul(row[1]));
        recipe.ingredients.emplace_back(id, weight);
    }
    return StatusOr{std::move(recipe)};
}

bool DB::DeleteRecipe(size_t id) { return Exec("DELETE FROM RECIPE WHERE ID=?1;", {{id}}).Ok(); }

StatusOr<std::vector<DB::DBRow>> DB::Exec(std::string_view sql,
                                          const std::vector<BindParameter>& params) {
    sqlite3_stmt* stmt = nullptr;
    int st = sqlite3_prepare_v2(db_, sql.data(), -1, &stmt, nullptr);
    if (st != SQLITE_OK || stmt == nullptr) {
        std::cerr << "Prepare failed: " << st << " SQL: " << sql << std::endl;
        return {ConvertSqliteToStatus(st), "sqlite3_prepare_v2 failed."};
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
            return {ConvertSqliteToStatus(st), "sqlite3_bind failed."};
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
        return StatusOr{std::move(rows)};
    }
    return {ConvertSqliteToStatus(st), "sqlite3_step failed."};
}

std::ostream& operator<<(std::ostream& out, const Ingredient& v) {
    return out << v.to_json().dump();
}

std::ostream& operator<<(std::ostream& out, const Tableware& v) {
    return out << v.to_json().dump();
}

std::ostream& operator<<(std::ostream& out, const RecipeIngredient& v) {
    return out << v.to_json().dump();
}

std::ostream& operator<<(std::ostream& out, const RecipeHeader& v) {
    return out << v.to_json().dump();
}

std::ostream& operator<<(std::ostream& out, const FullRecipe& v) {
    return out << v.to_json().dump();
}

bool Ingredient::operator==(const Ingredient& rhs) const {
    return name == rhs.name && kcal == rhs.kcal && id == rhs.id;
}

bool Tableware::operator==(const Tableware& rhs) const {
    return name == rhs.name && weight == rhs.weight && id == rhs.id;
}

bool RecipeIngredient::operator==(const RecipeIngredient& rhs) const {
    return ingredient_id == rhs.ingredient_id && weight == rhs.weight;
}

bool RecipeHeader::operator==(const RecipeHeader& rhs) const {
    return name == rhs.name && id == rhs.id;
}

bool FullRecipe::operator==(const FullRecipe& rhs) const {
    return header == rhs.header && description == rhs.description && ingredients == rhs.ingredients;
}

}  // namespace foodculator
