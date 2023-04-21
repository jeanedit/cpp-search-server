#include "test_example_functions.h"
#include "search_server.h"
using namespace std;

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

// -------- Начало модульных тестов поисковой системы ----------



// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	{
		SearchServer server(""s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1u);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
		SearchServer server("in the"s);
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
		SearchServer server(""s);
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
		SearchServer server(""s);
		for (size_t i = 0; i < doc_id.size(); ++i)
			server.AddDocument(doc_id[i], content[i], DocumentStatus::ACTUAL, ratings);
		const auto found_docs_1 = server.MatchDocument("cat -in"s, 34);

		ASSERT(get<0>(found_docs_1).empty());
		const auto found_docs_2 = server.MatchDocument("cat walking -the"s, 42);
		vector<string> words = { "cat","walking" };

		ASSERT_EQUAL(get<0>(found_docs_2), words);
		ASSERT_EQUAL(static_cast<int>(get<1>(found_docs_2)), static_cast<int>(DocumentStatus::ACTUAL));
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
		SearchServer server(""s);
		for (size_t i = 0; i < doc_id.size(); ++i)
			server.AddDocument(doc_id[i], content[i], DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("white cat long tail"s);
		ASSERT(found_docs[0].relevance - found_docs[1].relevance > 0.0);
	}

	{
		SearchServer server("и в на"s);
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
		SearchServer server(""s);
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
	SearchServer search_server("и в на"s);
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