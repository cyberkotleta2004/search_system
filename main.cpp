#include "search_server.h"
#include "log_duration.h"
#include <iostream>

int main() {
    SearchServer search_server("and in at"s);

    search_server.AddDocument(1, "curly cat curly tail"s, SearchServer::DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, SearchServer::DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, SearchServer::DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, SearchServer::DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, SearchServer::DocumentStatus::ACTUAL, {1, 1, 1});

    {
        LOG_DURATION_STREAM("Task_1", std::cout);
        search_server.FindTopDocuments("cat -dog");
    }

    {
        LOG_DURATION_STREAM("Task_2", std::cout);
        search_server.FindTopDocuments("dog -cat");
    }
}
