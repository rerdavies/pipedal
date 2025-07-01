/*
 *   Copyright (c) 2025 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */
/*
 *   Copyright (c) 2025 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */
#pragma once 

#include <unordered_map>
#include <list>
#include <stdexcept>

template <typename KEY, typename VALUE>
class LRUCache {
public:
    struct CacheNode {
        KEY key;
        VALUE value;
        CacheNode(KEY k, VALUE v) : key(k), value(v) {}
    };
private:
    size_t capacity;
    std::list<CacheNode> cache_list; // Doubly-linked list for LRU order
    std::unordered_map<KEY, typename std::list<CacheNode>::iterator> cache_map; // Map key to list iterator

public:
    explicit LRUCache(size_t cap) : capacity(cap) {
        if (cap == 0) {
            throw std::invalid_argument("Cache capacity must be greater than 0");
        }
    }

    // Get value by key, return false if not found
    bool get(const KEY& key, VALUE& value) {
        auto it = cache_map.find(key);
        if (it == cache_map.end()) {
            return false;
        }
        // Move to front (most recently used)
        cache_list.splice(cache_list.begin(), cache_list, it->second);
        value = it->second->value;
        return true;
    }

    // Put key-value pair into cache
    void put(const KEY& key, const VALUE& value) {
        auto it = cache_map.find(key);
        if (it != cache_map.end()) {
            // Update existing key
            cache_list.splice(cache_list.begin(), cache_list, it->second);
            it->second->value = value;
            return;
        }

        // Add new key-value pair
        cache_list.emplace_front(key, value);
        cache_map[key] = cache_list.begin();

        // Evict least recently used if over capacity
        if (cache_map.size() > capacity) {
            auto lru = cache_list.back();
            cache_map.erase(lru.key);
            cache_list.pop_back();
        }
    }

    // Check if key exists
    bool contains(const KEY& key) const {
        return cache_map.find(key) != cache_map.end();
    }

    // Get current size
    size_t size() const {
        return cache_map.size();
    }

    // Get capacity
    size_t get_capacity() const {
        return capacity;
    }
    const std::list<CacheNode> &cache() const {
        return cache_list;
    }
};

