#include <csignal>
#include <fstream>
#include <streambuf>
#include <string>
#include <unordered_map>

#include "db/db.h"
#include "fmt/format.h"
#include "httplib.h"
#include "json11/json11.hpp"
#include "tgbot/tgbot.h"

namespace {
std::string ReadHtml(const std::string& path) {
    std::ifstream in(path);
    std::string str{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    in.close();
    return str;
}

void ReplyErr(std::string msg, int status, httplib::Response* res) {
    res->set_content(std::move(msg), "text/plain");
    res->status = status;
}

// TODO(luckygeck): Move to separate file.
std::string RenderDialogflowResponse(std::string text) {
    std::vector<json11::Json> jsons;
    auto simpleResponse =
        json11::Json::object{{"simpleResponse", json11::Json::object{{"textToSpeech", text}}}};

    json11::Json ret = json11::Json::object{
        {"fulfillmentMessages",
         json11::Json::array{
             json11::Json::object{
                 {"text", json11::Json::object{{"text", json11::Json::array{std::move(text)}}}}},
         }},
        {
            "payload",
            json11::Json::object{
                {"google", json11::Json::object{{
                               "richResponse",
                               json11::Json::object{
                                   {"items", json11::Json::array{std::move(simpleResponse)}}},
                           }}}},
        },
    };
    return ret.dump();
}

}  // namespace

httplib::Server* server = nullptr;

void signal_handler(int signal) {
    if (server) {
        server->stop();
    }
}

int main(int argc, char** argv) {
    using foodculator::DB;

    if (argc != 3) {
        fmt::print(stderr, "usage: {} path_to_static_files path_to_database\n", argv[0]);
        return 1;
    }

    fmt::print("Working with sqlite db in {}\n", argv[2]);

    auto db = DB::Create(argv[2]);
    if (!db) {
        fmt::print(stderr, "DB::Create({}) failed.\n", argv[2]);
        return 1;
    }

    std::string path_to_static = argv[1];

    httplib::Server srv;

    std::vector<std::pair<const char*, const char*>> html_pages = {
        {"/", "/index.html"},
        {"/ingredients", "/ingredients.html"},
        {"/tableware", "/tableware.html"},
        {"/recipe", "/recipe.html"},
    };

    for (const auto& [page, path] : html_pages) {
        srv.Get(page, [abs_path = (path_to_static + path)](const httplib::Request& req,
                                                           httplib::Response& res) {
            res.set_content(ReadHtml(abs_path), "text/html");
        });
    }

    srv.Get("/get_ingredients", [&db](const httplib::Request& req, httplib::Response& res) {
        auto products = db->GetProducts();
        if (!products.Ok()) {
            ReplyErr(std::move(products.Error()), 500, &res);
            return;
        }
        res.set_content(json11::Json(std::move(products.Value())).dump(), "text/json");
    });

    srv.Post("/add_ingredient", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string err;
        json11::Json input = json11::Json::parse(req.body, err);
        if (!err.empty()) {
            ReplyErr("Failed to parse the request: " + err, 400, &res);
            return;
        }

        std::string name = input["product"].string_value();
        if (name.empty() || !input.has_shape({{"kcal", json11::Json::NUMBER}}, err)) {
            ReplyErr("Ingredient should have `product` (string) and `kcal` (number) fields.", 400,
                     &res);
            return;
        }

        double kcal = input["kcal"].number_value();
        if (kcal < 0.0) {
            ReplyErr("Ingredient cannot have negative `kcal` value.", 400, &res);
            return;
        }

        auto st = db->AddProduct(std::move(name), static_cast<uint32_t>(kcal));
        if (!st.Ok()) {
            ReplyErr(std::move(st.Error()), 500, &res);
            return;
        }
        res.set_content(std::to_string(st.Value()), "text/plain");
    });

    srv.Get(R"(/ingredient/(\d+))", [&db](const httplib::Request& req, httplib::Response& res) {
        size_t id = static_cast<size_t>(std::stoull(req.matches[1].str()));
        auto product = db->GetProduct(id);
        if (!product.Ok()) {
            int code = (product.Code() == foodculator::StatusCode::NOT_FOUND) ? 404 : 500;
            ReplyErr(std::move(product.Error()), code, &res);
            return;
        }
        res.set_content(json11::Json(std::move(product.Value())).dump(), "text/json");
    });

    srv.Delete(R"(/ingredient/(\d+))", [&db](const httplib::Request& req, httplib::Response& res) {
        if (!db->DeleteProduct(std::stoull(req.matches[1].str()))) {
            ReplyErr("DB request failed. Try again later.", 500, &res);
        }
    });

    srv.Get("/get_tableware", [&db](const httplib::Request& req, httplib::Response& res) {
        auto tw = db->GetTableware();
        if (!tw.Ok()) {
            ReplyErr(std::move(tw.Error()), 500, &res);
            return;
        }
        res.set_content(json11::Json(std::move(tw.Value())).dump(), "text/json");
    });

    srv.Post("/add_tableware", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string err;
        json11::Json input = json11::Json::parse(req.body, err);
        if (!err.empty()) {
            ReplyErr("Failed to parse the request: " + err, 400, &res);
            return;
        }

        std::string name = input["name"].string_value();
        if (name.empty() || !input.has_shape({{"weight", json11::Json::NUMBER}}, err)) {
            ReplyErr("The pot should have `name` (string) and `weight` (number) fields.", 400,
                     &res);
            return;
        }
        double weight_double = input["weight"].number_value();
        if (weight_double < 0.0) {
            ReplyErr("The weight couldn't be negative.", 400, &res);
            return;
        }

        uint32_t weight = static_cast<uint32_t>(weight_double);
        auto st = db->AddTableware(std::move(name), weight);
        if (!st.Ok()) {
            ReplyErr(std::move(st.Error()), 500, &res);
            return;
        }
        res.set_content(std::to_string(st.Value()), "text/plain");
    });

    srv.Delete(R"(/tableware/(\d+))", [&db](const httplib::Request& req, httplib::Response& res) {
        if (!db->DeleteTableware(std::stoull(req.matches[1].str()))) {
            ReplyErr("A pot wasn't deleted. Some SQL error occured.", 500, &res);
        }
    });

    srv.Get("/get_recipes", [&db](const httplib::Request& req, httplib::Response& res) {
        auto recipes = db->GetRecipes();
        if (!recipes.Ok()) {
            ReplyErr(std::move(recipes.Error()), 500, &res);
            return;
        }
        res.set_content(json11::Json(std::move(recipes.Value())).dump(), "text/json");
    });

    srv.Post("/create_recipe", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string err;
        json11::Json input = json11::Json::parse(req.body, err);
        if (!err.empty()) {
            ReplyErr("Failed to parse the request: " + err, 400, &res);
            return;
        }

        std::string name = input["header"]["name"].string_value();
        if (name.empty()) {
            ReplyErr("Recipe name should not be empty.", 400, &res);
            return;
        }

        std::map<size_t, uint32_t> ingredients;
        for (const auto& v : input["ingredients"].array_items()) {
            if (!v.has_shape({{"id", json11::Json::NUMBER}, {"weight", json11::Json::NUMBER}},
                             err)) {
                ReplyErr("Each ingredient should have id and weight number fields.", 400, &res);
                return;
            }

            double id = v["id"].number_value();
            double weight = v["weight"].number_value();
            if (id < 0.0 || weight < 0.0) {
                ReplyErr("id and weight must be >= 0.", 400, &res);
                return;
            }

            ingredients[static_cast<size_t>(id)] = static_cast<uint32_t>(weight);
        }

        std::string description = input["description"].string_value();

        auto st = db->CreateRecipe(name, description, ingredients);
        if (!st.Ok()) {
            ReplyErr(std::move(st.Error()), 500, &res);
            return;
        }
        res.set_content(std::to_string(st.Value()), "text/plain");
    });

    srv.Get(R"(/recipe/(\d+))", [&db](const httplib::Request& req, httplib::Response& res) {
        size_t id = std::stoull(req.matches[1].str());
        auto recipe = db->GetRecipeInfo(id);
        if (!recipe.Ok()) {
            int code = (recipe.Code() == foodculator::StatusCode::NOT_FOUND) ? 404 : 500;
            ReplyErr(std::move(recipe.Error()), code, &res);
            return;
        }

        res.set_content(json11::Json(std::move(recipe.Value())).dump(), "text/json");
    });

    srv.Delete(R"(/recipe/(\d+))", [&db](const httplib::Request& req, httplib::Response& res) {
        if (!db->DeleteRecipe(std::stoull(req.matches[1].str()))) {
            ReplyErr("DB request failed. Try again later.", 500, &res);
            return;
        }
    });

    srv.Post("/dialogflow", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string err;
        const json11::Json in = json11::Json::parse(req.body, err);
        if (!err.empty()) {
            ReplyErr("Failed to parse input as json: " + err, 400, &res);
            return;
        }

        const std::string& resp_id = in["responseId"].string_value();
        const std::string& session = in["session"].string_value();
        const auto& query = in["queryResult"];
        const std::string& query_text = query["queryText"].string_value();
        const std::string& intent_name = query["intent"]["displayName"].string_value();

        fmt::print("[dialogflow] id={} session={} query={} intent={}\n", resp_id, session,
                   query_text, intent_name);

        fmt::memory_buffer text;
        if (intent_name == "ingredients") {
            auto products = db->GetProducts();
            if (!products.Ok()) {
                ReplyErr(std::move(products.Error()), 500, &res);
                return;
            }

            fmt::format_to(text, "Наши ингредиенты:");
            for (const auto& ingredient : products.Value()) {
                fmt::format_to(text, "\n{} по {} калории,", ingredient.name, ingredient.kcal);
            }
        } else if (intent_name == "pots") {
            auto tw = db->GetTableware();
            if (!tw.Ok()) {
                ReplyErr(std::move(tw.Error()), 500, &res);
                return;
            }

            fmt::format_to(text, "Наша посуда:");
            for (const auto& pot : tw.Value()) {
                fmt::format_to(text, "\n{} по {} грам,", pot.name, pot.weight);
            }
        } else {
            // This intent is not supported.
            return;
        }
        res.set_content(RenderDialogflowResponse(fmt::to_string(text)), "text/json; charset=utf-8");
    });

    std::string version = "UNKNOWN";
    if (char* v = std::getenv("VERSION"); v) {
        version = v;
    }

    srv.Get("/version", [&version](const httplib::Request& req, httplib::Response& res) {
        res.set_content("Foodculator version: " + version, "text/plain");
    });

    srv.set_mount_point("/static", path_to_static.c_str());

    int port = 1234;
    if (char* v = std::getenv("PORT"); v) {
        port = std::stoi(v);
    }

    fmt::print("Foodculator version: {}\n", version);
    fmt::print("Listening on http://localhost:{}\n", port);

    server = &srv;
    std::signal(SIGTERM, signal_handler);

    srv.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        if (res.status != 200) {
            fmt::print(stderr, "{} {}:\tcode={} content={}\n", req.method, req.path, res.status,
                       res.body);
            return;
        }

        fmt::print("{} {}:\tsize={}b\n", req.method, req.path, res.body.length());
    });
    srv.listen("0.0.0.0", port);

    fmt::print("I'll be back!\n");
    return 0;
}
