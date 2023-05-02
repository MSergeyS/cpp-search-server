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
    vector<int> duplicates_id;  // id дубликатов, которые следует удалить
    set<set<string_view>> set_words; // набор наборов слов документов
    for (const int document_id : search_server) {
        // слова в документе document_id
        map<string_view, double> word_freq = search_server.GetWordFrequencies(
                document_id);
        // собираем эти слова в set
        set<string_view> words;
        for (const auto &pair : word_freq) {
            words.insert(pair.first);
        }

        if (set_words.count(words)) {
            // уже есть набор из таких слов
            duplicates_id.push_back(document_id);
        } else {
            // добавляем набор в набор наборов)
            set_words.emplace(words);
        }
    }

    // удаляем дубликаты
    for (const int id : duplicates_id) {
        cout << "Found duplicate document id "s << id << endl;
        search_server.RemoveDocument(id);
    }
}
