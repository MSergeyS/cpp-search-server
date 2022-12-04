#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
	// получения входной строки () от пользователя
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
	// разираем строку на слова (вектор слов)
    vector<string> words;
    string word;
    for (const char c : text) {
		// перебираем строку по символам
        if (c == ' ') {
			// если символ "пробел" - конец слова
            if (!word.empty()) {
				// добавляем слово в вектор
                words.push_back(word);
                word.clear();
            }
        } else {
			// добавляем символ в слово
            word += c;
        }
    }
    if (!word.empty()) {
		// добавляем в вектор последнее слово в строке
        words.push_back(word);
    }

    return words;
}

struct Document {
	// результат поиска
    int id;				// id документа
    double relevance;   // релевантность документа
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
		// 
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
		// добавляем документ в поисковой сервер
		
		// разбираем документ на слова и исключаем стоп-слова
        const vector<string> words = SplitIntoWordsNoStop(document);
		
		// для каждого слова документа
		for (const string& word : words) {
			// считаем TF(term frequency) слова
            double word_tf = double(count(words.begin(), words.end(), word)) / 
			                 double(words.size());
			// добавляем пару {id, tf} слова в контейнер слов поискового сервера				 
			documents_[word].insert({document_id, word_tf});	
		}
		// увеличиваем количество документов в поисковом сервере на 1
        document_count_ += 1;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
		// поиск документов с наибольшей релевантностью
		
		// разбираем поисковый запрос
        const Query query = ParseQuery(raw_query);
        
		// ищем во всех документах
        auto matched_documents = FindAllDocuments(query);

		// сортируем по убыванию релевантности
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
			 
		// оставляем только MAX_RESULT_DOCUMENT_COUNT документов с наибольшей релевантностью
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
	
	// поисковый запрос
	struct Query {
        set<string> plus_words;   // плюс слова (документы включаем в результат поиска)
        set<string> minus_words;  // минус слова (документы исключаем из результата поиска)
    };
	
	// база слов документов в поисковом сервера 
	// ( словарь:
	//   слово (ключ) -> словарь[ id документа (ключ) -> TF слова в документе]
    map<string, map<int, double>> documents_;

	// набор стоп-слов - по этим словам не ищем
    // (исключаются из документов поискового сервера и из поискового запроса)
    set<string> stop_words_;
    
	// счётчик документов в поисковом сервере
    int document_count_ = 0;

	
    bool IsStopWord(const string& word) const {
		// компаратор стоп-слов ( (слово == стоп-слово) -> true )
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
		// разбираем строку на слова и исключаем стоп-слова
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
    
    Query ParseQuery(const string& text) const {
		// разбираем поисковый запрос 
        Query query;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
				// отрезаем "-" и добавляем в множество минус слов
                query.minus_words.insert(word.substr(1));
            }
            else {
                query.plus_words.insert(word); // добавляем в множество плюс слов
            }
        }
        return query;
    }

    vector<Document> FindAllDocuments(const Query query) const {
		// поиск по всем документам поискового сервера
		
		vector<Document> matched_documents;
		
		// если поисковый запрос пустой - ничего не ищем (выходим)
		if (query.plus_words.empty()) {
			return matched_documents;
		}
		
		map<int, double> relevance;
		
		// для всех плюс-слов поискового запроса 
		for (const string& plus_word : query.plus_words) {
			// для документов, в которых есть плюс слова поискового запроса считаем релевантность
            if (documents_.count(plus_word) != 0) {
				for (auto [id, tf] : documents_.at(plus_word)) {
					// релевантность = сумма(TF * IDF) слов документа, которые есть в посиковом запросе
					// IDF (inverse document frequency)
                    relevance[id] += tf * log(double(document_count_) / 
					                 double(documents_.at(plus_word).size()));
			    }
			}
		}
		
		// исключаем документы, в которых есть минус слова
        for (const string& minus_word : query.minus_words) {
            if (documents_.count(minus_word) != 0) {
				for (auto [id, tf] : documents_.at(minus_word)) {
				    relevance.erase(id);
				}
			}
		}
		
		// заполняем структуру результатов поиска
        for (auto [id, rel] : relevance) {
            matched_documents.push_back({id, rel});
        }
		
        return matched_documents;
    }

};

SearchServer CreateSearchServer() {
	// создаем поисковый сервер
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());   // задаем стоп-слова

	// задаем количество документов в поисковом сервере
    const int document_count = ReadLineWithNumber();
	// заносим документы в поисковый сервер
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
	// создаем поисковый сервер
    const SearchServer search_server = CreateSearchServer();
    
	// задаем поисковый запроос
    const string query = ReadLine();
    
	// ищем и выводим результаты поиска
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}