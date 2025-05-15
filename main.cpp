#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <execution>
#include <chrono>

static constexpr size_t MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer;

struct Document {
    int id;
    int relevance;
};

struct Query {
    std::vector<std::string> plus_words;
    std::vector<std::string> minus_words;
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
    return search_server;
}

std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}

class SearchServer {
private:
    std::map<std::string, std::set<int>> word_to_documents_;
    std::set<std::string> stop_words_;

public:
    void AddDocument(int document_id, const std::string& document) {
        for (const std::string& word : SplitIntoWordsNoStop(document)) {
            word_to_documents_[word].insert(document_id);
        }
    }

    void SetStopWords(const std::string& text) {
        for (const std::string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    std::vector<Document> FindTopDocuments(const std::string& query) const {
        std::vector<Document> top_documents;
        top_documents.reserve(MAX_RESULT_DOCUMENT_COUNT);

        std::vector<Document> all_documents = FindAllDocuments(query);
        std::sort(std::execution::par, 
             all_documents.begin(), 
             all_documents.end(), 
             [](const Document& lhd, const Document& rhd) {
                 return lhd.relevance > rhd.relevance;
             });

        for(int i = 0; i < all_documents.size(); ++i) {
            if(i == MAX_RESULT_DOCUMENT_COUNT) break;
            top_documents.push_back(all_documents[i]);
        }
        return top_documents;
    }
private:  
    std::vector<std::string> SplitIntoWords(const std::string& text) const {
        std::vector<std::string> words;
        std::string word;
        for (const char c : text) {
            if (c == ' ') {
                words.push_back(word);
                word = "";
            } else {
                word += c;
            }
        }
        words.push_back(word);

        return words;
    }

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const {
        std::vector<std::string> words;
        for (const std::string& word : SplitIntoWords(text)) {
            if (stop_words_.count(word) == 0) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const std::string& query) const {
        Query query_words;
        for (const std::string& word : SplitIntoWords(query)) {
            if(word[0] == '-') {
                query_words.minus_words.push_back(word.substr(1));
            } else {
                query_words.plus_words.push_back(word);
            }
        }
        return query_words;
    }

    std::vector<Document> FindAllDocuments(const std::string& query) const {
        const Query query_words = ParseQuery(query);
        std::map<int, int> document_to_relevance;

        for (const std::string& plus_word : query_words.plus_words) {
            if (word_to_documents_.count(plus_word) == 0) {
                continue;
            }
            for (const int document_id : word_to_documents_.at(plus_word)) {
                ++document_to_relevance[document_id];
            }
        }

        for(const std::string& minus_word : query_words.minus_words) {
            if(word_to_documents_.count(minus_word) == 0) {
                continue;
            }

            for (const int document_id : word_to_documents_.at(minus_word)) {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> found_documents;
        for (auto [document_id, relevance] : document_to_relevance) {
            found_documents.push_back({document_id, relevance});
        }
        return found_documents;
    }
};

int main() {
    const SearchServer search_server = CreateSearchServer();
    const std::string query = ReadLine();

    for (auto [document_id, relevance]: search_server.FindTopDocuments(query)) {
        std::cout << "{ document_id = " << document_id << ", relevance = " << relevance << " }" << std::endl;
    }

}