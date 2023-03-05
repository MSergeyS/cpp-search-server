#pragma once

/**
 * @brief Результат поиска
 *
 */
struct Document {
	Document() = default;

	Document(int id, double relevance, int rating) :
			id(id), relevance(relevance), rating(rating) {
	}

	int id = 0;
	double relevance = 0.0;
	int rating = 0;
};

/**
 * @brief Cтатус документа в поисковом сервере (класс перечисления)
 *
 */
enum class DocumentStatus {
	ACTUAL,      // действительные
	IRRELEVANT,  // не подходящие
	BANNED,      // запрещённые
	REMOVED,     // удалённые
};
