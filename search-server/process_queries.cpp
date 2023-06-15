#include "process_queries.h"
#include <algorithm>
#include<execution>


std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> process_queries(queries.size());
    std::transform(std::execution::par,queries.begin(), queries.end(), process_queries.begin(), [&search_server](const std::string& str) { return search_server.FindTopDocuments(str); });
    return process_queries;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> process_queries(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), process_queries.begin(), [&search_server](const std::string& str) { return search_server.FindTopDocuments(str); });
    std::vector<Document> res;
    for (auto&& doc : process_queries) {
        res.insert(res.end(), doc.begin(), doc.end());
    }
    return res;
}