#include <deque>

#include "request_queue.h"
#include "document.h"
#include "search_server.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer &search_server) :
        search_server_(search_server), current_time_(0) {

}
// сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
vector<Document> RequestQueue::AddFindRequest(const string &raw_query,
        DocumentStatus status) {
    return search_server_.FindTopDocuments(raw_query, status);
}
vector<Document> RequestQueue::AddFindRequest(const string &raw_query) {
    ++current_time_;
    if (!requests_.empty()) {
        while ((current_time_ - requests_.front().time) >= min_in_day_) {
            if (!requests_.front().status) {
                --requests_.back().number_query_without_result;
            }
            requests_.pop_front();
        }
    }

    auto result = search_server_.FindTopDocuments(raw_query);
    QueryResult query_result;

    query_result.time = current_time_;
    if (result.empty()) {
        if (requests_.empty()) {
            query_result.number_query_without_result = 1;
        } else {
            query_result.number_query_without_result =
                    requests_.back().number_query_without_result + 1;
        }
    } else {
        query_result.status = true;
        query_result.number_query_without_result =
                requests_.back().number_query_without_result;
    }
    requests_.push_back(query_result);
    return result;
}
int RequestQueue::GetNoResultRequests() const {
    return requests_.back().number_query_without_result;
}
