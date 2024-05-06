#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

std::string getHostnameFromUrl(const std::string& url);

std::vector<std::pair<std::string, std::string>> extractUrls(const std::string& httpRaw);

bool verifyUrl(const std::string& url);

#endif // PARSER_H