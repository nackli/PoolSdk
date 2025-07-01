#pragma once
#ifndef __PLATFORM_LOCK_FREE_STACK_H_
#define __PLATFORM_LOCK_FREE_STACK_H_

template<typename T>
class LockFreeStack {
    typedef T           value_type;
    struct Node {
        value_type data;
        Node* next;
    };
    std::atomic<Node*> head{ nullptr };

public:
    void push(const value_type& value) {
        Node* new_node = new Node{ value, head.load() };
        while (!head.compare_exchange_strong(new_node->next, new_node));
    }

    bool pop(value_type& value) {
        Node* old_head = head.load();
        while (old_head &&
            !head.compare_exchange_strong(old_head, old_head->next)) {
        }
        if (!old_head) 
            return false;

        value = old_head->data;
        delete old_head;
        return true;
    }
};
#endif
