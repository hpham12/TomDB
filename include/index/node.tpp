//
// Created by Hieu Pham on 10/18/26.
//
#ifndef TOMDB_NODE_TPP
#define TOMDB_NODE_TPP

template<class K>
bool InnerNode<K>::add(K key, PageID child) {
    if (count == kCapacity) {
        std::cerr << "Cannot add addition entry because node is full" << '\n';
        return false;
    }

    if (count == 0) {
        keys[0] = key;
        children[1] = child;
        ++count;
        return true;
    }

    size_t keyIndex = getKeyIndex(key);

    // add key in to keys array
    for (size_t i = count - 1; i >= keyIndex; --i) {
        keys[i + 1] = keys[i];
    }

    keys[keyIndex] = key;

    // add child into children array
    size_t childIndex = keyIndex + 1;

    for (size_t i = count; i >= childIndex; --i) {
        children[i + 1] = children[i];
    }

    children[childIndex] = child;
    count++;

    return true;
}

template<class K>
size_t InnerNode<K>::getKeyIndex(K key) const {
    size_t low = 0;
    size_t high = count;
    size_t mid = 0;

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
size_t LeafNode<K>::getKeyIndex(K key) const {
    size_t low = 0;
    size_t high = count;
    size_t mid = 0;

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

    size_t keyIndex = getKeyIndex(key);

    // add key in to keys array
    for (size_t i = count - 1; i >= keyIndex; --i) {
        keys[i + 1] = keys[i];
    }

    keys[keyIndex] = key;

    // add child into children array
    size_t recordIndex = keyIndex;

    for (size_t i = count - 1; i >= recordIndex; --i) {
        records[i + 1] = records[i];
    }

    records[recordIndex] = record;

    count++;
    return true;
}

template<class K>
Record LeafNode<K>::getRecord(K key) const {
    // search for key
    size_t low = 0;
    size_t high = count - 1;
    size_t mid = 0;

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
