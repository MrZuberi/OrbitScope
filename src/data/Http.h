// Implements the Http class that provides HTTP client functionality
#pragma once

#include <string>
#include <vector>
#include <utility>

class Http
{
public:
    static bool Get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers, std::string& response);
};