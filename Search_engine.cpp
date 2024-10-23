#include "Search_engine.h"

std::vector<std::string> SearchEngine::preprocessText(const std::string& text, const std::string& language) {
    std::string lowerText = text;


    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);

    std::stringstream ss(lowerText);
    std::string word;
    std::vector<std::string> tokens;


    auto stopwords = stopwordsPerLanguage.find(language);
    if (stopwords == stopwordsPerLanguage.end()) {
        throw std::runtime_error("Language not supported");
    }


    while (ss >> word) {
        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
        if (stopwords->second.find(word) == stopwords->second.end()) {
            Porter2Stemmer::stem(word);
            tokens.push_back(word);
        }
    }

    return tokens;
}


SearchEngine::SearchEngine() {
    initializeStopwords();  
}


void SearchEngine::initializeStopwords() {
    stopwordsPerLanguage["en"] = {"the", "is", "and", "a", "in"};
    stopwordsPerLanguage["es"] = {"el", "la", "es", "y", "en"};
    stopwordsPerLanguage["fr"] = {"le", "la", "est", "et", "dans"};
}

void SearchEngine::addToIndex(const std::vector<std::string>& tokens, int docID) {
    sqlite3* db;
    sqlite3_open("search_index.db", &db);

    sqlite3_exec(db, create_tokens_table, 0, 0, nullptr);

    for (const auto& token : tokens) {
        std::string insert_token = "INSERT INTO tokens (token, docID) VALUES ('" + token + "', " + std::to_string(docID) + ");";
        sqlite3_exec(db, insert_token.c_str(), 0, 0, nullptr);
    }

    sqlite3_close(db);
}


void SearchEngine::indexDocuments(const std::vector<std::string>& docs, const std::string& language) {
    sqlite3* db;
    if (sqlite3_open("search_index.db", &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    sqlite3_exec(db, create_docs_table, nullptr, nullptr, nullptr);
    sqlite3_exec(db, create_tokens_table, nullptr, nullptr, nullptr);

    for (size_t i = 0; i < docs.size(); ++i) {
        std::string insert_doc = "INSERT INTO documents (docID, content) VALUES (?, ?);";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, insert_doc.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
            continue;
        }

        sqlite3_bind_int(stmt, 1, static_cast<int>(i));
        

        std::string content = docs[i];
        content = std::regex_replace(content, std::regex("'"), "''"); 
        sqlite3_bind_text(stmt, 2, content.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Error executing statement: " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_finalize(stmt);

        auto tokens = preprocessText(docs[i], language);
        addToIndex(tokens, static_cast<int>(i)); 
    }

    sqlite3_close(db);
}



std::set<int> SearchEngine::search(const std::string& query, const std::string& language) {
    if (queryCache.find(query) != queryCache.end()) {
        return queryCache[query]; 
    }
    auto tokens = preprocessText(query, language);
    std::set<int> result;
    sqlite3* db;

    if (sqlite3_open("search_index.db", &db) != SQLITE_OK) {
        throw std::runtime_error("Cannot open database");
    }

    std::set<std::string> allTokens(tokens.begin(), tokens.end());
    for (const auto& token : tokens) {
        if (synonymMap.find(token) != synonymMap.end()) {
            allTokens.insert(synonymMap[token].begin(), synonymMap[token].end());
        }
    }

    for (const auto& token : tokens) {
        std::string query_token = "SELECT docID FROM tokens WHERE token = ?;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, query_token.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                result.insert(sqlite3_column_int(stmt, 0));
            }
        }

        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
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

void SearchEngine::indexWebPage(const std::string& url, int docID, const std::string& language) {
    std::string htmlContent = fetchWebPage(url);
    std::string textContent = extractTextFromHTML(htmlContent);
    
    auto tokens = preprocessText(textContent, language);
    
    addToIndex(tokens, docID);
}


void SearchEngine::fetchMutliplteWebPages(const std::vector<std::string> &urls, std::vector<std::string> &contents)
{
    CURLM* multi_handle = curl_multi_init();
    std::vector<CURL*> curl_handles(urls.size());
    contents.resize(urls.size());

    for (size_t i = 0; i < urls.size(); ++i) {
        curl_handles[i] = curl_easy_init();
        curl_easy_setopt(curl_handles[i], CURLOPT_URL, urls[i].c_str());
        curl_easy_setopt(curl_handles[i], CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_handles[i], CURLOPT_WRITEDATA, &contents[i]);
        curl_multi_add_handle(multi_handle, curl_handles[i]);
    }

    int still_running;
    curl_multi_perform(multi_handle, &still_running);

    while (still_running) {
        curl_multi_perform(multi_handle, &still_running);
    }
    for (size_t i = 0; i < urls.size(); ++i) {
        curl_multi_remove_handle(multi_handle, curl_handles[i]);
        curl_easy_cleanup(curl_handles[i]);
    }
    curl_multi_cleanup(multi_handle);
}

void SearchEngine::computeTFIDF(const std::vector<std::string> &docs, const std::string& language) {
    sqlite3* db;
    sqlite3_open("search_index.db", &db);
    sqlite3_exec(db, create_tfidf_table, 0, 0, nullptr);

    std::unordered_map<std::string, int> docFreq;
    int numDocs = docs.size();

    for (int docID = 0; docID < numDocs; ++docID) {

        auto tokens = preprocessText(docs[docID], language);
        std::unordered_map<std::string, int> termCount;


        for (const auto& token : tokens) {
            termCount[token]++;
        }


        for (const auto& term : termCount) {
            docFreq[term.first]++;
            double tf = term.second / static_cast<double>(tokens.size());
            double idf = log(static_cast<double>(numDocs) / (1 + docFreq[term.first]));
            double tfidfValue = tf * idf;


            std::string insert_tfidf = "INSERT INTO tfidf (token, docID, tfidfValue) VALUES ('" + term.first + "', " + std::to_string(docID) + ", " + std::to_string(tfidfValue) + ");";
            sqlite3_exec(db, insert_tfidf.c_str(), 0, 0, nullptr);
        }
    }

    sqlite3_close(db);
}

void SearchEngine::initializeSynonyms()
{
    synonymMap["fast"] = {"quick", "speedy"};
    synonymMap["slow"] = {"lethargic", "unhurried"};
}

void SearchEngine::indexDocumentsParallel(const std::vector<std::string> &docs, const std::string &language)
{
    std::vector<std::thread> threads;

    for (size_t i = 0; i < docs.size(); ++i) {
        threads.emplace_back([&, i]() {
            auto tokens = preprocessText(docs[i], language); 
            std::lock_guard<std::mutex> guard(indexMutex);
            addToIndex(tokens, static_cast<int>(i)); 
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

std::vector<std::pair<int, double>> SearchEngine::searchTFIDF(const std::string& query, const std::string& language) {

    auto tokens = preprocessText(query, language);
    std::unordered_map<int, double> scores;


    for (const auto& token : tokens) {
        auto it = tfidf.find(token);
        if (it != tfidf.end()) {
            for (const auto& [docID, tfidfValue] : it->second) {
                scores[docID] += tfidfValue;
            }
        }
    }


    std::vector<std::pair<int, double>> rankedResults(scores.begin(), scores.end());
    std::sort(rankedResults.begin(), rankedResults.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    return rankedResults;
}
