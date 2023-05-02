#include <iostream>
#include <string>
#include <vector>

#include "string_processing.h"

using namespace std;

/**
 * @brief Разбирает строку на слова
 *
 * @param text Строка
 * @return Вектор слов
 */
vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> output;
    size_t first = 0;

    while (first < text.size()) {
        const auto second = text.find_first_of(" ", first);

        if (first != second)
            output.emplace_back(text.substr(first, second - first));

        if (second == string_view::npos)
            break;

        first = second + 1;
    }

    return output;
}
