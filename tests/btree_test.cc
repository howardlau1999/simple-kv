#include <gtest/gtest.h>
#include <btree.h>

TEST(BTreeTest, SingleInsert) {
    BTree* tree = new BTree(3);
    Value value = "test";
    tree->insert(1, value);
    Value value1;
    Status status = tree->find(1, value1);
    EXPECT_EQ(status, FOUND);
    EXPECT_EQ(value, value1);
}

TEST(BTreeTest, DeleteAfterInsert) {
    BTree* tree = new BTree(3);
    Value value = "test";
    tree->insert(1, value);
    tree->remove(1);
    Value value1;
    Status status = tree->find(1, value1);
    EXPECT_EQ(status, FAILED);
}

TEST(BTreeTest, UpdateAfterInsert) {
    BTree* tree = new BTree(3);
    Value value = "test";
    tree->insert(1, value);
    Value value1;
    Status status = tree->find(1, value1);
    EXPECT_EQ(status, FOUND);
    EXPECT_EQ(value, value1);
    Value newValue = "test2";
    tree->insert(1, newValue);
    status = tree->find(1, value1);
    EXPECT_EQ(status, FOUND);
    EXPECT_EQ(newValue, value1);
    delete tree;
}

TEST(BTreeTest, LeafSplit) {
    BTree* tree = new BTree(3);
    for (int i = 0; i < LEAF_DEGREE * 2; ++i) {
        Value value = "test";
        tree->insert(i, value);
        Value value1;
        Status status = tree->find(i, value1);
        EXPECT_EQ(status, FOUND);
        EXPECT_EQ(value, value1);
    }
    EXPECT_EQ(tree->getRoot()->getChildNum(), 2);
    delete tree;
}

TEST(BTreeTest, InnerNodeSplit) {
    BTree* tree = new BTree(3);
    for (int i = 0; i < LEAF_DEGREE * 9; ++i) {
        Value value = "test";
        tree->insert(i, value);
        Value value1;
        Status status = tree->find(i, value1);
        EXPECT_EQ(status, FOUND);
        EXPECT_EQ(value, value1);
    }
    EXPECT_EQ(tree->getRoot()->getChildNum(), 2);
    EXPECT_FALSE(tree->getRoot()->getChild(0)->ifLeaf());
    EXPECT_FALSE(tree->getRoot()->getChild(1)->ifLeaf());
    EXPECT_EQ(dynamic_cast<InnerNode*>(tree->getRoot()->getChild(0))->getChildNum(), 4);
    EXPECT_EQ(dynamic_cast<InnerNode*>(tree->getRoot()->getChild(1))->getChildNum(), 5);
    delete tree;
}

TEST(BTreeTest, LeafNodeDelete) {
    BTree* tree = new BTree(3);
    for (int i = 0; i < LEAF_DEGREE * 9; ++i) {
        Value value = "test";
        tree->insert(i, value);
        Value value1;
        Status status = tree->find(i, value1);
        EXPECT_EQ(status, FOUND);
        EXPECT_EQ(value, value1);
    }
    for (int i = LEAF_DEGREE * 9 - 1; i >= LEAF_DEGREE * 8; --i) {
        tree->remove(i);
    }
    EXPECT_EQ(tree->getRoot()->getChildNum(), 2);
    EXPECT_FALSE(tree->getRoot()->getChild(0)->ifLeaf());
    EXPECT_FALSE(tree->getRoot()->getChild(1)->ifLeaf());
    EXPECT_EQ(dynamic_cast<InnerNode*>(tree->getRoot()->getChild(0))->getChildNum(), 4);
    EXPECT_EQ(dynamic_cast<InnerNode*>(tree->getRoot()->getChild(1))->getChildNum(), 4);
}

TEST(BTreeTest, InnerNodeMerge) {
    BTree* tree = new BTree(3);
    for (int i = 0; i < LEAF_DEGREE * 9; ++i) {
        Value value = "test";
        tree->insert(i, value);
        Value value1;
        Status status = tree->find(i, value1);
        EXPECT_EQ(status, FOUND);
        EXPECT_EQ(value, value1);
    }
    for (int i = LEAF_DEGREE * 9 - 1; i >= LEAF_DEGREE * 7; --i) {
        tree->remove(i);
    }
    EXPECT_EQ(tree->getRoot()->getChildNum(), 7);
}

TEST(BTreeTest, InnerNodeMergeParentRight) {
    BTree* tree = new BTree(3);
    for (int i = 0; i < LEAF_DEGREE * 10; ++i) {
        Value value = "test";
        tree->insert(i, value);
        Value value1;
        Status status = tree->find(i, value1);
        EXPECT_EQ(status, FOUND);
        EXPECT_EQ(value, value1);
    }
    for (int i = LEAF_DEGREE * 4 - 1; i >= LEAF_DEGREE * 3; --i) {
        tree->remove(i);
    }
    EXPECT_EQ(tree->getRoot()->getChildNum(), 9);
}

TEST(BTreeTest, ScanFromExistingKey) {
    BTree* tree = new BTree(3);
    Value value = "test";
    for (int i = 0; i < 10; ++i) {
        tree->insert(i, value);
        Value value1;
        Status status = tree->find(i, value1);
        EXPECT_EQ(status, FOUND);
        EXPECT_EQ(value, value1);
    }
    int current = 0;
    for (Iterator it = tree->seek(0); it.isValid() && it.getKV().key <= 9; it.next()) {
        EXPECT_EQ(it.getKV().key, current);
        ++current;
    }
}