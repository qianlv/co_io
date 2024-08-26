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

    auto iter = tree.begin();
    auto iter2 = words.begin();
    for (; iter2 != words.end() && iter != tree.end(); ++iter2, ++iter) {
        if (iter->first != *iter2) {
            std::cerr << name << " Error: " << iter->first << " != " << *iter2 << std::endl;
            return;
        }
    }
}

void test5() {
    std::set<std::string> words = {
        "alligator", "alien",  "baloon", "chromodynamic", "romane",     "romanus",
        "romulus",   "rubens", "ruber",  "rubicon",       "rubicundus", "all",
        "rub",       "ba",     "kfg",
    };
    test(words, "test5");
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
    std::set<std::string> words;
    for (size_t i = 0; i < n; i++) {
        words.insert(randstring(rng));
    }

    test(words, "test_random");
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

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    test5();
    test_random(1000);
    if (argc > 1) {
        test_file(argv[1]);
    }
    return 0;
}
