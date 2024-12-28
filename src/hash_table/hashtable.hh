#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <class Key, class Value> class HashTable {

public:
  void insert(const Key &key, Value value) {
    if (this->contains(key))
      return;
    auto &bucket = buckets.at(hash(key));
    auto v = std::make_pair(key, std::move(value));

    std::visit(
        overloaded{[&](empty_t &) { bucket = std::move(v); },
                   [&](value_t &existing) {
                     collision_t vec;
                     vec.reserve(2);
                     vec.push_back(std::move(existing));
                     vec.push_back(std::move(v));
                     bucket = std::move(vec);
                   },
                   [&](collision_t &vec) { vec.push_back(std::move(v)); }},
        bucket);
  }

  auto get(const Key &key) const -> std::optional<Value> {
    const auto &bucket = buckets.at(hash(key));

    return std::visit(
        overloaded{[](const empty_t &) -> std::optional<Value> {
                     return std::nullopt;
                   },
                   [&](const value_t &val) -> std::optional<Value> {
                     return val.first == key ? std::make_optional(val.second)
                                             : std::nullopt;
                   },
                   [&](const collision_t &vec) -> std::optional<Value> {
                     auto it = std::find_if(
                         vec.begin(), vec.end(),
                         [&](const value_t &kvp) { return kvp.first == key; });
                     return it != vec.end() ? std::make_optional(it->second)
                                            : std::nullopt;
                   }},
        bucket);
  }

  auto contains(const Key &key) -> bool { return get(key).has_value(); }

  auto erase(const Key &key) -> void {
    auto &bucket = buckets.at(hash(key));

    std::visit(
        overloaded{[&](empty_t &) { /* Nothing to do if bucket is empty */ },
                   [&](value_t &val) {
                     if (val.first == key) {
                       bucket = empty_t{}; // Empty the bucket if key matches
                     }
                   },
                   [&](collision_t &vec) {
                     auto it = std::remove_if(
                         vec.begin(), vec.end(),
                         [&](const value_t &kvp) { return kvp.first == key; });
                     if (it != vec.end()) {
                       vec.erase(it, vec.end()); // Remove the key-value pair
                       if (vec.empty()) {
                         bucket = empty_t{}; // If the collision list is now
                                             // empty, reset bucket
                       }
                     }
                   }},
        bucket);
  }

  HashTable() = default;
  ~HashTable() = default;

public:
  using empty_t = std::monostate;
  using value_t = std::pair<Key, Value>;
  using collision_t = std::vector<value_t>;
  using bucket_t = std::variant<empty_t, value_t, collision_t>;

private:
  std::array<bucket_t, 10> buckets{};
  std::size_t hash(const Key &key) const {
    return std::hash<Key>{}(key) % buckets.size();
  }

public:
  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = value_t;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    Iterator(HashTable *table, size_t bucket_idx, size_t collision_idx = 0)
        : table_(table), bucket_idx_(bucket_idx),
          collision_idx_(collision_idx) {
      if (bucket_idx_ < table_->buckets.size()) {
        advance_to_valid();
      }
    }

    value_t &operator*() {
      auto &bucket = table_->buckets[bucket_idx_];
      if (std::holds_alternative<value_t>(bucket)) {
        return std::get<value_t>(bucket);
      } else {
        return std::get<collision_t>(bucket)[collision_idx_];
      }
    }

    Iterator &operator++() {
      advance();
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      advance();
      return tmp;
    }

    bool operator==(const Iterator &other) const {
      return bucket_idx_ == other.bucket_idx_ &&
             collision_idx_ == other.collision_idx_ && table_ == other.table_;
    }

    bool operator!=(const Iterator &other) const { return !(*this == other); }

  private:
    HashTable *table_;
    size_t bucket_idx_;
    size_t collision_idx_;

    void advance() {
      auto &bucket = table_->buckets[bucket_idx_];
      if (std::holds_alternative<collision_t>(bucket)) {
        if (collision_idx_ + 1 < std::get<collision_t>(bucket).size()) {
          ++collision_idx_;
          return;
        }
      }

      collision_idx_ = 0;
      ++bucket_idx_;
      advance_to_valid();
    }

    void advance_to_valid() {
      while (bucket_idx_ < table_->buckets.size()) {
        auto &bucket = table_->buckets[bucket_idx_];
        if (std::holds_alternative<value_t>(bucket) ||
            (std::holds_alternative<collision_t>(bucket) &&
             !std::get<collision_t>(bucket).empty())) {
          return;
        }
        ++bucket_idx_;
      }
    }
  };

  // Iterator methods
  Iterator begin() { return Iterator(this, 0); }

  Iterator end() { return Iterator(this, buckets.size()); }
};
