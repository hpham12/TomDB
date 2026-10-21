//
// Created by Hieu Pham on 10/18/26.
//
#ifndef TOMDB_NODE_TPP
#define TOMDB_NODE_TPP

template<class K>
int InnerNode<K>::addKey(K key) {
    if (count == kCapacity) {
        std::cerr << "Cannot add addition key because node is full" << '\n';
        return INVALID_INDEX;
    }

    int keyIndex = getKeyIndex(key);

    if (keyIndex == kCapacity) {
        return INVALID_INDEX;
    }

    // add key in to keys array
    for (int i = count - 1; i >= keyIndex; --i) {
        keys[i + 1] = keys[i];
    }

    keys[keyIndex] = key;

    return keyIndex;
}

template<class K>
bool InnerNode<K>::addChild(PageID child, int index) {
    if (count == kCapacity) {
        std::cerr << "Cannot add addition child because node is full" << '\n';
        return false;
    }

    for (int i = count - 1; i >= index; --i) {
        children[i + 1] = children[i];
    }

    children[index] = child;

    return true;
}

template<class K>
int InnerNode<K>::getKeyIndex(K key) const {
    int low = 0;
    int high = count;
    int mid = 0;

    while (low < high) {
        mid = (low + high) / 2;
        K midKey = keys[mid];

        if (midKey <= key) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    return low;
}

template<class K>
int LeafNode<K>::getKeyIndex(K key) const {
    int low = 0;
    int high = count;
    int mid = 0;

    while (low < high) {
        mid = (low + high) / 2;
        K midKey = keys[mid];

        if (midKey <= key) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    return low;
}

template<class K>
bool LeafNode<K>::add(K key, Record record) {
    if (count == kCapacity) {
        std::cerr << "Cannot add addition entry because leaf node is full" << '\n';
        return false;
    }

    if (count == 0) {
        keys[0] = key;
        records[0] = record;
        count++;
        return true;
    }

    int keyIndex = getKeyIndex(key);

    // add key in to keys array
    for (int i = count - 1; i >= keyIndex; --i) {
        keys[i + 1] = keys[i];
    }

    keys[keyIndex] = key;

    // add child into children array
    int recordIndex = keyIndex;

    for (int i = count - 1; i >= recordIndex; --i) {
        records[i + 1] = records[i];
    }

    records[recordIndex] = record;

    count++;
    return true;
}

template<class K>
Record LeafNode<K>::getRecord(K key) const {
    // search for key
    int low = 0;
    int high = count - 1;
    int mid = 0;

    while (low <= high) {
        mid = (low + high) / 2;
        K midKey = keys[mid];

        if (midKey == key) {
            return records[mid];
        }

        if (midKey < key){
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return INVALID_RECORD;
}

#endif //TOMDB_NODE_TPP
