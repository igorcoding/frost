#include <iostream>
#include <frost/http_response.h>
#include <frost/http_request.h>
#include <frost/http_server.h>

//#define ALLOW_PRINT 1

using namespace std;

void on_request(frost::http_request* req, frost::http_response* resp) {
    std::cout << "hi" << std::endl;

//        switch (req->method()) {
//            case frost::http_method::GET: {
//                char msg[] = "Hello there! Requested via GET.";
//                resp->write(frost::status_code::OK, msg, 0);
//            }
//            case frost::http_method::POST: {
//                char msg[] = "Hello there! Requested via POST.";
//                resp->write(frost::status_code::OK, msg, 0);
//            };
//            default: {
//                char msg[] = "Hello there! Requested via uknown method.";
//                resp->write(frost::status_code::METHOD_NOT_ALLOWED, msg, 0);
//            }
//        }
}

int main() {
    {
        ev::loop_ref loop = ev::get_default_loop();
        frost::http_server server(8000);
        server.on("/", [ ](frost::http_request& req, frost::http_response& resp) {
            char msg[] = "Hello there! Requested via GET.";
            resp.write(frost::status_code::OK, msg, 0);
            resp.finish();
        });

        server.on_signal(SIGINT, [ &server ]() {
            std::cerr << "cought signal" << std::endl;
            server.stop();
        });

        server.start();
        loop.run();

        ev_loop_destroy(loop.raw_loop);
    }
    return 0;
}