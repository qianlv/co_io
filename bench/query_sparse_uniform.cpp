#include <benchmark/benchmark.h>

#include "adaptive_radix_tree.hpp"
#include <map>
#include <random>
#include <unordered_map>

static constexpr size_t NUM_KEYS = 1600000;

static void BM_AdaptiveRadixTree(benchmark::State &state) {
  AdaptiveRadixTree<int> tree;
  std::hash<uint64_t> hasher;
  std::mt19937 rng(0);
  int count = 0;
  for (auto _ : state) {
    tree.insert(std::to_string(hasher(rng())), count);
    count += 1;
  }

  int result;
  std::mt19937 rng2(0);
  for (auto _ : state) {
    tree.search(std::to_string(hasher(rng2())), result);
  }
}

BENCHMARK(BM_AdaptiveRadixTree)->Iterations(NUM_KEYS);

static void BM_unordered_map(benchmark::State &state) {
  std::unordered_map<std::string, int> tree;
  std::hash<uint64_t> hasher;
  std::mt19937 rng(0);
  int count = 0;
  for (auto _ : state) {
    // tree.insert_or_assign(std::to_string(hasher(rng())), count);
    tree[std::to_string(hasher(rng()))] = count;
    count += 1;
  }

  int result;
  (void)result;
  std::mt19937 rng2(0);
  for (auto _ : state) {
    // tree.find(std::to_string(hasher(rng2())));
    result = tree[std::to_string(hasher(rng2()))];
  }
}

BENCHMARK(BM_unordered_map)->Iterations(NUM_KEYS);

static void BM_map(benchmark::State &state) {
  std::map<std::string, int> tree;
  std::hash<uint64_t> hasher;
  std::mt19937 rng(0);
  int count = 0;
  for (auto _ : state) {
    // tree.insert_or_assign(std::to_string(hasher(rng())), count);
    tree[std::to_string(hasher(rng()))] = count;
    count += 1;
  }

  int result = 0;
  (void)result;
  std::mt19937 rng2(0);
  for (auto _ : state) {
    // tree.at(std::to_string(hasher(rng2())));
    result = tree[std::to_string(hasher(rng2()))];
  }
}

BENCHMARK(BM_map)->Iterations(NUM_KEYS);
BENCHMARK_MAIN();
