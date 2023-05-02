#include "document.h"

using namespace std;

// перегрузка оператора == для типа Document
bool operator ==(const Document &lhs, const Document &rhs) {
    return (lhs.id == rhs.id && lhs.rating == rhs.rating
            && abs(lhs.relevance - rhs.relevance) < MIN_DELTA_RELEVANCE);
}
bool operator !=(const Document &lhs, const Document &rhs) {
    return !(lhs == rhs);
}
// перегрузка оператора < для типа Document
bool operator <(const Document &lhs, const Document &rhs) {
    //сравниваем документы по убыванию релевантности и рейтинга
    if (abs(lhs.relevance - rhs.relevance) < MIN_DELTA_RELEVANCE) {
        return lhs.rating < rhs.rating;
    }
    return lhs.relevance < rhs.relevance;
}

/**
 * @brief Оператор вывода для структуры Document
 */
// перегрузка оператора << для типа Document
ostream& operator<<(ostream &out, const Document &document) {
    out << "{ "s << "document_id = "s << document.id << ", "s << "relevance = "s
            << document.relevance << ", "s << "rating = "s << document.rating
            << " }"s;
    return out;
}
