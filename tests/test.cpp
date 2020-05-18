#include <functional>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

#include "db/db.h"

namespace foodculator {

template <class T>
std::string ToString(const std::vector<T>& data) {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i != 0) {
            ss << ",";
        }
        ss << data[i].ToString();
    }
    ss << "]";
    return ss.str();
}

std::optional<std::string> TestEmptyDatabase() {
    auto db = DB::Create(":memory:");
    auto ingredients = db->GetProducts();
    if (!ingredients.empty()) {
        std::stringstream err;
        err << "GetProducts() from empty DB = " << ToString(ingredients)
            << ". Want: []";
        return err.str();
    }

    auto tableware = db->GetTableware();
    if (!tableware.empty()) {
        std::stringstream err;
        err << "GetTableware() from empty DB = " << ToString(tableware)
            << ". Want: []";
        return err.str();
    }
    return {};
}

std::optional<std::string> TestAddProduct() {
    auto db = DB::Create(":memory:");

    std::vector<Ingredient> ingredients = {
        {"milk", 48}, {"sugar", 384}, {"salt", 0}, {"avocado", 160}};

    for (auto& ingr : ingredients) {
        auto st = db->AddProduct(ingr.name, ingr.kcal);
        if (st.code != DB::Result::OK) {
            std::stringstream err;
            err << "AddProduct(" << ingr.name << ", " << ingr.kcal
                << ") = " << st.code << ". Want " << DB::Result::OK;
            return err.str();
        }
        ingr.id = st.id;
    }

    const auto& dupl = ingredients[0];
    auto st = db->AddProduct(dupl.name, dupl.kcal);
    if (st.code != DB::Result::DUPLICATE) {
        std::stringstream err;
        err << "AddProduct(" << dupl.name << ", " << dupl.kcal
            << ") = " << st.code << ". Want " << DB::Result::DUPLICATE;
        return err.str();
    }

    std::sort(ingredients.begin(), ingredients.end(),
              [](const Ingredient& lhs, const Ingredient& rhs) {
                  return lhs.id < rhs.id;
              });

    auto got = db->GetProducts();
    std::sort(got.begin(), got.end(),
              [](const Ingredient& lhs, const Ingredient& rhs) {
                  return lhs.id < rhs.id;
              });

    auto want_str = ToString(ingredients);
    auto got_str = ToString(got);

    if (got.size() != ingredients.size() || want_str != got_str) {
        std::stringstream err;
        err << "GetProducts() = " << got_str << ". Want " << want_str;
        return err.str();
    }

    return {};
}

std::optional<std::string> TestDeleteProduct() {
    // TODO implement
    auto db = DB::Create(":memory:");
    return {};
}

std::optional<std::string> TestAddTableware() {
    // TODO implement
    auto db = DB::Create(":memory:");
    return {};
}

std::optional<std::string> TestDeleteTableware() {
    // TODO implement
    auto db = DB::Create(":memory:");
    return {};
}

}  // namespace foodculator

int main() {
    using namespace foodculator;
    using TestFunction = std::function<std::optional<std::string>(void)>;
    std::vector<std::pair<TestFunction, std::string_view>> tests = {
        {TestEmptyDatabase, "TestEmptyDatabase"},
        {TestAddProduct, "TestAddProduct"},
        {TestDeleteProduct, "TestDeleteProduct"},
        {TestAddTableware, "TestAddTableware"},
        {TestDeleteTableware, "TestDeleteTableware"}};

    bool failed = false;
    for (const auto& t : tests) {
        auto res = t.first();
        if (res.has_value()) {
            failed = true;
        }
        std::cout << t.second << ": " << res.value_or("passed") << std::endl;
    }
    return failed ? 1 : 0;
}
