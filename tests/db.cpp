#include "db/db.h"

#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"

namespace foodculator {

std::optional<std::string> AddProducts(DB* db, std::vector<Ingredient>* products) {
    for (auto& ingr : *products) {
        auto st = db->AddProduct(ingr.name, ingr.kcal);
        if (st.code != DB::Result::OK) {
            std::stringstream err;
            err << "AddProduct(" << ingr.name << ", " << ingr.kcal << ") = " << st.code << ". Want "
                << DB::Result::OK;
            return err.str();
        }
        ingr.id = st.id;
    }
    return std::nullopt;
}

std::optional<std::string> AddTableware(DB* db, std::vector<Tableware>* tableware) {
    for (auto& tw : *tableware) {
        auto st = db->AddTableware(tw.name, tw.weight);
        if (st.code != DB::Result::OK) {
            std::stringstream err;
            err << "AddTableware(" << tw.name << ", " << tw.weight << ") = " << st.code << ". Want "
                << DB::Result::OK;
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
std::optional<std::string> CompareVectors(std::string_view func, std::vector<T> got,
                                          std::vector<T> want) {
    std::sort(want.begin(), want.end(), [](const T& lhs, const T& rhs) { return lhs.id < rhs.id; });
    std::sort(got.begin(), got.end(), [](const T& lhs, const T& rhs) { return lhs.id < rhs.id; });

    auto want_str = ToString(want);
    auto got_str = ToString(got);
    if (want_str != got_str) {
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
        err << "GetProducts() from empty DB = " << ToString(ingredients) << ". Want: []";
        return err.str();
    }

    if (auto tableware = db->GetTableware(); !tableware.empty()) {
        std::stringstream err;
        err << "GetTableware() from empty DB = " << ToString(tableware) << ". Want: []";
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
        err << "DeleteTableware(" << id << ") from empty DB = false. Want true.";
        return err.str();
    }

    // TODO Check Recipes

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
    if (auto st = db->AddProduct(dupl.name, dupl.kcal); st.code != DB::Result::DUPLICATE) {
        std::stringstream err;
        err << "AddProduct(" << dupl.name << ", " << dupl.kcal << ") = " << st.code << ". Want "
            << DB::Result::DUPLICATE;
        return err.str();
    }

    return CompareVectors("GetProducts", db->GetProducts(), std::move(products));
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

        if (auto st = CompareVectors("GetProducts", db->GetProducts(), products); st) {
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
    if (auto st = db->AddTableware(dupl.name, dupl.weight); st.code != DB::Result::DUPLICATE) {
        std::stringstream err;
        err << "AddTableware(" << dupl.name << ", " << dupl.weight << ") = " << st.code << ". Want "
            << DB::Result::DUPLICATE;
        return err.str();
    }

    return CompareVectors("GetTableware", db->GetTableware(), std::move(tableware));
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

        if (auto st = CompareVectors("GetTableware", db->GetTableware(), tableware); st) {
            return st;
        }
    }

    return std::nullopt;
}

std::optional<std::string> TestCreateRecipe() {
    // TODO implement
    auto db = DB::Create(":memory:");

    std::vector<Ingredient> products = {
        {"milk", 48},
        {"flour", 364},
        {"egg", 156},
    };
    if (auto st = AddProducts(db.get(), &products); st) {
        return st;
    }

    std::vector<FullRecipe> recipes = {
        {{"pancake"},
         "do it",
         {
             {products[0].id, 500},
             {products[1].id, 200},
         }},
        {{"new cake"},
         "description",
         {
             {products.back().id, 200},
             {products[0].id, 50},
         }},
        {{"just salt"},
         "easy-peasy",
         {
             {products.back().id, 200},
         }},
    };
    std::vector<RecipeHeader> recipe_headers;
    for (auto& recipe : recipes) {
        std::map<size_t, uint32_t> ingredients;
        for (const auto& ing : recipe.ingredients) {
            ingredients[ing.ingredient_id] = ing.weight;
        }
        auto st = db->CreateRecipe(recipe.header.name, recipe.description, ingredients);
        if (st.code != DB::Result::OK) {
            std::stringstream err;
            err << "CreateRecipe(" << recipe.header.name << ", " << recipe.description << ", "
                << ToString(recipe.ingredients) << ") = " << st.code << ". Want " << DB::Result::OK;
            return err.str();
        }
        recipe.header.id = st.id;
        recipe_headers.emplace_back(recipe.header.name, st.id);
    }

    if (auto st = CompareVectors("GetRecipes", db->GetRecipes(), recipe_headers); st) {
        return st;
    }

    return std::nullopt;
}

std::optional<std::string> TestDeleteRecipe() {
    auto db = DB::Create(":memory:");

    std::vector<Ingredient> products = {
        {"milk", 48},
        {"flour", 364},
        {"egg", 156},
        {"salt", 0},
    };
    if (auto st = AddProducts(db.get(), &products); st) {
        return st;
    }

    std::vector<FullRecipe> recipes = {
        {{"pancake"},
         "do it",
         {
             {products[0].id, 500},
             {products[1].id, 200},
             {products[2].id, 300},
         }},
        {{"new cake"},
         "description",
         {
             {products.back().id, 200},
             {products[0].id, 50},
         }},
        {{"new cake 2"},
         "description 2",
         {
             {products.back().id, 200},
             {products[0].id, 50},
         }},
    };

    std::vector<RecipeHeader> recipe_headers;
    for (const auto& recipe : recipes) {
        std::map<size_t, uint32_t> ingredients;
        for (const auto& ing : recipe.ingredients) {
            ingredients[ing.ingredient_id] = ing.weight;
        }
        auto st = db->CreateRecipe(recipe.header.name, recipe.description, ingredients);
        if (st.code != DB::Result::OK) {
            std::stringstream err;
            err << "CreateRecipe(" << recipe.header.name << ", " << recipe.description << ", "
                << ToString(recipe.ingredients) << ") = " << st.code << ". Want " << DB::Result::OK;
            return err.str();
        }
        recipe_headers.emplace_back(recipe.header.name, st.id);
    }

    if (auto st = CompareVectors("GetRecipes", db->GetRecipes(), recipe_headers); st) {
        return st;
    }

    while (!recipe_headers.empty()) {
        size_t last_id = recipe_headers.back().id;
        recipe_headers.pop_back();

        if (!db->DeleteRecipe(last_id)) {
            std::stringstream err;
            err << "DeleteRecipe(" << last_id << ")=false. Want true.";
            return err.str();
        }

        if (auto st = CompareVectors("GetRecipes", db->GetRecipes(), recipe_headers); st) {
            return st;
        }
    }

    return std::nullopt;
}

}  // namespace foodculator

TEST(Legacy, AllOldTests) {
    using namespace foodculator;
    std::map<std::string_view, std::function<std::optional<std::string>()>> tests = {
        {"TestEmptyDatabase", TestEmptyDatabase},     {"TestAddProduct", TestAddProduct},
        {"TestDeleteProduct", TestDeleteProduct},     {"TestAddTableware", TestAddTableware},
        {"TestDeleteTableware", TestDeleteTableware}, {"TestCreateRecipe", TestCreateRecipe},
        {"TestDeleteRecipe", TestDeleteRecipe},
    };

    for (const auto& [name, fn] : tests) {
        auto res = fn();
        EXPECT_FALSE(res) << name << ": " << res.value();
    }
}
