#include "socket.h"
#include "parser.h"
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <cstring>


Socket::Socket(std::string hostname, int port, int pageLimit, int crawlDelay)
    : hostname(hostname), port(port), pageLimit(pageLimit), crawlDelay(crawlDelay) {
    pendingPages.push("/");
    discoveredPages["/"] = true;
    discoveredLinkedSites.clear();
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

Socket::SiteStats Socket::initiateDiscovery() {
    Socket::SiteStats stats;
    stats.hostname = hostname;

    std::cout << "Hostname " << hostname << std::endl;

     while (!pendingPages.empty() && (pageLimit == -1 || static_cast<int>(stats.discoveredPages.size()) < pageLimit)) {
        std::string path = pendingPages.front();
        pendingPages.pop();

        // Sleep for crawlDelay if this is not the first request
        if (path != "/") {
            usleep(crawlDelay);
        }

        auto startTime = std::chrono::high_resolution_clock::now();
        
        // If cannot create the conn, just ignore
        if (startConnection() != "") {
            stats.failedQueries++;
            continue;
        }

        std::string sendData = createHttpRequest(hostname, path);
        if (send(sock, sendData.c_str(), sendData.size(), 0) < 0) {
            stats.failedQueries++;
            continue;
        }

        char recievedDataBuffer[1024];
        int totalBytesRead = 0;
        std::string httpResponse = "";
        double responseTime = -1;
        while (true) {
            bzero(recievedDataBuffer, sizeof(recievedDataBuffer));
            int bytesRead = recv(sock, recievedDataBuffer, sizeof(recievedDataBuffer), 0);

            if (responseTime < -0.5) {
                // clock end
                auto endTime = std::chrono::high_resolution_clock::now();
                responseTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
            }

            if (bytesRead > 0) {
                std::string ss(recievedDataBuffer);
                httpResponse += ss;
                totalBytesRead += bytesRead;
            } else {
                break;
            }
        }

        closeConnection();

        stats.discoveredPages.push_back(std::make_pair(hostname + path, responseTime));

        std::vector<std::pair<std::string, std::string>> extractedUrls = extractUrls(httpResponse);
        for (auto url : extractedUrls) {
            if (url.first == "" || url.first == hostname) {
                // In case its the same host check if the path is discovered
                if (!discoveredPages[url.second]) {
                    pendingPages.push(url.second);
                    discoveredPages[url.second] = true;
                }
            } else {
                //In a different host, add to linkedSites
                if (!discoveredLinkedSites[url.first]) {
                    discoveredLinkedSites[url.first] = true;
                    stats.linkedSites.push_back(url.first);
                }
            }    
        }
        
    }

    double totalResponseTime = 0;
    for (auto page : stats.discoveredPages) {
        totalResponseTime += page.second;

        if (stats.minResponseTime < 0) stats.minResponseTime = static_cast<double>(page.second);
        else stats.minResponseTime = std::min(stats.minResponseTime, static_cast<double>(page.second));

        if (stats.maxResponseTime < 0) stats.maxResponseTime = static_cast<double>(page.second);
        else stats.maxResponseTime = std::max(stats.maxResponseTime, static_cast<double>(page.second));

    }

    if (!stats.discoveredPages.empty()) 
        stats.averageResponseTime = totalResponseTime / stats.discoveredPages.size();

    return stats;


}