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
        double averageResponseTime; = -1;
        double minResponseTime; = -1;
        double maxResponseTime = -1; 
        int failedQueries = 0; 

        std::vector<std::pair<std::string, int>> discoveredPages;
    };

    ClientSocket(
        std::string hostname,
        int port = 8989,
        int pagesLimit = -1,
        int crawlDelay = 1000
    );

private:
    std::string hostname;
    int socket;
    int pagesLimit;
    int port;
    int crawlDelay;
    std::queue<std::string> pendingPages;
    std::map<std::string, bool> discoveredPages;  

    std::string startConnection();
    std::string closeConnection();


};

#endif // SOCKET_H