#include "remove_duplicates.h"
#include <set>
#include <string>

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> remove_ids;
    std::set<std::set<std::string>> word_to_ids;

    for (const int document_id : search_server) {
        const auto word_map = search_server.GetWordFrequencies(document_id);
        std::set<std::string> key_for_map;
        for (const auto& [str, _] : word_map)
            key_for_map.insert(str);

        if (word_to_ids.count(key_for_map))
            remove_ids.insert(document_id);
        else word_to_ids.insert(key_for_map);
    }

    for (const int id : remove_ids) {
        search_server.RemoveDocument(id);
    }

}