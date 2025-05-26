#include "search_server.h"
#include <cmath>

SearchServer::Document::Document() : id_(0), rating_(0), relevance_(0.0) {}

SearchServer::Document::Document(int id, int rating, double relevance)
    : id_(id)
    , rating_(rating)
    , relevance_(relevance) 
{}

SearchServer::SearchServer(const std::string& stop_words_text) {   
    std::vector<std::string> stop_words_vector = SplitIntoWords(stop_words_text);

    for(auto& word : stop_words_vector) {
        CheckUnacceptableSymbols(word);
        stop_words_.insert(std::move(word));
    }
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if(document_id < 0) {
        throw std::invalid_argument("document_id can't be less than 0!");
    }

    if(auto it = id_to_rating_status_.find(document_id); it != id_to_rating_status_.end()) {
        throw std::invalid_argument("document already exists!");
    }
    std::map<std::string, int> words_to_count;
    std::vector<std::string> words_no_stop = SplitIntoWordsNoStop(document);
    for(const auto& word : words_no_stop) {
        CheckUnacceptableSymbols(word);
        ++words_to_count[word];
    }
    for(const auto& [word, count] : words_to_count) {
        word_to_documents_freqs_[word].insert({document_id, static_cast<double>(count) / words_no_stop.size()});
    }
    
    int avg_rating = ComputeAverageRating(ratings);
    id_to_rating_status_.insert({document_id, {avg_rating, status}});
    ++document_count_;
}

std::vector<SearchServer::Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    auto pred = [status](int id, DocumentStatus s, int r) {
        return s == status;
    };

    return FindTopDocuments(raw_query, pred);
}

std::vector<SearchServer::Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentsCount() const noexcept {
    return document_count_;
}

std::tuple<std::vector<std::string>, SearchServer::DocumentStatus> 
SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    std::vector<std::string> plus_words;
    SearchServer::Query query_words = ParseQuery(raw_query);

    for(const auto& mw : query_words.minus_words_) {
        if (auto it = word_to_documents_freqs_.find(mw); it != word_to_documents_freqs_.end() && it->second.contains(document_id)) {
            return {{}, id_to_rating_status_.at(document_id).second};
        }
    }
    for(const auto& pw : query_words.plus_words_) {
        if(auto it = word_to_documents_freqs_.find(pw); it != word_to_documents_freqs_.end() && it->second.contains(document_id)) {
            plus_words.push_back(pw);
        }
    }
    return {plus_words, id_to_rating_status_.at(document_id).second};
}

std::vector<std::string> SearchServer::SplitIntoWords(const std::string& text) const {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ' && !word.empty()) {
            words.push_back(word);
            word = "";
        } else if(c != ' '){
            word += c;
        }
    }

    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (stop_words_.count(word) == 0) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::GetRating(int document_id) const {
    if(auto it = id_to_rating_status_.find(document_id); it != id_to_rating_status_.end()) {
        return it->second.first;;
    }
    return 0;
}

std::vector<SearchServer::Document> SearchServer::FindAllDocuments(const std::string& raw_query) const {
    const SearchServer::Query query_words = ParseQuery(raw_query);
    std::map<int, double> document_to_relevance;

    for (const std::string& plus_word : query_words.plus_words_) {
        if (word_to_documents_freqs_.count(plus_word) == 0) {
            continue;
        }

        int word_meet_count = static_cast<int>(word_to_documents_freqs_.at(plus_word).size());
        for (const auto& [document_id, tf] : word_to_documents_freqs_.at(plus_word)) {
            double idf = std::log(static_cast<double>(document_count_) / word_meet_count);
            document_to_relevance[document_id] += tf * idf;
        }
    }

    for(const std::string& minus_word : query_words.minus_words_) {
        if(word_to_documents_freqs_.count(minus_word) == 0) {
            continue;
        }

        for (const auto [document_id, _] : word_to_documents_freqs_.at(minus_word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> found_documents;
    for (auto [document_id, relevance] : document_to_relevance) {
        found_documents.push_back({document_id, GetRating(document_id), relevance});
    }
    return found_documents;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& rates) {
    if(rates.empty()) return 0;
    int rates_summary = std::reduce(std::execution::par, rates.begin(), rates.end(), 0);
    int rates_size = rates.size();
    int avg_rating = rates_summary / rates_size;

    return avg_rating;
}

void SearchServer::CheckUnacceptableSymbols(const std::string& word) const {
    if(word.empty()) {
        throw std::invalid_argument("Stop words can't be empty!");
    }

    for(char ch : word) {
        if(ch >= 0 && ch <= 32) {
            throw std::invalid_argument("Stop words can't contain special symbols (ASCII 0 - 32)");
        }
    }
}

SearchServer::Query SearchServer::ParseQuery(const std::string& raw_query) const {
    SearchServer::Query query_words;
    for (const std::string& word : SplitIntoWordsNoStop(raw_query)) {
        CheckUnacceptableSymbols(word);
        if(word[0] == '-') {
            if(word.size() == 1 || word[1] == '-') {
                throw std::invalid_argument("Word can't be '-' or '--...'!");
            }
            query_words.minus_words_.insert(word.substr(1));
        } else {
            query_words.plus_words_.insert(word);
        }
    }
    return query_words;
}

std::ostream& operator<<(std::ostream& out, const SearchServer::Document& document) {
    out << "{ "s
    << "document_id = "s << document.id_ << ", "s
    << "relevance = "s << document.relevance_ << ", "s
    << "rating = "s << document.rating_
    << " }"s;
    return out;
}