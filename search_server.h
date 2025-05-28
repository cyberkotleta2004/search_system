#pragma once
#include <string>
#include <set>
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
        int rating_;
        double relevance_;
        std::unordered_map<std::string, double> word_to_freqs_;
        DocumentStatus status_;

        Document();
        Document(int rating, DocumentStatus status);
    };

private:
    struct Query {
        std::set<std::string> plus_words_;
        std::set<std::string> minus_words_;
    };

private:
    std::set<std::string> stop_words_;
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
            DocumentStatus status = id_to_rating_status_.at(id).second;
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

    auto begin() noexcept;
    auto end() noexcept;
    const auto begin() const noexcept;
    const auto end() const noexcept;
    const auto cbegin() noexcept;
    const auto cend() noexcept;

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
