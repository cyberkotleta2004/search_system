#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <execution>
#include <cmath>
#include <numeric>

static constexpr size_t MAX_RESULT_DOCUMENT_COUNT = 5;

struct Document {
    int id;
    int rating;
    double relevance;
};

struct Query {
    std::vector<std::string> plus_words;
    std::vector<std::string> minus_words;
};

class SearchServer {
private:
    std::map<std::string, std::map<int, double>> word_to_documents_freqs_;
    std::set<std::string> stop_words_;
    std::map<int, int> document_to_rating;
    int document_count_ = 0;

public:
    void AddDocument(int document_id, const std::string& document, const std::vector<int>& rates) {
        int avg_rating = ComputeAverageRating(rates);
        document_to_rating[document_id] = avg_rating;

        std::vector<std::string> words_no_stop = SplitIntoWordsNoStop(document);
        if(words_no_stop.empty()) return;

        std::map<std::string, int> words_to_count;
        for (const std::string& word : words_no_stop) {
            ++words_to_count[word];
        }
        for(const auto& [word, count] : words_to_count) {
            word_to_documents_freqs_[word].insert({document_id, static_cast<double>(count) / words_no_stop.size()});
        }
    }

    void SetStopWords(const std::string& text) {
        for (const std::string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void setDocumentsCount(int documents_count) {
        document_count_ = documents_count;
    }

    int GetDocumentsCount() const noexcept {
        return document_count_;
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

    int GetRating(int document_id) const {
        if(auto it = document_to_rating.find(document_id); it != document_to_rating.end()) {
            return it->second;
        }
        return 0;
    }

    std::vector<Document> FindAllDocuments(const std::string& query) const {
        const Query query_words = ParseQuery(query);
        std::map<int, double> document_to_relevance;

        for (const std::string& plus_word : query_words.plus_words) {
            if (word_to_documents_freqs_.count(plus_word) == 0) {
                continue;
            }

            int word_meet_count = word_to_documents_freqs_.at(plus_word).size();
            for (const auto& [document_id, tf] : word_to_documents_freqs_.at(plus_word)) {
                double idf = std::log(static_cast<double>(document_count_) / word_meet_count);
                document_to_relevance[document_id] += tf * idf;
            }
        }

        for(const std::string& minus_word : query_words.minus_words) {
            if(word_to_documents_freqs_.count(minus_word) == 0) {
                continue;
            }

            for (const auto [document_id, _] : word_to_documents_freqs_.at(minus_word)) {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> found_documents;
        for (auto [document_id, relevance] : document_to_relevance) {
            found_documents.push_back({document_id, GetRating(document_id), relevance});
        }
        return found_documents;
    }

    static int ComputeAverageRating(const std::vector<int>& rates) {
        int rates_summary = std::reduce(std::execution::par, rates.begin(), rates.end(), 0);
        int rates_size = rates.size();
        int avg_rating = rates_summary / rates_size;

        return avg_rating;
    }
};

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

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    const int document_count = ReadLineWithNumber();
    search_server.setDocumentsCount(document_count);

    for (int document_id = 0; document_id < document_count; ++document_id) {
        std::string text = ReadLine();

        int rates_count; std::cin >> rates_count;
        std::vector<int> rates(rates_count);

        int current_rate;
        for(int i = 0; i < rates_count; ++i) {
            std::cin >> current_rate;
            rates[i] = current_rate;
        }
        std::cin.ignore();

        search_server.AddDocument(document_id, text, rates);
    }
    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();
    const std::string query = ReadLine();

    for (auto [document_id, rating, relevance]: search_server.FindTopDocuments(query)) {
        std::cout << "{ document_id = " << document_id << ", relevance = " << relevance << ", rating = " << rating << " }" << std::endl;
    }
}