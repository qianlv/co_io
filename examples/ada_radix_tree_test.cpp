#include "utils/adaptive_radix_tree.hpp"
#include <fstream>
#include <random>
#include <vector>

void test1() {
  AdaptiveRadixTree<int> tree;
  tree.insert("tester", 1);
  tree.debug();
  tree.insert("test", 1);
  tree.debug();
}

void test2() {
  AdaptiveRadixTree<int> tree;
  tree.insert("test", 1);
  tree.debug();
  tree.insert("team", 1);
  tree.debug();
  tree.insert("toast", 1);
  tree.debug();
}

void test3() {
  AdaptiveRadixTree<int> tree;
  tree.insert("test", 1);
  tree.insert("toaster", 1);
  tree.insert("toasting", 1);
  tree.insert("slow", 1);
  tree.insert("slowly", 1);
  tree.debug();
}

void test4() {
  AdaptiveRadixTree<int> tree;
  tree.insert("romane", 1);
  tree.insert("romanus", 1);
  tree.insert("romulus", 1);
  tree.insert("rubens", 1);
  tree.insert("ruber", 1);
  tree.insert("rubicon", 1);
  tree.insert("rubicundus", 1);
  tree.debug();
}

void test5() {
  std::vector<std::string> words = {
      "alligator", "alien",  "baloon", "chromodynamic", "romane",     "romanus",
      "romulus",   "rubens", "ruber",  "rubicon",       "rubicundus", "all",
      "rub",       "ba",     "kfg",
  };
  AdaptiveRadixTree<int> tree;
  for (auto word : words) {
    tree.insert(word, 1);
  }
  tree.debug();
}

std::string randstring(std::mt19937 &rng) {
  static constexpr std::string_view alphas = "abcdefghijklmnopqrstuvwxyz";
  std::string s;
  int n = std::uniform_int_distribution<int>(10, 20)(rng);
  auto alphas_size = alphas.size();
  for (int i = 0; i < n; i++) {
    s += alphas[std::uniform_int_distribution<size_t>(0, alphas_size - 1)(rng)];
  }
  return s;
}

void test_random(size_t n) {
  std::random_device rd;
  auto seed = rd();
  std::cerr << "seed: " << seed << std::endl;
  std::mt19937 rng(seed);
  std::vector<std::string> words;
  for (size_t i = 0; i < n; i++) {
    words.push_back(randstring(rng));
  }
  AdaptiveRadixTree<int> tree;
  for (auto word : words) {
    tree.insert(word, 1);
  }
  tree.debug();
}

void test_file(std::string filename) {
  std::ifstream fin(filename);
  std::string word;
  std::vector<std::string> words;
  AdaptiveRadixTree<size_t> tree;
  size_t i = 0;
  while (fin >> word) {
    words.push_back(word);
    tree.insert(word, i);
    i += 1;
  }
  for (i = 0; i < words.size(); i++) {
    std::optional<size_t> result = tree.search(words[i]);
    if (!result) {
      std::cerr << words[i] << " -> " << "not found" << std::endl;
      return;
    }
    if (*result != i) {
      std::cerr << words[i] << " -> " << *result << std::endl;
      return;
    }
  }
  // tree.debug();
}

void test256() {
  std::vector<std::string> words;
  for (uint16_t i = 0; i < 256; i++) {
    words.push_back(std::string(1, static_cast<char>(i)));
  }
  AdaptiveRadixTree<int> tree;
  for (auto word : words) {
    tree.insert(word, 1);
  }
  tree.debug();
}

int main(int argc, char *argv[]) {
  test1();
  test2();
  test3();
  test4();
  test5();
  test_random(1000);
  if (argc > 1) {
    test_file(argv[1]);
  }
  test256();
  return 0;
}
