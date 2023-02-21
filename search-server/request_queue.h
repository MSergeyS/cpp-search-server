#pragma once

#include <string>
#include <vector>
#include <deque>

#include "document.h"
#include "search_server.h"

using namespace std;

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer &search_server);

    template<typename DocumentPredicate>
    vector<Document> AddFindRequest(const string &raw_query,
            DocumentPredicate document_predicate);
    vector<Document> AddFindRequest(const string &raw_query,
            DocumentStatus status);
    vector<Document> AddFindRequest(const string &raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        int time;
        bool status = false;
        int number_query_without_result = 0;
    };

    const static int min_in_day_ = 1440;
    deque<QueryResult> requests_;
    const SearchServer &search_server_;
    int current_time_ = 0;
};

template<typename DocumentPredicate>
vector<Document> RequestQueue::AddFindRequest(const string &raw_query,
        DocumentPredicate document_predicate) {
    return search_server_.FindTopDocuments(raw_query, document_predicate);
}
