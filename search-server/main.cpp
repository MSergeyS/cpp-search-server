#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// максимальное количество документов в результате поиска
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double MIN_DELTA_RELEVANCE = 1e-6;

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

class SearchServer {
public:
    template<typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words) :
            stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
//        for (auto &word : stop_words_) {
//            if (!IsValidWord(word)) {
//                throw invalid_argument("недопустимые символы !!!"s);
//            }
//        }
    }

    explicit SearchServer(const string &stop_words_text) :
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
     *  результаты заносит в структуры:
     *  - documents_ (id докутенса, ср.рейтинг, статус)
     *  - word_to_document_freqs_ (слово, map<id документа, TF>)
     *
     * @param document_id id документа
     * @param document    Текст документа
     * @param status      Статус документа
     * @param ratings     Рейтинги слов
     */
    void AddDocument(int document_id, const string &document,
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
//                    "invalid_argument("недопустимые символы!!!"s)"s);
//        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string &word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                DocumentData { ComputeAverageRating(ratings), status });
        documents_ids.push_back(document_id);
    }

    /**
     * @brief Ищет 5 документов с наибольшей релевантностью
     *
     * Ищет по поисковым словам и критерию, который определяется функцией
     * (функциональный объект, который поступает на вход)
     *
     * @param raw_query   Поисковые слова (слова, которые ищем)
     * @tparam document_predicate Критерий поиска (функция)
     * @return Результат поиска (вектор структур(id документа, релевантность, рейтинг))
     */
    template<typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string &raw_query,
            DocumentPredicate document_predicate) const {
        Query query = ParseQuery(raw_query);
//        if (!IsValidWord(raw_query)) {
//            throw invalid_argument("--!!!"s);
//        }
        vector<Document> documents = FindAllDocuments(query,
                document_predicate);

        sort(documents.begin(), documents.end(),
                [](const Document &lhs, const Document &rhs) {
                    if (abs(lhs.relevance - rhs.relevance)
                            < MIN_DELTA_RELEVANCE) {
                        return lhs.rating > rhs.rating;
                    } else {
                        return lhs.relevance > rhs.relevance;
                    }
                });
        if (documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return documents;
    }

    vector<Document> FindTopDocuments(const string &raw_query,
            DocumentStatus status) const {
        vector<Document> documents = FindTopDocuments(raw_query,
                [status](int document_id, DocumentStatus document_status,
                        int rating) {
                    return document_status == status;
                });
        return documents;
    }

    vector<Document> FindTopDocuments(const string &raw_query) const {
        vector<Document> documents = FindTopDocuments(raw_query,
                DocumentStatus::ACTUAL);
        return documents;
    }

    /**
     * @brief Получает количество документов в поисковом сервере
     *
     * @return Количество документов в поисковом сервере
     */
    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query,
            int document_id) const {
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
     * @brief Получает id документа по порядковому номеру в поисковом сервере
     *
     * @param index  Порядковый номер документа в поисковом сервере
     * @return id документа
     */
    int GetDocumentId(int index) const {
        if ((index < 0) || (index > (GetDocumentCount() - 1))) {
            throw out_of_range(
                    "id документа выходит за пределы допустимого диапазона!!!"s);
        }
        return documents_ids[index];
    }

private:
    /**
     * @brief Информация о документах
     *
     */
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    // стоп слова
    set<string> stop_words_;

    //
    map<string, map<int, double>> word_to_document_freqs_;

    // документы в поисковом сервере ({id документа, информация о документе (ср.рейтинг, статус)})
    map<int, DocumentData> documents_;
    vector<int> documents_ids;

    static bool IsValidWord(const string &word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    /**
     * @brief Проверяет слова из входного контейнера на отсутствие пустых элементов и
     *  недопустимых символов - затем преобразует в set
     *
     * @tparam StringContainer Контейнер слов
     * @param strings Слова (вектор или set)
     * @return set слов
     */
    template<typename StringContainer>
    static set<string> MakeUniqueNonEmptyStrings(
            const StringContainer &strings) {
        set<string> non_empty_strings;
        for (const string &str : strings) {
            if (!str.empty()) {
                if (!IsValidWord(str)) {
                    throw invalid_argument("недопустимые символы !!!"s);
                }
                non_empty_strings.insert(str);
            }
        }
        return non_empty_strings;
    }

    /**
     * @brief Компаратор стоп-слов
     *
     * (слово == стоп-слово) -> true
     *
     * @param word Слово, которое проверяем (есть ли оно в списке стоп-слов)
     * @return true - если слово есть в списке стоп-слов, false - если нет
     */
    bool IsStopWord(const string &word) const {
        return stop_words_.count(word) > 0;
    }

    /**
     * @brief Разбивает строку на слова и исключает стоп-слова
     *
     * @param text Строка, которую разбираем
     * @return Вектор слов, входящих в троку исключая стоп-слова
     */
    vector<string> SplitIntoWordsNoStop(const string &text) const {
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
    static int ComputeAverageRating(const vector<int> &ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    /**
     * @brief Тип слов из поискового запроса
     *
     */
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    /**
     * @brief Парсим (разбираем) слова из поискового запроса и
     *        проверяем на наличие запрещённых вариантов использования символа "-".
     *
     * @param text Строка поискового запроса
     * @return Структура (наборы слов поискового запроса)
     */
    QueryWord ParseQueryWord(string text) const {
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
     * @brief Слова поискового запроса (поисковые слова)
     *
     */
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    /**
     * @brief Парсим (разбираем) поисковый запрос
     *
     * @param text Строка поискового запроса
     * @return Структура (наборы слов поискового запроса)
     */
    Query ParseQuery(const string &text) const {
        Query query;
        for (const string &word : SplitIntoWords(text)) {
            QueryWord query_word = ParseQueryWord(word);

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
     * @brief Расчитываем IDF (inverse document frequency) слова
     *
     * @param word Слово
     * @return IDF
     */
    double ComputeWordInverseDocumentFreq(const string &word) const {
        return log(
                GetDocumentCount() * 1.0
                        / word_to_document_freqs_.at(word).size());
    }

    /**
     * @brief Ищем документы удовлетворяющие критериям поиска
     *
     * Ищет по поисковым словам и критерию, который определяется функцией
     * (функциональный объект, который поступает на вход)
     *
     * @param query Слова поискового запроса
     * @tparam document_predicate Критерий поиска (функция)
     * @return Вектор документов (id документа, релевантность, ср.рейтинг)
     */
    template<typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query &query,
            DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string &word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(
                    word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(
                    word)) {
                const auto &document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status,
                        document_data.rating)) {
                    document_to_relevance[document_id] += term_freq
                            * inverse_document_freq;
                }
            }
        }

        for (const string &word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back( { document_id, relevance,
                    documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

// ==================== для примера =========================
/**
 * @brief Вывод в консоль результатов поиска.
 *
 * @param document Найденные документы.
 */
void PrintDocument(const Document &document) {
    cout << "{ "s << "document_id = "s << document.id << ", "s
            << "relevance = "s << document.relevance << ", "s << "rating = "s
            << document.rating << " }"s << endl;
}

int main() {
    try {
//        SearchServer search_server("и в\x12 на"s);
        SearchServer search_server("и в на"s);

//        search_server.AddDocument(-1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(0, "пушистый кот пушистый хвост"s,
                DocumentStatus::ACTUAL, { 7, 2, 7 });
//        search_server.AddDocument(0, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
//        search_server.AddDocument(1, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, {1, 3, 2});
        search_server.AddDocument(1, "большой пёс скворец"s,
                DocumentStatus::ACTUAL, { 1, 3, 2 });

        cout << search_server.GetDocumentId(1) << endl;

        auto documents = search_server.FindTopDocuments("--пушистый"s);
        for (const Document &document : documents) {
            PrintDocument(document);
        }
    } catch (const invalid_argument &e) {
        cout << "Ошибка: "s << e.what() << endl;
    } catch (const out_of_range &e) {
        cout << "Ошибка: "s << e.what() << endl;
    }
}