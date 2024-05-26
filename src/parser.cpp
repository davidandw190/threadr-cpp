/**
 * @file parser.cpp
 * @brief Implementation of utility functions for parsing and processing URLs and HTTP responses.
 * 
 * It provides the implementation of  utility functions used for parsing URLs, extracting URLs 
 * from HTTP responses, and performing URL validation checks.
 */

#include "parser.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

const std::vector<std::string> URL_PREFIXES = {"https://", "http://"};
const std::vector<std::string> URL_STARTS = {"href=\"", "href='", "src=\"", "src='", "url(", "http://", "https://"};
const std::string URL_END_CHARS = "\"'#? ),";

/**
 * @brief Extracts the hostname from a given URL.
 * 
 * @param url The URL from which to extract the hostname.
 * @return The extracted hostname.
 */
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

/**
 * @brief Extracts the host path from a given URL.
 * 
 * @param url The URL from which to extract the host path.
 * @return The extracted host path.
 */
std::string getHostPathFromUrl(const std::string& url) {
    for (const auto& prefix : URL_PREFIXES) {
        if (url.compare(0, prefix.size(), prefix) == 0) {
            size_t start = url.find('/', prefix.size());
            return start == std::string::npos ? "/" : url.substr(start);
        }
    }
    return "/";
}

/**
 * @brief Checks if a string ends with a given suffix.
 * 
 * @param str The string to check.
 * @param suffix The suffix to check for.
 * @return True if the string ends with the suffix, otherwise false.
 */
bool hasSuffix(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/**
 * @brief Verifies if a domain is allowed based on a list of predefined domains.
 * 
 * @param url The URL to verify.
 * @return True if the domain is allowed, otherwise false.
 */
bool verifyDomain(const std::string& url) {
    const std::string ALLOWED_DOMAINS[] = {".com", ".sg", ".net", ".co", ".org", ".me", ".ro", ".html", ".htmx", ".gov", ".edu", ".uk", ".io", ".info", ".biz", ".us", ".ca", ".au", ".de", ".fr", ".it", ".nl", ".se", ".no", ".jp", ".br", ".es", ".mx", ".ru", ".ch", ".at", ".dk", ".be", ".nz", ".pl", ".cz", ".gr", ".pt", ".fi", ".hu", ".cn", ".tr", ".kr", ".tw", ".hk", ".vn", ".id", ".ph", ".my", ".th", ".ae", ".sa", ".il", ".eg", ".za", ".ua", ".ar", ".cl", ".pe", ".co", ".ve", ".ec", ".bo", ".py", ".uy", ".cr", ".pa", ".do", ".gt", ".sv", ".hn", ".ni", ".pr", ".jm", ".bb", ".tt", ".bs", ".gd", ".lc", ".vc", ".sr", ".gy", ".mq", ".gp", ".gf", ".aw", ".cw", ".sx", ".bq", ".an", ".pm", ".gl", ".fo", ".is", ".ie", ".lu", ".mc", ".ad", ".li", ".je", ".gg", ".im", ".gi", ".mt", ".cy", ".ax", ".fk", ".gs", ".bv", ".hm", ".tf", ".um", ".aq", ".io", ".sh", ".ac", ".cp", ".dg", ".eu", ".int", ".mil", ".museum", ".aero", ".arpa", ".cat", ".coop", ".jobs", ".pro", ".tel", ".travel", ".be", ".nz", ".pl",};
    for (const auto& domain : ALLOWED_DOMAINS) {
        if (hasSuffix(url, domain)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Verifies if a URL is valid based on allowed domains.
 * 
 * @param url The URL to verify.
 * @return True if the URL is valid, otherwise false.
 */
bool verifyUrl(const std::string& url) {
    std::string urlDomain = getHostnameFromUrl(url);
    return urlDomain.empty() || verifyDomain(urlDomain);
}

/**
 * @brief Verifies if the URL type is allowed.
 * 
 * @param url The URL to verify.
 * @return True if the URL type is allowed, otherwise false.
 */
bool verifyType(const std::string& url) {
    const std::string FORBIDDEN_TYPES[] = {".css", ".pdf", ".png", ".jpeg", ".jpg", ".ico"};
    for (const auto& type : FORBIDDEN_TYPES) {
        if (url.find(type) != std::string::npos) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Reformat HTTP response text to lowercase and remove unwanted characters.
 * 
 * @param httpText The HTTP response text to reformat.
 * @return The reformatted HTTP response text.
 */
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


/**
 * @brief Extracts URLs from HTTP response text.
 * 
 * @param httpText The HTTP response text containing URLs.
 * @param baseUrl The base URL from which the HTTP response was retrieved.
 * @return A vector of pairs representing the extracted URLs and their corresponding host paths.
 */
std::vector<std::pair<std::string, std::string>> extractUrls(const std::string& httpText, const std::string& baseUrl) {
    std::string httpRaw = reformatHttpResponse(httpText);
    std::vector<std::pair<std::string, std::string>> extractedUrls;

    size_t startPos = 0;
    std::string baseHost = getHostnameFromUrl(baseUrl);

    while (startPos < httpRaw.size()) {
        size_t foundPos = std::string::npos; // pos of found url
        std::string foundUrl; // the actual found url

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

            // Ensure correct handling of relative URLs
            if (!foundUrl.empty() && foundUrl[0] == '/') {

                // std::cout << " >>>>>BR1 BEF>>>>> Found URL: " << foundUrl << std::endl;

                foundUrl = baseUrl + foundUrl;
            } else if (!foundUrl.empty() && foundUrl.find("http") == std::string::npos) {
                
                // std::cout << " >>>>>BR2 BEF>>>>> Found URL: " << foundUrl << std::endl;

                foundUrl = "http:/" + baseHost + "/" + foundUrl;
            }

            // std::cout << " >>>>>>>>>>>>>>>> Found URL: " << foundUrl << std::endl;

            if (verifyUrl(foundUrl) && verifyType(foundUrl)) {
                std::string urlHost = getHostnameFromUrl(foundUrl);
                if (!urlHost.empty()) {
                    extractedUrls.push_back({urlHost, getHostPathFromUrl(foundUrl)});
                }
            }
            startPos = endPos;
        } else {
            break;
        }
    }

    return extractedUrls;
}

