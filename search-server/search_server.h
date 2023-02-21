#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>

#include "document.h"

using namespace std;

// ������������ ���������� ���������� � ���������� ������
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double MIN_DELTA_RELEVANCE = 1e-6;

class SearchServer {
public:
    template<typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);

    explicit SearchServer(const string &stop_words_text);

    void AddDocument(int document_id, const string &document,
            DocumentStatus status, const vector<int> &ratings);

    template<typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string &raw_query,
            DocumentPredicate document_predicate) const;

    vector<Document> FindTopDocuments(const string &raw_query,
            DocumentStatus status) const;

    vector<Document> FindTopDocuments(const string &raw_query) const;

    int GetDocumentCount() const;

    tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query,
            int document_id) const;

    int GetDocumentId(int index) const;

private:

    struct DocumentData {
        int rating;             // ��.�������
        DocumentStatus status;  // ������
    };

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    // ���� �����
    set<string> stop_words_;

    //
    map<string, map<int, double>> word_to_document_freqs_;

    // ��������� � ��������� ������� ({id ���������, ���������� � ��������� (��.�������, ������)})
    map<int, DocumentData> documents_;
    vector<int> documents_ids;

    static bool IsValidWord(const string &word);

    template<typename StringContainer>
    static set<string> MakeUniqueNonEmptyStrings(
            const StringContainer &strings);

    bool IsStopWord(const string &word) const;

    vector<string> SplitIntoWordsNoStop(const string &text) const;

    static int ComputeAverageRating(const vector<int> &ratings);

    QueryWord ParseQueryWord(string text) const;

    Query ParseQuery(const string &text) const;

    double ComputeWordInverseDocumentFreq(const string &word) const;

    template<typename DocumentPredicate>
    vector<Document> FindAllDocuments(const SearchServer::Query &query,
            DocumentPredicate document_predicate) const;

};

// ��������� �������

template<typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words) :
        stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
}

/**
 * @brief ���� 5 ���������� � ���������� ��������������
 *
 * ���� �� ��������� ������ � ��������, ������� ������������ ��������
 * (�������������� ������, ������� ��������� �� ����)
 *
 * @param raw_query   ��������� ����� (�����, ������� ����)
 * @tparam document_predicate �������� ������ (�������)
 * @return ��������� ������ (������ ��������(id ���������, �������������, �������))
 */
template<typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(const string &raw_query,
        DocumentPredicate document_predicate) const {
    Query query = ParseQuery(raw_query);
//        if (!IsValidWord(raw_query)) {
//            throw invalid_argument("--!!!"s);
//        }
    vector<Document> documents = FindAllDocuments(query, document_predicate);

    sort(documents.begin(), documents.end(),
            [](const Document &lhs, const Document &rhs) {
                if (std::abs(lhs.relevance - rhs.relevance)
                        < MIN_DELTA_RELEVANCE) {
                    return lhs.rating > rhs.rating;
                } else {
                    return lhs.relevance > rhs.relevance;
                }
            });
    if (documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return documents;
}

/**
 * @brief ���� ��������� ��������������� ��������� ������
 *
 * ���� �� ��������� ������ � ��������, ������� ������������ ��������
 * (�������������� ������, ������� ��������� �� ����)
 *
 * @param query ����� ���������� �������
 * @tparam document_predicate �������� ������ (�������)
 * @return ������ ���������� (id ���������, �������������, ��.�������)
 */
template<typename DocumentPredicate>
vector<Document> SearchServer::FindAllDocuments(
        const SearchServer::Query &query,
        DocumentPredicate document_predicate) const {
    map<int, double> document_to_relevance;
    for (const string &word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(
                word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(
                word)) {
            const DocumentData &document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status,
                    document_data.rating)) {
                document_to_relevance[document_id] += term_freq
                        * inverse_document_freq;
            }
        }
    }

    for (const string &word : query.minus_words) {
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

/**
 * @brief ��������� ����� �� �������� ���������� �� ���������� ������ ��������� �
 *  ������������ �������� - ����� ����������� � set
 *
 * @tparam StringContainer ��������� ����
 * @param strings ����� (������ ��� set)
 * @return set ����
 */
template<typename StringContainer>
set<string> SearchServer::MakeUniqueNonEmptyStrings(
        const StringContainer &strings) {
    set<string> non_empty_strings;
    for (const string &str : strings) {
        if (!str.empty()) {
            if (!IsValidWord(str)) {
                throw invalid_argument("������������ ������� !!!"s);
            }
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
