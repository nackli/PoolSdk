 /***************************************************************************************************************************************************/
 /*
 * @Author: Nack Li
 * @version 1.0
 * @copyright 2025 nackli. All rights reserved.
 * @License: MIT (https://opensource.org/licenses/MIT).
 * @Date: 2025-08-29
 * @LastEditTime: 2025-08-29
 */
 /***************************************************************************************************************************************************/
#pragma once
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <vector>
#include <string>

template <typename K, typename V>
class SkipListKVNode {
    typedef K                                       key_type;
    typedef V                                       value_type;
    typedef SkipListKVNode<key_type, value_type>    node_type;
    typedef std::vector<std::shared_ptr<node_type>> node_vtype;
public:
    key_type key;
    value_type value;
    node_vtype forward;

    SkipListKVNode(key_type k, value_type v, int level)
        : key(k), value(v), forward(level, nullptr) {}
};

template <typename K, typename V>
class SkipListKv {
    typedef K                                       key_type;
    typedef V                                       value_type;
    typedef SkipListKVNode<key_type, value_type>    node_type;
private:
    int m_iMaxLevel;  //number of layers
    int m_iCurLevel;  // Current level
    std::shared_ptr<node_type> m_pHeader;  // head
    std::mt19937 m_genRand;  // Random Number Generator
    std::uniform_real_distribution<> m_disReal;  // uniform distribution

    // Randomly generate node layers
    int random_level() {
        int level = 1;
        while (m_disReal(m_genRand) < 0.5 && level < m_iMaxLevel) {
            level++;
        }
        return level;
    }

public:
    SkipListKv(int max_level = 16)
        : m_iMaxLevel(max_level), m_iCurLevel(1),
        m_genRand(std::random_device{}()), m_disReal(0, 1) {
        m_pHeader = std::make_shared<node_type>(key_type(), value_type(), m_iMaxLevel);
    }

    // Insert key value pairs
    void insert(const key_type& key, const value_type& value) {
        std::vector<std::shared_ptr<node_type>> update(m_iMaxLevel);
        auto current = m_pHeader;

        //Starting from the highest level, search for the insertion position
        for (int i = m_iCurLevel - 1; i >= 0; i--) 
        {
            while (current->forward[i] != nullptr && current->forward[i]->key < key) 
                current = current->forward[i];
    
            update[i] = current;
        }

        current = current->forward[0];

        // If the key already exists, update the value
        if (current != nullptr && current->key == key) {
            current->value = value;
            return;
        }

        // Randomly determine the number of layers for a new node
        int level = random_level();

        /* If the number of layers of the new node is greater than the current number of layers,
        update the update array and the current number of layers
        */
        if (level > m_iCurLevel) {
            for (int i = m_iCurLevel; i < level; i++) {
                update[i] = m_pHeader;
            }
            m_iCurLevel = level;
        }

        // Create a new node
        auto new_node = std::make_shared<node_type>(key, value, level);

        // Insert new node
        for (int i = 0; i < level; i++) {
            new_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = new_node;
        }
    }

    // Search for the value corresponding to the key
    bool find(const key_type& key, value_type& value) {
        auto current = m_pHeader;

        // Start searching from the highest level
        for (int i = m_iCurLevel - 1; i >= 0; i--) {
            while (current->forward[i] != nullptr && current->forward[i]->key < key) {
                current = current->forward[i];
            }
        }

        current = current->forward[0];

        if (current != nullptr && current->key == key) {
            value = current->value;
            return true;
        }
        return false;
    }

    // delete key
    bool erase(const key_type& key) {
        std::vector<std::shared_ptr<node_type>> update(m_iMaxLevel);
        auto current = m_pHeader;

        // Starting from the highest level, search for the node to be deleted
        for (int i = m_iCurLevel - 1; i >= 0; i--) {
            while (current->forward[i] != nullptr && current->forward[i]->key < key) {
                current = current->forward[i];
            }
            update[i] = current;
        }

        current = current->forward[0];

        if (current == nullptr || current->key != key) {
            return false;
        }

        // Remove the node from each layer
        for (int i = 0; i < m_iCurLevel; i++) {
            if (update[i]->forward[i] != current) {
                break;
            }
            update[i]->forward[i] = current->forward[i];
        }

        // If the deleted node is at the highest level, update the current level
        while (m_iCurLevel > 1 && m_pHeader->forward[m_iCurLevel - 1] == nullptr) {
            m_iCurLevel--;
        }

        return true;
    }

    // Get jump table size (rough estimate)
    size_t size() const {
        size_t count = 0;
        auto node = m_pHeader->forward[0];
        while (node != nullptr) {
            count++;
            node = node->forward[0];
        }
        return count;
    }
};
