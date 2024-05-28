/**
 * @file crawler.cpp
 * @brief Implementation of a web crawler for discovering and analyzing web pages.
 * 
 * Implementation of the core Crawler class, which is responsible for crawling the
 * web pages, discovering links, and analyzing the response times of different URLs.
 */

#include "crawler.h"
#include <argparse/argparse.hpp>
#include "socket.h"
#include "parser.h"
#include "config.h"

Crawler::Crawler(const Config& config) : config(config), isThreadFinished(false) {}

void Crawler::start() {
    initialize();
    scheduleCrawlers();
}

Config parseCommandLineArgs(int argc, char *argv[]) {
    argparse::ArgumentParser program("threadr");

    program.add_argument("-t", "--maxThreads")
        .help("Maximum number of threads")
        .scan<'i', int>();

    program.add_argument("-d", "--crawlDepth")
        .help("Maximum crawl depth")
        .scan<'i', int>();

    program.add_argument("--pageLimit")
        .help("Maximum number of pages to crawl per site")
        .scan<'i', int>();

    program.add_argument("--linkedSitesLimit")
        .help("Maximum number of linked sites to discover per page")
        .scan<'i', int>();

    program.add_argument("--crawlDelay")
        .help("Delay between requests in milliseconds")
        .scan<'i', int>();

    program.add_argument("--configFile", "-cfg")
        .help("Path to configuration file, if not provided, start URLs must be provided as arguments. Any other provided args conflicting with the config provided will override the ones from the config file. ")
        .default_value(std::string(""));

    program.add_argument("startUrls")
        .help("List of starting URLs")
        .remaining();

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
         std::cerr << "Error: Invalid command line arguments provided. " << err.what() << std::endl;
        std::cerr << program;
        exit(1);
    }

    Config config;

    // read config from file if specified
    std::string configFilePath = program.get<std::string>("--configFile");
     if (!configFilePath.empty()) {
        std::ifstream configFile(configFilePath);
        if (!configFile.is_open()) {
            std::cerr << "Error: Unable to open configuration file: " << configFilePath << std::endl;
            exit(1);
        }
        
        try {
            std::string var, val, url;
            while (configFile >> var >> val) {
                if (var == "crawlDelay") config.crawlDelay = std::stoi(val);
                else if (var == "maxThreads") config.maxThreads = std::stoi(val);
                else if (var == "depthLimit") config.depthLimit = std::stoi(val);
                else if (var == "pageLimit") config.pageLimit = std::stoi(val);
                else if (var == "linkedSitesLimit") config.linkedSitesLimit = std::stoi(val);
                else if (var == "startUrls") {
                    for (int i = 0; i < std::stoi(val); i++) {
                        configFile >> url;
                        config.startUrls.push_back(url);
                    }
                }
            }
            configFile.close();
        } catch (const std::exception& e) {
            std::cerr << "Error: Exception occurred while reading the configuration file: " << e.what() << std::endl;
            exit(1);
        }
    } else {
        config.startUrls = program.get<std::vector<std::string>>("startUrls");
    }

    // override config with provided args
    if (program.present<int>("--maxThreads")) {
        config.maxThreads = program.get<int>("--maxThreads");
    }
    if (program.present<int>("--crawlDepth")) {
        config.depthLimit = program.get<int>("--crawlDepth");
    }
    if (program.present<int>("--pageLimit")) {
        config.pageLimit = program.get<int>("--pageLimit");
    }
    if (program.present<int>("--linkedSitesLimit")) {
        config.linkedSitesLimit = program.get<int>("--linkedSitesLimit");
    }
    if (program.present<int>("--crawlDelay")) {
        config.crawlDelay = program.get<int>("--crawlDelay");
    }

    // add start URLs from command line if provided
    std::vector<std::string> startUrls = program.get<std::vector<std::string>>("startUrls");
    if (!startUrls.empty()) {
        config.startUrls.insert(config.startUrls.end(), startUrls.begin(), startUrls.end());
    }

    // check start URLs are provided
    if (config.startUrls.empty()) {
        std::cerr << "Error: No start URLs provided. Specify start URLs in the command line or configuration file." << std::endl;
        std::cerr << program;
        exit(1);
    }

    return config;
}

/**
 * @brief Pareses the configuration from the configured file.
 * 
 * @return The configuration object read from the file.
 */
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
        std::cerr << "Error: Exception occurred while reading the configuration file: " << error.what() << std::endl;
        exit(1);
    }
}


/**
 * @brief Initializes the crawler with start URLs.
 * 
 * This method initializes the crawler state with start URLs and marks them as discovered.
 */
void Crawler::initialize() {
    crawlerState.threadsCount = 0;
    for (auto& url : config.startUrls) {
        crawlerState.pendingSites.push(std::make_pair(getHostnameFromUrl(url), 0));
        crawlerState.discoveredSites[getHostnameFromUrl(url)] = true;
    }

    // init the CSV file
    initializeResultsFile();
    
    std::cout << "Crawler initialized" << std::endl;
}

void Crawler::initializeResultsFile() {
    std::ofstream csvFile("crawl_results.csv");
    if (csvFile.is_open()) {
        csvFile << "WEBSITE,DEPTH,PAGES DISCOVERED,FAILED QUERIES,LINKED SITES,MIN RESPONSE TIME (ms),MAX RESPONSE TIME (ms),AVG RESPONSE TIME (ms),DISCOVERED PAGES\n";
        csvFile.close();
    } else {
        std::cerr << "Error: Unable to open CSV file" << std::endl;
        exit(1);
    }
}


/**
 * @brief Schedules crawlers for processing URLs.
 * 
 * It schedules crawlers to process URLs until all URLs have been crawled or no more threads are available.
 */
void Crawler::scheduleCrawlers() {
    while (crawlerState.threadsCount != 0 || !crawlerState.pendingSites.empty()) {
        std::unique_lock<std::mutex> m_lock(m_mutex);

        while (!crawlerState.pendingSites.empty() && crawlerState.threadsCount < config.maxThreads) {
            auto nextSite = crawlerState.pendingSites.front();
            crawlerState.pendingSites.pop();
            crawlerState.threadsCount++;

            std::thread(&Crawler::startCrawler, this, nextSite.first, nextSite.second).detach();

            std::cout << "Thread scheduled for " << nextSite.first << " with path " << nextSite.second << std::endl;
        }

        m_condVar.wait(m_lock, [this] { return isThreadFinished; });
        isThreadFinished = false;
    }
}


/**
 * @brief Starts crawling a given URL.
 * 
* @param baseUrl The base URL of the website to crawl.
 * @param currentDepth The current depth of the crawling process.
 */
void Crawler::startCrawler(std::string baseUrl, int currentDepth) {
    Socket clientSocket(baseUrl, 80, config.pageLimit, config.crawlDelay);
    std::cout << "Crawling " << baseUrl << " at depth " << currentDepth << std::endl;
    Socket::SiteStats stats = clientSocket.initiateDiscovery();
    std::lock_guard<std::mutex> m_lock(m_mutex);

    // output the stats
    writeResultsToCsv(stats, currentDepth);
    writeResultsToConsole(stats, currentDepth);
    

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

void Crawler::writeResultsToConsole(const Socket::SiteStats& stats, int currentDepth) {
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
}


/**
 * @brief Writes the crawling results to a CSV file.
 * 
 * @param stats The statistics to write to the CSV file.
 */
void Crawler::writeResultsToCsv(const Socket::SiteStats& stats, int currentDepth) {
    std::lock_guard<std::mutex> csvLock(csvMutex);
    std::ofstream csvFile("crawl_results.csv", std::ios::app);
    if (csvFile.is_open()) {
        csvFile << stats.hostname << ",";
        csvFile << currentDepth << ",";
        csvFile << stats.discoveredPages.size() << ",";
        csvFile << stats.failedQueries << ",";
        csvFile << stats.linkedSites.size() << ",";
        csvFile << (stats.minResponseTime < 0 ? "-" : std::to_string(stats.minResponseTime)) << ",";
        csvFile << (stats.maxResponseTime < 0 ? "-" : std::to_string(stats.maxResponseTime)) << ",";
        csvFile << (stats.averageResponseTime < 0 ? "-" : std::to_string(stats.averageResponseTime)) << ",";
        
        if (stats.discoveredPages.empty()) {
            csvFile << "None";
        } else {
            for (size_t i = 0; i < stats.discoveredPages.size(); ++i) {
                csvFile << stats.discoveredPages[i].first;
                if (i != stats.discoveredPages.size() - 1) {
                    csvFile << "; "; // semicolon as delimiter n the cell
                }
            }
        }
        csvFile << "\n";

        csvFile.close();
    } else {
        std::cerr << "Error opening CSV file for writing." << std::endl;
    }
}


int main(int argc, char *argv[]) {
    Config config;

    try {
        config = parseCommandLineArgs(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse command line arguments. " << e.what() << std::endl;
        return 1;
    }

    try {
        Crawler crawler(config);
        crawler.start();
        std::cout << "Crawler finished" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: Exception occurred during crawling. " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
