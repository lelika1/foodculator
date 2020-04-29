#include <memory>
#include <string>
#include <vector>

#include "json11.hpp"

class sqlite3;

struct Ingredient {
    std::string name_;
    uint32_t kcal_;

    Ingredient(const std::string& name, uint32_t kcal)
        : name_(name), kcal_(kcal) {}

    json11::Json to_json() const {
        return json11::Json::object{{"name", name_},
                                    {"kcal", std::to_string(kcal_)}};
    }
};

struct Tableware {
    std::string name_;
    uint32_t weight_;

    Tableware(const std::string& name, uint32_t weight)
        : name_(name), weight_(weight) {}

    json11::Json to_json() const {
        return json11::Json::object{{"name", name_},
                                    {"weight", std::to_string(weight_)}};
    }
};

class DB {
   public:
    static std::unique_ptr<DB> Create(const std::string& path);
    ~DB();

    int InsertProduct(const Ingredient& ingr);
    int InsertTableware(const Tableware& tw);
    std::vector<Ingredient> GetIngredients();
    std::vector<Tableware> GetTableware();

   private:
    explicit DB(sqlite3* db) : db_(db) {}

    int Insert(const char* sql);

    template <class T>
    std::vector<T> SelectAll(const char* sql);

    sqlite3* db_;
};
