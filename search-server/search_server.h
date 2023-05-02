#pragma once

#include <algorithm>
#include <cmath>
#include <execution>
#include <future>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "concurrent_map.h"
#include "document.h"
#include "string_processing.h"

using namespace std;

// максимальное количество документов в результате поиска
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const int MAX_SUBMAP_COUNT = 100;     // максимальное кол-во потоков выполенения

class SearchServer {
public:
    template<typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);

    explicit SearchServer(string stop_words_text);
    explicit SearchServer(string_view stop_words_text);

    void AddDocument(int document_id, string_view document,
            DocumentStatus status, const vector<int> &ratings);

    void RemoveDocument(int document_id);
    void RemoveDocument(const execution::sequenced_policy&, int document_id);
    void RemoveDocument(const execution::parallel_policy&, int document_id);

    vector<Document> FindTopDocuments(string_view raw_query) const;
    template<typename ExecutionPolicy>
    vector<Document> FindTopDocuments(const ExecutionPolicy &policy,
            string_view raw_query) const;

    // перегружает FindTopDocuments для поиска по статусу
    vector<Document> FindTopDocuments(string_view raw_query,
            DocumentStatus status) const;
    template<typename ExecutionPolicy>
    vector<Document> FindTopDocuments(const ExecutionPolicy &policy,
            string_view raw_query, DocumentStatus status) const;

    // возвращает отсортированный вектор документов по запросу
    template<typename DocumentPredicate>
    vector<Document> FindTopDocuments(string_view raw_query,
            DocumentPredicate document_predicate) const;
    template<typename ExecutionPolicy, typename DocumentPredicate>
    vector<Document> FindTopDocuments(const ExecutionPolicy &policy,
            string_view raw_query, DocumentPredicate document_predicate) const;

    size_t GetDocumentCount() const;

    using MatchDocumentResult = tuple<vector<string_view>, DocumentStatus>;
    MatchDocumentResult MatchDocument(string_view raw_query,
            int document_id) const;
    MatchDocumentResult MatchDocument(const execution::sequenced_policy&,
            string_view raw_query, int document_id) const;
    MatchDocumentResult MatchDocument(const execution::parallel_policy&,
            string_view raw_query, int document_id) const;

    int GetDocumentId(int index) const;

    set<int>::iterator begin();
    set<int>::iterator end();

    const map<string_view, double>& GetWordFrequencies(int document_id) const;

private:

    list<string> all_words_;

    struct DocumentData {
        int rating;             // ср.рейтинг
        DocumentStatus status;  // статус
    };

    struct QueryWord {
        string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        vector<string_view> plus_words;
        vector<string_view> minus_words;
    };

    // стоп слова
    set<string> stop_words_;

    // документы в поисковом сервере ({id документа, информация о документе (ср.рейтинг, статус)})
    map<int, DocumentData> documents_;
    set<int> documents_ids_;
    map<string_view, map<int, double>> word_to_document_freqs_;
    map<int, map<string_view, double>> document_to_word_freqs_;

    static bool IsValidWord(string_view word);

    template<typename StringContainer>
    static set<string> MakeUniqueNonEmptyStrings(
            const StringContainer &strings);

    bool IsStopWord(string_view word) const;

    vector<string_view> SplitIntoWordsNoStop(string_view text) const;

    static int ComputeAverageRating(const vector<int> &ratings);

    QueryWord ParseQueryWord(string_view text) const;

    Query ParseQuery(string_view text, bool skip_sort = false) const;

    double ComputeWordInverseDocumentFreq(string_view word) const;

    template<typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query &query,
            DocumentPredicate document_predicate) const;

    template<typename ExecutionPolicy, typename DocumentPredicate>
    vector<Document> FindAllDocuments(const ExecutionPolicy &policy,
            const Query &query, DocumentPredicate document_predicate) const;
};

// Шаблонные функции

template<typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words) :
        stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
}

/**
 * @brief Ищет 5 документов с наибольшей релевантностью
 *
 * Ищет по поисковым словам и критерию, который определяется функцией
 * (функциональный объект, который поступает на вход)
 *
 * @param raw_query   Поисковые слова (слова, которые ищем)
 * @tparam document_predicate Критерий поиска (функция)
 * @return Результат поиска (вектор структур(id документа, релевантность, рейтинг))
 */
template<typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(string_view raw_query,
        DocumentPredicate document_predicate) const {
    Query query = ParseQuery(raw_query, false);
    if (!IsValidWord(raw_query)) {
        throw invalid_argument("--!!!"s);
    }
    vector<Document> documents = FindAllDocuments(query, document_predicate);

    sort(documents.begin(), documents.end(),
            [](const Document &lhs, const Document &rhs) {
                return rhs < lhs;
            });
    if (documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return documents;
}

template<typename ExecutionPolicy, typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy &policy,
        string_view raw_query, DocumentPredicate document_predicate) const {

    if (is_same_v<decay_t<ExecutionPolicy>, execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, document_predicate);
    } else if (is_same_v<decay_t<ExecutionPolicy>, execution::parallel_policy>) {
        const auto query = ParseQuery(raw_query, false);

        vector<Document> matched_documents = FindAllDocuments(policy, query,
                document_predicate);

        sort(policy, matched_documents.begin(), matched_documents.end(),
                [](const Document &lhs, const Document &rhs) {
                    return rhs < lhs;
                });
        //оставляем только MAX_RESULT_DOCUMENT_COUNT первых результатов
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    } else {
        throw runtime_error("invalid parameter passed");
    }
}

template<typename ExecutionPolicy>
vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy &policy,
        string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query,
            [status](int document_id, DocumentStatus document_status,
                    int rating) {
                return document_status == status;
            });
}

template<typename ExecutionPolicy>
vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy &policy,
        string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

/**
 * @brief Проверяет слова из входного контейнера на отсутствие пустых элементов и
 *  недопустимых символов - затем преобразует в set
 *
 * @tparam StringContainer Контейнер слов
 * @param strings Слова (вектор или set)
 * @return set слов
 */
template<typename StringContainer>
set<string> SearchServer::MakeUniqueNonEmptyStrings(
        const StringContainer &strings) {
    set<string> non_empty_strings;
    for (string_view word : strings) {
        string str { word };
        if (!str.empty()) {
            if (!IsValidWord(str)) {
                throw invalid_argument("недопустимые символы !!!"s);
            }
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

/**
 * @brief Ищем документы удовлетворяющие критериям поиска
 *
 * Ищет по поисковым словам и критерию, который определяется функцией
 * (функциональный объект, который поступает на вход)
 *
 * @param query Слова поискового запроса
 * @tparam document_predicate Критерий поиска (функция)
 * @return Вектор документов (id документа, релевантность, ср.рейтинг)
 */
template<typename DocumentPredicate>
vector<Document> SearchServer::FindAllDocuments(
        const SearchServer::Query &query,
        DocumentPredicate document_predicate) const {

    map<int, double> document_to_relevance;
    for (string_view word : query.plus_words) {
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

    for (string_view word : query.minus_words) {
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

template<typename ExecutionPolicy, typename DocumentPredicate>
vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy &policy,
        const SearchServer::Query &query,
        DocumentPredicate document_predicate) const {

    if (is_same_v<decay_t<ExecutionPolicy>, execution::sequenced_policy>) {
        return FindAllDocuments(query, document_predicate);
    } else if (is_same_v<decay_t<ExecutionPolicy>, execution::parallel_policy>) {

        ConcurrentMap<int, double> document_to_relevance_par(MAX_SUBMAP_COUNT);
        const double documents_count = GetDocumentCount();

        // собираем вектор плюс слов
        vector<string_view> plus_words(query.plus_words.size());
        transform(query.plus_words.begin(), query.plus_words.end(),
                plus_words.begin(), [](auto &word) {
                    return word;
                });

        for_each(policy, plus_words.begin(), plus_words.end(),
                [this, &document_to_relevance_par, documents_count,
                        document_predicate](auto &word) {
                    if (word_to_document_freqs_.count(word) == 0) {
                        return;
                    }
                    // проходим по всем документам содержащим плюс слова
                    // считаем IDF для слова из запроса
                    // считаем IDF-TF для документа
                    const double inverse_document_freq =
                            ComputeWordInverseDocumentFreq(word);
                    for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(
                            word)) {
                        // добавляем только документы удовлетворяющие предикату
                        const auto &document = documents_.at(document_id);
                        if (document_predicate(document_id, document.status,
                                document.rating)) {
                            document_to_relevance_par[document_id].ref_to_value +=
                                    term_freq * inverse_document_freq;
                        }
                    }
                });

        map<int, double> document_to_relevance =
                document_to_relevance_par.BuildOrdinaryMap();

        // здесь не параллелим, чтобы не было гонки
        for_each(query.minus_words.begin(), query.minus_words.end(),
                [this, &document_to_relevance](auto &word) {
                    if (word_to_document_freqs_.count(word) == 0) {
                        return;
                    }
                    // проходим по всем документам содержащим минус-слово
                    for (const auto [document_id, _] : word_to_document_freqs_.at(
                            word)) {
                        document_to_relevance.erase(document_id);
                    }
                });

        vector<Document> matched_documents;
        matched_documents.reserve(document_to_relevance.size());
        for (const auto& [document_id, relevance] : document_to_relevance) {
            // формируем вектор документов на выдачу
            matched_documents.push_back(Document { document_id, relevance,
                    documents_.at(document_id).rating });
        }
        return matched_documents;
    } else {
        throw runtime_error("invalid parameter passed");
    }
}
