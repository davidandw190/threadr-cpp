#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

struct Config {
    int crawlDelay = 1500;
    int maxThreads = 15;
    int depthLimit = 5;
    int pageLimit = 20;
    int linkedSitesLimit = 20;
    bool verbose = false;
    bool enableCSVOutput = false;
    bool disableConsoleOutput = false;
    std::vector<std::string> startUrls;
};

#endif
