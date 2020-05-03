#include <csignal>
#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <unordered_map>

#include "db/db.h"
#include "httplib/httplib.h"
#include "json11/json11.hpp"
#include "tgbot/tgbot.h"

namespace {
std::string ReadHtml(const std::string& path) {
    std::ifstream in(path);
    std::string str{std::istreambuf_iterator<char>(in),
                    std::istreambuf_iterator<char>()};
    in.close();
    return str;
}

// TODO(luckygeck): Move to separate file.
std::string RenderDialogflowResponse(std::string text) {
    std::vector<json11::Json> jsons;
    auto simpleResponse = json11::Json::object{
        {"simpleResponse", json11::Json::object{{"textToSpeech", text}}}};

    json11::Json ret = json11::Json::object{
        {"fulfillmentMessages",
         json11::Json::array{
             json11::Json::object{
                 {"text",
                  json11::Json::object{
                      {"text", json11::Json::array{std::move(text)}}}}},
         }},
        {
            "payload",
            json11::Json::object{
                {"google", json11::Json::object{{
                               "richResponse",
                               json11::Json::object{
                                   {"items", json11::Json::array{std::move(
                                                 simpleResponse)}}},
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
    if (argc != 3) {
        std::cerr << "usage: " << argv[0]
                  << " path_to_static_files path_to_database" << std::endl;
        return 1;
    }
    std::cout << "Working with sqlite db in " << argv[2] << std::endl;
    auto db = DB::Create(argv[2]);
    if (!db) {
        std::cerr << "A problem with " << argv[2] << " occured." << std::endl;
        return 1;
    }

    std::string path_to_static = argv[1];

    httplib::Server srv;

    std::vector<std::pair<const char*, const char*>> html_pages = {
        {"/", "/index.html"},
        {"/ingredients", "/ingredients.html"},
        {"/tableware", "/tableware.html"},
        {"/new_recipe", "/recipe.html"}};

    for (const auto& it : html_pages) {
        srv.Get(it.first,
                [path = (path_to_static + it.second)](
                    const httplib::Request& req, httplib::Response& res) {
                    res.set_content(ReadHtml(path), "text/html");
                });
    }

    srv.Get("/get_ingredients", [&db](const httplib::Request& req,
                                      httplib::Response& res) {
        res.set_content(json11::Json(db->GetIngredients()).dump(), "text/json");
    });

    srv.Post("/add_ingredient", [&db](const httplib::Request& req,
                                      httplib::Response& res) {
        std::string err_msg;
        json11::Json js = json11::Json::parse(req.body, err_msg);

        if (!err_msg.empty() ||
            !js.has_shape({{"product", json11::Json::STRING},
                           {"kcal", json11::Json::NUMBER}},
                          err_msg)) {
            std::cout << "Error: " << err_msg << std::endl;

            res.set_content(
                "An ingredient wasn't added. Some information is missing.",
                "text/plain");
            res.status = 400;
            return;
        }

        std::string name = js["product"].string_value();
        uint32_t kcal = js["kcal"].int_value();
        if (name.empty()) {
            res.set_content(
                "An ingredient wasn't added. Some information is missing.",
                "text/plain");
            res.status = 400;
            return;
        };

        auto result = db->InsertProduct({name, kcal});
        if (result.code_ != DB::Result::OK) {
            if (result.code_ == DB::Result::DUPLICATE) {
                res.set_content(
                    "This ingredient already exists in the database.",
                    "text/plain");
            } else {
                res.set_content(
                    "An ingredient wasn't added. Some SQL error occured.",
                    "text/plain");
            }
            res.status = 500;
            return;
        }

        res.set_content(std::to_string(result.id_), "text/plain");
        res.status = 200;
    });

    srv.Delete(R"(/ingredient/(\d+))", [&db](const httplib::Request& req,
                                             httplib::Response& res) {
        auto id = std::stoi(req.matches[1].str());
        if (!db->DeleteProduct(id)) {
            res.set_content("A pot wasn't deleted. Some SQL error occured.",
                            "text/plain");
            res.status = 500;
            return;
        }
        res.status = 200;
    });

    srv.Get("/get_tableware", [&db](const httplib::Request& req,
                                    httplib::Response& res) {
        res.set_content(json11::Json(db->GetTableware()).dump(), "text/json");
    });

    srv.Post("/dialogflow", [&db](const httplib::Request& req,
                                  httplib::Response& res) {
        std::string err;
        const json11::Json in = json11::Json::parse(req.body, err);
        if (!err.empty()) {
            res.set_content("Failed to parse input as json: " + err,
                            "text/plain");
            res.status = 400;
            return;
        }

        const std::string& resp_id = in["responseId"].string_value();
        const std::string& session = in["session"].string_value();
        const auto& query = in["queryResult"];
        const std::string& query_text = query["queryText"].string_value();
        const std::string& intent_name =
            query["intent"]["displayName"].string_value();

        std::cout << "[dialogflow] id=" << resp_id << " session=" << session
                  << " query=" << query_text << " intent=" << intent_name
                  << std::endl;

        std::stringstream text;
        if (intent_name == "ingredients") {
            text << "Наши ингредиенты:";
            for (const auto& ingredient : db->GetIngredients()) {
                text << "\n"
                     << ingredient.name_ << " по " << ingredient.kcal_
                     << " калории,";
            }
        } else if (intent_name == "pots") {
            text << "Наша посуда:";
            for (const auto& pot : db->GetTableware()) {
                text << "\n" << pot.name_ << " по " << pot.weight_ << " грам,";
            }
        } else {
            // This intent is not supported.
            return;
        }
        res.set_content(RenderDialogflowResponse(text.str()),
                        "text/json; charset=utf-8");
    });

    srv.Post("/add_tableware", [&db](const httplib::Request& req,
                                     httplib::Response& res) {
        std::string err_msg;
        json11::Json js = json11::Json::parse(req.body, err_msg);

        if (!err_msg.empty() ||
            !js.has_shape({{"name", json11::Json::STRING},
                           {"weight", json11::Json::NUMBER}},
                          err_msg)) {
            std::cout << "Error: " << err_msg << std::endl;

            res.set_content("A pot wasn't added. Some information is missing.",
                            "text/plain");
            res.status = 400;
            return;
        }

        std::string name = js["name"].string_value();
        uint32_t weight = js["weight"].int_value();
        if (name.empty()) {
            res.set_content("A pot wasn't added. Some information is missing.",
                            "text/plain");
            res.status = 400;
            return;
        };

        auto result = db->InsertTableware({name, weight});
        if (result.code_ != DB::Result::OK) {
            if (result.code_ == DB::Result::DUPLICATE) {
                res.set_content("This pot already exists in the database.",
                                "text/plain");
            } else {
                res.set_content("A pot wasn't added. Some SQL error occured.",
                                "text/plain");
            }
            res.status = 500;
            return;
        }

        res.set_content(std::to_string(result.id_), "text/plain");
        res.status = 200;
    });

    srv.Delete(R"(/tableware/(\d+))", [&db](const httplib::Request& req,
                                            httplib::Response& res) {
        int id = std::stoi(req.matches[1].str());
        if (!db->DeleteTableware(id)) {
            res.set_content(
                "An ingredient wasn't deleted. Some SQL error occured.",
                "text/plain");
            res.status = 500;
            return;
        }
        res.status = 200;
    });

    std::string version = "UNKNOWN";
    if (char* v = std::getenv("VERSION"); v) {
        version = v;
    }

    srv.Get("/version", [&version](const httplib::Request& req,
                                   httplib::Response& res) {
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

    srv.set_logger([](const httplib::Request& req,
                      const httplib::Response& res) {
        std::cout << req.method << " " << req.path << ":\tcode=" << res.status
                  << " size=" << res.body.length() << "b" << std::endl;
    });
    srv.listen("0.0.0.0", port);

    std::cout << "I'll be back!" << std::endl;
    return 0;
}
