#include "utils/adaptive_radix_tree.hpp"
#include <fstream>
#include <random>
#include <set>
#include <unordered_map>
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
        std::cerr << "word " << word << std::endl;
        tree.insert(word, 1);
        tree.debug();
    }
    tree.debug();
}

std::string randstring(std::mt19937 &rng) {
    std::hash<uint64_t> hasher;
    return std::to_string(hasher(rng()));
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
    for (auto word : words) {
        if (!tree.search(word)) {
            std::cerr << word << " -> "
                      << "not found" << std::endl;
            return;
        }
    }
    // tree.debug();
}

void test_file(std::string filename) {
    std::ifstream fin(filename);
    std::string word;
    std::unordered_map<std::string, size_t> words;
    AdaptiveRadixTree<size_t> tree;
    size_t i = 0;
    while (fin >> word) {
        words[word] = i;
        tree.insert(word, i);
        i += 1;
    }

    for (const auto& [word, i] : words) {
        std::optional<size_t> result = tree.search(word);
        if (!result) {
            std::cerr << word << " -> "
                      << "not found" << std::endl;
            return;
        }
        if (*result != i) {
            std::cerr << word << " -> " << *result  << " != " << i << std::endl;
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
    test_random(1000000);
    if (argc > 1) {
        test_file(argv[1]);
    }
    test256();
    return 0;
}
