#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <chrono>

class Socket {
public:
    struct SiteStats {
        std::string hostname;
        std::vector<std::pair<std::string, double>> discoveredPages;
        std::vector<std::string> linkedSites;
        int failedQueries = 0;
        double minResponseTime = -1;
        double maxResponseTime = -1;
        double averageResponseTime = -1;
    };

    Socket(std::string hostname, int port, int pageLimit, int crawlDelay);
    SiteStats initiateDiscovery();

private:
    std::string hostname;
    int port;
    int pageLimit;
    int crawlDelay;
    int sock;

    std::queue<std::string> pendingPages;
    std::unordered_map<std::string, bool> discoveredPages;
    std::unordered_map<std::string, bool> discoveredLinkedSites;

    std::string startConnection();
    std::string closeConnection();
    std::string createHttpRequest(std::string host, std::string path);
    void handlePageCrawl(const std::string& path, SiteStats& stats);
    bool sendRequest(const std::string& request, SiteStats& stats);
    double receiveResponse(std::string& response, const std::chrono::high_resolution_clock::time_point& startTime);
    void processResponse(const std::string& response, SiteStats& stats);
    void computeStats(SiteStats& stats);
};

#endif // SOCKET_H