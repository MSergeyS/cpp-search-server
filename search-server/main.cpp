#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

//#include "search_server.h"

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */


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
    int id;           // id документа
    double relevance; // релевантность
    int rating;       // рейтинг
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

/**
 * @brief Поисковый сервер
 *
 */
class SearchServer {
public:
    /**
     * @brief Задает стоп-слова
     *
     *  Стоп-слова - слова исключаемые из документов и из поискового запроса при поиске
     *
     * @param text Строка текста
     */
    void SetStopWords(const string &text) {
        for (const string &word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
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
        // разбирает текст документа на слова и исключаем стоп слова
        const vector<string> words = SplitIntoWordsNoStop(document);
        // вес слова для подсчёта TF
        const double inv_word_count = 1.0 / words.size();
        for (const string &word : words) {
            // TF слов
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        // заносим в поисковый сервер
        documents_.emplace(document_id,
                DocumentData { ComputeAverageRating(ratings), status });
    }

    /**
     * @brief Ищет 5 документов с наибольшей релевантностью
     *
     * Ищет по поисковым словам и критерию, который определяется функцией
     * (функциональный объект, который поступает на вход)
     *
     * @tparam fnc_status Критерий поиска (функция)
     * @param raw_query   Поисковые слова (слова, которые ищем)
     * @return Результат поиска (вектор структур(id документа, релевантность, рейтинг))
     */
    template<typename FncStatus>
    vector<Document> FindTopDocuments(const string &raw_query,
            const FncStatus &fnc_status) const {

        // разбираем поисковый запрос
        const Query query = ParseQuery(raw_query);

        // ищем все документы, удовлетворяющие критериям поиска
        // (есть слова из поискового запроса и выполняется условия заданные функцией fnc_status)
        auto matched_documents = FindAllDocuments(query, fnc_status);

        // сортируем по убыванию релевантности
        sort(matched_documents.begin(), matched_documents.end(),
                [](const Document &lhs, const Document &rhs) {
                    if (abs(lhs.relevance - rhs.relevance)
                            < MIN_DELTA_RELEVANCE) {
                        // если релевантности очень близки по убыванию рейтинга
                        return lhs.rating > rhs.rating;
                    } else {
                        return lhs.relevance > rhs.relevance;
                    }
                });

        // оставляем только 5 документов
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    /**
     * @brief Ищем по поисковым словам и статусу документа
     *
     * @param raw_query Поисковые слова (слова, которые ищем)
     * @param status_f
     * @return
     */
    vector<Document> FindTopDocuments(const string &raw_query,
            DocumentStatus status_f = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query,
                [status_f](int document_id, DocumentStatus status, int rating) {
                    return status == status_f;
                });
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
        const Query query = ParseQuery(raw_query);
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
        return
        {   matched_words, documents_.at(document_id).status};
    }

private:
    /**
     * @brief Информация о документах
     *
     */
    struct DocumentData {
        int rating;    // ср.рейтинг (средний рейтинг слов, входящих в документ)
        DocumentStatus status; // статус документа
    };

    // стоп слова
    set<string> stop_words_;

    //
    map<string, map<int, double>> word_to_document_freqs_;

    // документы в поисковом сервере ({id документа, информация о документе (ср.рейтинг, статус)})
    map<int, DocumentData> documents_;

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
        int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    /**
     * @brief Слова поискового запроса (поисковые слова)
     *
     */
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return
        {   text, is_minus, IsStopWord(text)};
    }

    /**
     * @brief Парсим (разбираем) поисковый запрос
     *
     * @param text Строка поискового запроса
     * @return Структура (наборы слов поискового запроса)
     */
    Query ParseQuery(const string &text) const {
        Query query;
        // разбиваем строку поискового запроса на слова и исключаем стоп-слова
        for (const string &word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
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
     * @tparam query Слова поискового запроса
     * @param fnc_status Критерий поиска (функция)
     * @return Вектор документов (id документа, релевантность, ср.рейтинг)
     */
    template<typename FncStatus>
    vector<Document> FindAllDocuments(const Query &query,
            const FncStatus &fnc_status) const {

        map<int, double> document_to_relevance;  // релевантность документов
        for (const string &word : query.plus_words)
        // для каждого плюс-слова поискового запроса
        {
            if (word_to_document_freqs_.count(word) == 0)
            // если в документах поскового сервера нет данного плоюс-слова
                    {
                continue;
            }
            // Рассчитываем IDF
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(
                    word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(
                    word))
            // для всех документов, где есть данное плюс-слово
            {
                // загружаем параметры в функцию - критерий поиска
                const DocumentData &p_doc_data = documents_.at(document_id);

                if (fnc_status(document_id, p_doc_data.status,
                        p_doc_data.rating))
                        // если документ удовлетворяет критериям поиска
                        {
                    // рассчитываем релевантность (TF*IDF)
                    document_to_relevance[document_id] += term_freq
                            * inverse_document_freq;
                }
            }
        }

        // исключаем документы, в которых есть минус слова
        for (const string &word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                // если в документе есть минус-слово, удаляем его релевантность
                document_to_relevance.erase(document_id);
            }
        }

        // заполняем структуру результатов поиска
        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back( { document_id, relevance,
                    documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

/*
 Подставьте сюда вашу реализацию макросов
 ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
 */

template<typename T, typename U>
void AssertEqualImpl(const T &t, const U &u, const string &t_str,
        const string &u_str, const string &file, const string &func,
        unsigned line, const string &hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string &expr_str, const string &file,
        const string &func, unsigned line, const string &hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template<typename DataType>
void RunTestImpl(DataType value, const string &expr_str, const string &file,
        const string &func, unsigned line, const string &hint) {
    value();
    cerr << expr_str << " OK" << endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func, __FILE__, __FUNCTION__, __LINE__, ""s)

// -------- Начало модульных тестов поисковой системы ----------

// Добавление документов. Добавленный документ должен находиться по поисковому запросу,
// который содержит слова из документа.
void TestAddDocument() {
    const int doc_id = 42;
    const string content = "белый кот и модный ошейник"s;
    const vector<int> ratings = { 1, 2, 3 };

    // Сначала убеждаемся, что поиск слова, входящего в документ,
    // который мы хотим добавить, не находит нужный документ (возвращает пустой результат)
    SearchServer server;
    ASSERT_EQUAL(server.GetDocumentCount(), 0);
    ASSERT(server.FindTopDocuments("кот"s).empty());

    // Затем убеждаемся, что после добавления документа, поиск этого же слова
    // находит нужный документ
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT_EQUAL_HINT(server.GetDocumentCount(), 1,
            "Документ должен обавиться в поисковый сервис."s);
    const auto found_docs = server.FindTopDocuments("кот"s);
    ASSERT(found_docs.size() == 1);
    const Document &doc0 = found_docs[0];
    ASSERT_HINT(doc0.id == doc_id,
            "Добавленный документ должен находиться по поисковому запросу."s);
}

// Поддержка стоп-слов. Стоп-слова исключаются из текста документов.
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "белый кот и модный ошейник"s;
    const vector<int> ratings = { 1, 2, 3 };

    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("и"s);
        ASSERT(found_docs.size() == 1);
        const Document &doc0 = found_docs[0];
        ASSERT(doc0.id == doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("и в"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("и"s).empty(),
                "Стоп-слова должны быть исключены из документов."s);
    }
}

// Поддержка минус-слов. Документы, содержащие минус-слова из поискового запроса,
// не должны включаться в результаты поиска.
void TestMinusWords() {
    const int doc_id = 42;
    const string content = "белый кот и модный ошейник"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    // Сначала убеждаемся, что поиск слова находит нужный документ
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("кот"s);
    ASSERT(found_docs.size() == 1);
    const Document &doc0 = found_docs[0];
    ASSERT(doc0.id == doc_id);

    // Затем убеждаемся, что после добавления минус слова поиск этого же слова,
    // не находит документ
    ASSERT_HINT(server.FindTopDocuments("кот -ошейник"s).empty(),
            "Документы, содержащие минус-слова из поискового запроса,"s
                    + "не должны включаться в результаты поиска"s);
}

// Соответствие документов поисковому запросу.
// При этом должны быть возвращены все слова из поискового запроса, присутствующие в документе.
// Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestMatchingToSearchQuery() {
    const int doc_id = 42;
    const string content = "белый кот и модный чёрный ошейник"s;
    const vector<int> ratings = { 8, -3 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    // Сначала убеждаемся, что класс возвращает все слова из поискового запроса,
    // присутствующие в документе.
    {
        string raw_query = "кот модный пушистый"s;
        vector<string> query_words = { "кот"s, "модный"s };
        auto [words, _] = server.MatchDocument(raw_query, doc_id);
        size_t number_words = words.size();
        ASSERT_EQUAL(number_words, static_cast<size_t>(2));
        for (size_t i = 0; i < number_words; ++i) {
            ASSERT_EQUAL_HINT(words[i], query_words[i],
                    "Должны быть возвращены все слова из поискового запроса, "s
                            + "присутствующие в документе."s);
        }
    }

    // Если есть соответствие хотя бы по одному минус-слову,
    // должен возвращаться пустой список слов.
    {
        string raw_query = "кот -модный пушистый"s;
        auto [words, _] = server.MatchDocument(raw_query, doc_id);
        ASSERT_HINT(words.empty(),
                "Есть соответствие по оминус-слову - "s
                        + "должен возвращаться пустой список слов."s);
    }
}

// Вычисление рейтинга документов. Рейтинг добавленного документа равен
// среднему арифметическому оценок документа.
void TestRatingCalculation() {
    const int doc_id = 42;
    const string content = "белый кот и модный ошейник"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("кот"s);
    ASSERT(found_docs.size() == 1);
    const Document &doc0 = found_docs[0];
    ASSERT(doc0.id == doc_id);
    double average_rating = accumulate(ratings.begin(), ratings.end(), 0)
            / ratings.size();
    ASSERT_EQUAL_HINT(doc0.rating, average_rating,
            "Рейтинг добавленного документа должен быть равен"s
                    + "среднему арифметическому оценок документа."s);

}

// Сортировка найденных документов по релевантности.
// Возвращаемые при поиске документов результаты должны быть отсортированы
// в порядке убывания релевантности.
void TestSortingByRelevance() {
    const vector<int> doc_id = { 42, 57, 73, 123 };
    const vector<string> content = { "белый кот и модный чёрный ошейник"s,
            "пушистый кот пушистый хвост"s,
            "ухоженный пёс выразительные глаза"s, "ухоженный скворец евгений"s };
    const vector<vector<int>> ratings = { { 8, -3 }, { 7, 2, 7 },
            { 5, -12, 2, 1 }, { 9 } };
    const vector<DocumentStatus> statuses = { DocumentStatus::ACTUAL,
            DocumentStatus::ACTUAL, DocumentStatus::ACTUAL,
            DocumentStatus::BANNED };

    SearchServer server;
    server.SetStopWords("и в"s);
    for (size_t i = 0; i < doc_id.size(); ++i) {
        server.AddDocument(doc_id[i], content[i], statuses[i], ratings[i]);
    }
    auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    size_t size_found_docs = found_docs.size();

    ASSERT_EQUAL(size_found_docs, static_cast<size_t>(3));
    for (size_t i = 1; i < size_found_docs; ++i) {
        ASSERT_HINT(
                abs(found_docs[i].relevance - found_docs[i - 1].relevance)
                        > MIN_DELTA_RELEVANCE,
                "Результаты должны быть отсортированы в порядке убывания релевантности."s);
    }
}

// Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestSearchUserPredicate() {
    const vector<int> doc_id = { 42, 57, 73, 123 };
    const vector<string> content = { "белый кот и модный чёрный ошейник"s,
            "пушистый кот пушистый хвост"s,
            "ухоженный пёс выразительные глаза"s, "ухоженный скворец евгений"s };
    const vector<vector<int>> ratings = { { 8, -3 }, { 7, 2, 7 },
            { 5, -12, 2, 1 }, { 9 } };
    const vector<DocumentStatus> statuses = { DocumentStatus::ACTUAL,
            DocumentStatus::ACTUAL, DocumentStatus::IRRELEVANT,
            DocumentStatus::BANNED };

    SearchServer server;
    server.SetStopWords("и в"s);
    for (size_t i = 0; i < doc_id.size(); ++i) {
        server.AddDocument(doc_id[i], content[i], statuses[i], ratings[i]);
    }

    auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s,
            [](int document_id, DocumentStatus status, int rating) {
                return document_id % 2 == 0;
            });
    ASSERT_EQUAL_HINT(found_docs.size(), 1u,
            "Не правильно работает фильтрация результатов поиска с использованием "s
                    + "предиката, задаваемого пользователем."s);
    ASSERT_EQUAL_HINT(found_docs[0].id, 42,
            "Не правильно работает фильтрация результатов поиска с использованием "s
                    + "предиката, задаваемого пользователем."s);
    found_docs = server.FindTopDocuments("пушистый ухоженный пёс"s,
            [](int document_id, DocumentStatus status, int rating) {
                return status != DocumentStatus::BANNED;
            });
    ASSERT_EQUAL_HINT(found_docs.size(), 2u,
            "Не правильно работает фильтрация результатов поиска с использованием "s
                    + "предиката, задаваемого пользователем."s);
    ASSERT_EQUAL_HINT(found_docs[0].id, 57,
            "Не правильно работает фильтрация результатов поиска с использованием "s
                    + "предиката, задаваемого пользователем."s);
    ASSERT_EQUAL_HINT(found_docs[1].id, 73,
             "Не правильно работает фильтрация результатов поиска с использованием "s
                    + "предиката, задаваемого пользователем."s);
}

// Поиск документов, имеющих заданный статус.
void TestSearchByStatus() {
    const vector<int> doc_id = { 42, 57, 73, 123 };
    const vector<string> content = { "белый кот и модный чёрный ошейник"s,
            "пушистый кот пушистый хвост"s,
            "ухоженный пёс выразительные глаза"s, "ухоженный скворец евгений"s };
    const vector<vector<int>> ratings = { { 8, -3 }, { 7, 2, 7 },
            { 5, -12, 2, 1 }, { 9 } };
    const vector<DocumentStatus> statuses = { DocumentStatus::ACTUAL,
            DocumentStatus::ACTUAL, DocumentStatus::ACTUAL,
            DocumentStatus::BANNED };

    SearchServer server;
    server.SetStopWords("и в"s);
    for (size_t i = 0; i < doc_id.size(); ++i) {
        server.AddDocument(doc_id[i], content[i], statuses[i], ratings[i]);
    }

// поиск со статусом BANNED
    {
        const auto found_docs = server.FindTopDocuments(
                "пушистый ухоженный кот"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs.size(), static_cast<size_t>(1));
        const Document &doc0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc0.id, doc_id[3],
                "Система должна искать документы, имеющие заданный статус."s);
    }

// поиск со статусом ACTUAL
    {
        auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s,
                DocumentStatus::ACTUAL);
        size_t size_found_docs = found_docs.size();
        ASSERT_EQUAL(size_found_docs, 3u);
//        // сортируем по возрастанию id
//        sort(found_docs.begin(), found_docs.end(),
//                [](const Document &lhs, const Document &rhs) {
//                    return lhs.id < rhs.id;
//                });
//        for (size_t i = 0; i < size_found_docs; ++i) {
//            ASSERT_EQUAL_HINT(found_docs[i].id, doc_id[i],
//                    "Система должна искать документы, имеющие заданный статус."s);
//        }
        for (auto &doc : found_docs) {
            ASSERT_HINT(find(doc_id.begin(), doc_id.end(), doc.id) != doc_id.end()-1,
                    "Система должна искать документы, имеющие заданный статус."s);
        }
    }
}

// Корректное вычисление релевантности найденных документов.
void TestRelevanceCalculation() {
    const vector<int> doc_id = { 42, 57, 73, 123 };
    const vector<string> content = { "белый кот и модный чёрный ошейник"s,
            "пушистый кот пушистый хвост"s,
            "ухоженный пёс выразительные глаза"s, "ухоженный скворец евгений"s };
    const vector<vector<int>> ratings = { { 8, -3 }, { 7, 2, 7 },
            { 5, -12, 2, 1 }, { 9 } };

    SearchServer server;
    server.SetStopWords("и в"s);
    for (size_t i = 0; i < doc_id.size(); ++i) {
        server.AddDocument(doc_id[i], content[i], DocumentStatus::ACTUAL,
                ratings[i]);
    }

    const auto found_docs = server.FindTopDocuments("кот"s);
    vector<double> tf_idf;
    tf_idf = { (1.0 / 5.0) * log(4.0 / 2.0), (1.0 / 4.0) * log(4.0 / 2.0) };
    ASSERT(found_docs.size() == 2);
    ASSERT(found_docs[0].id == doc_id[1]);
    ASSERT(found_docs[1].id == doc_id[0]);
    ASSERT_HINT(abs(found_docs[0].relevance - tf_idf[1]) < MIN_DELTA_RELEVANCE,
            "Не корректно вычисляется релевантность найденных документов"s);
    ASSERT_HINT(abs(found_docs[1].relevance - tf_idf[0]) < MIN_DELTA_RELEVANCE,
            "Не корректно вычисляется релевантность найденных документов"s);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchingToSearchQuery);
    RUN_TEST(TestSortingByRelevance);
    RUN_TEST(TestRatingCalculation);
    RUN_TEST(TestSearchUserPredicate);
    RUN_TEST(TestSearchByStatus);
    RUN_TEST(TestRelevanceCalculation);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
// Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
