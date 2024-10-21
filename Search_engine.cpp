#include "Search_engine.h"

std::vector<std::string> SearchEngine::preprocessText(const std::string& text) {
    std::string lowerText = text;
    std::transform(lowerText.begin(),lowerText.end(),lowerText.begin(), ::tolower);
    std::stringstream ss(lowerText);
    std::string word;
    std::vector<std::string> tokens;

    while(ss >> word) {
        word.erase(std::remove_if(word.begin(),word.end(), ::ispunct),word.end());
        tokens.push_back(word);
    }
    return tokens;
};

void SearchEngine::addToIndex(const std::vector<std::string>& tokens, int docID) {
    for (const auto& token : tokens) {
        invertedIndex[token].insert(docID);
    }
}

void SearchEngine::indexDocuments(const std::vector<std::string&> docs) {
    for(int i =0;i<docs.size(); ++i) {
        auto tokens = preprocessText(docs[i]);
        addToIndex(tokens,i);
    }
}

std::set<int> SearchEngine::search(const std::string& query) {
    auto tokens = preprocessText(query);
    std::set<int> result;
    
    if (!tokens.empty()) {
        result = invertedIndex[tokens[0]];  

        for (const auto& token : tokens) {
            std::set<int> temp = invertedIndex[token];
            std::set<int> intersection;
            std::set_intersection(result.begin(), result.end(),
                                  temp.begin(), temp.end(),
                                  std::inserter(intersection, intersection.begin()));
            result = intersection;
        }
    }
    return result;  
}

size_t SearchEngine::WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s)
{
    size_t newLength = size * nmemb;
    s->append((char*)contents,newLength);
    return newLength;
}

std::string SearchEngine::fetchWebPage(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string pageContent;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();


    if(curl) {
        curl_easy_setopt(curl,CURLOPT_URL,url.c_str());
        curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,WriteCallback);
        curl_easy_setopt(curl,CURLOPT_WRITEDATA,&pageContent);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return pageContent;
}


std::string SearchEngine::extractTextFromHTML(const std::string& html) {
    std::string text;
    std::regex htmlTags("<[^>]*>");
    text = std::regex_replace(html,htmlTags," ");
    return text;
}

void SearchEngine::indexWebPage(const std::string& url, int docID) {
    std::string htmlContent = fetchWebPage(url);
    std::string textContent = extractTextFromHTML(htmlContent);
    auto tokens = preprocessText(textContent);
    addToIndex(tokens,docID);
}