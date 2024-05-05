#include "include/socket.h"
#include <netdb.h>
#include <unistd.h>
#include <cstring>


Socket::Socket(std::string hostname, int port, int pageLimit, int crawlDelay)
    : hostname(hostname), port(port), pageLimit(pageLimit), crawlDelay(crawlDelay) {
    pendingPages.push("/");
    discoveredPages["/"] = true;
}

std::string Socket::startConnection() {
    struct hostent *host;
    struct sockaddr_in serverAddr;

    host = gethostbyname(hostname.c_str());
    if (host == nullptr || host->h_addr == nullptr) {
         return "Error getting DNS info for hostname: " + hostname;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return "Cannot create socket!";
    }

    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(serverAddr.sin_zero), 8);

    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) == -1) {
        return "Cannot connect to server!";
    }

    return "";
}


std::string Socket::closeConnection() {
    if (close(sock) == -1) {
        return "Error closing socket!";
    }

    return "";
}

std::string Socket::createHttpRequest(std::string host, std::string path) {
    std::string request = "";
    request += "GET " + path + " HTTP/1.1\r\n";
    request += "HOST:" + host + "\r\n";
    request += "Connection: close\r\n\r\n";
    
    return request;
}