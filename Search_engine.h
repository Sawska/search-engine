#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <set>
#include <curl/curl.h>
#include <regex>

class SearchEngine {
    std::unordered_map<std::string,std::set<int>> invertedIndex;
    std::vector<std::string> preprocessText(const std::string& text);
    void addToIndex(const std::vector<std::string>& tokens,int docID);
    void indexDocuments(const std::vector<std::string&> docs);
    std::set<int> search(const std::string& query);
    size_t WriteCallback(void* contents,size_t size, size_t nmemb, std::string* s);
    std::string fetchWebPage(const std::string& url);
    std::string extractTextFromHTML(const std::string& html);
    void indexWebPage(const std::string& url, int docID);

};


#endif //SEARCH_ENGINE_H