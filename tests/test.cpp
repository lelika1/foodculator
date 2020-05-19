#include <cstddef>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>

#include "db/db.h"

namespace foodculator {

std::optional<std::string> AddProducts(DB* db,
                                       std::vector<Ingredient>* products) {
    for (auto& ingr : *products) {
        auto st = db->AddProduct(ingr.name, ingr.kcal);
        if (st.code != DB::Result::OK) {
            std::stringstream err;
            err << "AddProduct(" << ingr.name << ", " << ingr.kcal
                << ") = " << st.code << ". Want " << DB::Result::OK;
            return err.str();
        }
        ingr.id = st.id;
    }
    return std::nullopt;
}

std::optional<std::string> AddTableware(DB* db,
                                        std::vector<Tableware>* tableware) {
    for (auto& tw : *tableware) {
        auto st = db->AddTableware(tw.name, tw.weight);
        if (st.code != DB::Result::OK) {
            std::stringstream err;
            err << "AddTableware(" << tw.name << ", " << tw.weight
                << ") = " << st.code << ". Want " << DB::Result::OK;
            return err.str();
        }
        tw.id = st.id;
    }
    return std::nullopt;
}

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

template <class T>
std::optional<std::string> CompareVectors(std::string_view func,
                                          std::vector<T> got,
                                          std::vector<T> want) {
    std::sort(want.begin(), want.end(),
              [](const T& lhs, const T& rhs) { return lhs.id < rhs.id; });

    std::sort(got.begin(), got.end(),
              [](const T& lhs, const T& rhs) { return lhs.id < rhs.id; });

    auto want_str = ToString(want);
    auto got_str = ToString(got);
    if (got.size() != want.size() || want_str != got_str) {
        std::stringstream err;
        err << func << "() = " << got_str << ". Want " << want_str;
        return err.str();
    }

    return std::nullopt;
}

std::optional<std::string> TestEmptyDatabase() {
    auto db = DB::Create(":memory:");

    if (auto ingredients = db->GetProducts(); !ingredients.empty()) {
        std::stringstream err;
        err << "GetProducts() from empty DB = " << ToString(ingredients)
            << ". Want: []";
        return err.str();
    }

    if (auto tableware = db->GetTableware(); !tableware.empty()) {
        std::stringstream err;
        err << "GetTableware() from empty DB = " << ToString(tableware)
            << ". Want: []";
        return err.str();
    }

    const size_t id = 100;
    if (!db->DeleteProduct(id)) {
        std::stringstream err;
        err << "DeleteProduct(" << id << ") from empty DB = false. Want true.";
        return err.str();
    }

    if (!db->DeleteTableware(id)) {
        std::stringstream err;
        err << "DeleteTableware(" << id
            << ") from empty DB = false. Want true.";
        return err.str();
    }

    return std::nullopt;
}

std::optional<std::string> TestAddProduct() {
    auto db = DB::Create(":memory:");

    std::vector<Ingredient> products = {
        {"milk", 48},
        {"white sugar", 384},
        {"see-salt", 0},
        {"avocado", 160},
    };

    if (auto st = AddProducts(db.get(), &products); st) {
        return st;
    }

    const auto& dupl = products[0];
    auto st = db->AddProduct(dupl.name, dupl.kcal);
    if (st.code != DB::Result::DUPLICATE) {
        std::stringstream err;
        err << "AddProduct(" << dupl.name << ", " << dupl.kcal
            << ") = " << st.code << ". Want " << DB::Result::DUPLICATE;
        return err.str();
    }

    return CompareVectors("GetProducts", db->GetProducts(),
                          std::move(products));
}

std::optional<std::string> TestDeleteProduct() {
    auto db = DB::Create(":memory:");

    std::vector<Ingredient> products = {
        {"milk", 48},
        {"white sugar", 384},
        {"see-salt", 0},
        {"avocado", 160},
    };
    if (auto st = AddProducts(db.get(), &products); st) {
        return st;
    }

    while (!products.empty()) {
        size_t last_id = products.back().id;
        products.pop_back();

        if (!db->DeleteProduct(last_id)) {
            std::stringstream err;
            err << "DeleteProduct(" << last_id << ")=false. Want true.";
            return err.str();
        }

        if (auto st =
                CompareVectors("GetProducts", db->GetProducts(), products);
            st) {
            return st;
        }
    }

    return std::nullopt;
}

std::optional<std::string> TestAddTableware() {
    auto db = DB::Create(":memory:");

    std::vector<Tableware> tableware = {
        {"wok", 1080},
        {"small pot", 670},
        {"big-pot", 1000},
    };
    if (auto st = AddTableware(db.get(), &tableware); st) {
        return st;
    }

    const auto& dupl = tableware[0];
    auto st = db->AddTableware(dupl.name, dupl.weight);
    if (st.code != DB::Result::DUPLICATE) {
        std::stringstream err;
        err << "AddTableware(" << dupl.name << ", " << dupl.weight
            << ") = " << st.code << ". Want " << DB::Result::DUPLICATE;
        return err.str();
    }

    return CompareVectors("GetTableware", db->GetTableware(),
                          std::move(tableware));
}

std::optional<std::string> TestDeleteTableware() {
    auto db = DB::Create(":memory:");

    std::vector<Tableware> tableware = {
        {"wok", 1080},
        {"small pot", 670},
        {"big-pot", 1000},
    };
    if (auto st = AddTableware(db.get(), &tableware); st) {
        return st;
    }

    while (!tableware.empty()) {
        size_t last_id = tableware.back().id;
        tableware.pop_back();

        if (!db->DeleteTableware(last_id)) {
            std::stringstream err;
            err << "DeleteTableware(" << last_id << ")=false. Want true.";
            return err.str();
        }

        if (auto st =
                CompareVectors("GetTableware", db->GetTableware(), tableware);
            st) {
            return st;
        }
    }

    return std::nullopt;
}

}  // namespace foodculator

int main() {
    using namespace foodculator;
    std::map<std::string_view, std::function<std::optional<std::string>()>>
        tests = {
            {"TestEmptyDatabase", TestEmptyDatabase},
            {"TestAddProduct", TestAddProduct},
            {"TestDeleteProduct", TestDeleteProduct},
            {"TestAddTableware", TestAddTableware},
            {"TestDeleteTableware", TestDeleteTableware},
        };

    bool failed = false;
    for (const auto& [name, fn] : tests) {
        auto res = fn();
        if (res) {
            failed = true;
        }
        std::cout << name << ": " << res.value_or("passed") << std::endl;
    }
    return failed ? 1 : 0;
}
