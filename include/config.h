#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

struct Config {
    int crawlDelay = 2000;
    int maxThreads = 15;
    int depthLimit = 15;
    int pageLimit = 100;
    int linkedSitesLimit = 100;
    bool verbose = false;
    bool enableCSVOutput = false;
    bool disableConsoleOutput = false;
    std::vector<std::string> startUrls;
};

#endif
