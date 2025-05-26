#include "test_search_server.h"
#include <string>
#include <vector>
#include "search_server.h"
#include "testing_framework.h"

namespace {
    void TestAddedDocumentFind() {
        const int doc_id = 42;
        const std::string content = "cat in the city"s;
        const std::vector<int> ratings = {1, 2, 3};
        
        {
            SearchServer server;
            ASSERT_EQUAL(server.GetDocumentsCount(), 0);
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            ASSERT_EQUAL(server.GetDocumentsCount(), 1);
            ASSERT_EQUAL(server.FindTopDocuments("cat"s).size(), 1);
        }
    
        {
            SearchServer server;
            ASSERT_EQUAL(server.GetDocumentsCount(), 0);
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            ASSERT_EQUAL(server.GetDocumentsCount(), 1);
            ASSERT_EQUAL(server.FindTopDocuments("dog"s).size(), 0);
        }
    
        {
            SearchServer server;
            ASSERT_EQUAL(server.GetDocumentsCount(), 0);
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            ASSERT_EQUAL(server.GetDocumentsCount(), 1);
            ASSERT_EQUAL(server.FindTopDocuments("").size(), 0);
        }
    }
    
    void TestExcludeStopWordsFromAddedDocumentContent() {
        const int doc_id = 42;
        const std::string content = "cat in the city"s;
        const std::vector<int> ratings = {1, 2, 3};
        {
            SearchServer server;
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            const auto found_docs = server.FindTopDocuments("in"s);
            ASSERT_EQUAL(found_docs.size(), 1);
            const SearchServer::Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id_, doc_id);
        }
    
        {
            SearchServer server("in the"s);
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            ASSERT(server.FindTopDocuments("in"s).empty());
        }
    }
    
    void TestMinusWordsWorks() {
        const int doc_id = 42;
        const std::string content = "cat in the city"s;
        const std::vector<int> ratings = {1, 2, 3};
        
        {
            SearchServer server;
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            ASSERT_EQUAL(server.FindTopDocuments("cat -in"s).size(), 0);
        }
    
        {
            SearchServer server;
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            ASSERT_EQUAL(server.FindTopDocuments("-city"s).size(), 0);
        }
    
        {
            SearchServer server;
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            ASSERT_EQUAL(server.FindTopDocuments("").size(), 0);
        }
    }
    
    void TestDocumentsMatching() {
        const int doc_id = 42;
        const std::string content = "cat in the city"s;
        const std::vector<int> ratings = {1, 2, 3};
        
        {
            SearchServer server;
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            auto result = server.MatchDocument("cat dog in", doc_id);
            ASSERT_EQUAL(std::get<0>(result)[0], "cat"s);
            ASSERT_EQUAL(std::get<0>(result)[1], "in"s);
        }
    
        {
            SearchServer server;
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            auto result = server.MatchDocument("-cat dog in", doc_id);
            ASSERT(std::get<0>(result).empty());
        }
    }
    
    void TestRelevanceSortWorks() {
        
        {
            SearchServer server;
            server.AddDocument(1, "cat says meow"s, SearchServer::DocumentStatus::ACTUAL, {1, 2, 3});
            server.AddDocument(2, "dog says owf"s, SearchServer::DocumentStatus::ACTUAL, {1, 2, 3});
            server.AddDocument(3, "wdtfs"s, SearchServer::DocumentStatus::ACTUAL, {1, 2, 3});
            std::vector<SearchServer::Document> results = server.FindTopDocuments("cat says"s);
            ASSERT_EQUAL(results.size(), 2);
            ASSERT_EQUAL(results[0].id_, 1);
            ASSERT_EQUAL(results[1].id_, 2);
        }
    
        {
            SearchServer server;
            server.AddDocument(1, "cat says meow"s, SearchServer::DocumentStatus::ACTUAL, {1, 2, 3});
            server.AddDocument(2, "dog says owf"s, SearchServer::DocumentStatus::ACTUAL, {1, 2, 3});
            server.AddDocument(3, "wdtfs"s, SearchServer::DocumentStatus::ACTUAL, {1, 2, 3});
            std::vector<SearchServer::Document> results = server.FindTopDocuments("-cat says"s);
            ASSERT_EQUAL(results.size(), 1);
            ASSERT_EQUAL(results[0].id_, 2);
        }
    }
    
    void TestRatingCountsCorrectly() {
        {
            SearchServer server;
            server.AddDocument(1, "cat"s, SearchServer::DocumentStatus::ACTUAL, {-4, 2, -7, -7});
            std::vector<SearchServer::Document> results = server.FindTopDocuments("cat"s);
            ASSERT_EQUAL(results[0].rating_, -4);
        }
        {
            SearchServer server;
            server.AddDocument(1, "cat"s, SearchServer::DocumentStatus::ACTUAL, {1, 2, 3});
            std::vector<SearchServer::Document> results = server.FindTopDocuments("cat"s);
            ASSERT_EQUAL(results[0].rating_, 2);
        }
        {
            SearchServer server;
            server.AddDocument(1, "cat"s, SearchServer::DocumentStatus::ACTUAL, {0, -1, 1});
            std::vector<SearchServer::Document> results = server.FindTopDocuments("cat"s);
            ASSERT_EQUAL(results[0].rating_, 0);
        }
        {
            SearchServer server;
            server.AddDocument(1, "cat"s, SearchServer::DocumentStatus::ACTUAL, {});
            std::vector<SearchServer::Document> results = server.FindTopDocuments("cat"s);
            ASSERT_EQUAL(results[0].rating_, 0);
    
        }
        {
            SearchServer server;
            server.AddDocument(1, "cat"s, SearchServer::DocumentStatus::ACTUAL, {4});
            std::vector<SearchServer::Document> results = server.FindTopDocuments("cat"s);
            ASSERT_EQUAL(results[0].rating_, 4);
        }
    }
    
    void TestPredicateWorks() {
        const int doc_id = 42;
        const std::string content = "cat in the city"s;
        const std::vector<int> ratings = {1, 2, 3};
        
        {
            SearchServer server;
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            std::vector<SearchServer::Document> results = 
                server.FindTopDocuments("cat"s, [](int id, SearchServer::DocumentStatus s, int rating) {
                    return id == 42;
                });
    
            ASSERT_EQUAL(results.size(), 1);
        }
    
        {
            SearchServer server;
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            std::vector<SearchServer::Document> results = 
                server.FindTopDocuments("cat"s, [](int id, SearchServer::DocumentStatus s, int rating) {
                    return rating == 7;
                });
    
            ASSERT_EQUAL(results.size(), 0);
        }
    }
    
    void TestStatusPredWorks() {
        const int doc_id = 42;
        const std::string content = "cat in the city"s;
        const std::vector<int> ratings = {1, 2, 3};
        
        {
            SearchServer server;
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            std::vector<SearchServer::Document> results = 
                server.FindTopDocuments("cat"s, SearchServer::DocumentStatus::ACTUAL);
            ASSERT_EQUAL(results.size(), 1);
        }
    
        {
            SearchServer server;
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            std::vector<SearchServer::Document> results = server.FindTopDocuments("cat"s);
            ASSERT_EQUAL(results.size(), 1);
        }
    
        {
            SearchServer server;
            server.AddDocument(doc_id, content, SearchServer::DocumentStatus::ACTUAL, ratings);
            std::vector<SearchServer::Document> results = 
                server.FindTopDocuments("cat"s, SearchServer::DocumentStatus::BANNED);
            ASSERT_EQUAL(results.size(), 0);
        }
    }
    
    void TestRelevanceCountWorks() {
        SearchServer search_server("и в на"s);
        search_server.AddDocument(0, "белый кот и модный ошейник"s,        SearchServer:: DocumentStatus::ACTUAL, {8, -3});
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       SearchServer::DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, SearchServer::DocumentStatus::ACTUAL, {5, -12, 2, 1});
        search_server.AddDocument(3, "ухоженный скворец евгений"s,         SearchServer::DocumentStatus::BANNED, {9});
    
        std::vector<SearchServer::Document> documents = search_server.FindTopDocuments("пушистый ухоженный кот"s);
        ASSERT(documents[0].relevance_ - 0.866434 <= EPSILON * std::max(documents[0].relevance_, 0.866434));
        ASSERT(documents[1].relevance_ - 0.173287 <= EPSILON * std::max(documents[0].relevance_, 0.173287));
        ASSERT(documents[2].relevance_ - 0.173287 <= EPSILON * std::max(documents[0].relevance_, 0.173287));
    }
}

void TestSearchServer() {
    RUN_TEST(TestAddedDocumentFind);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWordsWorks);
    RUN_TEST(TestDocumentsMatching);
    RUN_TEST(TestRelevanceSortWorks);
    RUN_TEST(TestRatingCountsCorrectly);
    RUN_TEST(TestPredicateWorks);
    RUN_TEST(TestStatusPredWorks);
    RUN_TEST(TestRelevanceCountWorks);
}
