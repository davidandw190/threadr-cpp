#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <queue>
#include <vector>
#include <map>

class Socket {
public:
    // holds stats for the discovered website
    struct SiteStats {
        std::string hostname;
        double averageResponseTime = -1;
        double minResponseTime = -1;
        double maxResponseTime = -1; 
        int failedQueries = 0; 

        std::vector<std::string> linkedSites;
        std::vector<std::pair<std::string, int>> discoveredPages;
    };

    Socket(
        std::string hostname,
        int port = 8989,
        int pageLimit = -1,
        int crawlDelay = 1000
    );

    SiteStats initiateDiscovery();

private:
    std::string hostname;
    int sock;
    int pageLimit;
    int port;
    int crawlDelay;
    std::queue<std::string> pendingPages;
    std::map<std::string, bool> discoveredPages; 
    std::map<std::string, bool> discoveredLinkedSites; 

    std::string startConnection();
    std::string closeConnection();

    bool isSameHost(const std::string& url, const std::string& hostname);
    bool isSameUrl(const std::string& url, const std::string& hostname);

    bool containsRepeatedHostname(const std::string& url, const std::string& hostname);


    std::string getUrlHost(const std::string& url);

    std::string createHttpRequest(std::string host, std::string path);


};

#endif // SOCKET_H