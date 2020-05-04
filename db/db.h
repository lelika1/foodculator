#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "json11.hpp"

class sqlite3;

namespace foodculator {

struct Ingredient {
    std::string name_;
    uint32_t kcal_;
    size_t id_;

    Ingredient(std::string name, uint32_t kcal, size_t id = 0)
        : name_(std::move(name)), kcal_(kcal), id_(id) {}

    json11::Json to_json() const {
        return json11::Json::object{{"name", name_},
                                    {"kcal", std::to_string(kcal_)},
                                    {"id", std::to_string(id_)}};
    }
};

struct Tableware {
    std::string name_;
    uint32_t weight_;
    size_t id_;

    Tableware(std::string name, uint32_t weight, size_t id = 0)
        : name_(std::move(name)), weight_(weight), id_(id) {}

    json11::Json to_json() const {
        return json11::Json::object{{"name", name_},
                                    {"weight", std::to_string(weight_)},
                                    {"id", std::to_string(id_)}};
    }
};

class DB {
   public:
    struct Result {
        enum Code { OK = 0, DUPLICATE, ERROR };
        Code code_;
        size_t id_;
        Result(Code code, size_t id = 0) : code_(code), id_(id) {}
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
    explicit DB(sqlite3* db) : db_(db) {}

    Result Insert(std::string_view sql);

    using DBRow = std::vector<std::string>;
    std::vector<DBRow> Select(std::string_view sql);

    bool Delete(std::string_view sql);
    sqlite3* db_;
};
}  // namespace foodculator
