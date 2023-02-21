#include <iostream>

#include "search_server.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "document.h"
#include "paginator.h"

using namespace std;

int main() {
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
    // 1439 �������� � ������� �����������
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // ��� ��� 1439 �������� � ������� �����������
    request_queue.AddFindRequest("curly dog"s);
    // ����� �����, ������ ������ ������, 1438 �������� � ������� �����������
    request_queue.AddFindRequest("big collar"s);
    // ������ ������ ������, 1437 �������� � �������number_query_withouimett_result �����������
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests()
            << endl;

    search_server.AddDocument(6, "�������� ��� �������� �����"s,
            DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(7, "�������� �� � ������ �������"s,
            DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(8, "������� ��� ������ ������� "s,
            DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(9, "������� �� ������� �������"s,
            DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(10, "������� �� ������� �������"s,
            DocumentStatus::ACTUAL, { 1, 1, 1 });
    const auto search_results = search_server.FindTopDocuments("�������� ��"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);

    // ������� ��������� ��������� �� ���������
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        cout << *page << endl;
        cout << "������ ��������"s << endl;
    }

    return 0;
}
