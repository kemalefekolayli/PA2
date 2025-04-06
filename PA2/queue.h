#ifndef QUEUE_H
#define QUEUE_H

#include <iostream>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <thread>
#include <cassert>

using namespace std;

//  enqueue(), dequeue(), isEmpty(), and print()

template <typename T>
struct Node {
    T value;
    Node<T>* next;
};

template <typename T>
class Queue {
    private:
    Node<T>* head;
    Node<T>* tail;
    pthread_mutex_t head_lock, tail_lock;
    public:
        
    Queue() {
        Node<T>* sentinel = new Node<T>();
        sentinel->next = nullptr;
        head = tail = sentinel;
        pthread_mutex_init(&head_lock, nullptr);
        pthread_mutex_init(&tail_lock, nullptr);
    }

    void enqueue(T item) {
        Node<T>* newNode = nullptr;
        try {
            newNode = new Node<T>();
            newNode->value = item;
            newNode->next = nullptr;
        } 
        catch (const std::bad_alloc& e) {
            std::cerr << "Memory allocation failed: " << e.what() << std::endl;
            return;
        }
        
        pthread_mutex_lock(&tail_lock);
        tail->next = newNode;
        tail = newNode;
        pthread_mutex_unlock(&tail_lock);
    }
    T dequeue() {
        pthread_mutex_lock(&head_lock);
        Node<T>* temp = head;
        Node<T>* new_head = temp->next;
        
        if(new_head == nullptr) {
            pthread_mutex_unlock(&head_lock);
            throw std::runtime_error("Queue is empty");
        }
        
        T value = new_head->value;
        head = new_head;
        pthread_mutex_unlock(&head_lock);
        
        delete temp;
        return value;
    }
    bool isEmpty() {
        return (head->next == nullptr);
    }
    void print() {
        pthread_mutex_lock(&head_lock);
        
        if (head->next == nullptr) {
            std::cout << "Empty\n";
        } else {
            Node<T>* temp = head->next;
            while (temp != nullptr) {
                std::cout << temp->value << " ";
                temp = temp->next;
            }
            std::cout << std::endl;
        }
        
        pthread_mutex_unlock(&head_lock);
    }
    ~Queue() {
        // Free all nodes
        Node<T>* current = head;
        while (current != nullptr) {
            Node<T>* temp = current;
            current = current->next;
            delete temp;
        }
        
        // Destroy mutex locks
        pthread_mutex_destroy(&head_lock);
        pthread_mutex_destroy(&tail_lock);
    }
};