#include "utils/adaptive_radix_tree.hpp"
#include <algorithm>
#include <fstream>
#include <random>
#include <set>
#include <vector>

void test(std::set<std::string> words, std::string name) {
    AdaptiveRadixTree<int> tree;
    for (auto word : words) {
        tree.insert(word, 1);
    }

    std::random_device rand_d;
    auto seed = rand_d();
    // seed = 1890767699;
    std::cerr << "seed: " << seed << std::endl;
    std::mt19937 rng(seed);
    while (!words.empty()) {
        auto it = words.begin();
        std::advance(it, rng() % words.size());
        auto word = *it;
        words.erase(it);
        // std::cerr << "remove " << word << std::endl;
        if (!tree.remove(word)) {
            std::cerr << name << " remove " << word << " failed" << std::endl;
            tree.debug();
            return;
        }
        // if (!tree.search("overladen").has_value() && !first) {
        //     tree.debug();
        //     first = true;
        // }
    }
}

void test1() {
    AdaptiveRadixTree<int> tree;
    tree.insert("ab", 1);
    tree.insert("abc", 2);
    tree.insert("abcd", 3);
    tree.insert("abcde", 4);

    assert(tree.remove("abc"));
    assert(tree.remove("abcd"));
    assert(!tree.remove("abc"));
    assert(tree.remove("ab"));
    assert(!tree.remove("abc"));
    assert(tree.remove("abcde"));
}

void test2() {
    std::set<std::string> words = {
        "alligator", "alien",  "baloon", "chromodynamic", "romane",     "romanus",
        "romulus",   "rubens", "ruber",  "rubicon",       "rubicundus", "all",
        "rub",       "ba",     "kfg",
    };
    test(words, "test2");
}

void test_file(std::string filename) {
    std::ifstream fin(filename);
    std::string word;
    std::set<std::string> words;
    while (fin >> word) {
        words.insert(word);
    }

    test(words, "test_file");
}

void test256() {
    std::vector<std::string> words;
    for (uint16_t i = 0; i < 256; i++) {
        words.push_back(std::string(1, static_cast<char>(i)));
    }
    for (uint16_t i = 0; i < 256; i++) {
        words[i].push_back(static_cast<char>(256 - i));
    }
    AdaptiveRadixTree<int> tree;
    for (auto word : words) {
        tree.insert(word, 1);
    }
    for (auto word : words) {
        if (!tree.remove(word)) {
            std::cerr << "remove " << word << " failed" << std::endl;
            tree.debug();
            return;
        }
    }
}


int main(int argc, char *argv[]) {
    test1();
    test2();
    if (argc > 1) {
        test_file(argv[1]);
    }
    test256();
    return 0;
}
