#pragma once

#include <iterator>

template<typename T, T* T::*NEXT_PTR = &T::next>
class SinglyLinkedList
{
    T* m_list;

public:
    class Iterator
    {
        T* current;
        T* first;

    public:
        using self_type = Iterator;
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int;

        Iterator(pointer first) :
            current(first), first(first)
        {}

        Iterator() = default;

        self_type& operator++()
        {
            current = current->*NEXT_PTR;
            if (current == first) {
                current = nullptr;
            }
            return *this;
        }

        self_type operator++([[maybe_unused]] int junk)
        {
            const self_type copy = *this;
            ++*this;
            return copy;
        }

        bool operator==(const self_type& rhs) const
        {
            return current == rhs.current;
        }

        reference operator*() const
        {
            return *current;
        }

        pointer operator->() const
        {
            return *current;
        }
    };

    SinglyLinkedList(T* list) : m_list(list)
    {}

    Iterator begin()
    {
        return Iterator(m_list);
    }

    Iterator end()
    {
        return Iterator(nullptr);
    }
};

template<typename T, T* T::*NEXT_PTR = &T::next, T* T::*PREV_PTR = &T::prev>
class DoublyLinkedList
{
    T* m_list;

public:
    class Iterator
    {
        T* current;

    public:
        using self_type = Iterator;
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int;

        Iterator(pointer current) :
            current(current)
        {}

        Iterator() = default;

        self_type& operator++()
        {
            current = current->*NEXT_PTR;
            return *this;
        }

        self_type& operator--()
        {
            current = current->*PREV_PTR;
            return *this;
        }

        self_type operator++([[maybe_unused]] int junk)
        {
            self_type copy = *this;
            ++*this;
            return copy;
        }

        bool operator==(const self_type& rhs) const = default;

        reference operator*() const
        {
            return *current;
        }

        pointer operator->() const
        {
            return *current;
        }
    };

    DoublyLinkedList(T& list) : m_list(&list)
    {}

    Iterator begin()
    {
        auto next = m_list->next ? m_list->next : m_list;
        return Iterator(next);
    }

    Iterator end()
    {
        return Iterator(&m_list);
    }
};
