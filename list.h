#pragma once

namespace Tmpl8
{
    template<typename T>
    class List
    {
    private:
        T* m_Data;
        int m_Capacity;
        int m_Count;

        void Reallocate(int newCapacity)
        {
            T* newBlock = new T[newCapacity];
            for (int i = 0; i < m_Count; ++i)
            {
                newBlock[i] = m_Data[i];
            }
            delete[] m_Data;
            m_Data = newBlock;
            m_Capacity = newCapacity;
        }

    public:
        List() : m_Data(nullptr), m_Capacity(0), m_Count(0)
        {
            Reallocate(4);
        }

        ~List()
        {
            delete[] m_Data;
            m_Data = nullptr;
        }

        // Rule of Three: Copy Constructor (Voorkomt double free bij kopieën)
        List(const List& other) : m_Data(nullptr), m_Capacity(0), m_Count(0)
        {
            m_Capacity = other.m_Capacity;
            m_Count = other.m_Count;
            m_Data = new T[m_Capacity];
            for (int i = 0; i < m_Count; ++i)
            {
                m_Data[i] = other.m_Data[i];
            }
        }

        // Rule of Three: Copy Assignment Operator
        List& operator=(const List& other)
        {
            if (this != &other)
            {
                delete[] m_Data;
                m_Capacity = other.m_Capacity;
                m_Count = other.m_Count;
                m_Data = new T[m_Capacity];
                for (int i = 0; i < m_Count; ++i)
                {
                    m_Data[i] = other.m_Data[i];
                }
            }
            return *this;
        }

        void push_back(const T& element)
        {
            if (m_Count >= m_Capacity)
            {
                Reallocate(m_Capacity * 2);
            }
            m_Data[m_Count++] = element;
        }

        void clear()
        {
            m_Count = 0;
        }

        int size() const { return m_Count; }
        bool empty() const { return m_Count == 0; }

        T& operator[](int index) { return m_Data[index]; }
        const T& operator[](int index) const { return m_Data[index]; }
    };
}
