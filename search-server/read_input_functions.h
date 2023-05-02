#pragma once
#include <string>

#include "document.h"
#include "paginator.h"

using namespace std;

string ReadLine();

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
