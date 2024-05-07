
#include "crawler.h"
#include <fstream>

int main() {
    Crawler crawler;
    crawler.crawl();

    std::ofstream outputFile("results.txt", std::ios::trunc); 
     std::string testString = "Test string\n";

    if (outputFile) {
        std::string results = testString;;
        outputFile << results;
        outputFile.close();
    } else {
        std::cerr << "Error: Unable to open results.txt for writing!" << std::endl;
        return 1;
    }

    return 0;
}
