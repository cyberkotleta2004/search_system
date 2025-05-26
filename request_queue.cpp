#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server) 
{}

std::vector<SearchServer::Document> RequestQueue::AddFindRequest(const std::string& raw_query, SearchServer::DocumentStatus status) {
    QueryResult query_result = std::move(search_server_.FindTopDocuments(raw_query, status));
    AddQueryResult(std::move(query_result));
    return requests_.front().results_;
}

std::vector<SearchServer::Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    QueryResult query_result = std::move(search_server_.FindTopDocuments(raw_query));
    AddQueryResult(std::move(query_result));
    return requests_.front().results_;
}

int RequestQueue::GetNoResultRequests() const {
    return empty_requests_count_;
}

RequestQueue::QueryResult::QueryResult(std::vector<SearchServer::Document>&& results)
    : results_(std::move(results)) 
{}

RequestQueue::QueryResult::QueryResult(QueryResult&& other)
    : results_(std::move(other.results_)) 
{}

void RequestQueue::AddQueryResult(QueryResult&& query_result) {
    if(requests_.size() == min_in_day_) {
        if(requests_.back().results_.empty()) {
            --empty_requests_count_;
        }
        requests_.pop_back();
    }

    if(query_result.results_.empty()) {
        ++empty_requests_count_;
    }
    requests_.emplace_front(std::move(query_result));
}

