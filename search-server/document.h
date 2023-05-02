#pragma once

#include <iostream>

const double MIN_DELTA_RELEVANCE = 1e-6;

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

    friend bool operator ==(const Document &lhs, const Document &rhs);
    friend bool operator !=(const Document &lhs, const Document &rhs);
    friend bool operator <(const Document &lhs, const Document &rhs);
    friend std::ostream& operator <<(std::ostream &os, const Document &rhs);
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
