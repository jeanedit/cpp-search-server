#include "string_processing.h"
using namespace std;

vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> result;
    text.remove_prefix(std::min(text.find_first_not_of(" "), text.size()));

    while (!text.empty()) {
        int64_t space = text.find(' ');
        result.push_back(text.substr(0, space));
        text.remove_prefix(std::min(text.find_first_of(" "), text.size()));
        text.remove_prefix(std::min(text.find_first_of(" ") + text.find_first_not_of(" "), text.size()));
    }
    return result;
}