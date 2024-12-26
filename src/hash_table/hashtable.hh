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
  using empty_t = std::monostate;
  using value_t = std::pair<Key, Value>;
  using collision_t = std::vector<value_t>;
  using bucket_t = std::variant<empty_t, value_t, collision_t>;

public:
  void insert(const Key &key, Value value) {
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

  std::optional<Value> get(const Key &key) const {
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

private:
  std::array<bucket_t, 30> buckets{};

  std::size_t hash(const Key &key) const {
    return std::hash<Key>{}(key) % buckets.size();
  }
};
