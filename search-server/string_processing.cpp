#include <iostream>
#include <string>
#include <vector>

using namespace std;

/**
 * @brief Разбирает строку на слова
 *
 * @param text Строка
 * @return Вектор слов
 */
vector<string> SplitIntoWords(const string &text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else if (c >= '\0' && c < ' ') {
            throw invalid_argument("недопустимые символы !!!"s);
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}
