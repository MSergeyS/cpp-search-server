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
#include "read_input_functions.h"
#include "request_queue.h"
#include "search_server.h"

using namespace std;

SearchServer::SearchServer(const string &stop_words_text) :
		SearchServer(SplitIntoWords(stop_words_text)) // Invoke delegating constructor from string container
{
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
void SearchServer::AddDocument(int document_id, const string &document,
		DocumentStatus status, const vector<int> &ratings) {
	if (document_id < 0) {
		throw invalid_argument(
				"Попытка добавления документа с отрицательный id !!!"s);
	}
	if (documents_.count(document_id) != 0) {
		throw invalid_argument(
				"Попытка добавления документа с id ранее добавленного документа !!!"s);
	}
//        if (!IsValidWord(document)) {
//            throw invalid_argument(
//                    "invalid_argument("������������ �������!!!"s)"s);
//        }
	const vector<string> words = SplitIntoWordsNoStop(document);
	const double inv_word_count = 1.0 / words.size();
	for (const string &word : words) {
		word_to_document_freqs_[word][document_id] += inv_word_count;
		document_to_word_freqs_[document_id][word] += inv_word_count;
	}
	documents_.emplace(document_id,
			DocumentData { ComputeAverageRating(ratings), status });
	documents_ids_.push_back(document_id);
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
	if (documents_.count(document_id)) {
		documents_.erase(document_id);
	}
	auto inx = find(documents_ids_.begin(), documents_ids_.end(), document_id);
	if (inx != documents_ids_.end()) {
		documents_ids_.erase(inx);
	}
	auto word_freq = GetWordFrequencies(document_id);
	if (!word_freq.empty()) {
		document_to_word_freqs_.erase(document_id);
		for (auto [word, freq] : word_freq) {
			word_to_document_freqs_.at(word).erase(document_id);
		}
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
bool SearchServer::IsStopWord(const string &word) const {
	return stop_words_.count(word) > 0;
}

/**
 * @brief Парсим (разбираем) слова из поискового запроса и
 *        проверяем на наличие запрещённых вариантов использования символа "-".
 *
 * @param text Строка поискового запроса
 * @return Структура (наборы слов поискового запроса)
 */
SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
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
 * @brief Получает количество документов в поисковом сервере
 *
 * @return Количество документов в поисковом сервере
 */
int SearchServer::GetDocumentCount() const {
	return SearchServer::documents_.size();
}


tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(
		const string &raw_query, int document_id) const {
//        if (!IsValidWord(raw_query)) {
//            throw invalid_argument("!!!"s);
//        }
	Query query = ParseQuery(raw_query);
	vector<string> matched_words;
	for (const string &word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(word);
		}
	}
	for (const string &word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.clear();
			break;
		}
	}
	return tuple(matched_words, documents_.at(document_id).status);
}

/**
 * @brief Возвращает итератор на начало в поисковом сервере
 *
 * @return Возвращает возвращает итератор на первый элемент
 */
vector<int>::iterator SearchServer::begin() {
	return documents_ids_.begin();
}

/**
 * @brief Возвращает итератор на конец в поисковом сервере
 *
 * @return Возвращает возвращает итератор на элемент, следующий за последним элементом
 */
vector<int>::iterator SearchServer::end() {
	return documents_ids_.end();
}

/**
 * @brief Получает id документа по порядковому номеру в поисковом сервере
 *
 * @param index  Порядковый номер документа в поисковом сервере
 * @return id документа
 */
int SearchServer::GetDocumentId(int index) const {
	if ((index < 0) || (index > (GetDocumentCount() - 1))) {
		throw out_of_range(
				"id документа выходит за пределы допустимого диапазона!!!"s);
	}
	return documents_ids_[index];
}


bool SearchServer::IsValidWord(const string &word) {
	// A valid word must not contain special characters
	return none_of(word.begin(), word.end(), [](char c) {
		return c >= '\0' && c < ' ';
	});
}

/**
 * @brief Разбивает строку на слова и исключает стоп-слова
 *
 * @param text Строка, которую разбираем
 * @return Вектор слов, входящих в троку исключая стоп-слова
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

/**
 * @brief Получение частот слов (IDF - inverse document frequency) по id документа
 *
 * @param document_id id документа
 * @return контейнер слово - IDF
 */
const map<string, double>& SearchServer::GetWordFrequencies(
		int document_id) const {
	if (document_to_word_freqs_.count(document_id)) {
		return document_to_word_freqs_.at(document_id);
	}
	static map<string, double> empty_map;
	return empty_map;
}
