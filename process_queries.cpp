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