#pragma once

using namespace std;

template<typename Key, typename Value>
class ConcurrentMap {

private:
    struct Bucket {
        mutex submap_mutex_;
        map<Key, Value> submap_;
    };

    vector<Bucket> buckets_;

public:
    // не даст программе скомпилироваться при попытке использовать в качестве
    // типа ключа что-либо, кроме целых чисел
    static_assert(is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    // Структура Access должна вести себя так же, как в шаблоне Synchronized:
    // предоставлять ссылку на значение словаря и обеспечивать синхронизацию доступа к нему.
    struct Access {
        lock_guard<mutex> guard;
        Value &ref_to_value;

        Access(const Key &key, Bucket &bucket) :
                guard(bucket.submap_mutex_), ref_to_value(bucket.submap_[key]) {
        }
    };

    // Конструктор класса ConcurrentMap<Key, Value> принимает количество подсловарей,
    // на которые надо разбить всё пространство ключей.
    explicit ConcurrentMap(size_t bucket_count) :
            buckets_(bucket_count) {
    }

    // оператор [] ведёт себя так же, как аналогичный оператор у map:
    //    - если ключ key есть в словаре, должен возвращаться объект класса Access,
    //      содержащий ссылку на соответствующее ему значение.
    //    - если key в словаре нет, в него надо добавить пару (key, Value()) и
    //      вернуть объект класса Access, содержащий ссылку на только что добавленное значение.
    Access operator[](const Key &key) {
        // Ключи — целые числа. Операция взятия остатка от деления поможет выбрать, в подсловаре
        // с каким индексом хранить данный ключ.
        // Чтобы не мучиться с отрицательными числами, предварительно приведём ключ к типу uint64_t.
        auto &bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return {key, bucket};
    }

    // оператор erase
    void erase(const Key &key) {
        lock_guard < mutex
                > guard(
                        buckets_[static_cast<uint64_t>(key) % buckets_.size()].submap_mutex_);
        buckets_[static_cast<uint64_t>(key) % buckets_.size()].submap_.erase(
                key);
    }

    // Метод BuildOrdinaryMap сливает вместе части словаря и возвращать весь словарь целиком.
    // При этом он потокобезопасным, то есть корректно работает, когда другие потоки
    // выполняют операции с ConcurrentMap.
    map<Key, Value> BuildOrdinaryMap() {
        map<Key, Value> result;
        for (auto& [submap_mutex, submap] : buckets_) {
            lock_guard guard(submap_mutex);
            result.insert(submap.begin(), submap.end());
        }
        return result;
    }
};
