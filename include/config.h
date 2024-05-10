#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

struct Config {
    int crawlDelay = 1000;
    int maxThreads = 10;
    int depthLimit = 10;
    int pagesLimit = 10;
    int linkedSitesLimit = 10;
    std::vector<std::string> startUrls;
};

#endif
