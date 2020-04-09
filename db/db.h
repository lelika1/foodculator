#include <memory>
#include <string>

class sqlite3;

struct Ingredient {
    std::string name_;
    uint32_t kcal_;

    Ingredient(const std::string& name, uint32_t kcal)
        : name_(name), kcal_(kcal) {}
};

struct Tableware {
    std::string name_;
    uint32_t weight_;

    Tableware(const std::string& name, uint32_t weight)
        : name_(name), weight_(weight) {}
};

class DB {
   public:
    static std::unique_ptr<DB> Create(const std::string& path);
    ~DB();

    void AddNewProduct(const Ingredient& ingr);

   private:
    explicit DB(sqlite3* db) : db_(db) {}

    sqlite3* db_;
};
