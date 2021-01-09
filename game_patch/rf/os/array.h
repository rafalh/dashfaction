#pragma once

namespace rf
{
    template<typename T = char>
    class VArray
    {
    private:
        int num = 0;
        int capacity = 0;
        T *elements = nullptr;

    public:
        int size() const
        {
            return num;
        }

        T& operator[](int index) const
        {
            return elements[index];
        }

        T& get(int index) const
        {
            return elements[index];
        }

        void add(T element);
    };
    static_assert(sizeof(VArray<>) == 0xC);

    template<typename T, int N>
    class FArray
    {
        int num;
        T elements[N];

    public:
        int size() const
        {
            return num;
        }

        T& get(int index) const
        {
            return elements[index];
        }
    };

    template<typename T>
    class VList
    {
        T* head;
        int num_elements;

    public:
        T* first() const
        {
            return head;
        }

        int size() const
        {
            return num_elements;
        }

        bool empty() const
        {
            return num_elements == 0;
        }
    };
}
