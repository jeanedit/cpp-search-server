#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include<tuple>
#include<numeric>
using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;



string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
constexpr double eps = 1e-6;
class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }
    template<typename Status>
    vector<Document> FindTopDocuments(const string& raw_query,
        Status status) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, status);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < eps) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus dstatus, int rating) {return dstatus == status; });
    }
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus dstatus, int rating) {return dstatus == DocumentStatus::ACTUAL; });
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
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
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename Status>
    vector<Document> FindAllDocuments(const Query& query, Status status) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (status(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}


#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))


template <typename T>
void RunTestImpl(T& func, const string& expr_str) {
    func();
    cerr << expr_str << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl(func,#func) // напишите недостающий код

// -------- Начало модульных тестов поисковой системы ----------



// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

void TestMinusWordsCheck() {
    const vector<int> doc_id = { 34, 42 };
    const vector<string> content = { "cat in the city"s, "white cat in New York" };
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        for (size_t i = 0; i < doc_id.size(); ++i)
            server.AddDocument(doc_id[i], content[i], DocumentStatus::ACTUAL, ratings);
        const auto found_docs_1 = server.FindTopDocuments("cat -in"s);
        ASSERT(found_docs_1.empty());
        const auto found_docs_2 = server.FindTopDocuments("cat -the"s);
        ASSERT_EQUAL(found_docs_2.size(), 1u);
        ASSERT_EQUAL(found_docs_2[0].id, 42);
    }
}

void TestMatching() {
    const vector<int> doc_id = { 34, 42 };
    const vector<string> content = { "cat in the city"s, "white cat walking around" };
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        for (size_t i = 0; i < doc_id.size(); ++i)
            server.AddDocument(doc_id[i], content[i], DocumentStatus::ACTUAL, ratings);
        const auto found_docs_1 = server.MatchDocument("cat -in"s, 34);

        ASSERT(get<0>(found_docs_1).empty());
        const auto found_docs_2 = server.MatchDocument("cat walking -the"s, 42);
        vector<string> words = { "cat","walking" };

        ASSERT_EQUAL(get<0>(found_docs_2), words);
        ASSERT_EQUAL(staitc_cast<int>(get<1>(found_docs_2)), static_cast<int>(DocumentStatus::ACTUAL));
        const auto found_docs_3 = server.MatchDocument("cat walking -the -white"s, 42);
        ASSERT(get<0>(found_docs_3).empty());
    }
}

void TestRelevanceSortCheck() {
    const vector<int> doc_id = { 0, 1,2 };
    const vector<string> content = { "a colorful parrot with green wings and red tail is lost"s,
        "a grey hound with black ears is found at the railway station",
        "a white cat with long furry tail is found near the red square"
    };
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        for (size_t i = 0; i < doc_id.size(); ++i)
            server.AddDocument(doc_id[i], content[i], DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("white cat long tail"s);
        ASSERT(found_docs[0].relevance - found_docs[1].relevance > 0.0);
    }

    {
        SearchServer server;
        server.SetStopWords("и в на"s);
        server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
        server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
        const auto found_docs = server.FindTopDocuments("ухоженный кот"s);
        ASSERT(found_docs[0].relevance - found_docs[1].relevance > 0.0);
        ASSERT(found_docs[1].relevance - found_docs[2].relevance >= 0.0);
    }
}


void TestRatingCalc() {
    const vector<int> doc_id = { 0, 1,2 };
    const vector<string> content = { "a colorful parrot with green wings and red tail is lost"s,
        "a grey hound with black ears is found at the railway station",
        "a white cat with long furry tail is found near the red square"
    };
    const vector<vector<int>> ratings = { {1,2,3}, {3,5}, {2,7,1,6} };

    {
        SearchServer server;
        for (size_t i = 0; i < doc_id.size(); ++i)
            server.AddDocument(doc_id[i], content[i], DocumentStatus::ACTUAL, ratings[i]);
        const auto found_docs = server.FindTopDocuments("white cat long tail"s);

        ASSERT_EQUAL(found_docs[0].rating, 4.0);
        ASSERT_EQUAL(found_docs[1].rating, 2.0);
    }

}

void TestPredicatAndRelevanceCalc() {
    constexpr double eps = 1e-6;
    const vector<double> correct_relevances = { 0.866434 ,0.173287,0.231049 };
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    // Проверка правильности вычисления релевантности и правильный учет статуса (default ACTUAL)
    const auto found_docs_1 = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(found_docs_1[0].id, 1);
    ASSERT_EQUAL(found_docs_1[1].id, 0);
    ASSERT_EQUAL(found_docs_1[2].id, 2);
    ASSERT(abs(found_docs_1[0].relevance - correct_relevances[0]) < eps);
    ASSERT(abs(found_docs_1[1].relevance - correct_relevances[1]) < eps);

    // Проверка правильности вывода документов с заданным статусом 
    const auto found_docs_2 = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(found_docs_2[0].id, 3);
    ASSERT(abs(found_docs_2[0].relevance - correct_relevances[2]) < eps);

    // Проверка правильности вывода документов с четным id (тест кастомного предиката)
    const auto found_docs_3 = search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
    ASSERT_EQUAL(found_docs_3[0].id, 0);
    ASSERT_EQUAL(found_docs_3[1].id, 2);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWordsCheck);
    RUN_TEST(TestMatching);
    RUN_TEST(TestRelevanceSortCheck);
    RUN_TEST(TestRatingCalc);
    RUN_TEST(TestPredicatAndRelevanceCalc);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}