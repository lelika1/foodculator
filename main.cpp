#include "cpp-httplib/httplib.h"
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

int main() {
    httplib::Server srv;

    srv.Get("/", [](const httplib::Request& req, httplib::Response& res) {
        std::ifstream in("static/index.html");
        std::string str((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
        res.set_content(str, "text/html");
        in.close();
    });

    srv.Get("/stop", [&srv](const httplib::Request& req, httplib::Response& res) {
        srv.stop();
    });

    std::cout << "Listening on http://localhost:1234" << std::endl;
    srv.listen("localhost", 1234);
    return 0;
}