/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define HTTP_SOCKET_BACKLOG_SIZE 5

void error(const char *msg)  {
    perror(msg);
    exit(1);
}

static inline pid_t http_create_socket(uint16_t port, bool nonblock) {
    struct sockaddr_in serv_addr;
    int type = SOCK_STREAM;
    if (nonblock) {
        type |= SOCK_NONBLOCK;
    }
    pid_t sockfd = socket(AF_INET, type, 0);
    if (sockfd < 0) {
        perror("Can\'t create socket");
        exit(1);
    }

    int reuse_address = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(int)) < 0) {
        perror("setsockopt failed");
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    if (listen(sockfd, HTTP_SOCKET_BACKLOG_SIZE) < 0) {
        perror("listen failed");
        exit(1);
    }

    return sockfd;
}

int main(int argc, char *argv[]) {
    uint16_t port = 8000;

    int newsockfd;
    socklen_t clilen;
    const size_t s = 256;
    char buffer[s];
    struct sockaddr_in cli_addr;
    int n;

    pid_t sockfd = http_create_socket(port, false);

    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,
                       (struct sockaddr *) &cli_addr,
                       &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");
    bzero(buffer,s);
    n = read(newsockfd,buffer,s-1);
    if (n < 0) error("ERROR reading from socket");
    printf("Here is the message: %s\n",buffer);
    n = write(newsockfd,"I got your message",18);
    if (n < 0) error("ERROR writing to socket");
    close(newsockfd);
    close(sockfd);
    return 0;
}