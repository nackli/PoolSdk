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
#ifndef __PLATFORM_LOCK_FREE_CIRCULAR_QUEUE_H_
#define __PLATFORM_LOCK_FREE_CIRCULAR_QUEUE_H_
#include <atomic>
#include <memory>

template <typename T>
class LockFreeCircularQue {
private:
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next;

        Node() : next(nullptr) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;
    std::atomic<size_t> capacity;
    std::atomic<size_t> size;

public:
    explicit LockFreeCircularQue(size_t capacity)
        : capacity(capacity), size(0) {
        Node* dummy = new Node();
        head.store(dummy);
        tail.store(dummy);
    }

    ~LockFreeCircularQue() {
        while (Node* const old_head = head.load()) {
            head.store(old_head->next);
            delete old_head;
        }
    }

    bool enqueue(T new_value) {
        if (size.load() >= capacity.load()) {
            return false; // ��������
        }

        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
        Node* new_node = new Node();
        Node* old_tail = tail.load();

        old_tail->data.swap(new_data);
        old_tail->next.store(new_node);
        tail.store(new_node);

        size.fetch_add(1);
        return true;
    }

    bool dequeue(T& value) {
        Node* old_head = head.load();
        Node* next_node = old_head->next.load();

        if (!next_node) {
            return false; // ����Ϊ��
        }

        value = std::move(*(next_node->data));
        head.store(next_node);
        delete old_head;

        size.fetch_sub(1);
        return true;
    }

    bool is_empty() const {
        return size.load() == 0;
    }

    bool is_full() const {
        return size.load() >= capacity.load();
    }

    size_t get_size() const {
        return size.load();
    }

    size_t get_capacity() const {
        return capacity.load();
    }
};
#endif