/**
 * @file socket.cpp
 * @brief Implementation of the client Socket class for HTTP connection handling and page discovery.
 * 
 * Responsible for establishing HTTP connections, sending requests, and discovering pages and linked sites.
 */

#include "socket.h"
#include "parser.h"
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <cerrno>

/**
 * @brief Constructs a Socket object with the specified params.
 * 
 * @param hostname The hostname to connect to.
 * @param port The port number to connect to.
 * @param pageLimit The maximum number of pages to discover.
 * @param crawlDelay The delay between consecutive requests in milliseconds.
 */
Socket::Socket(std::string hostname, int port, int pageLimit, int crawlDelay)
    : hostname(hostname), port(port), pageLimit(pageLimit), crawlDelay(crawlDelay) {
    pendingPages.push("/");
    discoveredPages["/"] = true;
    discoveredLinkedSites.clear();
}


/**
 * @brief Establishes a connection with the web server.
 * 
 * @return A string containing an error message if an error occurs during the connection process, or an empty string if successful.
 */
std::string Socket::startConnection() {
    struct hostent *host;
    struct sockaddr_in serverAddr;

    host = gethostbyname(hostname.c_str());
    if (host == nullptr || host->h_addr == nullptr) {
        return "Error getting DNS info for hostname: " + hostname + " (" + hstrerror(h_errno) + ")";
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return "Cannot create socket: " + std::string(strerror(errno));
    }

    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(serverAddr.sin_zero), 8);

    struct timeval timeout;
    timeout.tv_sec = 10;  // 10 seconds timeout
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) == -1) {
        close(sock);
        return "Cannot connect to server: " + std::string(strerror(errno));
    }

    return "";
}


/**
 * @brief Closes the connection with the web server.
 * 
 * @return A string containing an error message if an error occurs while closing the connection, or an empty string if successful.
 */
std::string Socket::closeConnection() {
    if (close(sock) == -1) {
        return "Error closing socket: " + std::string(strerror(errno));
    }

    return "";
}


/**
 * @brief Creates an HTTP request message.
 * 
 * @param host The host to include in the request.
 * @param path The path to include in the request.
 * @return The HTTP request message as a string.
 */
std::string Socket::createHttpRequest(std::string host, std::string path) {
    std::string request = "";
    request += "GET " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "Connection: close\r\n\r\n";
    
    return request;
}


/**
 * @brief @brief Initiates the discovery process by sending HTTP requests and extracting URLs from the responses.
 * 
 * It iteratively discovers pages until the page limit is reached or there are no more pending pages.
 * 
 * @return SiteStats structure containing statistics about the discovered pages and linked sites.
 */
Socket::SiteStats Socket::initiateDiscovery() {
    Socket::SiteStats stats;
    stats.hostname = hostname;

    while (!pendingPages.empty() && (pageLimit == -1 || static_cast<int>(stats.discoveredPages.size()) < pageLimit)) {
        std::string path = pendingPages.front();
        pendingPages.pop();

        // sleep for crawlDelay if this is not the first request
        if (path != "/") {
            usleep(crawlDelay * 1000);
        }

        auto startTime = std::chrono::high_resolution_clock::now();
        
        // if cannot create the connection, just ignore
        std::string connectionError = startConnection();
        if (connectionError != "") {
            std::cerr << connectionError << std::endl;
            stats.failedQueries++;
            continue;
        }

        std::string sendData = createHttpRequest(hostname, path);
        if (send(sock, sendData.c_str(), sendData.size(), 0) < 0) {
            std::cerr << "Send failed: " << strerror(errno) << std::endl;
            stats.failedQueries++;
            closeConnection();
            continue;
        }

        char receivedDataBuffer[4080];
        int totalBytesRead = 0;
        std::string httpResponse = "";
        double responseTime = -1;
        while (true) {
            bzero(receivedDataBuffer, sizeof(receivedDataBuffer));
            int bytesRead = recv(sock, receivedDataBuffer, sizeof(receivedDataBuffer), 0);

            if (responseTime < -0.5) {
                // clock end
                auto endTime = std::chrono::high_resolution_clock::now();
                responseTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
            }

            if (bytesRead > 0) {
                httpResponse.append(receivedDataBuffer, bytesRead);
                totalBytesRead += bytesRead;
            } else if (bytesRead == 0) {
                break;  // Connection closed by peer
            } else {
                std::cerr << "Receive failed: " << strerror(errno) << std::endl;
                break;
            }
        }

        closeConnection();

        stats.discoveredPages.push_back(std::make_pair(hostname + path, responseTime));

        std::vector<std::pair<std::string, std::string>> extractedUrls = extractUrls(httpResponse, hostname);
        for (auto& url : extractedUrls) {
            if (url.first == "" || url.first == hostname) {
                // In case it's the same host, check if the path is discovered
                if (!discoveredPages[url.second]) {
                    pendingPages.push(url.second);
                    discoveredPages[url.second] = true;
                }
            } else {
                // In a different host, add to linkedSites
                if (!discoveredLinkedSites[url.first]) {
                    discoveredLinkedSites[url.first] = true;
                    stats.linkedSites.push_back(url.first);
                }
            }    
        }
    }

    double totalResponseTime = 0;
    for (auto& page : stats.discoveredPages) {
        totalResponseTime += page.second;

        if (stats.minResponseTime < 0) stats.minResponseTime = static_cast<double>(page.second);
        else stats.minResponseTime = std::min(stats.minResponseTime, static_cast<double>(page.second));

        if (stats.maxResponseTime < 0) stats.maxResponseTime = static_cast<double>(page.second);
        else stats.maxResponseTime = std::max(stats.maxResponseTime, static_cast<double>(page.second));
    }

    stats.averageResponseTime = (stats.discoveredPages.size() > 0) ? totalResponseTime / static_cast<double>(stats.discoveredPages.size()) : -1;

    return stats;
}