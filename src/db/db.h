#ifndef __SRC_DB_DB_H__
#define __SRC_DB_DB_H__

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "json11/json11.hpp"
#include "util/statusor.h"

class sqlite3;

namespace foodculator {

struct Ingredient {
    std::string name;
    uint32_t kcal;
    size_t id;

    Ingredient(std::string name, uint32_t kcal, size_t id = 0)
        : name(std::move(name)), kcal(kcal), id(id) {}

    bool operator==(const Ingredient& rhs) const;

    json11::Json to_json() const {
        return json11::Json::object{
            {"name", name}, {"kcal", std::to_string(kcal)}, {"id", std::to_string(id)}};
    }
};

std::ostream& operator<<(std::ostream& out, const Ingredient& v);

struct Tableware {
    std::string name;
    uint32_t weight;
    size_t id;

    Tableware(std::string name, uint32_t weight, size_t id = 0)
        : name(std::move(name)), weight(weight), id(id) {}

    bool operator==(const Tableware& rhs) const;

    json11::Json to_json() const {
        return json11::Json::object{
            {"name", name}, {"weight", std::to_string(weight)}, {"id", std::to_string(id)}};
    }
};

std::ostream& operator<<(std::ostream& out, const Tableware& v);

struct RecipeIngredient {
    size_t ingredient_id;
    uint32_t weight;

    RecipeIngredient(size_t id, uint32_t weight) : ingredient_id(id), weight(weight) {}

    bool operator==(const RecipeIngredient& rhs) const;

    json11::Json to_json() const {
        return json11::Json::object{{"id", std::to_string(ingredient_id)},
                                    {"weight", std::to_string(weight)}};
    }
};

std::ostream& operator<<(std::ostream& out, const RecipeIngredient& v);

struct RecipeHeader {
    std::string name;
    size_t id;

    RecipeHeader() {}
    RecipeHeader(std::string name, size_t id = 0) : name(std::move(name)), id(id) {}

    bool operator==(const RecipeHeader& rhs) const;

    json11::Json to_json() const {
        return json11::Json::object{{"name", name}, {"id", std::to_string(id)}};
    }
};

std::ostream& operator<<(std::ostream& out, const RecipeHeader& v);

struct FullRecipe {
    RecipeHeader header;
    std::string description;
    std::vector<RecipeIngredient> ingredients;

    bool operator==(const FullRecipe& rhs) const;

    json11::Json to_json() const {
        return json11::Json::object{{"header", header.to_json()},
                                    {"description", description},
                                    {"ingredients", ingredients}};
    }
};

std::ostream& operator<<(std::ostream& out, const FullRecipe& v);

class DB {
   public:
    static std::unique_ptr<DB> Create(std::string_view path);
    ~DB();

    StatusOr<size_t> AddProduct(std::string name, uint32_t kcal);
    StatusOr<Ingredient> GetProduct(size_t id);
    StatusOr<std::vector<Ingredient>> GetProducts();
    bool DeleteProduct(size_t id);

    StatusOr<size_t> AddTableware(std::string name, uint32_t weight);
    StatusOr<std::vector<Tableware>> GetTableware();
    bool DeleteTableware(size_t id);

    StatusOr<size_t> CreateRecipe(const std::string& name, const std::string& description,
                                  const std::map<size_t, uint32_t>& ingredients);
    StatusOr<std::vector<RecipeHeader>> GetRecipes();
    StatusOr<FullRecipe> GetRecipeInfo(size_t recipe_id);
    bool DeleteRecipe(size_t id);

   private:
    explicit DB(sqlite3* db) : db_(db) {}

    using BindParameter = std::variant<uint32_t, std::string>;
    StatusCode Insert(std::string_view table, const std::vector<std::string_view>& fields,
                      const std::vector<BindParameter>& params);
    StatusOr<size_t> SelectId(std::string_view table, const std::vector<std::string_view>& fields,
                              const std::vector<BindParameter>& params, std::string_view id_field);

    using DBRow = std::vector<std::string>;
    StatusOr<std::vector<DBRow>> Exec(std::string_view sql,
                                      const std::vector<BindParameter>& params);

    std::mutex mu_;
    sqlite3* db_;
};

}  // namespace foodculator

#endif
