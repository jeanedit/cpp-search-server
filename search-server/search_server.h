#pragma once
#include<map>
#include<set>
#include<vector>
#include<string>
#include<stdexcept>
#include<tuple>
#include<algorithm>
#include <execution>
#include <mutex>
#include <string_view>
#include<functional>
#include <thread>

#include "string_processing.h"
#include "document.h"
#include "concurrent_map.h"

//using namespace std::literals::string_literals;
const int MAX_RESULT_DOCUMENT_COUNT = 5;


#include <iostream>

class SearchServer {
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    inline static constexpr double eps = 1e-6;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (std::string_view word : stop_words_)
            if (!IsValidWord(word)) throw std::invalid_argument("inappropriate symbols");
    }

    explicit SearchServer(std::string_view stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) 
    {
    }

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) 
    {
    }

    void AddDocument(int document_id, std::string_view document, DocumentStatus status,
        const std::vector<int>& ratings);

    void RemoveDocument(int document_id);




    template<typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    template <typename DocumentPredicate,typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy,std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy,std::string_view raw_query, DocumentStatus status) const;

    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&&policy, std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query,
        int document_id) const;

    template<typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query,
        int document_id) const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

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
    const std::set<std::string,std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::map<int, std::map<std::string_view, double>> ids_to_word_freq_;
    std::set<std::string> all_words_;
    std::map<std::string_view, double > empty_map_;
    std::set<int> ids_;

    bool IsStopWord(std::string_view word) const;

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    static bool IsValidWord(std::string_view word);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(bool par, std::string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate,typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&& policy,const Query& query,
        DocumentPredicate document_predicate) const;
};


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    Query query = ParseQuery(false,raw_query);

    auto result = FindAllDocuments(query, document_predicate);

    sort(result.begin(), result.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < eps) {
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


template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    if constexpr (std::is_same_v<std::remove_reference_t<ExecutionPolicy&&>, const std::execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, document_predicate);
    }

    Query query = ParseQuery(false, raw_query);

    auto result = FindAllDocuments(policy,query, document_predicate);

    sort(result.begin(), result.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < eps) {
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

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const {
    if constexpr (std::is_same_v<std::remove_reference_t<ExecutionPolicy&&>, const std::execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, status);
    }
    return FindTopDocuments(policy,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const {
    if constexpr (std::is_same_v<std::remove_reference_t<ExecutionPolicy&&>, const std::execution::sequenced_policy>) {
        return FindTopDocuments(raw_query);
    }
    return FindTopDocuments(policy,raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (std::string_view word : query.plus_words) {
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

    for (std::string_view word : query.minus_words) {
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



template <typename DocumentPredicate,typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy,const Query& query,
    DocumentPredicate document_predicate) const {
    if constexpr (std::is_same_v<std::remove_reference_t<ExecutionPolicy&&>, const std::execution::sequenced_policy>) {
        return FindAllDocuments(query, document_predicate);
    }
    ConcurrentMap<int, double> document_to_relevance(std::thread::hardware_concurrency());

    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [this,&document_predicate,&document_to_relevance](const std::string_view word) {
            if (word_to_document_freqs_.count(word) != 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
        });


    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [this,&document_to_relevance](const std::string_view word) {
        if (word_to_document_freqs_.count(word) != 0) {
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.Erase(document_id);
            }
        }
        });


    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}



template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {
    if (ids_to_word_freq_.count(document_id)) {
        std::vector<std::string_view> temp_words(ids_to_word_freq_[document_id].size());

        std::transform(policy, ids_to_word_freq_[document_id].begin(), ids_to_word_freq_[document_id].end(), temp_words.begin(),
            [](const auto& word) { return word.first; });

        std::for_each(policy, temp_words.begin(), temp_words.end(),
            [this, document_id](const std::string_view word) { word_to_document_freqs_[word].erase(document_id); });

        ids_to_word_freq_.erase(document_id);
        ids_.erase(document_id);
        documents_.erase(document_id);
    }
}



template<typename ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query, int document_id) const {
    if constexpr (std::is_same_v<std::remove_reference_t<ExecutionPolicy&&>, const std::execution::sequenced_policy>) {
        return MatchDocument(raw_query, document_id);
    }
    if (!ids_.count(document_id)) {
        throw std::out_of_range("Document ID out of range");
    }
    auto query = ParseQuery(true, raw_query);

    std::sort(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(std::unique(query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());



    if (std::any_of(query.minus_words.begin(), query.minus_words.end(), [this, document_id](std::string_view word) {
        return ids_to_word_freq_.at(document_id).count(word);
        })) {
        return { {}, documents_.at(document_id).status };
    }

    std::sort(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(std::unique(query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());
    
    std::vector<std::string_view> matched_words(query.plus_words.size());


    auto it_for_resize = std::copy_if(query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [this, document_id](std::string_view word) {
        return ids_to_word_freq_.at(document_id).count(word);
        });
    matched_words.resize(std::distance(matched_words.begin(), it_for_resize));
    return { matched_words, documents_.at(document_id).status };
}
