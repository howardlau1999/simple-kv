#ifndef _BTREE_H_
#define _BTREE_H_

#include <stdlib.h>
#include <string>
#include <memory.h>
#include <stdlib.h>
#include <iostream>
#include <queue>

#define MAX_DEGREE 256
#define MIN_DEGREE 2
#define LEAF_DEGREE 5
#define INNER_DEGREE 4096

typedef u_char Byte;
typedef uint64_t Key;
struct Value {
    Byte bytes[256];
    Value& operator=(Value const& value) {
        memmove(bytes, value.bytes, sizeof(bytes));
    }
};

enum Status {
    OK,
    NOT_FOUND,
};

struct KeyValue {
    Key key;
    Value value;

    bool operator<(Key const& key) const {
        return this->key < key;
    }

    bool operator==(Key const& key) const {
        return this->key == key;
    }

    bool operator!=(Key const& key) const {
        return this->key != key;
    }

    KeyValue() = default;
    KeyValue(KeyValue const& kv) {
        this->key = kv.key;
        this->value = kv.value;
    }
    KeyValue(Key const& key, Value const& value) : key(key), value(value) {}
    KeyValue& operator=(KeyValue const& kv) {
        this->key = kv.key;
        this->value = kv.value;
        return *this;
    }
};


class InnerNode;
class LeafNode;
class Node;
class Iterator;

struct KeyNode {
    Key   key;
    Node* node;
};

class BTree;


class Node {
protected:
    friend class BTree;

    BTree* tree;
    size_t  degree;  // the degree of the node
    bool    isLeaf;

public:
    Node(BTree* tree, bool isLeaf);
    virtual ~Node() {}

    BTree* getTree() const;  // the tree that the node belongs to

    bool ifLeaf() const;  // judge whether the node is leaf

    /**
     * \brief insert key-value into this node.
     * \param k key of the key-value pair to be inserted, may duplicate.
     * \param v value of the key-value pair to be inserted
     * \return nonnull KeyNode with key the minimum key of the splitted, node the node splitted out
     * \note The key-value will finally be inserted into one leaf node.
     */
    virtual KeyNode insert(const Key& k, const Value& v) = 0;

    /**
     * \brief split this node into two nodes.
     * \return KeyNode with key the medium key to split keys of this node, node the node splitted out
     * \note you should manually insert the new node into parent node of the old one.
     */
    virtual KeyNode split() = 0;

    /**
     * \brief remove the key and related value.
     * \param k the key to be removed.
     * \param index the child index in parent.
     * \param ifDelete set to true if this node is to be deleted.
     * \return true if the key exists, and remove operation succeeds.
     * \note caller node should delete this node if ifDelete is true.
     */
    virtual bool remove(const Key& k, int index, InnerNode* parent, bool& ifDelete) = 0;

    /**
     * \brief update the value related to the key.
     * \param k the key whose related value to be modified
     * \param v the new value
     * \return true if the key exists and update operation succeeds
     */
    virtual bool update(const Key& k, const Value& v) = 0;

    /**
     * \brief find the value related to the key.
     * \param k the key whose value is to be identified.
     * \return the related value, MAX_VALUE if the key cannot be found
     */
    virtual Value find(const Key& k) const = 0;

    /**
     * \brief find the minimum key inside this node.
     * \return the minimum key
     */
    virtual Key getMinKey() const = 0;

    /**
     * \brief print information of this node to the screen
     */
    virtual void printNode() const = 0;

    virtual Iterator seek(Key const& key) const = 0;
};

class InnerNode : public Node {
private:
    friend class BTree;

    bool   isRoot;    // judge whether the node is root
    size_t n;         // amount of children
    Key*   keys;      // max (2 * d + 2) keys
    Node** children;  // max (2 * d + 2) node pointers

    int findIndex(const Key& k) const;

    void getBrother(int index, InnerNode* parent, InnerNode*& leftBro, InnerNode*& rightBro);
    void redistributeRight(int index, InnerNode* rightBro, InnerNode* parent);
    void redistributeLeft(int index, InnerNode* leftBro, InnerNode* parent);

    void mergeParentRight(InnerNode* parent, InnerNode* rightBro);
    void mergeParentLeft(InnerNode* parent, InnerNode* leftBro);

    void mergeLeft(InnerNode* LeftBro, const Key& k);
    void mergeRight(InnerNode* rightBro, const Key& k);

public:
    InnerNode(size_t d, BTree* tree, bool _ifRoot = false);
    ~InnerNode();

    KeyNode insert(const Key& k, const Value& v) override;
    void    insertNonFull(const Key& k, Node* node);
    KeyNode insertLeaf(const KeyNode& leaf);
    bool    remove(const Key& k, int index, InnerNode* parent, bool& ifDelete) override;
    bool    update(const Key& k, const Value& v) override;
    Value   find(const Key& k) const override;
    Key     getMinKey() const override;

    KeyNode split() override;
    void    removeChild(int KeyIdx, int childIdx);

    Node* getChild(size_t idx);
    Key   getKey(size_t idx);
    int   getKeyNum() const;
    int   getChildNum() const;
    bool  getIsRoot() const;
    void  printNode() const override;
    Iterator seek(Key const& key) const override;
};


// LeafNode structure is the leaf node in NVM that is buffered in the DRAM.
class LeafNode : public Node {
private:
    friend class BTree;
    friend class InnerNode;
    friend class Iterator;

    // the DRAM relative variables
    size_t    n;         // amount of entries
    LeafNode* prev;      // the address of previous leafnode
    LeafNode* next;      // the address of next leafnode
    KeyValue kvs[LEAF_DEGREE * 2];
    std::string    filePath;  // the file path of the leaf

    size_t size;
    
    LeafNode();             // only to prevent recursion
public:
    LeafNode(BTree* tree);           // allocate a new leaf
    ~LeafNode();

    KeyNode insert(const Key& k, const Value& v) override;
    void    insertNonFull(const Key& k, const Value& v);
    bool    update(const Key& k, const Value& v) override;
    bool    remove(const Key& k, int index, InnerNode* parent, bool& ifDelete) override;
    Value   find(const Key& k) const override;
    int     findIndex(const Key& k) const;
    Key     getMinKey() const override;
    Iterator seek(Key const& key) const override;

    // used by insert()
    KeyNode split() override;
    Key     findSplitKey() const;

    void printNode() const override;

    int      findFirstZero() const;
    int      findFirstGreater(Key const& key) const;
    int      getBit(int idx) const;
    int      numEntries() const;
    Key      getKey(int idx) const;
    Value    getValue(int idx) const;
};

class Iterator {
private:
    const LeafNode* curNode;
    int curSlot;
public:
    Iterator(const LeafNode* node, const int slot) : curNode(node), curSlot(slot) {}

    KeyValue getKV() {
        return this->curNode->kvs[curSlot];
    }

    bool hasNext() {
        if (!curNode) return false;
        if (curSlot + 1 >= curNode->n) {
            return this->curNode->next != nullptr;
        }
    }

    void next() {
        if (curSlot + 1 >= curNode->n) {
            curNode = curNode->next;
            curSlot = 0;
        } else {
            ++curSlot;
        }
    }
};

class BTree {
private:
    friend class InnerNode;
    InnerNode* root;
    size_t     degree;

    void recursiveDelete(Node* n);  // call by the ~BTree(), delete the whole tree

public:
    BTree(size_t degree);
    ~BTree();

    void      insert(Key const& k, Value const& v);
    bool      remove(Key const& k);
    bool      update(Key const& k, Value const& v);
    Iterator  seek(Key const& k);
    Value     find(Key const& k);
    LeafNode* findLeaf(Key const& K);

    InnerNode* getRoot();                       // get the root node of the tree
    void       changeRoot(InnerNode* newRoot);  // change the root of the tree

    void printTree();

    /**
     * \brief load data from persistent memory.
     * \return true if any data exists in the pmem.
     */
    bool bulkLoading();
};



#endif