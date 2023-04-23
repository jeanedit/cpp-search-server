#pragma once
#include<map>
#include<set>
#include<vector>
#include<string>
#include<stdexcept>
#include<tuple>
#include<algorithm>

#include "string_processing.h"
#include "document.h"
using namespace std; // только так проходят тесты
//using namespace std::literals::string_literals;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    inline static constexpr double eps = 1e-6;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const auto& word : stop_words_)
            if (!IsValidWord(word)) throw std::invalid_argument("inappropriate symbols");
    }

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from std::string container
    {
    }

    void AddDocument(int document_id, const std::string& document, DocumentStatus status,
        const std::vector<int>& ratings);

    void RemoveDocument(int document_id);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query,
        DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query,
        int document_id) const;

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    std::set<int>::const_iterator begin() const {
        return ids_.begin();
    }

    std::set<int>::const_iterator end() const {
        return ids_.end();
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::map<int, std::map<std::string, double>> ids_to_word_freq_; // Added
    std::map<std::string, double > empty_map_;
    std::set<int> ids_;

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    static bool IsValidWord(const std::string& word);

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string text) const;

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string& text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;
};


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query,
    DocumentPredicate document_predicate) const {
    Query query = ParseQuery(raw_query);

    auto result = FindAllDocuments(query, document_predicate);

    sort(result.begin(), result.end(),
        [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < eps) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
        result.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return result;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}