#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "document.h"
#include "request_queue.h"
#include "search_server.h"
#include "string_processing.h"

using namespace std;

SearchServer::SearchServer(string stop_words_text) :
        SearchServer(SplitIntoWords(stop_words_text)) {
}

SearchServer::SearchServer(string_view stop_words_text) :
        SearchServer(SplitIntoWords(stop_words_text)) {
}

/**
 * @brief Добавляет документ в поисковый сервер
 *
 *  - разбирает текст на слова,
 *  - исключает стоп-слова,
 *  - расчитывает ср.рейтинг (средний рейтинг слов в документе),
 *  - расчитывает TF (term frequency) слова в документе
 *  результаты заносит в контейнеры:
 *  - documents_ (id документа, ср.рейтинг, статус)
 *  - documents_ids_
 *  - word_to_document_freqs_ (слово, map<id документа, TF>)
 *  - document_to_word_freqs_ (id документа, map<слово, TF>)
 *
 * @param document_id id документа
 * @param document    Текст документа
 * @param status      Статус документа
 * @param ratings     Рейтинги слов
 */
void SearchServer::AddDocument(int document_id, string_view document,
        DocumentStatus status, const vector<int> &ratings) {
    if (document_id < 0) {
        throw invalid_argument(
                "Попытка добавления документа с отрицательный id !!!"s);
    }
    if (documents_.count(document_id) > 0) {
        throw invalid_argument(
                "Попытка добавления документа с id ранее добавленного документа !!!"s);
    }
    if (!IsValidWord(document)) {
        throw invalid_argument("недопустимые символы!!!"s);
    }
    vector<string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (string_view word : words) {
        string str_word { word };
        auto it_word = find(all_words_.begin(), all_words_.end(), str_word);
        if (it_word == all_words_.end()) {
            all_words_.push_back(str_word);
            it_word = --all_words_.end();
        }
        word_to_document_freqs_[*it_word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][*it_word] += inv_word_count;
    }

    for (string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id,
            DocumentData { ComputeAverageRating(ratings), status });
    documents_ids_.emplace(document_id);
}

/**
 * @brief Удаляет документ из поискового сервера
 *
 *  удаляет информацию о документе из контейнеров:
 *  - documents_ (id документа, ср.рейтинг, статус)
 *  - documents_ids_
 *  - word_to_document_freqs_ (слово, map<id документа, TF>)
 *  - document_to_word_freqs_ (id документа, map<слово, TF>)
 *
 * @param document_id id документа
 */
void SearchServer::RemoveDocument(int document_id) {
    // удаляем из documents_
    documents_.erase(document_id);
    // удаляем из documents_ids_
    auto inx = find(documents_ids_.begin(), documents_ids_.end(), document_id);
    if (inx != documents_ids_.end()) {
        documents_ids_.erase(inx);
    }

    auto word_freq = GetWordFrequencies(document_id);
    if (!word_freq.empty()) {
        // удаляем из document_to_word_freqs_
        document_to_word_freqs_.erase(document_id);
        // удаляем из word_to_document_freqs_
        for (auto [word, freq] : word_freq) {
            word_to_document_freqs_.at(word).erase(document_id);
            // если для слова в контейнере не осталось документов
            if (word_to_document_freqs_.at(word).empty()) {
                // слово удаляем из контейнера
                word_to_document_freqs_.erase(word);
            }
        }
    }
}

void SearchServer::RemoveDocument(const execution::sequenced_policy&,
        int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const execution::parallel_policy&,
        int document_id) {
    if (documents_.count(document_id) != 0) {

        // собираем вектор слов документа
        vector<string> words(document_to_word_freqs_.at(document_id).size());
        transform(execution::par,
                document_to_word_freqs_.at(document_id).begin(),
                document_to_word_freqs_.at(document_id).end(), words.begin(),
                [](auto &word) {
                    return word.first;
                }
        );

        // удаляем слова (можно распараллелить потому что из каждого словаря удалится максимум одна запись)
        for_each(execution::par, words.begin(), words.end(),
                [this, document_id](string word) {
                    word_to_document_freqs_.at(word).erase(document_id);
                });

        documents_.erase(document_id);
        documents_ids_.erase(document_id);
        document_to_word_freqs_.erase(document_id);
    }
}

/**
 * @brief Компаратор стоп-слов
 *
 * (слово == стоп-слово) -> true
 *
 * @param word Слово, которое проверяем (есть ли оно в списке стоп-слов)
 * @return true - если слово есть в списке стоп-слов, false - если нет
 */
bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(string{word}) > 0;
}

/**
 * @brief Парсим (разбираем) слова из поискового запроса и
 *        проверяем на наличие запрещённых вариантов использования символа "-".
 *
 * @param text Строка поискового запроса
 * @return Структура (наборы слов поискового запроса)
 */
SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        if (text[1] == '-') {
            throw invalid_argument("2 символа \"минус\" перед словом !!!"s);
        }
        if (text.size() == 1) {
            throw invalid_argument(
                    "отсутствует текст после символа \"минус\" !!!"s);
        }
        is_minus = true;
        text = text.substr(1);
    }
    return {text, is_minus, IsStopWord(text)};
}

/**
 * @brief Парсим (разбираем) поисковый запрос
 *
 * @param text Строка поискового запроса
 * @return Структура (наборы слов поискового запроса)
 */
SearchServer::Query SearchServer::ParseQuery(string_view text,
        bool skip_sort) const {
    SearchServer::Query query;
    for (string_view word : SplitIntoWords(text)) {
        SearchServer::QueryWord query_word = ParseQueryWord(word);

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            } else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    if (!skip_sort) {
        sort(query.plus_words.begin(), query.plus_words.end());
        query.plus_words.resize(
                distance(query.plus_words.begin(),
                        unique(query.plus_words.begin(),
                                query.plus_words.end())));
        sort(query.minus_words.begin(), query.minus_words.end());
        query.minus_words.resize(
                distance(query.minus_words.begin(),
                        unique(query.minus_words.begin(),
                                query.minus_words.end())));
    }
    return query;
}

/**
 * @brief Получает количество документов в поисковом сервере
 *
 * @return Количество документов в поисковом сервере
 */
size_t SearchServer::GetDocumentCount() const {
    return SearchServer::documents_.size();
}

SearchServer::MatchDocumentResult SearchServer::MatchDocument(
        string_view raw_query, int document_id) const {
    return MatchDocument(execution::seq, raw_query, document_id);
}

SearchServer::MatchDocumentResult SearchServer::MatchDocument(
        const execution::sequenced_policy&, string_view raw_query,
        int document_id) const {
    if (documents_ids_.count(document_id) == 0) {
        return { {}, {}};
    }

    if (!IsValidWord(raw_query)) {
        throw invalid_argument("!!!"s);
    }

    const Query query = ParseQuery(raw_query);

    const auto word_checker = [this, document_id](string_view word) {
        const auto it = word_to_document_freqs_.find(word);
        return it != word_to_document_freqs_.end()
                && it->second.count(document_id);
    };

    if (any_of(execution::seq, query.minus_words.begin(),
            query.minus_words.end(), word_checker)) {
        vector<string_view> empty;
        return {empty, documents_.at(document_id).status};
    }

    vector<string_view> matched_words(query.plus_words.size());
    auto words_end = copy_if(execution::seq, query.plus_words.begin(),
            query.plus_words.end(), matched_words.begin(), word_checker);
    sort(matched_words.begin(), words_end);
    words_end = unique(matched_words.begin(), words_end);
    matched_words.erase(words_end, matched_words.end());

    return make_tuple(matched_words, documents_.at(document_id).status);
}

SearchServer::MatchDocumentResult SearchServer::MatchDocument(
        const execution::parallel_policy&, string_view raw_query,
        int document_id) const {
    if (documents_ids_.count(document_id) == 0) {
        return { {}, {}};
    }

    if (!IsValidWord(raw_query)) {
        throw invalid_argument("!!!"s);
    }

    const Query query = ParseQuery(raw_query, true);

    const auto word_checker = [this, document_id](string_view word) {
        const auto it = word_to_document_freqs_.find(word);
        return it != word_to_document_freqs_.end()
                && it->second.count(document_id);
    };

    if (any_of(execution::par, query.minus_words.begin(),
            query.minus_words.end(), word_checker)) {
        vector<string_view> empty;
        return {empty, documents_.at(document_id).status};
    }

    vector<string_view> matched_words(query.plus_words.size());
    auto words_end = copy_if(execution::par, query.plus_words.begin(),
            query.plus_words.end(), matched_words.begin(), word_checker);
    sort(execution::par, matched_words.begin(), words_end);
    words_end = unique(execution::par, matched_words.begin(), words_end);
    matched_words.erase(words_end, matched_words.end());

    return make_tuple(matched_words, documents_.at(document_id).status);
}

/**
 * @brief Возвращает итератор на начало в поисковом сервере
 *
 * @return Возвращает возвращает итератор на первый элемент
 */
set<int>::iterator SearchServer::begin() {
    return documents_ids_.begin();
}

/**
 * @brief Возвращает итератор на конец в поисковом сервере
 *
 * @return Возвращает возвращает итератор на элемент, следующий за последним элементом
 */
set<int>::iterator SearchServer::end() {
    return documents_ids_.end();
}

bool SearchServer::IsValidWord(string_view word) {
    string str_word { word };
    // A valid word must not contain special characters
    return none_of(str_word.begin(), str_word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

/**
 * @brief Разбивает строку на слова и исключает стоп-слова
 *
 * @param text Строка, которую разбираем
 * @return Вектор слов, входящих в троку исключая стоп-слова
 */
vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string_view> words;
    for (string_view word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

/**
 * @brief Расчитываем средний рейтинг
 *
 *  (среднее арифметическое)
 *
 * @param ratings Рейтинги слов
 * @return  Ср.рейтинг слов
 */
int SearchServer::ComputeAverageRating(const vector<int> &ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

/**
 * @brief Расчитываем IDF (inverse document frequency) слова
 *
 * @param word Слово
 * @return IDF
 */
double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const {
    return log(
            GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size())
            / 2;
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query,
        DocumentStatus status) const {
    return FindTopDocuments(raw_query,
            [status](int document_id, DocumentStatus document_status,
                    int rating) {
                return document_status == status;
            });
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

/**
 * @brief Получение частот слов (IDF - inverse document frequency) по id документа
 *
 * @param document_id id документа
 * @return контейнер слово - IDF
 */
const map<string_view, double>& SearchServer::GetWordFrequencies(
        int document_id) const {
    if (document_to_word_freqs_.count(document_id)) {
        return document_to_word_freqs_.at(document_id);
    }
    static map<string_view, double> empty_map;
    return empty_map;
}
