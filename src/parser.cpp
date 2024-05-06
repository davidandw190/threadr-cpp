#include "parser.h"
#include <iostream>
#include <string>
#include <vector>

std::string getHostnameFromUrl(const std::string& url) {
    int offset = 0;
    offset = offset == 0 && url.compare(0, 8, "https://")==0 ? 8 : offset;
    offset = offset== 0 && url.compare(0, 7, "http://" )==0 ? 7 : offset;

    size_t pos = url.find('/', offset);
    std::string domain = url.substr(offset, (pos == string::npos ? url.length() : pos) - offset);

    return domain;
}

std::string getHostPathFromUrl(std::string url) {
    int offset = 0;
    offset = offset == 0 && url.compare(0, 8, "https://")==0 ? 8 : offset;
    offset = offset == 0 && url.compare(0, 7, "http://" )==0 ? 7 : offset;
    
    size_t pos = url.find("/", offset);
    std::string path = pos == string::npos ? "/" : url.substr(pos);

    // Remove extra slashes
    pos = path.find_first_not_of('/');
    if (pos == string::npos) path = "/";
    	else path.erase(0, pos - 1);
	
    return path;
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