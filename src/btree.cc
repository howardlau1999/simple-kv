#include <btree.h>
#include <algorithm>

std::ostream& operator<<(std::ostream& os, Value const& value) {
    os << value.bytes;
    return os;
}

Node::Node(BTree* tree, bool isLeaf) : tree(tree), isLeaf(isLeaf) {
}

BTree* Node::getTree() const { return tree; }

bool Node::ifLeaf() const { return isLeaf; }

// Initial the new InnerNode
InnerNode::InnerNode(size_t d, BTree* t, bool _isRoot)
    : Node(t, false) {
    degree   = t->degree;
    isRoot   = _isRoot;
    n        = 0;
    keys     = new Key[2 * d + 2];
    children = new Node*[2 * d + 2];
}

// delete the InnerNode
InnerNode::~InnerNode() {
    delete[] keys;
    delete[] children;
}

// binary search the first key in the innernode larger than input key
int InnerNode::findIndex(Key const& key) const {
    int pos = std::upper_bound(keys, keys + n, key) - keys;
    return pos;
}

// insert the node that is assumed not full
// insert format:
// ======================
// | key | node pointer |
// ======================
// WARNING: can not insert when it has no entry
void InnerNode::insertNonFull(Key const& key, Node* node) {
    int pos = findIndex(key);
    memmove(children + pos + 1, children + pos, sizeof(Node*) * (n - pos));
    memmove(keys + pos + 1, keys + pos, sizeof(Key) * (n - pos));
    children[pos] = node;
    keys[pos]     = key;
    n++;
}

// insert func
// return value is not NULL if split, returning the new child and a key to insert
KeyNode InnerNode::insert(Key const& key, Value const& value) {
    // 1. Insertion to the first leaf (only one leaf)
    if (this->isRoot && this->n == 0) {
        LeafNode* node = new LeafNode(tree);
        node->insert(key, value);
        insertNonFull(key, node);
        return {key, nullptr};
    }

    int     pos = findIndex(key);
    KeyNode childSplitKey;
    if (pos == 0) {
        childSplitKey = children[0]->insert(key, value);
        keys[0]       = children[0]->getMinKey();
    } else {
        childSplitKey = children[pos - 1]->insert(key, value);
    }
    if (childSplitKey.node) {
        if (n < 2 * degree + 1) {
            insertNonFull(childSplitKey.key, childSplitKey.node);
        } else {
            KeyNode    selfSplitKey = split();
            InnerNode* splitNode    = (InnerNode*)selfSplitKey.node;
            if (childSplitKey.key < selfSplitKey.key) {
                insertNonFull(childSplitKey.key, childSplitKey.node);
            } else {
                splitNode->insertNonFull(childSplitKey.key, childSplitKey.node);
            }

            if (this->isRoot) {
                InnerNode* newRoot = new InnerNode(tree->degree, tree, true);
                isRoot             = false;
                splitNode->isRoot  = false;
                newRoot->insertNonFull(getMinKey(), this);
                newRoot->insertNonFull(splitNode->getMinKey(), splitNode);
                tree->root = newRoot;
            }
            return selfSplitKey;
        }
    }
    return {key, nullptr};
}

// ensure that the leaves inserted are ordered
// used by the bulkLoading func
// inserted data: | minKey of leaf | LeafNode* |
KeyNode InnerNode::insertLeaf(const KeyNode& leaf) {
    keys[n]       = leaf.key;
    children[n++] = leaf.node;
    return leaf;
}

KeyNode InnerNode::split() {
    InnerNode* right = new InnerNode(degree, tree);

    right->n = n / 2;
    n -= right->n;

    std::copy(keys + n, keys + n + right->n, right->keys);
    std::copy(children + n, children + n + right->n, right->children);

    return {keys[n], right};
}

bool InnerNode::remove(Key const& key, int index, InnerNode* parent, bool& ifDelete) {
    // the InnerNode need to be redistributed or merged after deleting one of its child node.

    int pos = findIndex(key);
    if (pos == 0) return false;
    bool ifRemove = children[pos - 1]->remove(key, pos - 1, this, ifDelete);
    if (ifDelete) {
        removeChild(pos - 1, pos - 1);
        if (isRoot && n == 1 && !children[0]->ifLeaf()) {
            tree->root         = dynamic_cast<InnerNode*>(children[0]);
            tree->root->isRoot = true;
            ifDelete           = true;
            return ifRemove;
        }

        if (n < degree + 1 && !isRoot) {
            // The node does not satisfy constraints any more.
            // Need to do a lot of work:
            InnerNode *leftBro = nullptr, *rightBro = nullptr;
            getBrother(index, parent, leftBro, rightBro);

            if (parent->isRoot && parent->getChildNum() == 2) {
                // Case 6: the parent is root
                if (leftBro) {
                    mergeParentLeft(parent, leftBro);
                    ifDelete = true;
                } else if (rightBro) {
                    mergeParentRight(parent, rightBro);
                    ifDelete = true;
                }
                return ifRemove;
            }

            if (rightBro && rightBro->n > degree + 1) {
                // Case 2: the right brother has enough elements
                //         for redistribution
                redistributeRight(index, rightBro, parent);
                ifDelete = false;
                return ifRemove;

            } else if (leftBro && leftBro->n > degree + 1) {
                // Case 3: the left brother has enough elements
                //         for redistribution
                redistributeLeft(index, leftBro, parent);
                ifDelete = false;
                return ifRemove;
            }

            if (rightBro) {
                // Case 4: the right brother has to merge with this node
                mergeRight(rightBro, key);
                parent->keys[index + 1] = rightBro->getMinKey();
                ifDelete                = true;
                return ifRemove;
            } else if (leftBro) {
                // Case 5: the left brother has to merge with this node
                mergeLeft(leftBro, key);
                parent->keys[index - 1] = leftBro->getMinKey();
                ifDelete                = true;
                return ifRemove;
            }

            return false;
        }

        // Case 1: The tree still satisfies the constraints
        ifDelete = false;
        return ifRemove;
    }
    keys[0] = children[0]->getMinKey();
    return ifRemove;
}

// If the leftBro and rightBro exist, the rightBro is prior to be used
void InnerNode::getBrother(int index, InnerNode* parent, InnerNode*& leftBro, InnerNode*& rightBro) {
    if (parent && index > 0) leftBro = dynamic_cast<InnerNode*>(parent->children[index - 1]);
    if (parent && index + 1 < (int)parent->n) rightBro = dynamic_cast<InnerNode*>(parent->children[index + 1]);
}

// merge this node, its parent and left brother(parent is root)
void InnerNode::mergeParentLeft(InnerNode* /*parent*/, InnerNode* leftBro) {
    memmove(leftBro->children + leftBro->n, children, sizeof(Node*) * n);
    memmove(leftBro->keys + leftBro->n, keys, sizeof(Key) * n);
    leftBro->n += n;
    tree->root      = leftBro;
    leftBro->isRoot = true;
}

// merge this node, its parent and right brother(parent is root)
void InnerNode::mergeParentRight(InnerNode* /*parent*/, InnerNode* rightBro) {
    memmove(rightBro->children + this->n, rightBro->children, sizeof(Node*) * rightBro->n);
    memmove(rightBro->keys + this->n, rightBro->keys, sizeof(Key) * rightBro->n);
    memmove(rightBro->children, children, sizeof(Node*) * n);
    memmove(rightBro->keys, keys, sizeof(Key) * n);
    rightBro->n += n;
    tree->root       = rightBro;
    rightBro->isRoot = true;
}

// this node and its left brother redistribute
// the left has more entries
void InnerNode::redistributeLeft(int index, InnerNode* leftBro, InnerNode* parent) {
    // leftBro's max key becomes my min key

    // spare space
    memmove(children + 1, children, sizeof(Node*) * n);
    memmove(keys + 1, keys, sizeof(Key) * n);

    // move the key
    --leftBro->n;
    keys[0]     = leftBro->keys[leftBro->n];
    children[0] = leftBro->children[leftBro->n];
    ++this->n;

    // update parent
    parent->keys[index] = this->getMinKey();
}

// this node and its right brother redistribute
// the right has more entries
void InnerNode::redistributeRight(int index, InnerNode* rightBro, InnerNode* parent) {
    // rightBro's min key becomes my max key

    // move the key
    --rightBro->n;
    keys[n]     = rightBro->keys[0];
    children[n] = rightBro->children[0];
    ++this->n;

    memmove(rightBro->keys, rightBro->keys + 1, sizeof(Key) * rightBro->n);
    memmove(rightBro->children, rightBro->children + 1, sizeof(Node*) * rightBro->n);

    // update parent
    parent->keys[index + 1] = rightBro->getMinKey();
}

// merge all entries to its left bro, delete this node after merging.
void InnerNode::mergeLeft(InnerNode* leftBro, const Key& /*k*/) {
    memmove(leftBro->children + leftBro->n, children, sizeof(Node*) * n);
    memmove(leftBro->keys + leftBro->n, keys, sizeof(Key) * n);
    leftBro->n += n;
}

// merge all entries to its right bro, delete this node after merging.
void InnerNode::mergeRight(InnerNode* rightBro, const Key& /*k*/) {
    memmove(rightBro->children + this->n, rightBro->children, sizeof(Node*) * rightBro->n);
    memmove(rightBro->keys + this->n, rightBro->keys, sizeof(Key) * rightBro->n);
    memmove(rightBro->children, children, sizeof(Node*) * n);
    memmove(rightBro->keys, keys, sizeof(Key) * n);
    rightBro->n += n;
}

// remove a children from the current node, used by remove func
void InnerNode::removeChild(int /*keyIdx*/, int childIdx) {
    delete children[childIdx];

    --n;
    // Simply move the keys and children
    memmove(keys + childIdx, keys + childIdx + 1, sizeof(Key) * (n - childIdx));
    memmove(children + childIdx, children + childIdx + 1, sizeof(Node*) * (n - childIdx));
}

// update the target entry, return true if the update succeed.
bool InnerNode::update(Key const& key, Value const& value) {
    int pos = findIndex(key);
    return pos != 0 && children[pos - 1]->update(key, value);
}

// find the target value with the search key, return MAX_VALUE if it fails.
Value InnerNode::find(Key const& key) const {
    Iterator it = this->seek(key);
    if (!it.hasNext()) return {{'\0'}};
    else return it.getKV().value;
}

Key InnerNode::getMinKey() const {
    return keys[0];
}

// get the children node of this InnerNode
Node* InnerNode::getChild(size_t idx) {
    return children[idx];
}

// get the key of this InnerNode
Key InnerNode::getKey(size_t idx) {
    if (idx < this->n) {
        return this->keys[idx];
    } else {
        return 0;
    }
}

// print the InnerNode
void InnerNode::printNode() const {
    std::cout << "||#|";
    for (size_t i = 0; i < this->n; i++) {
        std::cout << " " << this->keys[i] << " |#|";
    }
    std::cout << "|"
         << "    ";
}

int  InnerNode::getKeyNum() const { return this->n - 1; }
int  InnerNode::getChildNum() const { return this->n; }
bool InnerNode::getIsRoot() const { return this->isRoot; }

// print the LeafNode
void LeafNode::printNode() const {
    std::cout << "||";
    for (int i = 0; i < n; ++i) {
        auto const& kv = this->kvs[i];
        std::cout << kv.key << ": " << kv.value << " |";
    }
    std::cout << "|" << " ====>> ";
}

// Construct an empty leaf and set the 
LeafNode::LeafNode(BTree* t) : Node(t, true) {
    n      = 0;
    degree = LEAF_DEGREE;
    prev = next = nullptr;
}

LeafNode::LeafNode() : Node(nullptr, true) {}

LeafNode::~LeafNode() {
}

// insert an entry into the leaf, need to split it if it is full
KeyNode LeafNode::insert(Key const& key, Value const& value) {
    KeyNode newChild{key, nullptr};
    if (update(key, value)) return newChild;
    if (n >= 2 * degree - 1) {
        newChild = split();
        if (key < newChild.key) {
            insertNonFull(key, value);
        } else {
            dynamic_cast<LeafNode*>(newChild.node)->insertNonFull(key, value);
        }
    } else {
        insertNonFull(key, value);
    }
    return newChild;
}

// insert into the leaf node that is assumed not full
void LeafNode::insertNonFull(Key const& key, Value const& value) {
    int pos = std::upper_bound(this->kvs, this->kvs + n, key, [](Key const& key, KeyValue const& kv) {
        return key < kv.key;
    }) - this->kvs;
    memmove(this->kvs + pos + 1, this->kvs + pos, (n - pos) * sizeof(KeyValue));
    n++;
    this->kvs[pos] = KeyValue(key, value);
}

// split the leaf node
// here, if we call leafNode::split, this node must be full
// so bitmap must be all one-bit.
KeyNode LeafNode::split() {
    LeafNode* newNode  = new LeafNode(tree);
    Key       splitKey = findSplitKey();

    newNode->n = n / 2;
    n -= newNode->n;

    next          = newNode;
    newNode->prev = this;

    memmove(newNode->kvs, this->kvs + n, newNode->n * sizeof(KeyValue));

    return {splitKey, newNode};
}

// use to find a intermediate key and delete entries less than middle
// called by the split func to generate new leaf-node
// qsort first then find
Key LeafNode::findSplitKey() const {
    return this->kvs[(n + 1) / 2].key;
}

// remove an entry from the leaf
// if it has no entry after removement return TRUE to indicate outer func to delete this leaf.
// need to call PAllocator to set this leaf free and reuse it
bool LeafNode::remove(Key const& key, int /*index*/, InnerNode* /*parent*/, bool& ifDelete) {
    int idx = findIndex(key);
    if (idx == -1) return false;
    memmove(this->kvs + idx, this->kvs + idx + 1, (n - idx) * sizeof(KeyValue));
    if (--n == 0) ifDelete = true;
    return true;
}

// update the target entry
// return TRUE if the update succeed
bool LeafNode::update(Key const& key, Value const& value) {
    int pos = findIndex(key);
    if (pos == -1) return false;
    this->kvs[pos].value = value;
    return true;
}

int LeafNode::findIndex(Key const& key) const {
    // Binary Search
    int pos = std::lower_bound(this->kvs, this->kvs + this->n, key) - this->kvs;
    // Not Found
    if (pos >= n || this->kvs[pos] != key)
        return -1;
    return pos;
}

// if the entry can not be found, return the max Value
Value LeafNode::find(Key const& key) const {
    Iterator it = this->seek(key);
    if (!it.hasNext()) return {{'\0'}};
    else return it.getKV().value;
}

Iterator InnerNode::seek(Key const& key) const {
    int pos = findIndex(key) - 1;
    if (pos < 0) return Iterator(nullptr, -1);
    return children[pos]->seek(key);
}

Iterator LeafNode::seek(Key const& key) const {
    int idx = findIndex(key);
    if (idx == -1) return Iterator(nullptr, -1);
    else return Iterator(this, idx);
}

Key LeafNode::getMinKey() const {
    return this->kvs[0].key;
}


void BTree::recursiveDelete(Node* n) {
    if (n->isLeaf) {
        delete n;
    } else {
        InnerNode* node = dynamic_cast<InnerNode*>(n);
        for (size_t i = 0; i < node->n; i++) {
            recursiveDelete(node->children[i]);
        }
        delete n;
    }
}

BTree::BTree(size_t t_degree) {
    this->degree = t_degree;
    bulkLoading();
}

BTree::~BTree() {
    recursiveDelete(this->root);
}

InnerNode* BTree::getRoot() {
    return this->root;
}

Iterator BTree::seek(Key const& key) {
    if (root != NULL) {
        return root->seek(key);
    }
    return Iterator(nullptr, -1);
}

void BTree::changeRoot(InnerNode* newRoot) {
    this->root = newRoot;
}

void BTree::insert(Key const& k, Value const& v) {
    if (root != NULL) {
        root->insert(k, v);
    }
}

bool BTree::remove(Key const& k) {
    if (root != NULL) {
        bool       ifDelete = false;
        InnerNode* temp     = root;
        bool       ifRemove = root->remove(k, -1, nullptr, ifDelete);
        if (ifDelete) delete temp;
        return ifRemove;
    }
    return false;
}

bool BTree::update(Key const& k, Value const& v) {
    if (root != NULL) {
        return root->update(k, v);
    }
    return false;
}

Value BTree::find(Key const& key) {
    if (root != NULL) {
        return root->find(key);
    }
    return {{'\0'}};
}

// call the InnerNode and LeafNode print func to print the whole tree
void BTree::printTree() {
    std::queue<Node*> q;
    q.push(root);
    while (!q.empty()) {
        Node* cur = q.front();
        q.pop();
        cur->printNode();
        if (!cur->isLeaf) {
            InnerNode* node = dynamic_cast<InnerNode*>(cur);
            for (size_t i = 0; i < node->n; ++i) {
                q.push(node->children[i]);
            }
        }
        std::cout << std::endl;
    }
}

bool BTree::bulkLoading() {
    this->root = new InnerNode(degree, this, true);
    return true;
}

