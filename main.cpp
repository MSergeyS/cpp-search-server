#include <iostream>
#include <iterator>

#include "document.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "remove_duplicates.h"
#include "request_queue.h"
#include "search_server.h"
#include "test_example_functions.h"

using namespace std;

int main() {
	/*
	 SearchServer search_server("and in at"s);
	 RequestQueue request_queue(search_server);
	 search_server.AddDocument(1, "curly cat curly tail"s,
	 DocumentStatus::ACTUAL, { 7, 2, 7 });
	 search_server.AddDocument(2, "curly dog and fancy collar"s,
	 DocumentStatus::ACTUAL, { 1, 2, 3 });
	 search_server.AddDocument(3, "big cat fancy collar "s,
	 DocumentStatus::ACTUAL, { 1, 2, 8 });
	 search_server.AddDocument(4, "big dog sparrow Eugene"s,
	 DocumentStatus::ACTUAL, { 1, 3, 2 });
	 search_server.AddDocument(5, "big dog sparrow Vasiliy"s,
	 DocumentStatus::ACTUAL, { 1, 1, 1 });
	 // 1439 запросов с нулевым результатом
	 for (int i = 0; i < 1439; ++i) {
	 request_queue.AddFindRequest("empty request"s);
	 }
	 // все еще 1439 запросов с нулевым результатом
	 request_queue.AddFindRequest("curly dog"s);
	 // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
	 request_queue.AddFindRequest("big collar"s);
	 // первый запрос удален, 1437 запросов с нулевымnumber_query_withouimett_result результатом
	 request_queue.AddFindRequest("sparrow"s);
	 cout << "Total empty requests: "s << request_queue.GetNoResultRequests()
	 << endl;

	 search_server.AddDocument(6, "пушистый кот пушистый хвост"s,
	 DocumentStatus::ACTUAL, { 7, 2, 7 });
	 search_server.AddDocument(7, "пушистый пёс и модный ошейник"s,
	 DocumentStatus::ACTUAL, { 1, 2, 3 });
	 search_server.AddDocument(8, "большой кот модный ошейник "s,
	 DocumentStatus::ACTUAL, { 1, 2, 8 });
	 search_server.AddDocument(9, "большой пёс скворец евгений"s,
	 DocumentStatus::ACTUAL, { 1, 3, 2 });
	 search_server.AddDocument(10, "большой пёс скворец василий"s,
	 DocumentStatus::ACTUAL, { 1, 1, 1 });
	 const auto search_results = search_server.FindTopDocuments("пушистый пёс"s);
	 int page_size = 2;
	 const auto pages = Paginate(search_results, page_size);

	 // 1439 запросов с нулевым результатом
	 for (auto page = pages.begin(); page != pages.end(); ++page) {
	 cout << *page << endl;
	 cout << "Разрыв страницы"s << endl;
	 }

	 for (const int document_id : search_server) {
	 cout << document_id;
	 }
	 cout << endl;

	 search_server.RemoveDocument(2);
	 //search_server.RemoveDocument(2);

	 for (const int document_id : search_server) {
	 cout << document_id;
	 }
	 cout << endl;
	 map<string, double> a = search_server.GetWordFrequencies(2);

	 for (auto [word, freq] : a) {
	 cout << word << " : "s << freq << endl;
	 }
	 */

	SearchServer search_server1("and with"s);

	AddDocument(search_server1, 1, "funny pet and nasty rat"s,
			DocumentStatus::ACTUAL, { 7, 2, 7 });
	AddDocument(search_server1, 2, "funny pet with curly hair"s,
			DocumentStatus::ACTUAL, { 1, 2 });

	// дубликат документа 2, будет удалён
	AddDocument(search_server1, 3, "funny pet with curly hair"s,
			DocumentStatus::ACTUAL, { 1, 2 });

	// отличие только в стоп-словах, считаем дубликатом
	AddDocument(search_server1, 4, "funny pet and curly hair"s,
			DocumentStatus::ACTUAL, { 1, 2 });

	// множество слов такое же, считаем дубликатом документа 1
	AddDocument(search_server1, 5, "funny funny pet and nasty nasty rat"s,
			DocumentStatus::ACTUAL, { 1, 2 });

	// добавились новые слова, дубликатом не является
	AddDocument(search_server1, 6, "funny pet and not very nasty rat"s,
			DocumentStatus::ACTUAL, { 1, 2 });

	// множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
	AddDocument(search_server1, 7, "very nasty rat and not very funny pet"s,
			DocumentStatus::ACTUAL, { 1, 2 });

	// есть не все слова, не является дубликатом
	AddDocument(search_server1, 8, "pet with rat and rat and rat"s,
			DocumentStatus::ACTUAL, { 1, 2 });

	// слова из разных документов, не является дубликатом
	AddDocument(search_server1, 9, "nasty rat with curly hair"s,
			DocumentStatus::ACTUAL, { 1, 2 });

	cout << "Before duplicates removed: "s << search_server1.GetDocumentCount()
			<< endl;
	RemoveDuplicates(search_server1);
	cout << "After duplicates removed: "s << search_server1.GetDocumentCount()
			<< endl;

	return 0;
}
