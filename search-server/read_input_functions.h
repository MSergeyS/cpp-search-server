#pragma once
#include <string>

#include "document.h"
#include "paginator.h"

using namespace std;

string ReadLine();

vector<string> SplitIntoWords(const string &text);

ostream& operator<<(ostream &out, const Document &document);

/**
 * @brief Оператор вывода для
 */
template<typename Iterator>
ostream& operator<<(ostream &out, const IteratorRange<Iterator> &range) {
    for (Iterator it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}
