#include "db/db.h"

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

    auto products = db->GetProducts();
    ASSERT_TRUE(products.Ok()) << "GetProducts() = {code: " << ToString(products.Code())
                               << ", error: " << products.Error() << "};";
    EXPECT_THAT(products.Value(), testing::IsEmpty());

    auto product = db->GetProduct(UNKNOWN_ID);
    ASSERT_FALSE(product.Ok());
    EXPECT_EQ(product.Code(), StatusCode::NOT_FOUND);

    EXPECT_TRUE(db->DeleteProduct(UNKNOWN_ID));

    auto tw = db->GetTableware();
    ASSERT_TRUE(tw.Ok()) << "GetTableware() = {code: " << ToString(tw.Code())
                         << ", error: " << tw.Error() << "};";
    EXPECT_THAT(tw.Value(), testing::IsEmpty());

    EXPECT_TRUE(db->DeleteTableware(UNKNOWN_ID));

    auto recipes = db->GetRecipes();
    ASSERT_TRUE(recipes.Ok()) << "GetRecipes() = {code: " << ToString(recipes.Code())
                              << ", error: " << recipes.Error() << "};";
    EXPECT_THAT(recipes.Value(), testing::IsEmpty());

    EXPECT_TRUE(db->DeleteRecipe(UNKNOWN_ID));

    auto recipe = db->GetRecipeInfo(UNKNOWN_ID);
    ASSERT_FALSE(recipe.Ok());
    EXPECT_EQ(recipe.Code(), StatusCode::NOT_FOUND);
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
        auto st = db->AddProduct(v.name, v.kcal);
        ASSERT_TRUE(st.Ok()) << "AddProduct(" << v.name << ", " << v.kcal
                             << ") = {code: " << ToString(st.Code()) << ", error: " << st.Error()
                             << "};";
        v.id = st.Value();
    }

    const auto& dup = want[0];
    auto dup_st = db->AddProduct(dup.name, dup.kcal);
    ASSERT_FALSE(dup_st.Ok()) << "duplicate insertion should fail";
    ASSERT_EQ(dup_st.Code(), StatusCode::INVALID_ARGUMENT) << "duplicate insertion should fail";

    auto all = db->GetProducts();
    ASSERT_TRUE(all.Ok()) << "GetProducts() = {code: " << ToString(all.Code())
                          << ", error: " << all.Error() << "};";
    ASSERT_THAT(all.Value(), testing::UnorderedElementsAreArray(want));

    while (!want.empty()) {
        size_t last_id = want.back().id;
        want.pop_back();

        ASSERT_TRUE(db->DeleteProduct(last_id)) << last_id;

        auto products = db->GetProducts();
        ASSERT_TRUE(products.Ok()) << "GetProducts() = {code: " << ToString(products.Code())
                                   << ", error: " << products.Error() << "};";
        ASSERT_THAT(products.Value(), testing::UnorderedElementsAreArray(want));
    }
}

TEST(DB, GetProduct) {
    auto db = DB::Create(":memory:");

    std::string name = "milk";
    std::uint32_t kcal = 48;

    auto st = db->AddProduct(name, kcal);
    ASSERT_TRUE(st.Ok()) << "AddProduct(" << name << ", " << kcal
                         << ") = {code: " << ToString(st.Code()) << ", error: " << st.Error()
                         << "};";

    auto product = db->GetProduct(st.Value());
    ASSERT_TRUE(product.Ok()) << "GetProduct(" << st.Value()
                              << ") = {code: " << ToString(product.Code())
                              << ", error: " << product.Error() << "};";

    EXPECT_EQ(product.Value().name, name);
    EXPECT_EQ(product.Value().kcal, kcal);
    EXPECT_EQ(product.Value().id, st.Value());
}

TEST(DB, DeleteProduct_WithRecipe) {
    auto db = DB::Create(":memory:");

    auto milk_id = db->AddProduct("milk", 48).Value();
    auto flour_id = db->AddProduct("flour", 364).Value();

    auto pancake_id =
        db->CreateRecipe("pancake", "do it", {{milk_id, 500}, {flour_id, 200}}).Value();

    ASSERT_FALSE(db->DeleteProduct(milk_id));
}

TEST(DB, AddDeleteTableware) {
    auto db = DB::Create(":memory:");

    std::vector<Tableware> want = {
        {"wok", 1080},
        {"small pot", 6},
        {"big-pot", 10},
    };
    for (auto& v : want) {
        auto st = db->AddTableware(v.name, v.weight);
        ASSERT_TRUE(st.Ok()) << "AddTableware(" << v.name << ", " << v.weight
                             << ") = {code: " << ToString(st.Code()) << ", error: " << st.Error()
                             << "};";
        v.id = st.Value();
    }

    const auto& dup = want[0];
    auto dup_st = db->AddTableware(dup.name, dup.weight);
    ASSERT_FALSE(dup_st.Ok()) << "duplicate insertion should fail";
    ASSERT_EQ(dup_st.Code(), StatusCode::INVALID_ARGUMENT) << "duplicate insertion should fail ";

    auto all = db->GetTableware();
    ASSERT_TRUE(all.Ok()) << "GetTableware() = {code: " << ToString(all.Code())
                          << ", error: " << all.Error() << "};";
    ASSERT_THAT(all.Value(), testing::UnorderedElementsAreArray(want));

    while (!want.empty()) {
        size_t last_id = want.back().id;
        want.pop_back();

        ASSERT_TRUE(db->DeleteTableware(last_id)) << last_id;

        auto tw = db->GetTableware();
        ASSERT_TRUE(tw.Ok()) << "GetTableware() = {code: " << ToString(tw.Code())
                             << ", error: " << tw.Error() << "};";
        ASSERT_THAT(tw.Value(), testing::UnorderedElementsAreArray(want));
    }
}

TEST(DB, GetRecipeInfo) {
    auto db = DB::Create(":memory:");

    auto milk_id = db->AddProduct("milk", 48).Value();
    auto flour_id = db->AddProduct("flour", 364).Value();
    auto egg_id = db->AddProduct("egg", 156).Value();

    auto pancake_id =
        db->CreateRecipe("pancake", "do it", {{milk_id, 500}, {flour_id, 200}, {egg_id, 0}});
    ASSERT_TRUE(pancake_id.Ok())
        << "CreateRecipe(pancake, do it, {{milk_id, 500}, {flour_id, 200}, {egg_id, 0}}) = {code: "
        << ToString(pancake_id.Code()) << ", error: " << pancake_id.Error() << "};";

    auto got = db->GetRecipeInfo(pancake_id.Value());
    ASSERT_TRUE(got.Ok()) << "GetRecipeInfo(" << pancake_id.Value()
                          << ") = {code: " << ToString(got.Code()) << ", error: " << got.Error()
                          << "};";
    EXPECT_EQ(got.Value().header.name, "pancake");
    EXPECT_EQ(got.Value().header.id, pancake_id.Value());
    EXPECT_EQ(got.Value().description, "do it");
    EXPECT_THAT(got.Value().ingredients,
                testing::UnorderedElementsAre(RecipeIngredient(milk_id, 500),
                                              RecipeIngredient(flour_id, 200)))
        << "all non-zero weight ingredients should be present";
}

TEST(DB, CreateRecipe_Duplicate) {
    auto db = DB::Create(":memory:");

    auto milk_id = db->AddProduct("milk", 48).Value();
    auto flour_id = db->AddProduct("flour", 364).Value();

    ASSERT_TRUE(db->CreateRecipe("pie", "3.14", {{milk_id, 500}, {flour_id, 200}}).Ok());
    ASSERT_EQ(db->CreateRecipe("pie", "3.14 inserting again", {{flour_id, 100}}).Code(),
              StatusCode::INVALID_ARGUMENT);
}

TEST(DB, CreateRecipe_UnknownIngredient) {
    auto db = DB::Create(":memory:");

    auto milk_id = db->AddProduct("milk", 48).Value();

    ASSERT_EQ(db->CreateRecipe("name", "unknown ingredient id", {{milk_id, 100}, {-1, 200}}).Code(),
              StatusCode::INVALID_ARGUMENT);

    auto recipes = db->GetRecipes();
    ASSERT_TRUE(recipes.Ok()) << "GetRecipes() = {code: " << ToString(recipes.Code())
                              << ", error: " << recipes.Error() << "};";
    ASSERT_THAT(recipes.Value(), testing::IsEmpty())
        << "completely roll back after unsuccessfull CreateRecipe";
}

TEST(DB, CreateDeleteRecipe) {
    auto db = DB::Create(":memory:");

    auto milk_id = db->AddProduct("milk", 48).Value();
    auto flour_id = db->AddProduct("flour", 364).Value();
    auto egg_id = db->AddProduct("egg", 156).Value();

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
        auto recipe = db->CreateRecipe(name, descr, ingredients);
        ASSERT_TRUE(recipe.Ok()) << "CreateRecipe(name=" << name << ", description=" << descr
                                 << ", ingredients=" << testing::PrintToString(ingredients)
                                 << ") = {code: " << ToString(recipe.Code())
                                 << ", error: " << recipe.Error() << "};";
        recipe_headers.emplace_back(name, recipe.Value());
    }

    auto all = db->GetRecipes();
    ASSERT_TRUE(all.Ok()) << "GetRecipes() = {code: " << ToString(all.Code())
                          << ", error: " << all.Error() << "};";
    ASSERT_THAT(all.Value(), testing::UnorderedElementsAreArray(recipe_headers));

    while (!recipe_headers.empty()) {
        size_t last_id = recipe_headers.back().id;
        recipe_headers.pop_back();

        ASSERT_TRUE(db->DeleteRecipe(last_id)) << last_id;
        auto recipes = db->GetRecipes();
        ASSERT_TRUE(recipes.Ok()) << "GetRecipes() = {code: " << ToString(recipes.Code())
                                  << ", error: " << recipes.Error() << "};";
        ASSERT_THAT(recipes.Value(), testing::UnorderedElementsAreArray(recipe_headers));
    }
}

}  // namespace
}  // namespace foodculator
