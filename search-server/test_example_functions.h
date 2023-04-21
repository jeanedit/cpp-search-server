#pragma once

#include<iostream>
#include <iomanip>
#include <string>
using namespace std::literals::string_literals;


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
	const std::string& func, unsigned line, const std::string& hint) {
	if (t != u) {
		std::cout << std::boolalpha;
		std::cout << file << "("s << line << "): "s << func << ": "s;
		std::cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		if (!hint.empty()) {
			std::cout << " Hint: "s << hint;
		}
		std::cout << std::endl;
		abort();
	}
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s) 

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint)) 

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
	const std::string& hint);



#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s) 

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint)) 


template <typename T>
void RunTestImpl(T& func, const std::string& expr_str) {
	func();
	std::cerr << expr_str << " OK" << std::endl;
}


#define RUN_TEST(func) RunTestImpl(func,#func) // напишите недостающий код 

// -------- Ќачало модульных тестов поисковой системы ----------



// “ест провер€ет, что поискова€ система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent();

void TestMinusWordsCheck();

void TestMatching();

void TestRelevanceSortCheck();


void TestRatingCalc();

void TestPredicatAndRelevanceCalc();

// ‘ункци€ TestSearchServer €вл€етс€ точкой входа дл€ запуска тестов
void TestSearchServer();

// --------- ќкончание модульных тестов поисковой системы -----------