#include "db/db.h"

#include <gmock/gmock-matchers.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace foodculator {
namespace {

TEST(DB, EmptyDatabase) {
    auto db = DB::Create(":memory:");

    const size_t UNKNOWN_ID = 100;
    EXPECT_THAT(db->GetProducts(), testing::IsEmpty());
    EXPECT_THAT(db->GetTableware(), testing::IsEmpty());
    EXPECT_TRUE(db->DeleteProduct(UNKNOWN_ID));
    EXPECT_TRUE(db->DeleteTableware(UNKNOWN_ID));

    EXPECT_THAT(db->GetRecipes(), testing::IsEmpty());
    EXPECT_TRUE(db->DeleteRecipe(UNKNOWN_ID));
    EXPECT_FALSE(db->GetRecipeInfo(UNKNOWN_ID).has_value());
}

TEST(DB, AddDeleteProduct) {
    auto db = DB::Create(":memory:");

    std::vector<Ingredient> want = {
        {"milk", 48},
        {"white sugar", 384},
        {"see-salt", 0},
        {"avocado", 160},
    };
    for (auto& v : want) {
        auto [code, id] = db->AddProduct(v.name, v.kcal);
        ASSERT_EQ(code, DB::Result::OK) << "AddProduct(" << v.name << ", " << v.kcal << ")";
        v.id = id;
    }

    const auto& dup = want[0];
    ASSERT_EQ(db->AddProduct(dup.name, dup.kcal).code, DB::Result::INVALID_ARGUMENT)
        << "duplicate insertion should fail";
    ASSERT_THAT(db->GetProducts(), testing::UnorderedElementsAreArray(want));

    while (!want.empty()) {
        size_t last_id = want.back().id;
        want.pop_back();

        ASSERT_TRUE(db->DeleteProduct(last_id)) << last_id;
        ASSERT_THAT(db->GetProducts(), testing::UnorderedElementsAreArray(want));
    }
}

TEST(DB, DeleteProduct_WithRecipe) {
    auto db = DB::Create(":memory:");

    auto milk_id = db->AddProduct("milk", 48).id;
    auto flour_id = db->AddProduct("flour", 364).id;
    auto pancake_id = db->CreateRecipe("pancake", "do it", {{milk_id, 500}, {flour_id, 200}}).id;

    EXPECT_FALSE(db->DeleteProduct(milk_id));
}

TEST(DB, AddDeleteTableware) {
    auto db = DB::Create(":memory:");

    std::vector<Tableware> want = {
        {"wok", 1080},
        {"small pot", 6},
        {"big-pot", 10},
    };
    for (auto& v : want) {
        auto [code, id] = db->AddTableware(v.name, v.weight);
        ASSERT_EQ(code, DB::Result::OK) << "AddTableware(" << v.name << ", " << v.weight << ")";
        v.id = id;
    }

    const auto& dup = want[0];
    ASSERT_EQ(db->AddTableware(dup.name, dup.weight).code, DB::Result::INVALID_ARGUMENT)
        << "duplicate insertion should fail";
    ASSERT_THAT(db->GetTableware(), testing::UnorderedElementsAreArray(want));

    while (!want.empty()) {
        size_t last_id = want.back().id;
        want.pop_back();

        ASSERT_TRUE(db->DeleteTableware(last_id)) << last_id;
        ASSERT_THAT(db->GetTableware(), testing::UnorderedElementsAreArray(want));
    }
}

TEST(DB, GetRecipeInfo) {
    auto db = DB::Create(":memory:");

    auto milk_id = db->AddProduct("milk", 48).id;
    auto flour_id = db->AddProduct("flour", 364).id;
    auto egg_id = db->AddProduct("egg", 156).id;

    auto pancake_id =
        db->CreateRecipe("pancake", "do it", {{milk_id, 500}, {flour_id, 200}, {egg_id, 0}}).id;

    auto got = db->GetRecipeInfo(pancake_id);
    ASSERT_TRUE(got);
    EXPECT_EQ(got->header.name, "pancake");
    EXPECT_EQ(got->header.id, pancake_id);
    EXPECT_EQ(got->description, "do it");
    EXPECT_THAT(got->ingredients, testing::UnorderedElementsAre(RecipeIngredient(milk_id, 500),
                                                                RecipeIngredient(flour_id, 200)))
        << "all non-zero weight ingredients should be present";
}

TEST(DB, CreateRecipe_Duplicate) {
    auto db = DB::Create(":memory:");

    auto milk_id = db->AddProduct("milk", 48).id;
    auto flour_id = db->AddProduct("flour", 364).id;

    ASSERT_EQ(db->CreateRecipe("pie", "3.14", {{milk_id, 500}, {flour_id, 200}}).code,
              DB::Result::OK);
    ASSERT_EQ(db->CreateRecipe("pie", "3.14 inserting again", {{flour_id, 100}}).code,
              DB::Result::INVALID_ARGUMENT);
}

TEST(DB, CreateRecipe_UnknownIngredient) {
    auto db = DB::Create(":memory:");

    auto milk_id = db->AddProduct("milk", 48).id;
    ASSERT_EQ(db->CreateRecipe("name", "unknown ingredient id", {{milk_id, 100}, {-1, 200}}).code,
              DB::Result::INVALID_ARGUMENT);
    EXPECT_THAT(db->GetRecipes(), testing::IsEmpty())
        << "completely roll back after unsuccessfull CreateRecipe";
}

TEST(DB, CreateDeleteRecipe) {
    auto db = DB::Create(":memory:");

    auto milk_id = db->AddProduct("milk", 48).id;
    auto flour_id = db->AddProduct("flour", 364).id;
    auto egg_id = db->AddProduct("egg", 156).id;

    struct {
        std::string name;
        std::string descr;
        std::map<size_t, uint32_t> ingredients;
    } recipes[] = {
        {"pancake", "do it", {{milk_id, 500}, {flour_id, 200}}},
        {"new cake", "description", {{egg_id, 0}, {flour_id, 50}}},
        {"just salt", "easy-peasy", {{egg_id, 200}}},
    };

    std::vector<RecipeHeader> recipe_headers;
    for (const auto& [name, descr, ingredients] : recipes) {
        auto [code, id] = db->CreateRecipe(name, descr, ingredients);
        EXPECT_EQ(code, DB::Result::OK)
            << "CreateRecipe(name=" << name << ", description=" << descr
            << ", ingredients=" << testing::PrintToString(ingredients) << ")";
        recipe_headers.emplace_back(name, id);
    }

    EXPECT_THAT(db->GetRecipes(), testing::UnorderedElementsAreArray(recipe_headers));

    while (!recipe_headers.empty()) {
        size_t last_id = recipe_headers.back().id;
        recipe_headers.pop_back();

        EXPECT_TRUE(db->DeleteRecipe(last_id)) << last_id;
        EXPECT_THAT(db->GetRecipes(), testing::UnorderedElementsAreArray(recipe_headers));
    }
}

}  // namespace
}  // namespace foodculator
