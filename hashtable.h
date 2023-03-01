#pragma once
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <vector>

template <class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class RobinHoodHashTable {
public:
    using Node = std::pair<const KeyType, ValueType>;

    explicit RobinHoodHashTable(Hash hasher = Hash()) : hasher_(hasher), data_(1, nullptr), psl_(1, EMPTY), size_(0) {
    }
    RobinHoodHashTable(const RobinHoodHashTable &other)
        : hasher_(other.hasher_), data_(other.data_.size(), nullptr), psl_(other.psl_), size_(other.size_) {
        for (size_t i = 0; i < data_.size(); ++i) {
            if (other.data_[i] == nullptr) {
                data_[i] = nullptr;
            } else {
                data_[i] = new Node(*other.data_[i]);
            }
        }
    }
    RobinHoodHashTable(RobinHoodHashTable &&other)
        : hasher_(other.hasher_), data_(other.data_), psl_(other.psl_), size_(other.size_) {
        other.data_.clear();
    }
    RobinHoodHashTable &operator=(const RobinHoodHashTable &other) {
        if (&other == this) {
            return *this;
        }
        clear();
        hasher_ = other.hasher_;
        psl_ = other.psl_;
        size_ = other.size_;
        data_.resize(other.data_.size());
        for (size_t i = 0; i < data_.size(); ++i) {
            if (other.data_[i] == nullptr) {
                data_[i] = nullptr;
            } else {
                data_[i] = new Node(*other.data_[i]);
            }
        }
        return *this;
    }
    RobinHoodHashTable &operator=(RobinHoodHashTable &&other) {
        hasher_ = other.hasher_;
        psl_ = other.psl_;
        size_ = other.size_;
        data_ = other.data_;
        other.data_.clear();
        return *this;
    }

    ~RobinHoodHashTable() {
        clear();
    }

    template <typename Iterator>
    RobinHoodHashTable(Iterator begin, Iterator end, Hash hasher = Hash())
        : hasher_(hasher), data_(1, nullptr), psl_(1, EMPTY), size_(0) {
        for (auto it = begin; it != end; ++it) {
            insert(*it);
        }
    }
    RobinHoodHashTable(std::initializer_list<std::pair<KeyType, ValueType>> list, Hash hasher = Hash())
        : hasher_(hasher), data_(list.size() + 1, nullptr), psl_(list.size() + 1, EMPTY), size_(0) {
        for (const Node node : list) {
            insert(node);
        }
    }

    size_t size() const {
        return size_;
    }
    bool empty() const {
        return size_ == 0;
    }
    Hash hash_function() const {
        return hasher_;
    }

    void insert(std::pair<KeyType, ValueType> node) {
        insert(new Node(node));
    }

    void erase(const KeyType &key) {
        size_t index = hasher_(key) % data_.size();
        size_t current_psl = 0;
        while (psl_[index] != EMPTY && psl_[index] >= current_psl) {
            if (psl_[index] == current_psl) {
                if (data_[index]->first == key) {
                    delete data_[index];
                    data_[index] = nullptr;
                    psl_[index] = EMPTY;
                    --size_;
                    size_t next_index = index + 1;
                    if (next_index == data_.size()) {
                        next_index = 0;
                    }
                    while (psl_[next_index] != EMPTY && psl_[next_index] != 0) {
                        psl_[index] = psl_[next_index] - 1;
                        data_[index] = data_[next_index];
                        index = next_index;
                        next_index = next_index + 1;
                        if (next_index == data_.size()) {
                            next_index = 0;
                        }
                    }
                    psl_[index] = EMPTY;
                    data_[index] = nullptr;
                    return;
                }
            }
            ++index;
            if (index == data_.size()) {
                index = 0;
            }
            ++current_psl;
        }
    }

    template <bool isConst>
    class Iterator {
    public:
        using DataType = std::conditional_t<isConst, typename std::vector<Node *>::const_iterator,
                                            typename std::vector<Node *>::iterator>;
        using PslType = std::conditional_t<isConst, typename std::vector<size_t>::const_iterator,
                                           typename std::vector<size_t>::iterator>;

        Iterator() = default;
        Iterator(const Iterator &) = default;
        Iterator(Iterator &&) = default;
        Iterator &operator=(const Iterator &) = default;
        Iterator &operator=(Iterator &&) = default;

        bool operator==(const Iterator &other) const {
            return data_iterator_ == other.data_iterator_;
        }
        bool operator!=(const Iterator &other) const {
            return !operator==(other);
        }

        Iterator(DataType data_iterator, PslType psl_iterator, PslType psl_end_iterator)
            : data_iterator_(data_iterator), psl_iterator_(psl_iterator), psl_end_iterator_(psl_end_iterator) {
            while (psl_iterator_ != psl_end_iterator_ && *psl_iterator_ == EMPTY) {
                ++psl_iterator_;
                ++data_iterator_;
            }
        }

        Iterator &operator++() {
            do {
                ++data_iterator_;
                ++psl_iterator_;
            } while (psl_iterator_ != psl_end_iterator_ && *psl_iterator_ == EMPTY);
            return *this;
        }
        Iterator operator++(int) {
            Iterator copy(*this);
            ++*this;
            return copy;
        }
        std::conditional_t<isConst, const Node &, Node &> operator*() {
            return **data_iterator_;
        }
        std::conditional_t<isConst, const Node *, Node *> operator->() {
            return *data_iterator_;
        }

    private:
        DataType data_iterator_;
        PslType psl_iterator_;
        PslType psl_end_iterator_;
    };

    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    iterator begin() {
        return iterator(data_.begin(), psl_.begin(), psl_.end());
    }

    iterator end() {
        return iterator(data_.end(), psl_.end(), psl_.end());
    }

    const_iterator begin() const {
        return const_iterator(data_.begin(), psl_.begin(), psl_.end());
    }

    const_iterator end() const {
        return const_iterator(data_.end(), psl_.end(), psl_.end());
    }

    iterator find(const KeyType &key) {
        size_t index = hasher_(key) % data_.size();
        size_t current_psl = 0;
        while (psl_[index] != EMPTY && psl_[index] >= current_psl) {
            if (psl_[index] == current_psl) {
                if (data_[index]->first == key) {
                    return iterator(data_.begin() + index, psl_.begin() + index, psl_.end());
                }
            }
            ++index;
            if (index == data_.size()) {
                index = 0;
            }
            ++current_psl;
        }
        return end();
    }

    const_iterator find(const KeyType &key) const {
        size_t index = hasher_(key) % data_.size();
        size_t current_psl = 0;
        while (psl_[index] != EMPTY && psl_[index] >= current_psl) {
            if (psl_[index] == current_psl) {
                if (data_[index]->first == key) {
                    return const_iterator(data_.begin() + index, psl_.begin() + index, psl_.end());
                }
            }
            ++index;
            if (index == data_.size()) {
                index = 0;
            }
            ++current_psl;
        }
        return end();
    }

    ValueType &operator[](const KeyType &key) {
        auto it = find(key);
        if (it == end()) {
            insert({key, ValueType()});
            return find(key)->second;
        } else {
            return it->second;
        }
    }

    const ValueType &at(const KeyType &key) const {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("Given key is not in table");
        }
        return it->second;
    }

    void clear() {
        for (auto &elem : data_) {
            if (elem != nullptr) {
                delete elem;
            }
            elem = nullptr;
        }
        psl_.assign(data_.size(), EMPTY);
        size_ = 0;
    }

private:
    void insert(Node *node) {
        size_t index = hasher_(node->first) % data_.size();
        size_t current_psl = 0;
        ++size_;
        Node *current_node = node;
        while (psl_[index] != EMPTY) {
            if (psl_[index] < current_psl) {
                std::swap(current_psl, psl_[index]);
                std::swap(current_node, data_[index]);
            } else if (psl_[index] == current_psl) {
                if (data_[index] -> first == current_node -> first) {
                    --size_;
                    delete current_node;
                    return;
                }
            }
            ++index;
            if (index == data_.size()) {
                index = 0;
            }
            ++current_psl;
        }
        psl_[index] = current_psl;
        data_[index] = current_node;
        if (data_.size() * MAX_LOAD_FACTOR_ < size_) {
            rebuild();
        }
    }
    void rebuild() {
        std::vector<Node *> data_clone = data_;
        std::vector<size_t> psl_clone = psl_;
        data_.assign(data_.size(), nullptr);
        while (data_.size() * MAX_LOAD_FACTOR_ < size_) {
            data_.assign(2 * data_.size(), nullptr);
        }
        size_ = 0;
        psl_.assign(data_.size(), EMPTY);
        for (size_t i = 0; i < data_clone.size(); ++i) {
            if (psl_clone[i] != EMPTY) {
                insert(data_clone[i]);
            }
        }
    }
    Hash hasher_;
    std::vector<Node *> data_;
    std::vector<size_t> psl_;
    size_t size_;
    static const size_t EMPTY = -1;
    constexpr static const double MAX_LOAD_FACTOR_ = 0.6;
};

template <typename KeyType, typename ValueType, typename Hash = std::hash<KeyType>>
using HashMap = RobinHoodHashTable<KeyType, ValueType, Hash>;

template <typename KeyType, typename ValueType, typename Hash>
const size_t RobinHoodHashTable<KeyType, ValueType, Hash>::EMPTY;
template <typename KeyType, typename ValueType, typename Hash>
const double RobinHoodHashTable<KeyType, ValueType, Hash>::MAX_LOAD_FACTOR_;
