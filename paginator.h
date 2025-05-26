#pragma once
#include <vector>

template <typename Iterator>
class IteratorRange {   
private:
    Iterator begin_;
    Iterator end_;

public:
    IteratorRange(Iterator begin, Iterator end)
        : begin_(begin), end_(end) {}

    Iterator begin() const { return begin_; }

    Iterator end() const { return end_; }

    size_t size() {
        return static_cast<size_t>(std::distance(begin_, end_));
    }
};

template <typename Iterator>
class Paginator {
private:
    std::vector<IteratorRange<Iterator>> pages_;
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        while(begin != end) {
            Iterator current_end = begin;
            size_t step = std::min(page_size, static_cast<size_t>(std::distance(begin, end)));
            std::advance(current_end, step);
            pages_.emplace_back(begin, current_end);
            begin = current_end;
        }
    }

    auto begin() const { return pages_.begin(); }
    auto end() const { return pages_.end(); }
    size_t size() const { return pages_.size(); }
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
