#include <iostream>
#include <string>
#include <vector>

#include "document.h"
#include "read_input_functions.h"

using namespace std;

/**
 * @brief Получает строку от пользователя
 *
 * @return Строка, введенная пользователем
 */
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
