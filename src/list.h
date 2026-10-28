#pragma once

namespace Tmpl8
{
    // A hand written std::vector, written out for the assignment so the
    // new[]/delete[] and the doubling stay visible.
    template<typename T>
    class List
    {
    private:
        T* m_Data;
        int m_Capacity; // how many elements fit in the block
        int m_Count;    // how many are in use

        // A heap block cannot grow in place. 
        // So allocate a bigger one and copy any pointer into the old block dangles after this.
        void Reallocate(int newCapacity)
        {
            T* newBlock = new T[newCapacity];
            for (int i = 0; i < m_Count; ++i) newBlock[i] = m_Data[i];

            delete[] m_Data;
            m_Data = newBlock;
            m_Capacity = newCapacity;
        }

    public:
        // Initialised first, because Reallocate does delete[] m_Data.
        List() : m_Data(nullptr), m_Capacity(0), m_Count(0)
        {
            Reallocate(4);
        }

        ~List()
        {
            delete[] m_Data;
            m_Data = nullptr;
        }

        // Rule of Three. The default copy would share one block between two
        // Lists and both destructors would free it.
        List(const List& other) : m_Data(nullptr), m_Capacity(0), m_Count(0)
        {
            m_Capacity = other.m_Capacity;
            m_Count = other.m_Count;
            m_Data = new T[m_Capacity];
            for (int i = 0; i < m_Count; ++i) m_Data[i] = other.m_Data[i];
        }

        List& operator=(const List& other)
        {
            if (this != &other) // without this, a = a frees what it then copies
            {
                delete[] m_Data;

                m_Capacity = other.m_Capacity;
                m_Count = other.m_Count;
                m_Data = new T[m_Capacity];
                for (int i = 0; i < m_Count; ++i) m_Data[i] = other.m_Data[i];
            }
            return *this;
        }

        // Doubling, so the copy happens less often as the list grows.
        void push_back(const T& element)
        {
            if (m_Count >= m_Capacity) Reallocate(m_Capacity * 2);
            m_Data[m_Count++] = element;
        }

        void clear() { m_Count = 0; }

        // Swaps the last element into the hole, so the ORDER changes. Removing
        // while looping must not advance the index:
        //     for (int i = 0; i < list.size(); )
        //         if (dead) list.remove_at_swap(i); else ++i;
        void remove_at_swap(int index)
        {
            m_Data[index] = m_Data[--m_Count];
        }

        int size() const { return m_Count; }
        bool empty() const { return m_Count == 0; }

        // No bounds checking, same as a raw array.
        T& operator[](int index) { return m_Data[index]; }
        const T& operator[](int index) const { return m_Data[index]; }

        T* begin() { return m_Data; }
        T* end() { return m_Data + m_Count; }
        const T* begin() const { return m_Data; }
        const T* end() const { return m_Data + m_Count; }
    };
}
