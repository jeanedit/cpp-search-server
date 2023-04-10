
#include <stdexcept>
#include <iostream>
using namespace std;

#include "string_processing.h"
#include "read_input_functions.h"
#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"
#include "document.h"

int main() {
    try {
        SearchServer search_server("и в на"s);
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(2, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        //search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        //search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
        const auto documents = search_server.FindTopDocuments("и пушистый"s);
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    catch (const std::invalid_argument& i) {
        std::cout << "Mistake: "s << i.what() << std::endl;
    }
    catch (const std::out_of_range& o) {
        std::cout << "Mistake: "s << o.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown error"s << std::endl;
    }
}