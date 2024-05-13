
#include "crawler.h"

Crawler::Crawler(const Config& config) : config(config) {}


void Crawler::start() {
    initialize();
}

void Crawler::initialize() {
    crawlerState.threadsCount = 0;
    for (auto url : config.startUrls) {
        crawlerState.pendingSites.push(std::make_pair(getHostnameFromUrl(url), 0));
        crawlerState.discoveredSites[getHostnameFromUrl(url)] = true;
    }
    


    std::cout << "Crawler initialized" << std::endl;
}


void Crawler::scheduleCrawlers() {
    while (crawlerState.threadCount != 0 || !crawlerState.pendingSites.empty()) {
        m_mutex.lock();
        isThreadFinished = false;
        while (!crawlerState.pendingSites.empty() && crawlerState.threadsCount < config.maxThreads) {
            auto nextSite = crawlerState.pendingSites.front();
            crawlerState.pendingSites.pop();
            crawlerState.threadsCount++;

            std::thread t(&Crawler::startCrawler, this, nextSite.first, nextSite.second);
            if (t.joinable()) t.detach();
        }
        m_mutex.unlock();

        std::unique_lock<std::mutex> m_lock(m_mutex);
        while (!threadFinished) m_condVar.wait(m_lock);
    }
}

void Crawler::startCrawler(std::string baseUrl, int currentDepth) {
    // TODO: Implement thsi
}


int main() {
    Crawler crawler;
    crawler.crawl();
    return 0;
}
