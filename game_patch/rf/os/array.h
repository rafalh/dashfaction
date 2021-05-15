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
        [[nodiscard]] int size() const
        {
            return num;
        }

        [[nodiscard]] T& operator[](int index)
        {
            return elements[index];
        }

        [[nodiscard]] const T& operator[](int index) const
        {
            return elements[index];
        }

        [[nodiscard]] T& get(int index) const
        {
            return elements[index];
        }

        [[nodiscard]] T* begin()
        {
            return &elements[0];
        }

        [[nodiscard]] const T* begin() const
        {
            return &elements[0];
        }

        [[nodiscard]] T* end()
        {
            return &elements[num];
        }

        [[nodiscard]] const T* end() const
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
        [[nodiscard]] int size() const
        {
            return num;
        }

        [[nodiscard]] T& operator[](int index)
        {
            return elements[index];
        }

        [[nodiscard]] const T& operator[](int index) const
        {
            return elements[index];
        }

        [[nodiscard]] T& get(int index) const
        {
            return elements[index];
        }

        [[nodiscard]] T* begin()
        {
            return &elements[0];
        }

        [[nodiscard]] const T* begin() const
        {
            return &elements[0];
        }

        [[nodiscard]] T* end()
        {
            return &elements[num];
        }

        [[nodiscard]] const T* end() const
        {
            return &elements[num];
        }
    };
}
