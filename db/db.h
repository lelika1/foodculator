#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "json11/json11.hpp"

class sqlite3;

namespace foodculator {

struct Ingredient {
    std::string name;
    uint32_t kcal;
    size_t id;

    Ingredient(std::string name, uint32_t kcal, size_t id = 0)
        : name(std::move(name)), kcal(kcal), id(id) {}

    friend void PrintTo(const Ingredient& v, std::ostream* os) { *os << v.to_json().dump(); }
    json11::Json to_json() const {
        return json11::Json::object{
            {"name", name}, {"kcal", std::to_string(kcal)}, {"id", std::to_string(id)}};
    }
};

struct Tableware {
    std::string name;
    uint32_t weight;
    size_t id;

    Tableware(std::string name, uint32_t weight, size_t id = 0)
        : name(std::move(name)), weight(weight), id(id) {}

    friend void PrintTo(const Tableware& v, std::ostream* os) { *os << v.to_json().dump(); }
    json11::Json to_json() const {
        return json11::Json::object{
            {"name", name}, {"weight", std::to_string(weight)}, {"id", std::to_string(id)}};
    }
};

struct RecipeIngredient {
    size_t ingredient_id;
    uint32_t weight;

    RecipeIngredient(size_t id, uint32_t weight) : ingredient_id(id), weight(weight) {}

    friend void PrintTo(const RecipeIngredient& v, std::ostream* os) { *os << v.to_json().dump(); }
    json11::Json to_json() const {
        return json11::Json::object{{"id", std::to_string(ingredient_id)},
                                    {"weight", std::to_string(weight)}};
    }
};

struct RecipeHeader {
    std::string name;
    size_t id;

    RecipeHeader() {}
    RecipeHeader(std::string name, size_t id = 0) : name(std::move(name)), id(id) {}

    friend void PrintTo(const RecipeHeader& v, std::ostream* os) { *os << v.to_json().dump(); }
    json11::Json to_json() const {
        return json11::Json::object{{"name", name}, {"id", std::to_string(id)}};
    }
};

struct FullRecipe {
    RecipeHeader header;
    std::string description;
    std::vector<RecipeIngredient> ingredients;

    friend void PrintTo(const FullRecipe& v, std::ostream* os) { *os << v.to_json().dump(); }
    json11::Json to_json() const {
        return json11::Json::object{{"header", header.to_json()},
                                    {"description", description},
                                    {"ingredients", ingredients}};
    }
};

class DB {
   public:
    struct Result {
        enum Code { OK = 0, INVALID_ARGUMENT, ERROR };
        Code code = OK;
        size_t id;
    };

    static std::unique_ptr<DB> Create(std::string_view path);
    ~DB();

    Result AddProduct(std::string name, uint32_t kcal);
    std::vector<Ingredient> GetProducts();
    bool DeleteProduct(size_t id);

    Result AddTableware(std::string name, uint32_t weight);
    std::vector<Tableware> GetTableware();
    bool DeleteTableware(size_t id);

    Result CreateRecipe(const std::string& name, const std::string& description,
                        const std::map<size_t, uint32_t>& ingredients);
    std::vector<RecipeHeader> GetRecipes();
    FullRecipe GetRecipeInfo(size_t recipe_id);
    bool DeleteRecipe(size_t id);

   private:
    using DBRow = std::vector<std::string>;
    using BindParameter = std::variant<uint32_t, std::string>;

    explicit DB(sqlite3* db) : db_(db) {}

    DB::Result::Code Insert(std::string_view table, const std::vector<std::string_view>& fields,
                            const std::vector<BindParameter>& params);

    DB::Result SelectId(std::string_view table, const std::vector<std::string_view>& fields,
                        const std::vector<BindParameter>& params, std::string_view id_field);

    struct ExecResult {
        int status;
        std::vector<DBRow> rows;
    };

    ExecResult Exec(std::string_view sql, const std::vector<BindParameter>& params);

    std::mutex mu_;
    sqlite3* db_;
};

}  // namespace foodculator
