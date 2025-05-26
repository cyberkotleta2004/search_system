#pragma once
#include "search_server.h"
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    RequestQueue(const RequestQueue&) = delete;
    RequestQueue& operator=(const RequestQueue&) = delete;

    template <typename DocumentPredicate>
    std::vector<SearchServer::Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        QueryResult query_result = std::move(search_server_.FindTopDocuments(raw_query, document_predicate));
        AddQueryResult(std::move(query_result));
        return requests_.front().results_;
    }

    std::vector<SearchServer::Document> AddFindRequest(const std::string& raw_query, SearchServer::DocumentStatus status);
    std::vector<SearchServer::Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::vector<SearchServer::Document> results_;

        QueryResult(std::vector<SearchServer::Document>&& results);
        QueryResult(QueryResult&& other);
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int empty_requests_count_ = 0;
    const SearchServer& search_server_;

    void AddQueryResult(QueryResult&& query_result);
};