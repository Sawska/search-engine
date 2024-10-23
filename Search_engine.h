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
#include <unordered_set>
#include <cmath>
#include <thread>
#include <mutex>
#include <sqlite3.h>
#include <porter2_stemmer.h>

class SearchEngine {
    public:
    void addToIndex(const std::vector<std::string>& tokens,int docID);
    std::set<int> SearchEngine::search(const std::string& query, const std::string& language);
    std::vector<std::string> preprocessText(const std::string& text, const std::string& language);
    size_t WriteCallback(void* contents,size_t size, size_t nmemb, std::string* s);
    std::string fetchWebPage(const std::string& url);
    std::string extractTextFromHTML(const std::string& html);
    void indexWebPage(const std::string& url, int docID, const std::string& language);
    void fetchMutliplteWebPages(const std::vector<std::string>& urls,std::vector<std::string>& contents);
    void computeTFIDF(const std::vector<std::string> &docs, const std::string& language);
    void initializeSynonyms();
    void indexDocumentsParallel(const std::vector<std::string>& docs, const std::string& language);
    SearchEngine();
    void indexDocuments(const std::vector<std::string>& docs, const std::string& language);
    private: 
    const char* create_docs_table = "CREATE TABLE IF NOT EXISTS documents (docID INTEGER PRIMARY KEY, content TEXT);";
    const char* create_tokens_table = "CREATE TABLE IF NOT EXISTS tokens (token TEXT, docID INTEGER);";
    const char* create_tfidf_table = "CREATE TABLE IF NOT EXISTS tfidf (token TEXT, docID INTEGER, tfidfValue REAL);";
    std::mutex indexMutex;
    std::unordered_map<std::string,std::set<int>> invertedIndex;
    std::unordered_map<std::string, std::unordered_set<std::string>> stopwordsPerLanguage;
    std::unordered_map<std::string,std::unordered_map<int,double>> tfidf;
    std::unordered_map<std::string, std::set<int>> queryCache;
    std::vector<std::pair<int, double>> searchTFIDF(const std::string& query, const std::string& language);
    std::unordered_map<std::string, std::unordered_set<std::string>> synonymMap;
    void computeTFIDF(const std::vector<std::string> &docs, const std::string& language);
    void SearchEngine::initializeStopwords();
};


#endif //SEARCH_ENGINE_H