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

using namespace std;

enum class Color { TREE_RED, TREE_BLACK };

template <typename T>
struct Node {
    typedef T                                       value_type;
    typedef Node<value_type>                        node_type;
    typedef shared_ptr<node_type>                   shared_node_type;
    value_type data;
    Color color;
    shared_node_type left;
    shared_node_type right;
    shared_node_type parent;

    Node(value_type val) : data(val), color(Color::TREE_RED),
        left(nullptr), right(nullptr), parent(nullptr) {}
};

template <typename T>
class RedBlackTree {
    typedef T                                       value_type;
    typedef Node<value_type>                        node_type;
    typedef shared_ptr<node_type>                   shart_node_type;
public:
    RedBlackTree() {
        nil = make_shared<node_type>(0);
        nil->color = Color::TREE_BLACK;
        root = nil;
    }

    void insert(value_type key)
    {
        auto z = make_shared<node_type>(key);
        auto y = nil;
        auto x = root;

        while (x != nil) {
            y = x;
            if (z->data < x->data) {
                x = x->left;
            }
            else {
                x = x->right;
            }
        }

        z->parent = y;

        if (y == nil) {
            root = z;
        }
        else if (z->data < y->data) {
            y->left = z;
        }
        else {
            y->right = z;
        }

        z->left = nil;
        z->right = nil;
        z->color = Color::TREE_RED;

        insertFixup(z);
    }

    void remove(value_type key)
    {
        auto z = root;
        while (z != nil) {
            if (z->data == key) {
                break;
            }
            else if (key < z->data) {
                z = z->left;
            }
            else {
                z = z->right;
            }
        }

        if (z == nil) {
            cout << "Key not found in the tree" << endl;
            return;
        }

        auto y = z;
        auto yOriginalColor = y->color;
        shart_node_type x;

        if (z->left == nil) {
            x = z->right;
            transplant(z, z->right);
        }
        else if (z->right == nil) {
            x = z->left;
            transplant(z, z->left);
        }
        else {
            y = minimum(z->right);
            yOriginalColor = y->color;
            x = y->right;

            if (y->parent == z) {
                x->parent = y;
            }
            else {
                transplant(y, y->right);
                y->right = z->right;
                y->right->parent = y;
            }

            transplant(z, y);
            y->left = z->left;
            y->left->parent = y;
            y->color = z->color;
        }

        if (yOriginalColor == Color::TREE_BLACK) {
            deleteFixup(x);
        }
    }

    shart_node_type search(value_type key)
    {
        auto current = root;
        while (current != nil) {
            if (current->data == key) {
                return current;
            }
            else if (key < current->data) {
                current = current->left;
            }
            else {
                current = current->right;
            }
        }
        return nullptr;
    }
    void printTree()
    {
        if (root != nil) {
            printHelper(root, "", true);
        }
        else {
            cout << "Tree is empty" << endl;
        }
    }
private:
    shart_node_type root;
    shart_node_type nil;  // �ڱ��ڵ㣬����NULL

    // ��������
    void leftRotate(shart_node_type x)
    {
        auto y = x->right;
        x->right = y->left;

        if (y->left != nil) {
            y->left->parent = x;
        }

        y->parent = x->parent;

        if (x->parent == nil) {
            root = y;
        }
        else if (x == x->parent->left) {
            x->parent->left = y;
        }
        else {
            x->parent->right = y;
        }

        y->left = x;
        x->parent = y;
    }

    void rightRotate(shart_node_type x)
    {
        auto y = x->left;
        x->left = y->right;

        if (y->right != nil) {
            y->right->parent = x;
        }

        y->parent = x->parent;

        if (x->parent == nil) {
            root = y;
        }
        else if (x == x->parent->right) {
            x->parent->right = y;
        }
        else {
            x->parent->left = y;
        }

        y->right = x;
        x->parent = y;
    }

    void insertFixup(shart_node_type z)
    {
        while (z->parent->color == Color::TREE_RED) {
            if (z->parent == z->parent->parent->left) {
                auto y = z->parent->parent->right;

                if (y->color == Color::TREE_RED) {
                    // Case 1
                    z->parent->color = Color::TREE_BLACK;
                    y->color = Color::TREE_BLACK;
                    z->parent->parent->color = Color::TREE_RED;
                    z = z->parent->parent;
                }
                else {
                    if (z == z->parent->right) {
                        // Case 2
                        z = z->parent;
                        leftRotate(z);
                    }
                    // Case 3
                    z->parent->color = Color::TREE_BLACK;
                    z->parent->parent->color = Color::TREE_RED;
                    rightRotate(z->parent->parent);
                }
            }
            else {
                // �Գ����
                auto y = z->parent->parent->left;

                if (y->color == Color::TREE_RED) {
                    z->parent->color = Color::TREE_BLACK;
                    y->color = Color::TREE_BLACK;
                    z->parent->parent->color = Color::TREE_RED;
                    z = z->parent->parent;
                }
                else {
                    if (z == z->parent->left) {
                        z = z->parent;
                        rightRotate(z);
                    }
                    z->parent->color = Color::TREE_BLACK;
                    z->parent->parent->color = Color::TREE_RED;
                    leftRotate(z->parent->parent);
                }
            }
        }
        root->color = Color::TREE_BLACK;
    }

    void transplant(shart_node_type u, shart_node_type v)
    {
        if (u->parent == nil) {
            root = v;
        }
        else if (u == u->parent->left) {
            u->parent->left = v;
        }
        else {
            u->parent->right = v;
        }
        v->parent = u->parent;
    }

    void deleteFixup(shart_node_type x)
    {
            while (x != root && x->color == Color::TREE_BLACK) {
                if (x == x->parent->left) {
                    auto w = x->parent->right;

                    if (w->color == Color::TREE_RED) {
                        // Case 1
                        w->color = Color::TREE_BLACK;
                        x->parent->color = Color::TREE_RED;
                        leftRotate(x->parent);
                        w = x->parent->right;
                    }

                    if (w->left->color == Color::TREE_BLACK && w->right->color == Color::TREE_BLACK) {
                        // Case 2
                        w->color = Color::TREE_RED;
                        x = x->parent;
                    }
                    else {
                        if (w->right->color == Color::TREE_BLACK) {
                            // Case 3
                            w->left->color = Color::TREE_BLACK;
                            w->color = Color::TREE_RED;
                            rightRotate(w);
                            w = x->parent->right;
                        }
                        // Case 4
                        w->color = x->parent->color;
                        x->parent->color = Color::TREE_BLACK;
                        w->right->color = Color::TREE_BLACK;
                        leftRotate(x->parent);
                        x = root;
                    }
                }
                else {
                    // �Գ����
                    auto w = x->parent->left;

                    if (w->color == Color::TREE_RED) {
                        w->color = Color::TREE_BLACK;
                        x->parent->color = Color::TREE_RED;
                        rightRotate(x->parent);
                        w = x->parent->left;
                    }

                    if (w->right->color == Color::TREE_BLACK && w->left->color == Color::TREE_BLACK) {
                        w->color = Color::TREE_RED;
                        x = x->parent;
                    }
                    else {
                        if (w->left->color == Color::TREE_BLACK) {
                            w->right->color = Color::TREE_BLACK;
                            w->color = Color::TREE_RED;
                            leftRotate(w);
                            w = x->parent->left;
                        }
                        w->color = x->parent->color;
                        x->parent->color = Color::TREE_BLACK;
                        w->left->color = Color::TREE_BLACK;
                        rightRotate(x->parent);
                        x = root;
                    }
                }
            }
            x->color = Color::TREE_BLACK;
    }

    shart_node_type minimum(shart_node_type node)
    {
        while (node->left != nil) {
            node = node->left;
        }
        return node;
    }

    void printHelper(shart_node_type node, string indent, bool last)
    {
        if (node != nil) {
            cout << indent;
            if (last) {
                cout << "R----";
                indent += "   ";
            }
            else {
                cout << "L----";
                indent += "|  ";
            }

            string color = node->color == Color::TREE_RED ? "RED" : "BLACK";
            cout << node->data << "(" << color << ")" << endl;
            printHelper(node->left, indent, false);
            printHelper(node->right, indent, true);
        }
    }
};

