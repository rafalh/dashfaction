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

        T& operator[](int index)
        {
            return elements[index];
        }

        const T& operator[](int index) const
        {
            return elements[index];
        }

        T& get(int index) const
        {
            return elements[index];
        }

        T* begin()
        {
            return &elements[0];
        }

        const T* begin() const
        {
            return &elements[0];
        }

        T* end()
        {
            return &elements[num];
        }

        const T* end() const
        {
            return &elements[num];
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

        T& operator[](int index)
        {
            return elements[index];
        }

        const T& operator[](int index) const
        {
            return elements[index];
        }

        T& get(int index) const
        {
            return elements[index];
        }

        T* begin()
        {
            return &elements[0];
        }

        const T* begin() const
        {
            return &elements[0];
        }

        T* end()
        {
            return &elements[num];
        }

        const T* end() const
        {
            return &elements[num];
        }
    };
}
