#include "db/db.h"

#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace foodculator {
namespace {

void AddProducts(DB* db, std::vector<Ingredient>* products) {
    for (auto& ingr : *products) {
        auto st = db->AddProduct(ingr.name, ingr.kcal);
        ASSERT_EQ(st.code, DB::Result::OK)
            << "AddProduct(" << ingr.name << ", " << ingr.kcal << ")";
        ingr.id = st.id;
    }
}

void AddTableware(DB* db, std::vector<Tableware>* tableware) {
    for (auto& tw : *tableware) {
        auto st = db->AddTableware(tw.name, tw.weight);
        ASSERT_EQ(st.code, DB::Result::OK)
            << "AddTableware(" << tw.name << ", " << tw.weight << ")";
        tw.id = st.id;
    }
}

MATCHER(EqAsStrings, "") {
    return testing::PrintToString(std::get<0>(arg)) == testing::PrintToString(std::get<1>(arg));
}

TEST(DB, EmptyDatabase) {
    auto db = DB::Create(":memory:");

    EXPECT_THAT(db->GetProducts(), testing::IsEmpty());
    EXPECT_THAT(db->GetTableware(), testing::IsEmpty());
    EXPECT_TRUE(db->DeleteProduct(100 /*id*/));
    EXPECT_TRUE(db->DeleteTableware(100 /*id*/));
    // TODO Check Recipes
}

TEST(DB, AddProduct) {
    auto db = DB::Create(":memory:");

    std::vector<Ingredient> products = {
        {"milk", 48},
        {"white sugar", 384},
        {"see-salt", 0},
        {"avocado", 160},
    };

    AddProducts(db.get(), &products);

    const auto& dupl = products[0];
    ASSERT_EQ(db->AddProduct(dupl.name, dupl.kcal).code, DB::Result::INVALID_ARGUMENT)
        << "name=" << dupl.name << " kcal=" << dupl.kcal;
    EXPECT_THAT(db->GetProducts(), testing::UnorderedPointwise(EqAsStrings(), products));
}

TEST(DB, DeleteProduct) {
    auto db = DB::Create(":memory:");

    std::vector<Ingredient> products = {
        {"milk", 48},
        {"white sugar", 384},
        {"see-salt", 0},
        {"avocado", 160},
    };

    AddProducts(db.get(), &products);

    while (!products.empty()) {
        size_t last_id = products.back().id;
        products.pop_back();

        EXPECT_TRUE(db->DeleteProduct(last_id)) << last_id;
        EXPECT_THAT(db->GetProducts(), testing::UnorderedPointwise(EqAsStrings(), products));
    }
}

TEST(DB, AddTableware) {
    auto db = DB::Create(":memory:");

    std::vector<Tableware> tableware = {
        {"wok", 1080},
        {"small pot", 670},
        {"big-pot", 1000},
    };
    AddTableware(db.get(), &tableware);

    const auto& dupl = tableware[0];

    ASSERT_EQ(db->AddTableware(dupl.name, dupl.weight).code, DB::Result::INVALID_ARGUMENT)
        << "name=" << dupl.name << " weight=" << dupl.weight;
    EXPECT_THAT(db->GetTableware(), testing::UnorderedPointwise(EqAsStrings(), tableware));
}

TEST(DB, DeleteTableware) {
    auto db = DB::Create(":memory:");

    std::vector<Tableware> tableware = {
        {"wok", 1080},
        {"small pot", 670},
        {"big-pot", 1000},
    };
    AddTableware(db.get(), &tableware);

    while (!tableware.empty()) {
        size_t last_id = tableware.back().id;
        tableware.pop_back();

        EXPECT_TRUE(db->DeleteTableware(last_id)) << last_id;
        EXPECT_THAT(db->GetTableware(), testing::UnorderedPointwise(EqAsStrings(), tableware));
    }
}

TEST(DB, CreateRecipe) {
    // TODO implement
    auto db = DB::Create(":memory:");

    std::vector<Ingredient> products = {
        {"milk", 48},
        {"flour", 364},
        {"egg", 156},
    };
    AddProducts(db.get(), &products);

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
        EXPECT_EQ(st.code, DB::Result::OK)
            << "CreateRecipe(name=" << recipe.header.name << ", description=" << recipe.description
            << ", ingredients=" << testing::PrintToString(ingredients) << ")";
        recipe.header.id = st.id;
        recipe_headers.emplace_back(recipe.header.name, st.id);
    }
    EXPECT_THAT(db->GetRecipes(), testing::UnorderedPointwise(EqAsStrings(), recipe_headers));
}

TEST(DB, DeleteRecipe) {
    auto db = DB::Create(":memory:");

    std::vector<Ingredient> products = {
        {"milk", 48},
        {"flour", 364},
        {"egg", 156},
        {"salt", 0},
    };
    AddProducts(db.get(), &products);

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
        EXPECT_EQ(st.code, DB::Result::OK)
            << "CreateRecipe(name=" << recipe.header.name << ", description=" << recipe.description
            << ", ingredients=" << testing::PrintToString(ingredients) << ")";
        recipe_headers.emplace_back(recipe.header.name, st.id);
    }

    EXPECT_THAT(db->GetRecipes(), testing::UnorderedPointwise(EqAsStrings(), recipe_headers));

    while (!recipe_headers.empty()) {
        size_t last_id = recipe_headers.back().id;
        recipe_headers.pop_back();

        EXPECT_TRUE(db->DeleteRecipe(last_id)) << last_id;
        EXPECT_THAT(db->GetRecipes(), testing::UnorderedPointwise(EqAsStrings(), recipe_headers));
    }
}

}  // namespace
}  // namespace foodculator
