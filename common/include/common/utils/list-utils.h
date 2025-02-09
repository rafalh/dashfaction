#pragma once

#include <iterator>

template<typename T, T* const T::*NEXT_PTR = &T::next>
class SinglyLinkedList
{
    std::reference_wrapper<T*> m_list;

public:
    class Iterator
    {
        T* current;
        const T* first;

    public:
        using self_type = Iterator;
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int;

        Iterator() :
            Iterator{nullptr}
        {}

        Iterator(const pointer first) :
            current{first},
            first{first}
        {}

        self_type& operator++()
        {
            current = current->*NEXT_PTR;
            if (current == first) {
                current = nullptr;
            }
            return *this;
        }

        self_type operator++([[maybe_unused]] const int junk)
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

    SinglyLinkedList(T*& list) :
        m_list{list}
    {}

    Iterator begin()
    {
        return Iterator{m_list};
    }

    Iterator end()
    {
        return Iterator{nullptr};
    }
};

template<typename T, T* const T::*NEXT_PTR = &T::next, T* const T::*PREV_PTR = &T::prev>
class DoublyLinkedList
{
    std::reference_wrapper<T> m_list;

public:
    class Iterator
    {
        T* current;

    public:
        using self_type = Iterator;
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = int;

        Iterator() :
            Iterator{nullptr}
        {}

        Iterator(const pointer current) :
            current{current}
        {}

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

        self_type operator--([[maybe_unused]] const int junk) {
            const self_type copy = *this;
            --*this;
            return copy;
        }

        self_type operator++([[maybe_unused]] const int junk)
        {
            const self_type copy = *this;
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

    DoublyLinkedList(T& list) :
        m_list{list}
    {}

    Iterator begin()
    {
        T* const next = m_list.get().next;
        if (next) {
            return Iterator{next};
        } else {
            return end();
        }
    }

    Iterator end()
    {
        return Iterator{&m_list.get()};
    }
};
