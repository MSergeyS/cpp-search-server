#pragma once

#include <vector>

using namespace std;

template<typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) :
            begin_(begin), end_(end), size_(distance(begin_, end_)) {
    }

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    size_t size() const {
        return size_;
    }

private:
    Iterator begin_, end_;
    size_t size_;
};

template<typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for (size_t inx = distance(begin, end); inx > 0;) {
            const size_t current_page_size = min(page_size, inx);
            const Iterator current_page_end = next(begin, current_page_size);
            pages_.push_back( { begin, current_page_end });

            inx -= current_page_size;
            begin = current_page_end;
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() const {
        return pages_.size();
    }

private:
    vector<IteratorRange<Iterator>> pages_;
};

template<typename Container>
auto Paginate(const Container &c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
