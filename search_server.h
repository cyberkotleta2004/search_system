#pragma once
#include <string>
#include <set>
#include <vector>
#include <map>
#include <iostream>
#include <execution>
#include <algorithm>
#include <mutex>
#include "paginator.h"

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
        int id_;
        int rating_;
        double relevance_;
        std::map<std::string, double, std::less<>> word_to_freqs_;
        DocumentStatus status_;

        Document();
        Document(int id, int rating, DocumentStatus status);
    };

private:
    struct Query {
        std::set<std::string_view> plus_words_;
        std::set<std::string_view> minus_words_;
    };

private:
    std::set<std::string, std::less<>> stop_words_;
    std::map<int, Document> id_to_document_;
    std::map<std::string, int, std::less<>> word_to_count_;
    int document_count_ = 0;

public:
    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) 
    {
        for(const std::string& word : stop_words) {
            CheckUnacceptableSymbols(std::string_view(word));
            stop_words_.insert(std::string(word));
        }
    }

    
    explicit SearchServer(std::string_view stop_words_text);
    
    explicit SearchServer(const std::string& string) 
        : SearchServer(std::string_view(string)) 
    {}

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
            std::string_view raw_query, DocumentPredicate document_predicate) const {

        std::vector<Document> top_documents;
        top_documents.reserve(MAX_RESULT_DOCUMENT_COUNT);

        std::vector<Document> all_documents = FindAllDocuments(raw_query);
        std::sort(std::execution::par, 
             all_documents.begin(), 
             all_documents.end(), 
             [](const Document& lhd, const Document& rhd) -> bool {
                if(std::abs(lhd.relevance_ - rhd.relevance_) <= 
                        EPSILON * std::max(std::abs(lhd.relevance_), std::abs(rhd.relevance_))) {
                    return lhd.rating_ > rhd.rating_; 
                }
                return lhd.relevance_ > rhd.relevance_;
             });

        for(int i = 0; i < static_cast<int>(all_documents.size()); ++i) {
            if(top_documents.size() == MAX_RESULT_DOCUMENT_COUNT) break;
        
            int id = all_documents[i].id_;
            DocumentStatus status = all_documents[i].status_;
            int rating = all_documents[i].rating_;

            if(document_predicate(id, status, rating)) {
                top_documents.push_back(all_documents[i]);
            }
        }
        return top_documents;
    }

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    int GetDocumentCount() const noexcept;

    std::tuple<std::vector<std::string_view>, DocumentStatus> 
    MatchDocument(std::string_view raw_query, int document_id) const;

    template <typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> 
    MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query, int document_id) const {
        std::vector<std::string_view> plus_words;
        SearchServer::Query query_words = ParseQuery(policy, raw_query);
        const Document& current_document = id_to_document_.at(document_id);
        
        for(std::string_view mw : query_words.minus_words_) {
            if(auto it = current_document.word_to_freqs_.find(mw); it != current_document.word_to_freqs_.end()) {
                return {{}, current_document.status_};
            }
        }
        for(std::string_view pw : query_words.plus_words_) {
            if(auto it = current_document.word_to_freqs_.find(pw); it != current_document.word_to_freqs_.end()) {
                plus_words.push_back(it->first);
            }
        }
        return {plus_words, current_document.status_};
    }

    void RemoveDocument(int document_id);

    using iterator = std::map<int, Document>::iterator;
    using const_iterator = std::map<int, Document>::const_iterator;


    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() noexcept;
    const_iterator cend() noexcept;

private:  
    std::vector<std::string_view> SplitIntoWords(std::string_view text) const;

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    int GetRating(int document_id) const;

    std::vector<Document> FindAllDocuments(std::string_view raw_query) const;

    static int ComputeAverageRating(const std::vector<int>& rates);

    void CheckUnacceptableSymbols(std::string_view word) const;

    Query ParseQuery(std::string_view raw_query) const;

    template <typename ExecutionPolicy>
    Query ParseQuery(ExecutionPolicy&& policy, std::string_view raw_query) const {
        Query query_words;
        const std::vector<std::string_view> words = SplitIntoWordsNoStop(raw_query);

        Query result;
        std::mutex mutex;

        std::for_each(
            policy,
            words.begin(), words.end(),
            [&](std::string_view word) {
                CheckUnacceptableSymbols(word);
                if(word.empty()) {
                    return;
                }
                if (word[0] == '-') {
                    if (word.size() == 1 || word[1] == '-') {
                        throw std::invalid_argument("Word can't be '-' or start with '--'");
                    }
                    std::string_view minus_word = word.substr(1);
                    std::lock_guard<std::mutex> lock(mutex);
                    result.minus_words_.insert(minus_word);
                } else {
                    std::lock_guard<std::mutex> lock(mutex);
                    result.plus_words_.insert(word);
                }
            });
        return result;
    }
};

std::ostream& operator<<(std::ostream& out, const SearchServer::Document& document);

void RemoveDuplicates(SearchServer& search_server);