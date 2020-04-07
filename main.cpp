#include <sqlite3.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

#include "cpp-httplib/httplib.h"

std::vector<std::string> Split(const std::string &str,
                               const std::string &delimiter) {
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

int main() {
    httplib::Server srv;

    srv.Get("/", [](const httplib::Request &req, httplib::Response &res) {
        std::ifstream in("static/index.html");
        std::string str{std::istreambuf_iterator<char>(in),
                        std::istreambuf_iterator<char>()};
        in.close();
        res.set_content(str, "text/html");
    });

    srv.Get("/addingredient", [](const httplib::Request &req, httplib::Response &res) {
        std::ifstream in("static/ingredients.html");
        std::string str{std::istreambuf_iterator<char>(in),
                        std::istreambuf_iterator<char>()};
        in.close();
        res.set_content(str, "text/html");
    });

    srv.Post("/addingredient", [](const httplib::Request &req, httplib::Response &res) {
        std::unordered_map<std::string, std::string> params;
        for (const auto &e : Split(req.body, "&")) {
            auto element = Split(e, "=");
            if (element.size() != 2) {
                res.status = 400;
                return;
            }

            params[element[0]] = element[1];
        }

        if (params["product"].empty() || params["kcal"].empty()) {
            res.set_content("An ingredient wasn't added. Some information is missing.",
                            "text/plain");
            res.status = 500;
            return;
        };

        uint32_t kcal = std::stoi(params["kcal"]);
        std::stringstream ss;
        ss << "A new ingredient '" << params["product"] << "' with "
           << kcal << "kcal for 100g was added.";
        res.set_content(ss.str(), "text/plain");
    });

    srv.Get("/stop", [&srv](const httplib::Request &req, httplib::Response &res) {
        srv.stop();
    });

    srv.set_mount_point("/static", "./static");

    std::cout << "Listening on http://localhost:1234" << std::endl;
    srv.listen("localhost", 1234);
    return 0;
}
