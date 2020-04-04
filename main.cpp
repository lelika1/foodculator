#include "cpp-httplib/httplib.h"
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

int main() {
    httplib::Server srv;

    srv.Get("/", [](const httplib::Request& req, httplib::Response& res) {
        std::ifstream in("static/index.html");
        std::string str{std::istreambuf_iterator<char>(in),
                        std::istreambuf_iterator<char>()};
        in.close();
        res.set_content(str, "text/html");
    });

    srv.Get("/stop", [&srv](const httplib::Request& req, httplib::Response& res) {
        srv.stop();
    });

    srv.set_mount_point("/static", "./static");

    std::cout << "Listening on http://localhost:1234" << std::endl;
    srv.listen("localhost", 1234);
    return 0;
}