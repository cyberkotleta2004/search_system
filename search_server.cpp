#include "search_server.h"
#include <cmath>
#include <algorithm>

SearchServer::Document::Document() 
    : rating_(0)
    , relevance_(0.0)
    , status_(DocumentStatus::ACTUAL) {}

SearchServer::Document::Document(int id, int rating, DocumentStatus status)
    : id_(id)
    , rating_(rating)
    , status_(status) {}

SearchServer::SearchServer(std::string_view stop_words_text) {   
    std::vector<std::string_view> stop_words_vector = SplitIntoWords(stop_words_text);

    for(std::string_view word : stop_words_vector) {
        CheckUnacceptableSymbols(word);
        stop_words_.insert(std::string(word));
    }
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if(document_id < 0) {
        throw std::invalid_argument("document_id can't be less than 0!");
    }

    if(auto it = id_to_document_.find(document_id); it != id_to_document_.end()) {
        throw std::invalid_argument("document already exists!");
    }

    std::unordered_map<std::string_view, int> word_to_count;
    std::vector<std::string_view> words_no_stop = SplitIntoWordsNoStop(document);
    for(std::string_view word : words_no_stop) {
        CheckUnacceptableSymbols(word);
        ++word_to_count[word];
    }
    
    std::set<std::string_view> words_no_stop_set(words_no_stop.begin(), words_no_stop.end());
    for(std::string_view word : words_no_stop_set) {
        ++word_to_count_[std::string(word)];
    }
    
    int avg_rating = ComputeAverageRating(ratings);
    id_to_document_.insert({document_id, {document_id, avg_rating, status}});
    
    for(const auto& [word, count] : word_to_count) {
        id_to_document_[document_id].word_to_freqs_[std::string(word)] = static_cast<double>(count) / words_no_stop.size();
    }
    
    ++document_count_;
}

std::vector<SearchServer::Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    auto pred = [status](int id, DocumentStatus s, int r) {
        return s == status;
    };

    return FindTopDocuments(raw_query, pred);
}

std::vector<SearchServer::Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const noexcept {
    return document_count_;
}

std::tuple<std::vector<std::string_view>, SearchServer::DocumentStatus> 
SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    return SearchServer::MatchDocument(std::execution::seq, raw_query, document_id);
}

 void SearchServer::RemoveDocument(int document_id) {
        auto doc_it = id_to_document_.find(document_id);
        if (doc_it == id_to_document_.end()) {
            return;
        }

        for (const auto& [word, _] : doc_it->second.word_to_freqs_) {
            auto count_it = word_to_count_.find(word);
            if (count_it != word_to_count_.end()) {
                if (count_it->second == 1) {
                    word_to_count_.erase(count_it);
                } else {
                    --count_it->second;
                }
            }
        }

    id_to_document_.erase(doc_it);
    --document_count_;
}

SearchServer::iterator SearchServer::begin() noexcept {
    return id_to_document_.begin();
}

SearchServer::iterator SearchServer::end() noexcept {
    return id_to_document_.end();
}
SearchServer::const_iterator SearchServer::begin() const noexcept {
    return id_to_document_.begin();
}
SearchServer::const_iterator SearchServer::end() const noexcept {
    return id_to_document_.end();
}
SearchServer::const_iterator SearchServer::cbegin() noexcept {
    return id_to_document_.cbegin();
}
SearchServer::const_iterator SearchServer::cend() noexcept {
    return id_to_document_.cend();
}

std::vector<std::string_view> SearchServer::SplitIntoWords(std::string_view text) const {
    std::vector<std::string_view> words;
    size_t pos = 0;
    size_t len = text.length();

    while(pos < len) {
        size_t space_pos = text.find(' ', pos);

        if(space_pos == text.npos) {
            words.emplace_back(text.substr(pos));
            break;
        }

        if(space_pos > pos) {
            words.emplace_back(text.substr(pos, space_pos - pos));
        }

        pos = space_pos + 1;
    }
    return words;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (stop_words_.count(word) == 0) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::GetRating(int document_id) const {
    return id_to_document_.at(document_id).rating_;
}

std::vector<SearchServer::Document> SearchServer::FindAllDocuments(std::string_view raw_query) const {
    const SearchServer::Query query_words = ParseQuery(raw_query);
    std::unordered_map<int, double> document_to_relevance;

    for(const auto& [document_id, document] : id_to_document_) {
        for (std::string_view plus_word : query_words.plus_words_) {
            if(!document.word_to_freqs_.contains(plus_word)) {
                continue;
            }
            auto word_meet_count_it = word_to_count_.find(plus_word);
            int word_meet_count = word_meet_count_it->second;
            
            auto tf_it = document.word_to_freqs_.find(plus_word);
            double tf = tf_it->second;

            double idf = std::log(static_cast<double>(document_count_) / word_meet_count);
            document_to_relevance[document_id] += tf * idf;
        }
    }


    for(const auto& [document_id, document] : id_to_document_) {
        for (std::string_view minus_word : query_words.minus_words_) {
            if(!document.word_to_freqs_.contains(minus_word)) {
                continue;
            } else {
                document_to_relevance.erase(document_id);
            }
        }
    }
 
    std::vector<Document> found_documents;
    for (auto [document_id, relevance] : document_to_relevance) {
        found_documents.push_back(id_to_document_.at(document_id));
        found_documents.back().relevance_ = relevance;
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

void SearchServer::CheckUnacceptableSymbols(std::string_view word) const {
    if(word.empty()) {
        throw std::invalid_argument("Stop words can't be empty!");
    }

    for(char ch : word) {
        if(ch >= 0 && ch <= 32) {
            throw std::invalid_argument("Stop words can't contain special symbols (ASCII 0 - 32)");
        }
    }
}

SearchServer::Query SearchServer::ParseQuery(std::string_view raw_query) const {
    return ParseQuery(std::execution::seq, raw_query);
}

void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> sets;
    std::vector<int> duplicates_ids;

    for (const auto& [document_id, document] : search_server) {
        std::set<std::string> current_set;
        for(const auto& [word, _] : document.word_to_freqs_) {
            current_set.insert(word);
        }

        if(sets.contains(current_set)) {
            duplicates_ids.push_back(document_id);
        }
        sets.insert(current_set);
    }

    for(auto id : duplicates_ids) {
        search_server.RemoveDocument(id);
    }
}

std::ostream& operator<<(std::ostream& out, const SearchServer::Document& document) {
    out << "{ "s
    << "document_id = "s << document.id_ << ", "s
    << "relevance = "s << document.relevance_ << ", "s
    << "rating = "s << document.rating_
    << " }"s;
    return out;
}