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

/**
 * @brief Получает строку от пользователя
 *
 * @return Строка, введенная пользователем
 */
string
ReadLine ()
{
  string s;
  getline (cin, s);
  return s;
}

/**
 * @brief Разбирает строку на слова
 *
 * @param text Строка
 * @return Вектор слов
 */
vector<string>
SplitIntoWords (const string &text)
{
  vector<string> words;
  string word;
  for (const char c : text)
    {
      if (c == ' ')
        {
          if (!word.empty ())
            {
              words.push_back (word);
              word.clear ();
            }
        }
      else
        {
          word += c;
        }
    }
  if (!word.empty ())
    {
      words.push_back (word);
    }

  return words;
}


/**
 * @brief Результат поиска
 *
 */
struct Document
{
  int id;           // id документа
  double relevance; // релевантность
  int rating;       // рейтинг
};

/**
 * @brief Cтатус документа в поисковом сервере (класс перечисления)
 *
 */
enum class DocumentStatus
{
  ACTUAL,      // действительные
  IRRELEVANT,  // не подходящие
  BANNED,      // запрещённые
  REMOVED,     // удалённые
};


/**
 * @brief Поисковый сервер
 *
 */
class SearchServer
{
public:
  /**
   * @brief Задает стоп-слова
   *
   *  Стоп-слова - слова исключаемые из документов и из поискового запроса при поиске
   *
   * @param text Строка текста
   */
  void
  SetStopWords (const string &text)
  {
    for (const string &word : SplitIntoWords (text))
      {
        stop_words_.insert (word);
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
  void
  AddDocument (int document_id, const string &document, DocumentStatus status,
               const vector<int> &ratings)
  {
    // разбирает текст документа на слова и исключаем стоп слова
    const vector<string> words = SplitIntoWordsNoStop (document);
    // вес слова для подсчёта TF
    const double inv_word_count = 1.0 / words.size ();
    for (const string &word : words)
      {
        // TF слов
        word_to_document_freqs_[word][document_id] += inv_word_count;
      }
    // заносим в поисковый сервер
    documents_.emplace (document_id, DocumentData
      { ComputeAverageRating (ratings), status });
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
    vector<Document>
    FindTopDocuments (const string &raw_query,
                      const FncStatus &fnc_status) const
    {

      // разбираем поисковый запрос
      const Query query = ParseQuery (raw_query);

      // ищем все документы, удовлетворяющие критериям поиска
      // (есть слова из поискового запроса и выполняется условия заданные функцией fnc_status)
      auto matched_documents = FindAllDocuments (query, fnc_status);

      // сортируем по убыванию релевантности
      sort (matched_documents.begin (), matched_documents.end (), []
      (const Document &lhs, const Document &rhs)
        {
          if (abs(lhs.relevance - rhs.relevance) < 1e-6)
            {
              // если релевантности очень близки по убыванию рейтинга
              return lhs.rating > rhs.rating;
            }
          else
            {
              return lhs.relevance > rhs.relevance;
            }
        });

      // оставляем только 5 документов
      if (matched_documents.size () > MAX_RESULT_DOCUMENT_COUNT)
        {
          matched_documents.resize (MAX_RESULT_DOCUMENT_COUNT);
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
  vector<Document>
  FindTopDocuments (const string &raw_query, DocumentStatus status_f = DocumentStatus::ACTUAL) const
  {
    return FindTopDocuments (raw_query, [status_f]
    (int document_id, DocumentStatus status, int rating)
      {
        return status == status_f;
      });
  }

  /**
   * @brief Получает количество документов в поисковом сервере
   *
   * @return Количество документов в поисковом сервере
   */
  int
  GetDocumentCount () const
  {
    return documents_.size ();
  }

  tuple<vector<string>, DocumentStatus>
  MatchDocument (const string &raw_query, int document_id) const
  {
    const Query query = ParseQuery (raw_query);
    vector<string> matched_words;
    for (const string &word : query.plus_words)
      {
        if (word_to_document_freqs_.count (word) == 0)
          {
            continue;
          }
        if (word_to_document_freqs_.at (word).count (document_id))
          {
            matched_words.push_back (word);
          }
      }
    for (const string &word : query.minus_words)
      {
        if (word_to_document_freqs_.count (word) == 0)
          {
            continue;
          }
        if (word_to_document_freqs_.at (word).count (document_id))
          {
            matched_words.clear ();
            break;
          }
      }
    return
      { matched_words, documents_.at(document_id).status};
  }

private:
  /**
   * @brief Информация о документах
   *
   */
  struct DocumentData
  {
    int rating;            // ср.рейтинг (средний рейтинг слов, входящих в документ)
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
  bool
  IsStopWord (const string &word) const
  {
    return stop_words_.count (word) > 0;
  }

  /**
   * @brief Разбивает строку на слова и исключает стоп-слова
   *
   * @param text Строка, которую разбираем
   * @return Вектор слов, входящих в троку исключая стоп-слова
   */
  vector<string>
  SplitIntoWordsNoStop (const string &text) const
  {
    vector<string> words;
    for (const string &word : SplitIntoWords (text))
      {
        if (!IsStopWord (word))
          {
            words.push_back (word);
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
   */static int
  ComputeAverageRating (const vector<int> &ratings)
  {
    if (ratings.empty ())
      {
        return 0;
      }
    int rating_sum = 0;
    for (const int rating : ratings)
      {
        rating_sum += rating;
      }
    return rating_sum / static_cast<int> (ratings.size ());
  }

  /**
   * @brief Слова поискового запроса (поисковые слова)
   *
   */struct Query
  {
    set<string> plus_words;
    set<string> minus_words;
  };

  /**
   * @brief Парсим (разбираем) поисковый запрос
   *
   * @param text Строка поискового запроса
   * @return Структура (наборы слов поискового запроса)
   */
  Query
  ParseQuery (const string &text) const
  {
    Query query;
    // разбиваем строку поискового запроса на слова и исключаем стоп-слова
    for (const string &word : SplitIntoWordsNoStop (text))
      {
        if (word[0] == '-')
          {
            // отрезаем "-" и добавляем в множество минус слов
            query.minus_words.insert (word.substr (1));
          }
        else
          {
            query.plus_words.insert (word); // добавляем в множество плюс слов
          }
      }
    return query;
  }

  /**
   * @brief Расчитываем IDF (inverse document frequency) слова
   *
   * @param word Слово
   * @return IDF
   */double
  ComputeWordInverseDocumentFreq (const string &word) const
  {
    return log (
        GetDocumentCount () * 1.0 / word_to_document_freqs_.at (word).size ());
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
    vector<Document>
    FindAllDocuments (const Query &query, const FncStatus &fnc_status) const
    {

      map<int, double> document_to_relevance;  // релевантность документов
      for (const string &word : query.plus_words)
        // для каждого плюс-слова поискового запроса
        {
          if (word_to_document_freqs_.count (word) == 0)
            // если в документах поскового сервера нет данного плоюс-слова
            {
              continue;
            }
          // Рассчитываем IDF
          const double inverse_document_freq = ComputeWordInverseDocumentFreq (word);
          for (const auto [document_id, term_freq] : word_to_document_freqs_.at (word))
            // для всех документов, где есть данное плюс-слово
            {
              // загружаем параметры в функцию - критерий поиска
              if (fnc_status (document_id, documents_.at (document_id).status,
                              documents_.at (document_id).rating))
                // если документ удовлетворяет критериям поиска
                {
                  // рассчитываем релевантность (TF*IDF)
                  document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

      // исключаем документы, в которых есть минус слова
      for (const string &word : query.minus_words)
        {
          if (word_to_document_freqs_.count (word) == 0)
            {
              continue;
            }
          for (const auto [document_id, _] : word_to_document_freqs_.at (word))
            {
              // если в документе есть минус-слово, удаляем его релевантность
              document_to_relevance.erase (document_id);
            }
        }

      // заполняем структуру результатов поиска
      vector<Document> matched_documents;
      for (const auto [document_id, relevance] : document_to_relevance)
        {
          matched_documents.push_back (
            { document_id, relevance, documents_.at (document_id).rating });
        }
      return matched_documents;
    }
};

// ==================== для примера =========================

void
PrintDocument (const Document &document)
{
  cout << "{ "s << "document_id = "s << document.id << ", "s << "relevance = "s
      << document.relevance << ", "s << "rating = "s << document.rating << " }"s
      << endl;
}
int
main ()
{
  SearchServer search_server;
  search_server.SetStopWords ("и в на"s);
  search_server.AddDocument (0, "белый кот и модный ошейник"s,
                             DocumentStatus::ACTUAL,
                               { 8, -3 });
  search_server.AddDocument (1, "пушистый кот пушистый хвост"s,
                             DocumentStatus::ACTUAL,
                               { 7, 2, 7 });
  search_server.AddDocument (2, "ухоженный пёс выразительные глаза"s,
                             DocumentStatus::ACTUAL,
                               { 5, -12, 2, 1 });
  search_server.AddDocument (3, "ухоженный скворец евгений"s,
                             DocumentStatus::BANNED,
                               { 9 });
  cout << "ACTUAL by default:"s << endl;
  for (const Document &document : search_server.FindTopDocuments (
      "пушистый ухоженный кот"s))
    {
      PrintDocument (document);
    }
  cout << "BANNED:"s << endl;
  for (const Document &document : search_server.FindTopDocuments (
      "пушистый ухоженный кот"s, DocumentStatus::BANNED))
    {
      PrintDocument (document);
    }
  cout << "Even ids:"s << endl;
  for (const Document &document : search_server.FindTopDocuments (
      "пушистый ухоженный кот"s, []
      (int document_id, DocumentStatus status, int rating)
        { return document_id % 2 == 0;}))
    {
      PrintDocument (document);
    }
  return 0;
}