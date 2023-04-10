#pragma once
#include<iterator>
#include<sstream>
#include<vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
        : first_(begin)
        , last_(end)
        , size_(distance(first_, last_)) {
    }
    Iterator begin() const {
        return first_;
    }
    Iterator end() const {
        return last_;
    }
    size_t size() const {
        return size_;
    }
private:
    Iterator first_, last_;
    size_t size_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range) {
    for (Iterator it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}
template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t size) {
        int dist = distance(begin, end);
        if (dist <= size) {
            pages_.push_back(IteratorRange<Iterator>(begin, end));
        }
        else {
            Iterator temp = begin;
            while (temp != end) {
                if (distance(temp, end) <= size){
                    pages_.push_back(IteratorRange<Iterator>(temp, end));
                    break;
                }
                advance(temp, size);
                pages_.push_back(IteratorRange<Iterator>(begin, temp));
                advance(begin, size);
            }
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
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
