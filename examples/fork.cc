#include <iostream>
#include <frost/http_response.h>
#include <frost/http_request.h>
#include <frost/http_server.h>
#include <unistd.h>

using namespace std;

int main() {
    frost::http_server* server = new frost::http_server(8000);
    server->on("/", [](frost::http_request* req, frost::http_response* resp) {
        char msg[] = "Hello from server!\r\n";
        resp->write(frost::status_code::OK, msg, 0);
        resp->finish();
    });

    server->on_signal(SIGINT, [server]() {
        std::cerr << "cought signal" << std::endl;
        server->stop();
    });

    server->listen();

    int max_childs = 4;
    for (int i = 0; i < max_childs; ++i) {
        pid_t pid = fork();
        if (pid > 0) {
            // master
            std::cout << "Forked child with pid: " << pid << std::endl;
        } else if (pid == 0) {
            // child
            server->notify_fork_child();
            server->accept();
            break;
        } else {
            // error
            perror("fork failed");
            break;
        }
    }

    delete server;
    return 0;
}