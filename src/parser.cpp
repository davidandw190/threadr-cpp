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
        // TODO: Implement parsing !!
    }

    return extractedUrls;
}

std::string reformatHttpResponse(const std::string& httpText) {
    // TODO: Implement this !!
}