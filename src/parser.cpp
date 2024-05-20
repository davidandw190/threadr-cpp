#include "parser.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

const std::vector<std::string> URL_PREFIXES = {"https://", "http://"};
const std::vector<std::string> URL_STARTS = {"href=\"", "href='", "src=\"", "src='", "url(", "http://", "https://"};
const std::string URL_END_CHARS = "\"'#? ),";

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

bool hasSuffix(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
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

bool verifyUrl(const std::string& url) {
    std::string urlDomain = getHostnameFromUrl(url);
    return urlDomain.empty() || verifyDomain(urlDomain);
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

std::string reformatHttpResponse(const std::string& httpText) {
    const std::string ALLOWED_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,/\":#?+-_= ";
    std::map<char, char> charMap;

    for (char ch : ALLOWED_CHARS) {
        charMap[ch] = ch;
    }

    charMap['\n'] = ' ';
    charMap['\t'] = ' ';

    std::string result;
    for (char ch : httpText) {
        if (charMap.find(ch) != charMap.end()) {
            result += tolower(charMap[ch]);
        }
    }
    return result;
}

std::vector<std::pair<std::string, std::string>> extractUrls(const std::string& httpText) {
    std::string httpRaw = reformatHttpResponse(httpText);
    
    std::vector<std::pair<std::string, std::string>> extractedUrls;

    size_t startPos = 0;

    while (startPos < httpRaw.size()) {
        size_t foundPos = std::string::npos;
        std::string foundUrl;

        for (const auto& startText : URL_STARTS) {
            foundPos = httpRaw.find(startText, startPos);
            if (foundPos != std::string::npos) {
                foundPos += startText.size();
                break;
            }
        }

        if (foundPos != std::string::npos) {
            size_t endPos = httpRaw.find_first_of(URL_END_CHARS, foundPos);
            if (endPos == std::string::npos) {
                endPos = httpRaw.size();
            }
            foundUrl = httpRaw.substr(foundPos, endPos - foundPos);

            // Handling relative URLs
            if (foundUrl.compare(0, 1, "/") == 0) {
                foundUrl = "http://" + getHostnameFromUrl(httpRaw) + foundUrl;
            } else if (foundUrl.compare(0, 2, "./") == 0) {
                foundUrl = "http://" + getHostnameFromUrl(httpRaw) + "/" + foundUrl.substr(2);
            }

            if (verifyUrl(foundUrl) && verifyType(foundUrl)) {
                extractedUrls.push_back({getHostnameFromUrl(foundUrl), getHostPathFromUrl(foundUrl)});
            }
            startPos = endPos;
        } else {
            break;
        }
    }

    return extractedUrls;
}
