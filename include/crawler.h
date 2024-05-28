#ifndef CRAWLER_H
#define CRAWLER_H

#include "socket.h"
#include "parser.h"
#include "config.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <iomanip>
#include <condition_variable>


class Crawler {
public:
    Crawler(const Config& config);
    void start();
private:
    Config config;
    struct CrawlerState {
        int threadsCount;
        std::queue<std::pair<std::string, int>> pendingSites;
        std::map<std::string, bool> discoveredSites;
    } crawlerState;    

    std::mutex m_mutex;

    std::mutex csvMutex;

    std::condition_variable m_condVar;
    bool isThreadFinished;

    void initialize();
    void initializeResultsFile();
    void startCrawler(std::string baseUrl, int currentDepth);
    void scheduleCrawlers();
    void writeResultsToConsole(const Socket::SiteStats& stats, int currentDepth);
    void writeResultsToCsv(const Socket::SiteStats& stats, int currentDepth);
    
};

#endif // CRAWLER_H