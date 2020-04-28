#include <sqlite3.h>

#include <csignal>
#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

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

std::vector<std::string> Split(const std::string& str,
                               const std::string& delimiter) {
    std::vector<std::string> result;
    size_t start = 0;
    auto end = str.find(delimiter);
    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    result.push_back(str.substr(start, end));
    return result;
}

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

    srv.Post("/get_ingredients",
             [&db](const httplib::Request& req, httplib::Response& res) {
                 std::stringstream ss;
                 ss << R"({"products":[)";
                 const auto& products = db->GetIngredients();
                 for (size_t i = 0; i < products.size(); ++i) {
                     ss << R"({"name":")" << products[i].name_
                        << R"(", "kcal":)" << products[i].kcal_ << "}";
                     if (i != products.size() - 1) {
                         ss << ",";
                     }
                 }
                 ss << R"(]})";

                 res.set_content(ss.str(), "text/json");
             });

    srv.Post("/add_ingredient", [&db](const httplib::Request& req,
                                      httplib::Response& res) {
        std::unordered_map<std::string, std::string> params;
        for (const auto& e : Split(req.body, "&")) {
            auto element = Split(e, "=");
            if (element.size() != 2) {
                res.status = 400;
                return;
            }

            params[element[0]] = element[1];
        }

        if (params["product"].empty() || params["kcal"].empty()) {
            res.set_content(
                "An ingredient wasn't added. Some information is missing.",
                "text/plain");
            res.status = 500;
            return;
        };

        uint32_t kcal = std::stoi(params["kcal"]);
        auto st = db->InsertProduct({params["product"], kcal});
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
        ss << "A new ingredient '" << params["product"] << "' with " << kcal
           << " kcal for 100 g was added.";

        res.set_content(ss.str(), "text/plain");
    });

    srv.Post("/get_tableware",
             [&db](const httplib::Request& req, httplib::Response& res) {
                 std::stringstream ss;
                 ss << R"({"tableware":[)";
                 const auto& tableware = db->GetTableware();
                 for (size_t i = 0; i < tableware.size(); ++i) {
                     ss << R"({"name":")" << tableware[i].name_
                        << R"(", "weight":)" << tableware[i].weight_ << "}";
                     if (i != tableware.size() - 1) {
                         ss << ",";
                     }
                 }
                 ss << R"(]})";

                 res.set_content(ss.str(), "text/json");
             });

    srv.Post("/add_tableware", [&db](const httplib::Request& req,
                                     httplib::Response& res) {
        std::unordered_map<std::string, std::string> params;
        for (const auto& e : Split(req.body, "&")) {
            auto element = Split(e, "=");
            if (element.size() != 2) {
                res.status = 400;
                return;
            }

            params[element[0]] = element[1];
        }

        if (params["name"].empty() || params["weight"].empty()) {
            res.set_content("A pot wasn't added. Some information is missing.",
                            "text/plain");
            res.status = 500;
            return;
        };

        uint32_t weight = std::stoi(params["weight"]);
        auto st = db->InsertTableware({params["name"], weight});
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
        ss << "A new pot '" << params["name"] << "' with weight " << weight
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

    srv.listen("0.0.0.0", port);

    std::cout << "I'll be back!" << std::endl;
    return 0;
}
