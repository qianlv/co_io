import random
from typing import Tuple


class RadixTreeNode:
    def __init__(self, prefix: str, is_key: bool = False):
        self.prefix = prefix
        self.childs: dict[str, RadixTreeNode] = {}
        self.is_key = is_key

    def match(self, prefix: str) -> Tuple[str, str, str]:
        i = 0
        while i < len(prefix) and i < len(self.prefix) and prefix[i] == self.prefix[i]:
            i += 1

        return self.prefix[:i], self.prefix[i:], prefix[i:]

    def clone(self) -> "RadixTreeNode":
        node = RadixTreeNode(self.prefix, self.is_key)
        node.childs = self.childs.copy()
        return node

    def debug(self, width=0):
        s = " " * width + self.prefix + f"({'l' if self.is_key else 'n'})"
        print(s)
        for _, c in self.childs.items():
            c.debug(len(s))


class RadixTree:
    def __init__(self):
        self.root = RadixTreeNode("")

    def insert(self, word):
        current = self.root

        while True:
            common_prefix, current_remaining, word_remaining = current.match(word)

            # print(
            #     f"common_prefix = {common_prefix}, current_remaining = {current_remaining}, word_remaining = {word_remaining}"
            # )

            if (
                current_remaining
            ):  # two case, first case: abcd, abcef -> abc, d, ef. second case: abcde, abcd -> abcd, e, ""
                new_node = current.clone()
                new_node.prefix = current_remaining

                current.prefix = common_prefix
                current.childs.clear()
                current.childs[current_remaining[0]] = new_node
                if word_remaining:  # first case
                    current.is_key = False
                    current.childs[word_remaining[0]] = RadixTreeNode(
                        word_remaining, True
                    )
            elif (
                not current_remaining
            ):  # two case, first case: abcd, abcdef -> abcd, "", ef, second case: abcd, abcd -> abcd, "", ""
                pass

            if not word_remaining:
                break

            if word_remaining and word_remaining[0] not in current.childs:
                current.childs[word_remaining[0]] = RadixTreeNode(word_remaining, True)

            current = current.childs[word_remaining[0]]
            word = word_remaining

        current.is_key = True

    def search(self, word):
        current = self.root

        while True:
            _, current_remaining, word_remaining = current.match(word)
            # print(
            #     f"common_prefix = {common_prefix}, current_remaining = {current_remaining}, word_remaining = {word_remaining}"
            # )
            if current_remaining:
                return False
            if not word_remaining:
                return current.is_key
            if word_remaining[0] not in current.childs:
                return False
            current = current.childs[word_remaining[0]]
            word = word_remaining

    def remove(self, word):
        stacks = []
        current = self.root

        while True:
            stacks.append(current)
            _, current_remaining, word_remaining = current.match(word)
            if current_remaining:
                return False
            if not word_remaining:
                if current.is_key:
                    break
                else:
                    return False
            if word_remaining[0] not in current.childs:
                return False

            current = current.childs[word_remaining[0]]
            word = word_remaining


        # print(", ".join(s.prefix for s in stacks))

        if not current.childs:
            parent = stacks[-2]
            parent.childs.pop(current.prefix[0])
            stacks.pop()
        else:
            current.is_key = False

        while stacks:
            parent = stacks.pop()
            if len(parent.childs) > 1 or parent == self.root:
                break
            current, *_ = parent.childs.values()
            parent.prefix += current.prefix
            parent.childs = current.childs
            parent.is_key = current.is_key

    def debug(self):
        self.root.debug()


def test1():
    tree = RadixTree()
    tree.insert("tester")
    tree.debug()
    tree.insert("test")
    tree.debug()


def test2():
    tree = RadixTree()
    tree.insert("test")
    tree.debug()
    tree.insert("team")
    tree.debug()
    tree.insert("toast")
    tree.debug()


def test3():
    tree = RadixTree()
    tree.insert("test")
    tree.insert("toaster")
    tree.insert("toasting")
    tree.insert("slow")
    tree.insert("slowly")
    tree.debug()


def test4():
    tree = RadixTree()
    tree.insert("romane")
    tree.insert("romanus")
    tree.insert("romulus")
    tree.insert("rubens")
    tree.insert("ruber")
    tree.insert("rubicon")
    tree.insert("rubicundus")
    tree.debug()

    print(tree.search("romane"))
    print(tree.search("romanus"))
    print(tree.search("romulus"))
    print(tree.search("rubens"))
    print(tree.search("ruber"))
    print(tree.search("rubicon"))
    print(tree.search("rubicundus"))
    print(tree.search("test"))
    print(tree.search(""))


def test5():
    words = [
        "alligator",
        "alien",
        "baloon",
        "chromodynamic",
        "romane",
        "romanus",
        "romulus",
        "rubens",
        "ruber",
        "rubicon",
        "rubicundus",
        "all",
        "rub",
        "ba",
    ]
    tree = RadixTree()
    for word in words[:]:
        tree.insert(word)
        # tree.debug()
    tree.debug()
    print(tree.search("ba"))
    print(tree.search("all"))
    print(tree.search("alligator"))


def random_word():
    len = random.randint(10, 20)
    return "".join(random.choice("abcdefghijklmnopqrstuvwxyz") for _ in range(len))


def test_random():
    seed = random.randint(0, 10000)
    print(seed)
    random.seed(seed)
    n = 100000
    words = [random_word() for _ in range(n)]
    tree = RadixTree()
    for word in words:
        tree.insert(word)
        assert tree.search(word)

    # tree.debug()

    for word in words:
        assert tree.search(word)


def test6():
    tree = RadixTree()
    tree.insert("footer")
    tree.debug()
    tree.insert("foo")
    tree.insert("foobar")
    tree.insert("first")
    tree.insert("foot")
    tree.debug()
    print("+++")
    tree.remove("first")
    tree.debug()
    tree.remove("foot")
    tree.debug()
    tree.remove("foo")
    tree.debug()
    tree.remove("foobar")
    print("+++")
    tree.debug()
    tree.remove("footer")
    tree.debug()


if __name__ == "__main__":
    # test1()
    # print("------------")
    # test2()
    # print("------------")
    # test3()
    # print("------------")
    # test4()
    # print("------------")
    test5()
    # test_random()
    # test6()
