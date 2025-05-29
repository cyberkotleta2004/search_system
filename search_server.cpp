#include "search_server.h"
#include <cmath>

SearchServer::Document::Document() 
    : rating_(0)
    , relevance_(0.0)
    , status_(DocumentStatus::ACTUAL) {}

SearchServer::Document::Document(int id, int rating, DocumentStatus status)
    : id_(id)
    , rating_(rating)
    , status_(status) {}

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

    if(auto it = id_to_document_.find(document_id); it != id_to_document_.end()) {
        throw std::invalid_argument("document already exists!");
    }
    std::unordered_map<std::string, int> word_to_count;
    std::vector<std::string> words_no_stop = SplitIntoWordsNoStop(document);
    for(const auto& word : words_no_stop) {
        CheckUnacceptableSymbols(word);
        ++word_to_count[word];
        ++word_to_count_[word];
    }

    int avg_rating = ComputeAverageRating(ratings);
    id_to_document_.insert({document_id, {document_id, avg_rating, status}});

    for(const auto& [word, count] : word_to_count) {
        id_to_document_[document_id].word_to_freqs_[word] = static_cast<double>(count) / words_no_stop.size();
    }
    
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

int SearchServer::GetDocumentCount() const noexcept {
    return document_count_;
}

std::tuple<std::vector<std::string>, SearchServer::DocumentStatus> 
SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    std::vector<std::string> plus_words;
    SearchServer::Query query_words = ParseQuery(raw_query);
    const Document& current_document = id_to_document_.at(document_id);
    
    for(const auto& mw : query_words.minus_words_) {
        if(auto it = current_document.word_to_freqs_.find(mw); it != current_document.word_to_freqs_.end()) {
            return {{}, current_document.status_};
        }
    }
    for(auto& pw : query_words.plus_words_) {
        if(auto it = current_document.word_to_freqs_.find(pw); it != current_document.word_to_freqs_.end()) {
            plus_words.push_back(std::move(pw));
        }
    }
    return {plus_words, current_document.status_};
}

void SearchServer::RemoveDocument(int document_id) {
    if(auto it = id_to_document_.find(document_id); it != id_to_document_.end()) {
        for(const auto& [word, _] : it->second.word_to_freqs_) {
            if(word_to_count_.at(word) == 1) {
                word_to_count_.erase(word);
            } else {
                --word_to_count_[word];
            }
        }

        id_to_document_.erase(document_id);
        --document_count_;
    }
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
    return id_to_document_.at(document_id).rating_;
}

std::vector<SearchServer::Document> SearchServer::FindAllDocuments(const std::string& raw_query) const {
    const SearchServer::Query query_words = ParseQuery(raw_query);
    std::unordered_map<int, double> document_to_relevance;

    for(const auto& [document_id, document] : id_to_document_) {
        for (const std::string& plus_word : query_words.plus_words_) {
            if(!document.word_to_freqs_.contains(plus_word)) {
                continue;
            }
            int word_meet_count = static_cast<int>(word_to_count_.at(plus_word));
            double tf = document.word_to_freqs_.at(plus_word);
            double idf = std::log(static_cast<double>(document_count_) / word_meet_count);
            document_to_relevance[document_id] += tf * idf;
        }
    }


    for(const auto& [document_id, document] : id_to_document_) {
        for (const std::string& minus_word : query_words.minus_words_) {
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

void RemoveDuplicates(SearchServer& search_server) {
    std::unordered_set<std::unordered_set<std::string>, HashSetHasher> sets;
    std::vector<int> duplicates_ids;

    for (const auto& [document_id, document] : search_server) {
        std::unordered_set<std::string> current_set;
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