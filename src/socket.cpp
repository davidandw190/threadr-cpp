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
        return " [!] Error getting DNS info for hostname: " + hostname + " (" + hstrerror(h_errno) + ")";
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return " [!] Error: Cannot create socket: " + std::string(strerror(errno));
    }

    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(serverAddr.sin_zero), 8);

    struct timeval timeout;
    timeout.tv_sec = 15;  // 15 seconds timeout
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) == -1) {
        close(sock);
        return " [!] Error: Cannot connect to server: " + std::string(strerror(errno));
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
        return " [!] Error closing socket: " + std::string(strerror(errno));
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
 * @brief Initiates the discovery process by sending HTTP requests and extracting URLs from the responses.
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
        
        handlePageCrawl(path, stats);
    }

    computeStats(stats);
    return stats;
}

/**
 * @brief Handles the crawling of a single page.
 * 
 * @param path The path to crawl.
 * @param stats The SiteStats object to update with the crawl results.
 */
void Socket::handlePageCrawl(const std::string& path, Socket::SiteStats& stats) {
    std::cout << "Crawling " << hostname << " with path " << path << std::endl;

    if (path != "/") {
        usleep(crawlDelay * 1000);
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    std::string connectionError = startConnection();
    if (!connectionError.empty()) {
        std::cerr << connectionError << std::endl;
        stats.failedQueries++;
        return;
    }

    std::string sendData = createHttpRequest(hostname, path);
    if (!sendRequest(sendData, stats)) {
        return;
    }

    std::string httpResponse;
    double responseTime = receiveResponse(httpResponse, startTime);
    closeConnection();

    stats.discoveredPages.push_back(std::make_pair(hostname + path, responseTime));

    processResponse(httpResponse, stats);
}

/**
 * @brief Sends an HTTP request.
 * 
 * @param request The HTTP request message to send.
 * @param stats The SiteStats object to update in case of failure.
 * @return True if the request was sent successfully, false otherwise.
 */
bool Socket::sendRequest(const std::string& request, Socket::SiteStats& stats) {
    if (send(sock, request.c_str(), request.size(), 0) < 0) {
        std::cerr << "Send failed: " << strerror(errno) << std::endl;
        stats.failedQueries++;
        closeConnection();
        return false;
    }
    return true;
}

/**
 * @brief Receives an HTTP response in chuncks.
 * 
 * @param response The string to store the received response.
 * @param startTime The start time of the request to compute response time.
 * @return The response time in milliseconds.
 */
double Socket::receiveResponse(std::string& response, const std::chrono::high_resolution_clock::time_point& startTime) {
    char receivedDataBuffer[4080];
    double responseTime = -1;
    int totalBytesRead = 0;

    while (true) {
        bzero(receivedDataBuffer, sizeof(receivedDataBuffer));
        int bytesRead = recv(sock, receivedDataBuffer, sizeof(receivedDataBuffer), 0);

        if (responseTime < -0.5) {
            auto endTime = std::chrono::high_resolution_clock::now();
            responseTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        }

        if (bytesRead > 0) {
            response.append(receivedDataBuffer, bytesRead);
            totalBytesRead += bytesRead;
        } else if (bytesRead == 0) {
            break;  // connection closed by peer
        } else {
            std::cerr << "Receive failed: " << strerror(errno) << std::endl;
            break;
        }
    }

    return responseTime;
}

/**
 * @brief Processes the HTTP response to extract URLs and update stats.
 * 
 * @param response The HTTP response received from the server.
 * @param stats The SiteStats object to update with the extracted URLs.
 */
void Socket::processResponse(const std::string& response, Socket::SiteStats& stats) {
    std::vector<std::pair<std::string, std::string>> extractedUrls = extractUrls(response, hostname);
    for (const auto& url : extractedUrls) {
        if (url.first.empty() || url.first == hostname) {
            if (!discoveredPages[url.second]) {
                pendingPages.push(url.second);
                discoveredPages[url.second] = true;
            }
        } else {
            if (!discoveredLinkedSites[url.first]) {
                discoveredLinkedSites[url.first] = true;
                stats.linkedSites.push_back(url.first);
            }
        }
    }
}

/**
 * @brief computes statistics for the discovered pages.
 * 
 * @param stats The SiteStats object to update with the computed statistics.
 */
void Socket::computeStats(Socket::SiteStats& stats) {
    double totalResponseTime = 0;

    for (const auto& page : stats.discoveredPages) {
        totalResponseTime += page.second;

        if (stats.minResponseTime < 0) {
            stats.minResponseTime = page.second;
        } else {
            stats.minResponseTime = std::min(stats.minResponseTime, page.second);
        }

        if (stats.maxResponseTime < 0) {
            stats.maxResponseTime = page.second;
        } else {
            stats.maxResponseTime = std::max(stats.maxResponseTime, page.second);
        }
    }

    stats.averageResponseTime = (stats.discoveredPages.size() > 0) 
        ? totalResponseTime / static_cast<double>(stats.discoveredPages.size()) 
        : -1;
}
