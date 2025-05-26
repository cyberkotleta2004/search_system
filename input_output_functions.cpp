#include "input_output_functions.h"
#include <iostream>

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

void PrintDocument(const SearchServer::Document& document) {
    std::cout << "{ "s
    << "document_id = "s << document.id_ << ", "s
    << "relevance = "s << document.relevance_ << ", "s
    << "rating = "s << document.rating_
    << " }"s << std::endl;
}
