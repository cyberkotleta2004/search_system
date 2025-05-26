#pragma once

#include <string>
#include <vector>
#include "search_server.h"

std::string ReadLine();

int ReadLineWithNumber();

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, SearchServer::DocumentStatus status);

void PrintDocument(const SearchServer::Document& document);