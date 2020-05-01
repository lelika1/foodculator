#include <memory>
#include <string>
#include <vector>

#include "json11.hpp"

class sqlite3;

struct Ingredient {
    std::string name_;
    uint32_t kcal_;
    size_t id_;

    Ingredient(const std::string& name, uint32_t kcal, size_t id = 0)
        : name_(name), kcal_(kcal), id_(id) {}

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

    Tableware(const std::string& name, uint32_t weight, size_t id = 0)
        : name_(name), weight_(weight), id_(id) {}

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

    static std::unique_ptr<DB> Create(const std::string& path);
    ~DB();

    Result InsertProduct(const Ingredient& ingr);
    Result InsertTableware(const Tableware& tw);
    std::vector<Ingredient> GetIngredients();
    std::vector<Tableware> GetTableware();

   private:
    explicit DB(sqlite3* db) : db_(db) {}

    Result Insert(const char* sql);

    template <class T>
    std::vector<T> SelectAll(const char* sql);

    sqlite3* db_;
};
