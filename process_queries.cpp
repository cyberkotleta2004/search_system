#include "process_queries.h"
#include <algorithm>
#include <execution>

std::vector<std::vector<SearchServer::Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {
    std::vector<std::vector<SearchServer::Document>> query_results(queries.size());

    std::transform(
        std::execution::par,
        queries.begin(), queries.end(),
        query_results.begin(),
        [&search_server](const std::string& query) {
            return search_server.FindTopDocuments(query);
        }
    );
    return query_results;
}

std::vector<SearchServer::Document> ProcessQueriesJoined(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {
    std::vector<std::vector<SearchServer::Document>> documents_by_queries = ProcessQueries(search_server, queries);

    size_t size_to_reserve = 0;
    for(const auto& vector : documents_by_queries) {
        size_to_reserve += vector.size();
    }


    std::vector<SearchServer::Document> results_joined;
    results_joined.reserve(size_to_reserve);

    for(auto& vector : documents_by_queries) {
        std::move(vector.begin(), vector.end(), std::back_inserter(results_joined));
    }

    return results_joined;

}