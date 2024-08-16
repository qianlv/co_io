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

    def debug(self, width = 0):
        if self.prefix:
            print(" " * width + self.prefix)
        for _, c in self.childs.items():
            c.debug(width + 2)


class RadixTree:
    def __init__(self):
        self.root = RadixTreeNode("")

    def insert(self, word):
        current = self.root

        while True:
            common_prefix, current_remaining, word_remaining = current.match(word)

            print(f"common_prefix = {common_prefix}, current_remaining = {current_remaining}, word_remaining = {word_remaining}")

            if (
                current_remaining
            ):  # two case, first case: abcd, abcef -> abc, d, ef. second case: abcde, abcd -> abcd, e, ""
                new_node = current.clone()
                new_node.prefix = current_remaining

                current.prefix = common_prefix
                current.childs.clear()
                current.childs[new_node.prefix[0]] = new_node
                if word_remaining:  # first case
                    current.is_key = False
                    current.childs[word_remaining[0]] = RadixTreeNode(
                        word_remaining, True
                    )
                else:  # second case
                    current.is_key = True
            elif (
                not current_remaining
            ):  # two case, first case: abcd, abcdef -> abcd, "", ef, second case: abcd, abcd -> abcd, "", ""
                pass

            if word_remaining and word_remaining[0] not in current.childs:
                current.childs[word_remaining[0]] = RadixTreeNode(word_remaining, True)

            if not word_remaining:
                break

            current = current.childs[word_remaining[0]]
            word = word_remaining

    def search(self, word):
        current = self.root

        while True:
            _, current_remaining, word_remaining = current.match(word)
            if not current_remaining and not word_remaining and current.is_key:
                return True

            if not word_remaining or word_remaining[0] not in current.childs:
                return False

            current = current.childs[word_remaining[0]]
            word = word_remaining

        return False

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


if __name__ == "__main__":
    test1()
    print("------------")
    test2()
    print("------------")
    test3()
    print("------------")
    test4()
    print("------------")
