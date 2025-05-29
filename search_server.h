#pragma once
#include <string>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <execution>
#include <algorithm>
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
        std::unordered_map<std::string, double> word_to_freqs_;
        DocumentStatus status_;

        Document();
        Document(int id, int rating, DocumentStatus status);
    };

private:
    struct Query {
        std::unordered_set<std::string> plus_words_;
        std::unordered_set<std::string> minus_words_;
    };

private:
    std::unordered_set<std::string> stop_words_;
    std::unordered_map<int, Document> id_to_document_;
    std::unordered_map<std::string, int> word_to_count_;
    int document_count_ = 0;

public:
    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) 
    {
        for(const auto& word : stop_words) {
            CheckUnacceptableSymbols(word);
            stop_words_.insert(word);
        }
    }

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
            const std::string& raw_query, DocumentPredicate document_predicate) const {

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

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    int GetDocumentCount() const noexcept;

    std::tuple<std::vector<std::string>, DocumentStatus> 
    MatchDocument(const std::string& raw_query, int document_id) const;

    const std::unordered_map<std::string, double>& GetWordFrequencies(int document_id) const;
    void RemoveDocument(int document_id);

    using iterator = std::unordered_map<int, Document>::iterator;
    using const_iterator = std::unordered_map<int, Document>::const_iterator;


    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() noexcept;
    const_iterator cend() noexcept;

private:  
    std::vector<std::string> SplitIntoWords(const std::string& text) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    int GetRating(int document_id) const;

    std::vector<Document> FindAllDocuments(const std::string& raw_query) const;

    static int ComputeAverageRating(const std::vector<int>& rates);

    void CheckUnacceptableSymbols(const std::string& word) const;

    Query ParseQuery(const std::string& raw_query) const;
};

std::ostream& operator<<(std::ostream& out, const SearchServer::Document& document);

struct HashSetHasher {
    size_t operator() (const std::unordered_set<std::string>& s) const {
        size_t hash = 0;
        for(const std::string& word : s) {
            hash ^= std::hash<std::string>{}(word) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

void RemoveDuplicates(SearchServer& search_server);