#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>

#include "search_server.h"
#include "request_queue.h"
#include "document.h"
#include "read_input_functions.h"

using namespace std;

SearchServer::SearchServer(const string &stop_words_text) :
        SearchServer(SplitIntoWords(stop_words_text)) // Invoke delegating constructor from string container
{
}

/**
 * @brief ��������� �������� � ��������� ������
 *
 *  - ��������� ����� �� �����,
 *  - ��������� ����-�����,
 *  - ����������� ��.������� (������� ������� ���� � ���������),
 *  - ����������� TF (term frequency) ����� � ���������
 *  ���������� ������� � ���������:
 *  - documents_ (id ���������, ��.�������, ������)
 *  - word_to_document_freqs_ (�����, map<id ���������, TF>)
 *
 * @param document_id id ���������
 * @param document    ����� ���������
 * @param status      ������ ���������
 * @param ratings     �������� ����
 */
void SearchServer::AddDocument(int document_id, const string &document,
        DocumentStatus status, const vector<int> &ratings) {
    if (document_id < 0) {
        throw invalid_argument(
                "������� ���������� ��������� � ������������� id !!!"s);
    }
    if (documents_.count(document_id) != 0) {
        throw invalid_argument(
                "������� ���������� ��������� � id ����� ������������ ��������� !!!"s);
    }
//        if (!IsValidWord(document)) {
//            throw invalid_argument(
//                    "invalid_argument("������������ �������!!!"s)"s);
//        }
    const vector<string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const string &word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id,
            DocumentData { ComputeAverageRating(ratings), status });
    documents_ids.push_back(document_id);
}

/**
 * @brief ���������� ����-����
 *
 * (����� == ����-�����) -> true
 *
 * @param word �����, ������� ��������� (���� �� ��� � ������ ����-����)
 * @return true - ���� ����� ���� � ������ ����-����, false - ���� ���
 */
bool SearchServer::IsStopWord(const string &word) const {
    return stop_words_.count(word) > 0;
}

/**
 * @brief ������ (���������) ����� �� ���������� ������� �
 *        ��������� �� ������� ����������� ��������� ������������� ������� "-".
 *
 * @param text ������ ���������� �������
 * @return ��������� (������ ���� ���������� �������)
 */
SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        if (text[1] == '-') {
            throw invalid_argument("2 ������� \"�����\" ����� ������ !!!"s);
        }
        if (text.size() == 1) {
            throw invalid_argument(
                    "����������� ����� ����� ������� \"�����\" !!!"s);
        }
        is_minus = true;
        text = text.substr(1);
    }
    return {text, is_minus, IsStopWord(text)};
}

/**
 * @brief ������ (���������) ��������� ������
 *
 * @param text ������ ���������� �������
 * @return ��������� (������ ���� ���������� �������)
 */
SearchServer::Query SearchServer::ParseQuery(const string &text) const {
    SearchServer::Query query;
    for (const string &word : SplitIntoWords(text)) {
        SearchServer::QueryWord query_word = ParseQueryWord(word);

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

/**
 * @brief �������� ���������� ���������� � ��������� �������
 *
 * @return ���������� ���������� � ��������� �������
 */
int SearchServer::GetDocumentCount() const {
    return SearchServer::documents_.size();
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(
        const string &raw_query, int document_id) const {
//        if (!IsValidWord(raw_query)) {
//            throw invalid_argument("!!!"s);
//        }
    SearchServer::Query query = ParseQuery(raw_query);
    vector<string> matched_words;
    for (const string &word : query.plus_words) {
        if (SearchServer::word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (SearchServer::word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const string &word : query.minus_words) {
        if (SearchServer::word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (SearchServer::word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return tuple(matched_words, documents_.at(document_id).status);
}

/**
 * @brief �������� id ��������� �� ����������� ������ � ��������� �������
 *
 * @param index  ���������� ����� ��������� � ��������� �������
 * @return id ���������
 */
int SearchServer::GetDocumentId(int index) const {
    if ((index < 0) || (index > (GetDocumentCount() - 1))) {
        throw out_of_range(
                "id ��������� ������� �� ������� ����������� ���������!!!"s);
    }
    return documents_ids[index];
}

bool SearchServer::IsValidWord(const string &word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

/**
 * @brief ��������� ������ �� ����� � ��������� ����-�����
 *
 * @param text ������, ������� ���������
 * @return ������ ����, �������� � ����� �������� ����-�����
 */
vector<string> SearchServer::SplitIntoWordsNoStop(const string &text) const {
    vector<string> words;
    for (const string &word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

/**
 * @brief ����������� ������� �������
 *
 *  (������� ��������������)
 *
 * @param ratings �������� ����
 * @return  ��.������� ����
 */
int SearchServer::ComputeAverageRating(const vector<int> &ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

/**
 * @brief ����������� IDF (inverse document frequency) �����
 *number_query_without_result
 * @param word �����
 * @return IDF
 */
double SearchServer::ComputeWordInverseDocumentFreq(const string &word) const {
    return std::log(
            GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

vector<Document> SearchServer::FindTopDocuments(const string &raw_query,
        DocumentStatus status) const {
    vector<Document> documents = FindTopDocuments(raw_query,
            [status](int document_id, DocumentStatus document_status,
                    int rating) {
                return document_status == status;
            });
    return documents;
}

vector<Document> SearchServer::FindTopDocuments(const string &raw_query) const {
    vector<Document> documents = FindTopDocuments(raw_query,
            DocumentStatus::ACTUAL);
    return documents;
}
