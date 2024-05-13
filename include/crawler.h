#ifndef CRAWLER_H
#define CRAWLER_H

#include "config.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <map>

class Crawler {
public:
    Crawler(const Config& config);
    void start();
private:
    Config config;
    struct CrawlerState {
        int threadsCount;
        std::map<std::string, bool> discoveredSites;
    } crawlerState;    

    std::mutex m_mutex
    std::condition_variable m_condVar;
    bool isThreadFinished;

    void initialize();
    void scheduleCrawlers();
    Config readConfigFile();
};

#endif // CRAWLER_H