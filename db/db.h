#include <sqlite3.h>

#include <string>

class DBCreateException : public std::exception {
   public:
    const char* what() const throw() override {
        return "It is impossible to open DB or create table.";
    }
};

struct Ingredient {
    std::string name_;
    uint32_t kcal_;
    Ingredient(const std::string& name_, uint32_t kcal_);
};

struct Tableware {
    std::string name_;
    uint32_t weight_;
    Tableware(const std::string& name_, uint32_t weight_);
};

class DB {
   public:
    DB(const std::string& path);
    ~DB();

    void AddNewProduct(const Ingredient& ingr);

   private:
    sqlite3* db;
};
