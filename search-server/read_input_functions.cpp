#include <string>
#include <vector>
#include <iostream>

#include "read_input_functions.h"
#include "document.h"

using namespace std;

/**
 * @brief �������� ������ �� ������������
 *
 * @return ������, ��������� �������������
 */
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

/**
 * @brief �������� ������ ��� ��������� Document
 */
ostream& operator<<(ostream &out, const Document &document) {
    out << "{ "s << "document_id = "s << document.id << ", "s << "relevance = "s
            << document.relevance << ", "s << "rating = "s << document.rating
            << " }"s;
    return out;
}
