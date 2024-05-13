
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
    Socket sock = Socket(baseUrl, 8989, config.pageLimit, config.crawlDelay);
    SiteStats stats = clientSocket.startDiscovering();

    m_mutex.lock();
    std::cout << "----------------------------------------------------------------------------" << std::endl;
    std::cout << " - Website: " << stats.hostname << std::endl;
    std::cout << " - Depth (distance from the starting pages): " << currentDepth << std::endl;
    std::cout << " - Pages Discovered: " << stats.discoveredPages.size() << std::endl;
    std::cout << " - Failed Queries: " << stats.failedQueries << std::endl;
    std::cout << " - Linked Sites: " << stats.linkedSites.size() << std::endl;

    if (stats.minResponseTime < 0) std::cout << " - Min. Response Time: -" << std::endl;
    else std::cout << " - Min. Response Time: " << stats.minResponseTime << "ms" << std::endl;

    if (stats.maxResponseTime < 0) std::cout << "Max. Response Time: -" << std::endl;
    else std::cout << " - Max. Response Time: " << stats.maxResponseTime << "ms" << std::endl;

    if (stats.averageResponseTime < 0) std::cout << "Average Response Time: -" << std::endl;
    else std::cout << " - Avg Response Time: " << stats.averageResponseTime << "ms" << std::endl;

    if (!stats.discoveredPages.empty()) {
        std::cout << "\n [*] List of visited pages:" << std::endl;
        std::cout << "    " << std::setw(15) << "Response Time" << "    " << "URL" << std::endl;
        for (auto page : stats.discoveredPages) {
            std::cout << "    " << std::setw(13) << page.second << "ms" << "    " << page.first << std::endl;
        }
    }

    if (currentDepth < config.depthLimit) {
        for (int i = 0; i < std::min(static_cast<int>(stats.linkedSites.size()), config.linkedSitesLimit); i++) {
            std::string site = stats.linkedSites[i];
            if (!crawlerState.discoveredSites[site]) {
                crawlerState.pendingSites.push(std::make_pair(site, currentDepth+1));
                crawlerState.discoveredSites[site] = true;
            }
        }
    }

    crawlerState.threadsCount--;
    isThreadFinished = true;
    m_mutex.unlock();

    m_condVar.notify_one();
}


int main() {
    Crawler crawler;
    crawler.crawl();
    return 0;
}
