#include <btree.h>

int main() {
    BTree *tree = new BTree(5);
    for (int i = 1; i <= 28; ++i) {
        tree->insert(i, {'t', 'e', 's', 't', '\0'});
    }
    tree->remove(3);
    tree->remove(6);
    tree->remove(7);
    tree->remove(8);
    tree->remove(11);
    for (Iterator it = tree->seek(1); it.hasNext(); it.next()) {
        std::cout << it.getKV().key << ": " << it.getKV().value.bytes << std::endl;
    }
    tree->printTree();
    delete tree;
    return 0;
}