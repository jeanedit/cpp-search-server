#include "remove_duplicates.h"
#include <set>
#include <string>
#include<algorithm>
#include <utility>
#include<iterator>

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> remove_ids;
    std::set<std::set<std::string_view>> word_to_ids;

    for (const int document_id : search_server) {
        const auto word_map = search_server.GetWordFrequencies(document_id);
        std::set<std::string_view> key_for_map;

        std::transform(word_map.cbegin(), word_map.cend(), std::inserter(key_for_map, key_for_map.begin()), [](const std::pair<std::string_view, double>& pair) {return pair.first; });

        if (word_to_ids.count(key_for_map))
            remove_ids.insert(document_id);
        else word_to_ids.insert(key_for_map);
    }

    for (const int id : remove_ids) {
        search_server.RemoveDocument(id);
    }

}