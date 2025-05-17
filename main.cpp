#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <execution>
#include <cmath>
#include <numeric>
#include <type_traits>

using namespace std::string_literals;

static constexpr size_t MAX_RESULT_DOCUMENT_COUNT = 5;
static constexpr double EPSILON = 1e-6;

class SearchServer {
public:
    enum class DocumentStatus {
        ACTUAL,
        IRRELEVANT,
        BANNED,
        REMOVED
    };

    struct Document {
        int id;
        int rating;
        double relevance;
    };

private:
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

private:
    std::map<std::string, std::map<int, double>> word_to_documents_freqs_;
    std::set<std::string> stop_words_;
    std::map<int, int> document_to_rating;
    std::map<int, DocumentStatus> document_to_status;
    int document_count_ = 0;

public:
    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& rates) {
        std::vector<std::string> words_no_stop = SplitIntoWordsNoStop(document);
        if(words_no_stop.empty()) return;

        int avg_rating = ComputeAverageRating(rates);
        document_to_rating[document_id] = avg_rating;

        document_to_status[document_id] = status;

        std::map<std::string, int> words_to_count;
        for (const std::string& word : words_no_stop) {
            ++words_to_count[word];
        }
        for(const auto& [word, count] : words_to_count) {
            word_to_documents_freqs_[word].insert({document_id, static_cast<double>(count) / words_no_stop.size()});
        }
        ++document_count_;
    }

    void SetStopWords(const std::string& text) {
        for (const std::string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    int GetDocumentsCount() const noexcept {
        return document_count_;
    }

    template <typename Predicate>
    std::vector<Document> FindTopDocuments(
            const std::string& query, Predicate pred) const {

        std::vector<Document> top_documents;
        top_documents.reserve(MAX_RESULT_DOCUMENT_COUNT);

        std::vector<Document> all_documents = FindAllDocuments(query);
        std::sort(std::execution::par, 
             all_documents.begin(), 
             all_documents.end(), 
             [](const Document& lhd, const Document& rhd) -> bool {
                if(std::abs(lhd.relevance - rhd.relevance) <= 
                        EPSILON * std::max(std::abs(lhd.relevance), std::abs(rhd.relevance))) {
                    return lhd.rating > rhd.rating; 
                }
                return lhd.relevance > rhd.relevance;
             });

        for(int i = 0; i < static_cast<int>(all_documents.size()); ++i) {
            if(top_documents.size() == MAX_RESULT_DOCUMENT_COUNT) break;
        
            int id = all_documents[i].id;
            DocumentStatus status = document_to_status.at(id);
            int rating = all_documents[i].rating;

            if(pred(id, status, rating)) {
                top_documents.push_back(all_documents[i]);
            }
        }
        return top_documents;
    }

    std::vector<Document> FindTopDocuments(const std::string& query) const {
        auto pred = [](int id, DocumentStatus s, int r) {
            return s == DocumentStatus::ACTUAL;
        };

        return FindTopDocuments(query, pred);
    }

    std::tuple<std::vector<std::string>, DocumentStatus> 
    MatchDocument(const std::string& raw_query, int document_id) const {
        std::vector<std::string> plus_words;
        Query query_words = ParseQuery(raw_query);

        for(const auto& mw : query_words.minus_words) {
            if(word_to_documents_freqs_.at(mw).contains(document_id)) {
                return {{}, document_to_status.at(document_id)};
            }
        }
        for(const auto& pw : query_words.plus_words) {
            if(word_to_documents_freqs_.at(pw).contains(document_id)) {
                plus_words.push_back(pw);
            }
        }
        return {plus_words, document_to_status.at(document_id)};
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

    Query ParseQuery(const std::string& raw_query) const {
        Query query_words;
        for (const std::string& word : SplitIntoWords(raw_query)) {
            if(word[0] == '-') {
                query_words.minus_words.insert(word.substr(1));
            } else {
                query_words.plus_words.insert(word);
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

            int word_meet_count = static_cast<int>(word_to_documents_freqs_.at(plus_word).size());
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

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, SearchServer::DocumentStatus status) {
    std::cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const std::string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

// SearchServer CreateSearchServer() {
//     SearchServer search_server;
//     search_server.SetStopWords(ReadLine());
//     const int document_count = ReadLineWithNumber();

//     for (int document_id = 0; document_id < document_count; ++document_id) {
//         std::string text = ReadLine();

//         int rates_count; std::cin >> rates_count;
//         std::vector<int> rates(rates_count);

//         int current_rate;
//         for(int i = 0; i < rates_count; ++i) {
//             std::cin >> current_rate;
//             rates[i] = current_rate;
//         }
//         std::cin.ignore();

//         search_server.AddDocument(document_id, text, rates);
//     }
//     return search_server;
// }

void PrintDocument(const SearchServer::Document& document) {
    std::cout << "{ "s
    << "document_id = "s << document.id << ", "s
    << "relevance = "s << document.relevance << ", "s
    << "rating = "s << document.rating
    << " }"s << std::endl;
}

int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        SearchServer::DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       SearchServer::DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, SearchServer::DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         SearchServer::DocumentStatus::BANNED, {9});

    std::cout << "ACTUAL by default:"s << std::endl;
    for (const SearchServer::Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    std::cout << "ACTUAL:"s << std::endl;
    for (const SearchServer::Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, SearchServer::DocumentStatus status, int rating) { return status == SearchServer::DocumentStatus::ACTUAL; })) {
        PrintDocument(document);
    }

    std::cout << "Even ids:"s << std::endl;
    for (const SearchServer::Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, SearchServer::DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
}