#pragma once
#include <future>
#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <utility>


using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    using mutex_map = std::pair<std::mutex, std::map<Key, Value>>;
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) :submaps_(bucket_count) {};

    Access operator[](const Key& key) {
        const size_t residual = static_cast<size_t> (key) % submaps_.size();
        return { std::lock_guard<std::mutex>(submaps_[residual].first),submaps_[residual].second[key] };

    };

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;

        for (auto& [mutex_, map_] : submaps_) {
            std::lock_guard guard(mutex_);
            result.insert(map_.begin(), map_.end());
        }
        return result;
    };

    bool Erase(const Key& key) {
        const size_t residual = static_cast<size_t> (key) % submaps_.size();
        std::lock_guard<std::mutex>(submaps_[residual].first);
        return submaps_[residual].second.erase(key);
    }

private:
    std::vector<mutex_map> submaps_;
};