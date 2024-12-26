#include "hash_table/hashtable.hh"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <climits>
#include <random>
#include <string>

TEST_CASE("HashTable Basic Operations", "[hash_table]") {
  HashTable<int, std::string> table;

  SECTION("Empty state") {
    REQUIRE(!table.get(1).has_value());
    REQUIRE(!table.get(-1).has_value());
    REQUIRE(!table.get(0).has_value());
  }

  SECTION("Single element operations") {
    table.insert(1, "one");
    REQUIRE(table.get(1).value() == "one");

    // Insert same key with different value
    table.insert(1, "new_one");
    REQUIRE(table.get(1).value() == "one"); // Original value preserved

    // Boundary values
    table.insert(INT_MAX, "max");
    table.insert(INT_MIN, "min");
    REQUIRE(table.get(INT_MAX).value() == "max");
    REQUIRE(table.get(INT_MIN).value() == "min");
  }
}

TEST_CASE("HashTable Collision Handling", "[hash_table]") {
  HashTable<int, std::string> table;

  SECTION("Multiple elements same bucket") {
    // Force collisions by using multiples of bucket size
    constexpr int BUCKET_SIZE = 30;
    table.insert(0, "zero");
    table.insert(BUCKET_SIZE, "thirty");
    table.insert(BUCKET_SIZE * 2, "sixty");

    REQUIRE(table.get(0).value() == "zero");
    REQUIRE(table.get(BUCKET_SIZE).value() == "thirty");
    REQUIRE(table.get(BUCKET_SIZE * 2).value() == "sixty");
  }

  SECTION("High load factor") {
    // Insert more items than buckets
    for (int i = 0; i < 100; i++) {
      table.insert(i, std::to_string(i));
    }

    // Verify all values still accessible
    for (int i = 0; i < 100; i++) {
      REQUIRE(table.get(i).value() == std::to_string(i));
    }
  }
}

TEST_CASE("HashTable Complex Types", "[hash_table]") {
  SECTION("String keys") {
    HashTable<std::string, int> table;

    table.insert("", 0);
    table.insert("hello", 1);
    table.insert(std::string(1000, 'a'), 2); // Long string

    REQUIRE(table.get("").value() == 0);
    REQUIRE(table.get("hello").value() == 1);
    REQUIRE(table.get(std::string(1000, 'a')).value() == 2);
  }

  SECTION("Complex values") {
    HashTable<int, std::vector<std::string>> table;

    std::vector<std::string> v1{"a", "b", "c"};
    std::vector<std::string> v2{"d", "e", "f"};
    std::vector<std::string> empty;

    table.insert(1, v1);
    table.insert(2, v2);
    table.insert(3, empty);

    REQUIRE(table.get(1).value() == v1);
    REQUIRE(table.get(2).value() == v2);
    REQUIRE(table.get(3).value().empty());
  }
}

TEST_CASE("HashTable Edge Cases", "[hash_table]") {
  HashTable<std::string, std::string> table;

  SECTION("Special characters in keys") {
    table.insert("\0", "null");
    table.insert("\n", "newline");
    table.insert("\t", "tab");

    REQUIRE(table.get("\0").value() == "null");
    REQUIRE(table.get("\n").value() == "newline");
    REQUIRE(table.get("\t").value() == "tab");
  }

  SECTION("Unicode strings") {
    table.insert("🔑", "key");
    table.insert("值", "value");

    REQUIRE(table.get("🔑").value() == "key");
    REQUIRE(table.get("值").value() == "value");
  }
}

TEST_CASE("HashTable Performance Checks", "[hash_table][.slow]") {
  HashTable<int, int> table;

  SECTION("Mass insertion and retrieval") {
    // Insert 1000 elements
    for (int i = 0; i < 1000; i++) {
      table.insert(i, i * i);
    }

    // Verify all elements
    for (int i = 0; i < 1000; i++) {
      REQUIRE(table.get(i).value() == i * i);
    }
  }

  SECTION("Collision stress test") {
    constexpr int BUCKET_SIZE = 30;
    // Create many collisions
    for (int i = 0; i < 100; i++) {
      table.insert(i * BUCKET_SIZE, i);
    }

    // Verify all collided elements
    for (int i = 0; i < 100; i++) {
      REQUIRE(table.get(i * BUCKET_SIZE).value() == i);
    }
  }
}

TEST_CASE("Benchmark HashTable vs unordered_map", "[benchmark]") {
  std::mt19937 gen(42);
  auto random_string = [&gen](size_t length) {
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
    std::string str(length, 0);
    for (size_t i = 0; i < length; ++i) {
      str[i] = alphanum[dis(gen)];
    }
    return str;
  };

  const size_t num_elements = 10000;
  std::vector<std::string> keys;
  keys.reserve(num_elements);
  for (size_t i = 0; i < num_elements; ++i) {
    keys.push_back(random_string(10));
  }

  BENCHMARK("HashTable Insert") {
    HashTable<std::string, int> table;
    for (size_t i = 0; i < num_elements; ++i) {
      table.insert(keys[i], i);
    }
    return table;
  };

  BENCHMARK("unordered_map Insert") {
    std::unordered_map<std::string, int> map;
    for (size_t i = 0; i < num_elements; ++i) {
      map.insert({keys[i], i});
    }
    return map;
  };

  BENCHMARK("HashTable Lookup") {
    HashTable<std::string, int> table;
    for (size_t i = 0; i < num_elements; ++i) {
      table.insert(keys[i], i);
    }
    int sum = 0;
    for (const auto &key : keys) {
      auto val = table.get(key);
      if (val)
        sum += val.value();
    }
    return sum;
  };

  BENCHMARK("unordered_map Lookup") {
    std::unordered_map<std::string, int> map;
    for (size_t i = 0; i < num_elements; ++i) {
      map.insert({keys[i], i});
    }
    int sum = 0;
    for (const auto &key : keys) {
      auto it = map.find(key);
      if (it != map.end())
        sum += it->second;
    }
    return sum;
  };

  BENCHMARK("HashTable Mixed Operations") {
    HashTable<std::string, int> table;
    int sum = 0;
    for (size_t i = 0; i < num_elements; ++i) {
      table.insert(keys[i], i);
      if (i % 2 == 0) {
        auto val = table.get(keys[i / 2]);
        if (val)
          sum += val.value();
      }
    }
    return sum;
  };

  BENCHMARK("unordered_map Mixed Operations") {
    std::unordered_map<std::string, int> map;
    int sum = 0;
    for (size_t i = 0; i < num_elements; ++i) {
      map.insert({keys[i], i});
      if (i % 2 == 0) {
        auto it = map.find(keys[i / 2]);
        if (it != map.end())
          sum += it->second;
      }
    }
    return sum;
  };
}
