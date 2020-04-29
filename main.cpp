#include <sqlite3.h>

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

namespace {
std::string ReadHtml(const std::string& path) {
    std::ifstream in(path);
    std::string str{std::istreambuf_iterator<char>(in),
                    std::istreambuf_iterator<char>()};
    in.close();
    return str;
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
            res.status = 500;
            return;
        };

        auto st = db->InsertProduct({name, kcal});
        if (st != SQLITE_OK) {
            if (st == SQLITE_CONSTRAINT) {
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

        std::stringstream ss;
        ss << "A new ingredient '" << name << "' with " << kcal
           << " kcal for 100 g was added.";

        res.set_content(ss.str(), "text/plain");
    });

    srv.Get("/get_tableware", [&db](const httplib::Request& req,
                                    httplib::Response& res) {
        res.set_content(json11::Json(db->GetTableware()).dump(), "text/json");
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
            res.status = 500;
            return;
        };

        auto st = db->InsertTableware({name, weight});
        if (st != SQLITE_OK) {
            if (st == SQLITE_CONSTRAINT) {
                res.set_content("This pot already exists in the database.",
                                "text/plain");
            } else {
                res.set_content("A pot wasn't added. Some SQL error occured.",
                                "text/plain");
            }
            res.status = 500;
            return;
        }

        std::stringstream ss;
        ss << "A new pot '" << name << "' with weight " << weight
           << " g was added.";

        res.set_content(ss.str(), "text/plain");
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
