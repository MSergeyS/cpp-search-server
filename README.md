# __SearchServer__
### __*Финальный проект: поисковый сервер*__
  *SearchServer - система поиска документов по ключевым словам.*

  *Проект курса Яндекс Практикума "Разработчик С++"*

---
## Основные функции:
- ранжирование результатов поиска по статистической мере __*TF-IDF*__;
- обработка __*стоп-слов*__ (не учитываются поисковой системой и не влияют на результаты поиска);
- обработка __*минус-слов*__ (документы, содержащие минус-слова, не будут включены в результаты поиска);
- создание и обработка очереди запросов;
- удаление дубликатов документов;
- постраничное разделение результатов поиска;
- возможность работы в многопоточном режиме;

## Принцип работы
Создание экземпляра класса ```SearchServer```. В конструктор передаётся строка с стоп-словами, разделенными пробелами. Вместо строки можно передавать произвольный контейнер (с последовательным доступом к элементам с возможностью использования в ```for-range``` цикле)

С помощью метода ```AddDocument``` добавляются документы для поиска. В метод передаётся id документа, статус, рейтинг, и сам документ в формате строки.

Метод ```FindTopDocuments``` возвращает вектор документов, согласно соответствию переданным ключевым словам. Результаты отсортированы по статистической мере TF-IDF. Возможна дополнительная фильтрация документов по id, статусу и рейтингу. Метод реализован как в однопоточной так и в многопоточной версии.

Класс ```RequestQueue``` реализует очередь запросов к поисковому серверу с сохранением результатов поиска.

## Архитектура проекта
Инициализация поисковой системы происходит при добавлении контейнера со стоп-словами, разделенными пробелами. В архитектуре представлены следующие модули:
1. В __```search_server```__ расположена базовая логика системы и её сущности. С помощью метода ```AddDocument``` в базу системы добавляются документы, после чего происходит их обработка: проверка номера документа и его слов на валидность, разбивка строк на отдельные слова с исключением стоп-слов, вычисление среднего рейтинга и занесение слов в индекс. Также здесь сосредоточены методы по парсингу поискового запроса, определению степени соответствия документов в базе поисковому запросу (матчингу) и выдаче топ-5 наиболее релевантных документов.
2. __```read_input_functions```__ считывает текстовые запросы из потока ввода.
3. В __```string_processing```__ происходит разбиение строки на слова. Здесь стоит упомянуть, что в систему внедрён введённый в стандарте C++17 тип ```std::string_view```, позволяющий более экономично передавать неизменную строку в другой участок кода.
4. __```document хранит```__ в себе структуру документа, а также метод его вывода в поток.
5. __```paginator```__ позволяет разбить поисковую выдачу на страницы.
6. В __```request_queue```__ сосредоточена логика обработки очереди из запросов.
7. __```process_queries```__ делегирует обработку запросов нескольким потокам процессора.
8. __```concurrent_map```__ реализует многопоточность при использовании контейнера STL ```std::map```: словарь разбивается на несколько подсловарей с непересекающимся набором ключей, каждый из которых защищён отдельным мьютексом. Тогда при обращении разных потоков к разным ключам они нечасто будут попадать в один и тот же подсловарь, а значит, смогут параллельно его обрабатывать.
9. __```test_example_functions```__ содержит юнит-тесты.

## Сборка и установка
Сборка с помощью любой IDE либо сборка из командной строки

## Системные требования
Компилятор С++ с поддержкой стандарта C++17 и выше
