#include "search_server.h"
#include <math.h>
#include<numeric>

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
	const std::vector<int>& ratings) {
	const std::vector<std::string> words = SplitIntoWordsNoStop(document);
	QueryWord query_word;
	if (documents_.count(document_id) != 0 || document_id < 0) throw std::invalid_argument("inappropriate id");
	const double inv_word_count = 1.0 / words.size();
	for (const std::string& word : words) {
		query_word = ParseQueryWord(word);
		ids_to_word_freq_[document_id][word] += inv_word_count;
		word_to_document_freqs_[word][document_id] += inv_word_count;
	}
	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
	ids_.emplace(document_id);
}


void SearchServer::RemoveDocument(int document_id) {
	if (ids_to_word_freq_.count(document_id)) {
		for (const auto& [word, _] : ids_to_word_freq_[document_id]) {
			word_to_document_freqs_.at(word).erase(document_id);
		}
		ids_to_word_freq_.erase(document_id);
		ids_.erase(ids_.find(document_id));
		documents_.erase(document_id);
	}

}


std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
	return FindTopDocuments(
		raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
			return document_status == status;
		});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
	return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
	int document_id) const {
	Query query = ParseQuery(raw_query);
	std::vector<std::string> matched_words;
	for (const std::string& word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(word);
		}
	}
	for (const std::string& word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.clear();
			break;
		}
	}
	std::tuple<std::vector<std::string>, DocumentStatus> match = { matched_words, documents_.at(document_id).status };
	return match;
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
	if (ids_to_word_freq_.count(document_id)) return ids_to_word_freq_.at(document_id);
	else return empty_map_;
}


bool SearchServer::IsStopWord(const std::string& word) const {
	return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
	std::vector<std::string> words;
	for (const std::string& word : SplitIntoWords(text)) {
		if (!IsStopWord(word)) {
			words.push_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
	if (ratings.empty()) {
		return 0;
	}
	return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

bool SearchServer::IsValidWord(const std::string& word) {
	return none_of(word.begin(), word.end(), [](char c) {
		return c >= '\0' && c < ' ';
		});
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
	bool is_minus = false;
	if (!IsValidWord(text)) throw std::invalid_argument("inappropriate symbols");
	// Word shouldn't be empty
	if (text[0] == '-') {
		is_minus = true;
		text = text.substr(1);
		if (text.empty() || text[0] == '-') throw std::invalid_argument("wrong minus word format");
	}
	QueryWord queryword = { text, is_minus, IsStopWord(text) };
	return queryword;
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
	Query query;
	for (const std::string& word : SplitIntoWords(text)) {
		QueryWord query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			if (query_word.is_minus) {
				query.minus_words.insert(query_word.data);
			}
			else {
				query.plus_words.insert(query_word.data);
			}
		}
	}
	return query;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
