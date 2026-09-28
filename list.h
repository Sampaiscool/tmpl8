#pragma once

namespace Tmpl8
{
    /*
    A hand written replacement for std::vector, written out by hand for the
    assignment so the new[]/delete[] and the doubling strategy stay visible
    instead of being hidden inside the STL.

    m_Data     -> the heap block holding the elements
    m_Capacity -> how many elements FIT in that block
    m_Count    -> how many are actually in use
    */
    template<typename T>
    class List
    {
    private:
        T* m_Data;
        int m_Capacity;
        int m_Count;

        // A heap block cannot grow in place, so allocate a bigger one, copy
        // everything across and free the old one. Any pointer into the old
        // block is dangling after this, the elements physically moved.
        void Reallocate(int newCapacity)
        {
            T* newBlock = new T[newCapacity];
            for (int i = 0; i < m_Count; ++i) newBlock[i] = m_Data[i];

            delete[] m_Data;
            m_Data = newBlock;
            m_Capacity = newCapacity;
        }

    public:
        // Members are initialised first because Reallocate does delete[] m_Data,
        // and deleting an uninitialised pointer would crash.
        List() : m_Data(nullptr), m_Capacity(0), m_Count(0)
        {
            Reallocate(4);
        }

        ~List()
        {
            delete[] m_Data;
            m_Data = nullptr;
        }

        /*
        Rule of Three. The compiler's default copy would copy the POINTER, so
        two Lists would share one block and both destructors would free it.
        These two do a deep copy instead, giving each List its own memory.
        */
        List(const List& other) : m_Data(nullptr), m_Capacity(0), m_Count(0)
        {
            m_Capacity = other.m_Capacity;
            m_Count = other.m_Count;
            m_Data = new T[m_Capacity];
            for (int i = 0; i < m_Count; ++i) m_Data[i] = other.m_Data[i];
        }

        List& operator=(const List& other)
        {
            if (this != &other) // without this, a = a frees the data it then copies
            {
                delete[] m_Data;

                m_Capacity = other.m_Capacity;
                m_Count = other.m_Count;
                m_Data = new T[m_Capacity];
                for (int i = 0; i < m_Count; ++i) m_Data[i] = other.m_Data[i];
            }
            return *this;
        }

        // Doubling rather than growing by one: copying everything is expensive,
        // so it has to happen less often the bigger the list gets.
        void push_back(const T& element)
        {
            if (m_Count >= m_Capacity) Reallocate(m_Capacity * 2);
            m_Data[m_Count++] = element;
        }

        // Keeps the block so refilling does not have to allocate again.
        void clear() { m_Count = 0; }

        int size() const { return m_Count; }
        bool empty() const { return m_Count == 0; }

        // No bounds checking, same as a raw array.
        T& operator[](int index) { return m_Data[index]; }
        const T& operator[](int index) const { return m_Data[index]; }

        // Enough for a range-for: for (const Collider& c : colliders)
        T* begin() { return m_Data; }
        T* end() { return m_Data + m_Count; }
        const T* begin() const { return m_Data; }
        const T* end() const { return m_Data + m_Count; }
    };
}
