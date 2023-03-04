#include <iostream>
#include <map>
#include <set>
#include <string>

#include "remove_duplicates.h"
#include "search_server.h"


/**
 * @brief Функцию поиска и удаления дубликатов
 *
 *  Дубликатами считаются документы, у которых наборы встречающихся слов совпадают.
 *  Совпадение частот необязательно. Порядок слов неважен, а стоп-слова игнорируются.
 *  Функция использует только доступные к этому моменту методы поискового сервера.
 *  При обнаружении дублирующихся документов функция удаляет документ с большим id
 *  из поискового сервера, и при этом сообщает id удалённого документа в соответствии
 *  с форматом:
 *    "Found duplicate document id N"
 *    N - id удаляемого документа.
 *
 * @param search_server ссылка на поисковый сервер
 */
void RemoveDuplicates(SearchServer &search_server) {
	vector<int> duplicates_id;
	map<set<string>, int> words_id;
	for (const int document_id : search_server) {
		map<string, double> word_freq = search_server.GetWordFrequencies(
				document_id);
		set<string> words;
		for (const auto &pair : word_freq) {
			words.insert(pair.first);
		}
		if (!words_id.empty()) {
			if (words_id.count(words)) {
				duplicates_id.push_back(document_id);
			} else {
				words_id[words] = document_id;
			}
		} else {
			words_id[words] = document_id;
		}
	}

	for (const int id : duplicates_id) {
		cout << "Found duplicate document id "s << id << endl;
		search_server.RemoveDocument(id);
	}
}
