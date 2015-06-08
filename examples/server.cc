#include <iostream>
#include <frost/http_response.h>
#include <frost/http_request.h>
#include <frost/http_server.h>

using namespace std;

void on_request(frost::http_request* req, frost::http_response* resp) {
    std::cout << "hi" << std::endl;
}

int main() {
    cout << "Hello, World!" << endl;
    frost::http_server server(8000);
    server.on("/some/sneaky/path", [](frost::http_request* req, frost::http_response* resp) {

    });

    server.on_signal(SIGINT, [&server]() {
        std::cerr << "cought sig" << std::endl;
        server.stop();
    });

    server.start();
    return 0;
}