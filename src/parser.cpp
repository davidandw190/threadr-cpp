#include "parser.h"
#include <iostream>
#include <string>
#include <vector>

std::string getHostnameFromUrl(const std::string& url) {
    const std::string prefixes[] = {"https://", "http://"};
    for (const auto& prefix : prefixes) {
        if (url.compare(0, prefix.size(), prefix) == 0) {
            size_t start = prefix.size();
            size_t end = url.find('/', start);
            return url.substr(start, end == std::string::npos ? std::string::npos : end - start);
        }
    }
    return "";
}

std::string getHostPathFromUrl(const std::string& url) {
    const std::string prefixes[] = {"https://", "http://"};
    for (const auto& prefix : prefixes) {
        if (url.compare(0, prefix.size(), prefix) == 0) {
            size_t start = url.find('/', prefix.size());
            return start == std::string::npos ? "/" : url.substr(start);
        }
    }
    return "/";
}

std::vector<std::pair<std::string, std::string>> extractUrls(const std::string& httpText) {
    std::string httpRaw = reformatHttpResponse(httpText);
    const std::string urlStarts[] = {"href=\"", "href = \"", "http://", "https://"};
    const std::string urlEndChars = "\"#?, ";
    
    std::vector<std::pair<std::string, std::string>> extractedUrls;

    for (const auto& startText : urlStarts) {
        size_t startPos = 0;

        while ((startPos = httpRaw.find(startText, startPos)) != std::string::npos)  {
            startPos += startText.size();
            size_t endPos = httpRaw.find_first_of(urlEndChars, startPos);

            if (endPos == std::string::npos) {
                endPos = httpRaw.size();
            }

            std::string foundUrl = httpRaw.substr(startPos, endPos - startPos);

            extractedUrls.push_back({getHostnameFromUrl(foundUrl), getHostPathFromUrl(foundUrl)});
            
            httpRaw.erase(0, endPos);
        }
    }

    return extractedUrls;
}

std::string reformatHttpResponse(const std::string& httpText) {
    std::string allowedChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01233456789.,/\":#?+-_= ";
    std::map<char, char> charMap;
    
    for (char ch : allowedChars) {
        charMap[ch] = ch;
    }
    
    charMap['\n'] = ' ';

    std::string result;
    for (char ch : httpText) {
        if (charMap.find(ch) != charMap.end()) {
            result += tolower(charMap[ch]);
        }
    }
    return result;
}