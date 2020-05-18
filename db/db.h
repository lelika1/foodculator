#include <memory>
#include <mutex>
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

    Ingredient(std::string n, uint32_t k, size_t i = 0)
        : name(std::move(n)), kcal(k), id(i) {}

    json11::Json to_json() const {
        return json11::Json::object{{"name", name},
                                    {"kcal", std::to_string(kcal)},
                                    {"id", std::to_string(id)}};
    }
};

struct Tableware {
    std::string name;
    uint32_t weight;
    size_t id;

    Tableware(std::string n, uint32_t w, size_t i = 0)
        : name(std::move(n)), weight(w), id(i) {}

    json11::Json to_json() const {
        return json11::Json::object{{"name", name},
                                    {"weight", std::to_string(weight)},
                                    {"id", std::to_string(id)}};
    }
};

class DB {
   public:
    struct Result {
        enum Code { OK = 0, DUPLICATE, ERROR };
        Code code;
        size_t id;
    };

    static std::unique_ptr<DB> Create(std::string_view path);
    ~DB();

    Result AddProduct(const Ingredient& ingr);
    Result AddTableware(const Tableware& tw);
    std::vector<Ingredient> GetIngredients();
    std::vector<Tableware> GetTableware();

    bool DeleteProduct(size_t id);
    bool DeleteTableware(size_t id);

   private:
    using DBRow = std::vector<std::string>;
    using BindParameter = std::variant<uint32_t, std::string>;

    explicit DB(sqlite3* db) : db_(db) {}

    DB::Result Insert(std::string_view table,
                      const std::vector<std::string_view>& fields,
                      std::string_view id_field,
                      const std::vector<BindParameter>& params);

    struct ExecResult {
        int status;
        std::vector<DBRow> rows;
    };

    ExecResult Exec(std::string_view sql,
                    const std::vector<BindParameter>& params);

    std::mutex mu_;
    sqlite3* db_;
};
}  // namespace foodculator
