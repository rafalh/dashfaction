#pragma once

#include <iterator>

template<typename T, T* T::*NEXT_PTR = &T::next>
class SinglyLinkedList
{
    T*& m_list;

public:
    class Iterator
    {
        T* current;
        T* first;

    public:
        typedef Iterator self_type;
        typedef T value_type;
        typedef T& reference;
        typedef T* pointer;
        typedef std::forward_iterator_tag iterator_category;
        typedef int difference_type;

        Iterator(pointer first) :
            current(first), first(first)
        {}

        self_type operator++()
        {
            current = current->*NEXT_PTR;
            if (current == first) {
                current = nullptr;
            }
            return *this;
        }

        self_type operator++(int junk)
        {
            self_type copy = *this;
            ++copy;
            return copy;
        }

        bool operator==(const self_type& rhs) const
        {
            return current == rhs.current;
        }

        bool operator!=(const self_type& rhs) const
        {
            return !(*this == rhs);
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

    SinglyLinkedList(T*& list) : m_list(list)
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
    T& m_list;

public:
    class Iterator
    {
        T* current;

    public:
        typedef Iterator self_type;
        typedef T value_type;
        typedef T& reference;
        typedef T* pointer;
        typedef std::forward_iterator_tag iterator_category;
        typedef int difference_type;

        Iterator(pointer current) :
            current(current)
        {}

        self_type operator++()
        {
            current = current->*NEXT_PTR;
            return *this;
        }

        self_type operator--()
        {
            current = current->*PREV_PTR;
            return *this;
        }

        self_type operator++(int junk)
        {
            self_type copy = *this;
            ++copy;
            return copy;
        }

        bool operator==(const self_type& rhs) const
        {
            return current == rhs.current;
        }

        bool operator!=(const self_type& rhs) const
        {
            return !(*this == rhs);
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

    DoublyLinkedList(T& list) : m_list(list)
    {}

    Iterator begin()
    {
        return Iterator(m_list.next);
    }

    Iterator end()
    {
        return Iterator(&m_list);
    }
};
