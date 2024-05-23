#include "crawler.h"
#include "socket.h"
#include "parser.h"
#include "config.h"

Crawler::Crawler(const Config& config) : config(config), isThreadFinished(false) {}

void Crawler::start() {
    initialize();
    scheduleCrawlers();
}

Config readConfigFile() {
    try {
        std::ifstream cfFile("config.txt");
        std::string var, val, url;
        Config cf;
        while (cfFile >> var >> val) {
            if (var == "crawlDelay") cf.crawlDelay = std::stoi(val);
            else if (var == "maxThreads") cf.maxThreads = std::stoi(val);
            else if (var == "depthLimit") cf.depthLimit = std::stoi(val);
            else if (var == "pageLimit") cf.pageLimit = std::stoi(val);
            else if (var == "linkedSitesLimit") cf.linkedSitesLimit = std::stoi(val);
            else if (var == "startUrls") {
                for (int i = 0; i < std::stoi(val); i++) {
                    cfFile >> url;
                    cf.startUrls.push_back(url);
                }
            }
        }
        cfFile.close();
        std::cout << "Configuration file read successfully" << std::endl;
        return cf;
    } catch (std::exception& error) {
        std::cerr << "Exception (@readConfigFile): " << error.what() << std::endl;
        exit(1);
    }
}

void Crawler::initialize() {
    crawlerState.threadsCount = 0;
    for (auto& url : config.startUrls) {
        crawlerState.pendingSites.push(std::make_pair(getHostnameFromUrl(url), 0));
        crawlerState.discoveredSites[getHostnameFromUrl(url)] = true;
    }

    std::cout << "Crawler initialized" << std::endl;
}

void Crawler::scheduleCrawlers() {
    while (crawlerState.threadsCount != 0 || !crawlerState.pendingSites.empty()) {
        std::unique_lock<std::mutex> m_lock(m_mutex);

        while (!crawlerState.pendingSites.empty() && crawlerState.threadsCount < config.maxThreads) {
            auto nextSite = crawlerState.pendingSites.front();
            crawlerState.pendingSites.pop();
            crawlerState.threadsCount++;

            std::thread(&Crawler::startCrawler, this, nextSite.first, nextSite.second).detach();
        }

        m_condVar.wait(m_lock, [this] { return isThreadFinished; });
        isThreadFinished = false;
    }
}

void Crawler::startCrawler(std::string baseUrl, int currentDepth) {
    Socket clientSocket(baseUrl, 80, config.pageLimit, config.crawlDelay);
    std::cout << "Crawling " << baseUrl << " at depth " << currentDepth << std::endl;
    Socket::SiteStats stats = clientSocket.initiateDiscovery();
    std::lock_guard<std::mutex> m_lock(m_mutex);

    // Output the stats
    std::cout << "----------------------------------------------------------------------------" << std::endl;
    std::cout << " - Website: " << stats.hostname << std::endl;
    std::cout << " - Depth (distance from the starting pages): " << currentDepth << std::endl;
    std::cout << " - Pages Discovered: " << stats.discoveredPages.size() << std::endl;
    std::cout << " - Failed Queries: " << stats.failedQueries << std::endl;
    std::cout << " - Linked Sites: " << stats.linkedSites.size() << std::endl;

    if (stats.minResponseTime < 0) std::cout << " - Min. Response Time: -" << std::endl;
    else std::cout << " - Min. Response Time: " << stats.minResponseTime << "ms" << std::endl;

    if (stats.maxResponseTime < 0) std::cout << " - Max. Response Time: -" << std::endl;
    else std::cout << " - Max. Response Time: " << stats.maxResponseTime << "ms" << std::endl;

    if (stats.averageResponseTime < 0) std::cout << " - Avg Response Time: -" << std::endl;
    else std::cout << " - Avg Response Time: " << stats.averageResponseTime << "ms" << std::endl;

    if (!stats.discoveredPages.empty()) {
        std::cout << "\n [*] List of visited pages:" << std::endl;
        std::cout << "    " << std::setw(15) << "Response Time" << "    " << "URL" << std::endl;
        for (auto& page : stats.discoveredPages) {
            std::cout << "    " << std::setw(13) << page.second << "ms" << "    " << page.first << std::endl;
        }
    }

    if (currentDepth < config.depthLimit) {
        for (int i = 0; i < std::min(static_cast<int>(stats.linkedSites.size()), config.linkedSitesLimit); i++) {
            std::string site = stats.linkedSites[i];
            if (crawlerState.discoveredSites.find(site) == crawlerState.discoveredSites.end()) {
                crawlerState.pendingSites.push(std::make_pair(site, currentDepth + 1));
                crawlerState.discoveredSites[site] = true;
            }
        }
    }

    crawlerState.threadsCount--;
    isThreadFinished = true;
    m_condVar.notify_one();
}



int main() {
    Crawler crawler(readConfigFile());
    crawler.start();
    std::cout << "Crawler finished" << std::endl;
    return 0;
}