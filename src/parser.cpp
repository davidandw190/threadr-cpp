#include "parser.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>

const std::vector<std::string> URL_PREFIXES = {"https://", "http://"};
const std::vector<std::string> URL_STARTS = {"href=\"", "href = \"", "http://", "https://"};
const std::string URL_END_CHARS = "\"#?, ";

std::string getHostnameFromUrl(const std::string& url) {
    for (const auto& prefix : URL_PREFIXES) {
        if (url.compare(0, prefix.size(), prefix) == 0) {
            size_t start = prefix.size();
            size_t end = url.find('/', start);
            return url.substr(start, end == std::string::npos ? std::string::npos : end - start);
        }
    }
    return "";
}

std::string getHostPathFromUrl(const std::string& url) {
    for (const auto& prefix : URL_PREFIXES) {
        if (url.compare(0, prefix.size(), prefix) == 0) {
            size_t start = url.find('/', prefix.size());
            return start == std::string::npos ? "/" : url.substr(start);
        }
    }
    return "/";
}

std::vector<std::pair<std::string, std::string>> extractUrls(const std::string& httpText) {
    std::string httpRaw = reformatHttpResponse(httpText);
    
    std::vector<std::pair<std::string, std::string>> extractedUrls;

    for (const auto& startText : URL_STARTS) {
        size_t startPos = 0;

        while ((startPos = httpRaw.find(startText, startPos)) != std::string::npos)  {
            startPos += startText.size();
            size_t endPos = httpRaw.find_first_of(urlEndChars, startPos);

            if (endPos == std::string::npos) {
                endPos = httpRaw.size();
            }

            std::string foundUrl = httpRaw.substr(startPos, endPos - startPos);

            if (verifyUrl(foundUrl)) {
                extractedUrls.push_back({getHostnameFromUrl(foundUrl), getHostPathFromUrl(foundUrl)});
            }
            
            httpRaw.erase(0, endPos);
        }
    }

    return extractedUrls;
}

std::string reformatHttpResponse(const std::string& httpText) {
    const std::string ALLOWED_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01233456789.,/\":#?+-_= ";
    std::map<char, char> charMap;
    
    for (char ch : ALLOWED_CHARS) {
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

bool verifyUrl(const std::string& url) {
    std::string urlDomain = getHostnameFromUrl(url);
    return urlDomain.empty() || verifyDomain(urlDomain);
}

bool verifyDomain(const std::string& url) {
    const std::string ALLOWED_DOMAINS[] = {".com", ".sg", ".net", ".co", ".org", ".me"};
    for (const auto& domain : ALLOWED_DOMAINS) {
        if (hasSuffix(url, domain)) {
            return true;
        }
    }
    return false;
}

bool verifyType(const std::string& url) {
    const std::string FORBIDDEN_TYPES[] = {".css", ".js", ".pdf", ".png", ".jpeg", ".jpg", ".ico"};
    for (const auto& type : FORBIDDEN_TYPES) {
        if (url.find(type) != std::string::npos) {
            return false;
        }
    }
    return true;
}

bool hasSuffix(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}