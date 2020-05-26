#include <csignal>
#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>
#include <unordered_map>

#include "httplib.h"
#include "db/db.h"
#include "json11/json11.hpp"
#include "tgbot/tgbot.h"

namespace {
std::string ReadHtml(const std::string& path) {
    std::ifstream in(path);
    std::string str{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    in.close();
    return str;
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
        std::cerr << "usage: " << argv[0] << " path_to_static_files path_to_database" << std::endl;
        return 1;
    }

    std::cout << "Working with sqlite db in " << argv[2] << std::endl;

    auto db = DB::Create(argv[2]);
    if (!db) {
        std::cerr << "DB::Create(" << argv[2] << ") failed." << std::endl;
        return 1;
    }

    std::string path_to_static = argv[1];

    httplib::Server srv;

    std::vector<std::pair<const char*, const char*>> html_pages = {
        {"/", "/index.html"},
        {"/ingredients", "/ingredients.html"},
        {"/tableware", "/tableware.html"},
        {"/new_recipe", "/recipe.html"},
    };

    for (const auto& [page, path] : html_pages) {
        srv.Get(page, [abs_path = (path_to_static + path)](const httplib::Request& req,
                                                           httplib::Response& res) {
            res.set_content(ReadHtml(abs_path), "text/html");
        });
    }

    srv.Get("/get_ingredients", [&db](const httplib::Request& req, httplib::Response& res) {
        res.set_content(json11::Json(db->GetProducts()).dump(), "text/json");
    });

    srv.Post("/add_ingredient", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string err;
        json11::Json input = json11::Json::parse(req.body, err);
        if (!err.empty()) {
            res.set_content("Failed to parse the request: " + err, "text/plain");
            res.status = 400;
            return;
        }

        std::string name = input["product"].string_value();
        if (name.empty() || !input.has_shape({{"kcal", json11::Json::NUMBER}}, err)) {
            res.set_content("Ingredient should have `product` (string) and `kcal` (number) fields.",
                            "text/plain");
            res.status = 400;
            return;
        }

        double kcal = input["kcal"].number_value();
        if (kcal < 0.0) {
            res.set_content("Ingredient cannot have negative `kcal` value.", "text/plain");
            res.status = 400;
            return;
        }

        auto [code, id] = db->AddProduct(std::move(name), static_cast<uint32_t>(kcal));
        switch (code) {
            case DB::Result::OK: {
                res.set_content(std::to_string(id), "text/plain");
                return;
            }
            case DB::Result::INVALID_ARGUMENT: {
                res.set_content("This ingredient already exists in the database.", "text/plain");
                res.status = 500;
                return;
            }
            default: {
                res.set_content("DB request failed. Try again later.", "text/plain");
                res.status = 500;
            }
        }
    });

    srv.Get(R"(/ingredient/(\d+))", [&db](const httplib::Request& req, httplib::Response& res) {
        // TODO:: implement
    });

    srv.Delete(R"(/ingredient/(\d+))", [&db](const httplib::Request& req, httplib::Response& res) {
        if (size_t id = std::stoull(req.matches[1].str()); !db->DeleteProduct(id)) {
            res.set_content("DB request failed. Try again later.", "text/plain");
            res.status = 500;
        }
    });

    srv.Get("/get_tableware", [&db](const httplib::Request& req, httplib::Response& res) {
        res.set_content(json11::Json(db->GetTableware()).dump(), "text/json");
    });

    srv.Post("/add_tableware", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string err;
        json11::Json input = json11::Json::parse(req.body, err);
        if (!err.empty()) {
            res.set_content("Failed to parse the request: " + err, "text/plain");
            res.status = 400;
            return;
        }

        std::string name = input["name"].string_value();
        if (name.empty() || !input.has_shape({{"weight", json11::Json::NUMBER}}, err)) {
            res.set_content("Ingredient should have `name` (string) and `weight` (number) fields.",
                            "text/plain");
            res.status = 400;
            return;
        }
        double weight = input["weight"].number_value();
        if (weight < 0.0) {
            res.set_content("The weight couldn't be negative.", "text/plain");
            res.status = 400;
            return;
        }

        auto [code, id] = db->AddTableware(std::move(name), static_cast<uint32_t>(weight));
        switch (code) {
            case DB::Result::OK: {
                res.set_content(std::to_string(id), "text/plain");
                return;
            }
            case DB::Result::INVALID_ARGUMENT: {
                res.set_content("This pot already exists in the database.", "text/plain");
                res.status = 500;
                return;
            }
            default: {
                res.set_content("DB request failed. Try again later.", "text/plain");
                res.status = 500;
            }
        }
    });

    srv.Delete(R"(/tableware/(\d+))", [&db](const httplib::Request& req, httplib::Response& res) {
        if (size_t id = std::stoull(req.matches[1].str()); !db->DeleteTableware(id)) {
            res.set_content("A pot wasn't deleted. Some SQL error occured.", "text/plain");
            res.status = 500;
        }
    });

    srv.Get("/get_recipes", [&db](const httplib::Request& req, httplib::Response& res) {
        res.set_content(json11::Json(db->GetRecipes()).dump(), "text/json");
    });

    srv.Post("/create_recipe", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string err;
        json11::Json input = json11::Json::parse(req.body, err);
        if (!err.empty()) {
            res.set_content("Failed to parse the request: " + err, "text/plain");
            res.status = 400;
            return;
        }

        std::string name = input["header"]["name"].string_value();
        if (name.empty()) {
            res.set_content("Recipe name should not be empty.", "text/plain");
            res.status = 400;
            return;
        }

        std::map<size_t, uint32_t> ingredients;
        for (const auto& v : input["ingredients"].array_items()) {
            if (!v.has_shape({{"id", json11::Json::NUMBER}, {"weight", json11::Json::NUMBER}},
                             err)) {
                res.set_content("Each ingredient should have id and weight number fields.",
                                "text/plain");
                res.status = 400;
                return;
            }

            double id = v["id"].number_value();
            double weight = v["weight"].number_value();
            if (id < 0.0 || weight < 0.0) {
                res.set_content("id and weight must be >= 0.", "text/plain");
                res.status = 400;
                return;
            }

            ingredients[static_cast<size_t>(id)] = static_cast<uint32_t>(weight);
        }

        std::string description = input["description"].string_value();
        auto [code, id] = db->CreateRecipe(name, description, ingredients);
        if (code != DB::Result::OK) {
            res.set_content("DB request failed. Try again later.", "text/plain");
            res.status = 500;
            return;
        }

        res.set_content(std::to_string(id), "text/plain");
    });

    srv.Get(R"(/recipe/(\d+))", [&db](const httplib::Request& req, httplib::Response& res) {
        auto recipe = db->GetRecipeInfo(std::stoull(req.matches[1].str()));
        if (!recipe) {
            res.set_content("The recipe wasn't found.", "text/plain");
            res.status = 404;
            return;
        }

        res.set_content(json11::Json(recipe.value()).dump(), "text/json");
    });

    srv.Delete(R"(/recipe/(\d+))", [&db](const httplib::Request& req, httplib::Response& res) {
        if (size_t id = std::stoull(req.matches[1].str()); !db->DeleteRecipe(id)) {
            res.set_content("DB request failed. Try again later.", "text/plain");
            res.status = 500;
            return;
        }
    });

    srv.Post("/dialogflow", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string err;
        const json11::Json in = json11::Json::parse(req.body, err);
        if (!err.empty()) {
            res.set_content("Failed to parse input as json: " + err, "text/plain");
            res.status = 400;
            return;
        }

        const std::string& resp_id = in["responseId"].string_value();
        const std::string& session = in["session"].string_value();
        const auto& query = in["queryResult"];
        const std::string& query_text = query["queryText"].string_value();
        const std::string& intent_name = query["intent"]["displayName"].string_value();

        std::cout << "[dialogflow] id=" << resp_id << " session=" << session
                  << " query=" << query_text << " intent=" << intent_name << std::endl;

        std::stringstream text;
        if (intent_name == "ingredients") {
            text << "Наши ингредиенты:";
            for (const auto& ingredient : db->GetProducts()) {
                text << "\n" << ingredient.name << " по " << ingredient.kcal << " калории,";
            }
        } else if (intent_name == "pots") {
            text << "Наша посуда:";
            for (const auto& pot : db->GetTableware()) {
                text << "\n" << pot.name << " по " << pot.weight << " грам,";
            }
        } else {
            // This intent is not supported.
            return;
        }
        res.set_content(RenderDialogflowResponse(text.str()), "text/json; charset=utf-8");
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

    std::cout << "Foodculator version: " << version << std::endl;
    std::cout << "Listening on http://localhost:" << port << std::endl;

    server = &srv;
    std::signal(SIGTERM, signal_handler);

    srv.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        if (res.status != 200) {
            std::cerr << req.method << " " << req.path << ":\tcode=" << res.status
                      << " content=" << res.body << std::endl;
            return;
        }

        std::cout << req.method << " " << req.path << ":\tsize=" << res.body.length() << "b"
                  << std::endl;
    });
    srv.listen("0.0.0.0", port);

    std::cout << "I'll be back!" << std::endl;
    return 0;
}
