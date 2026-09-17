#pragma once

namespace Tmpl8
{
    /*
    This code:
    A hand written replacement for std::vector. It is a dynamic array: a
    single block of memory on the heap that grows automaticly when it runs
    full, so you dont have to know upfront how many elements you will store.

    Why a template?
    template<typename T> means the compiler generates a seperate version of
    this class for every type you use it with (List<int>, List<TileChunk>...).
    So we get one implementation that works for all types without losing any
    speed, because it is all resolved at compile time instead of runtime.

    Why write it ourselves instead of using std::vector?
    For the assignment, and because it makes the memory management visible:
    you can actually see the new[]/delete[] and the doubling strategy that
    std::vector normally hides from you.

    The three members:
    m_Data     -> pointer to the heap block holding the elements
    m_Capacity -> how many elements FIT in that block
    m_Count    -> how many elements are actually IN use right now
    */
    template<typename T>
    class List
    {
    private:
        T* m_Data;       // the heap block, owned by this class
        int m_Capacity;  // how much room we allocated
        int m_Count;     // how much of that room is filled

        /*
        This code:
        Grows (or creates) the storage block.

        You cannot resize a heap allocation in place in C++, so we:
        1. allocate a brand new, bigger block
        2. copy every existing element over into it
        3. delete[] the old block so it does not leak
        4. point m_Data at the new block and remember the new capacity

        Important: every pointer or reference you had to an old element is
        dead after this, because the elements physicaly moved to a different
        address in memory.
        */
        void Reallocate(int newCapacity)
        {
            T* newBlock = new T[newCapacity]; // fresh, bigger block

            // copy the old elements into the new block
            for (int i = 0; i < m_Count; ++i)
            {
                newBlock[i] = m_Data[i];
            }

            delete[] m_Data;          // give the old block back to the heap
            m_Data = newBlock;
            m_Capacity = newCapacity;
        }

    public:
        /*
        This code:
        Starts the list empty and immediately reserves room for 4 elements, so
        the first few push_backs do not each trigger a reallocation.
        The member initializer list sets everything to a safe value first,
        because Reallocate does delete[] m_Data and deleting a random
        uninitialized pointer would crash.
        */
        List() : m_Data(nullptr), m_Capacity(0), m_Count(0)
        {
            Reallocate(4);
        }

        /*
        This code:
        The destructor gives the whole block back to the heap. delete[] (with
        the brackets!) is required because we allocated with new[]. Using
        plain delete on an array is undefined behavior.
        Setting the pointer to nullptr afterwards is not strictly needed here,
        but it makes a dangling pointer impossible.
        */
        ~List()
        {
            delete[] m_Data;
            m_Data = nullptr;
        }

        /*
        This code:
        Rule of Three, part 2: the copy constructor.

        Without this, C++ would generate a default copy that just copies the
        m_Data POINTER. Then two Lists would point at the same heap block and
        both destructors would delete it -> double free -> crash.

        So instead we do a deep copy: allocate our own block of the same size
        and copy every element into it. Now both lists own seperate memory.
        */
        // Rule of Three: Copy Constructor (Voorkomt double free bij kopieen)
        List(const List& other) : m_Data(nullptr), m_Capacity(0), m_Count(0)
        {
            m_Capacity = other.m_Capacity;
            m_Count = other.m_Count;
            m_Data = new T[m_Capacity]; // our own block, not shared
            for (int i = 0; i < m_Count; ++i)
            {
                m_Data[i] = other.m_Data[i];
            }
        }

        /*
        This code:
        Rule of Three, part 3: the copy assignment operator. Same deep copy
        idea as above, but for a list that already exists (a = b).

        Two extra details:
        - the 'this != &other' check guards against self assignment (a = a).
          Without it we would delete our own data and then copy from the block
          we just freed.
        - we delete[] our old block first, otherwise the memory it used stays
          allocated with nothing pointing at it, wich is a leak.
        It returns *this by reference so you can chain assignments (a = b = c).
        */
        // Rule of Three: Copy Assignment Operator
        List& operator=(const List& other)
        {
            if (this != &other) // self assignment guard
            {
                delete[] m_Data; // drop our current block so it does not leak

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

        /*
        This code:
        Adds one element to the end of the list.

        The growth strategy:
        When the block is full we DOUBLE the capacity instead of adding 1.
        Copying everything over is expensive, so doubling makes it happen
        less and less often as the list grows. Spread over many push_backs
        the cost per element stays roughly constant.

        m_Data[m_Count++] writes at the first free slot and bumps the counter
        in the same statement.
        */
        void push_back(const T& element)
        {
            if (m_Count >= m_Capacity)
            {
                Reallocate(m_Capacity * 2); // full, so double the room
            }
            m_Data[m_Count++] = element;
        }

        /*
        This code:
        Empties the list by only resetting the counter to 0. The heap block
        itself is kept, so refilling the list afterwards does not have to
        allocate again. The old values are still physicaly in memory but they
        are unreachable and get overwritten by the next push_backs.
        */
        void clear()
        {
            m_Count = 0;
        }

        // How many elements are stored (not the capacity)
        int size() const { return m_Count; }
        bool empty() const { return m_Count == 0; }

        /*
        This code:
        Index access, so a List can be used exactly like a normal array
        (myList[3]). Both versions return a REFERENCE so no copy is made.

        Why two versions?
        The first one is used on a normal list and lets you change the element.
        The second (const) one is used on a const List and only lets you read
        it. Without the const version you could not even read from a list that
        was passed as 'const List&'.

        Note: there is no bounds checking here, an index outside the list is
        undefined behavior, same as with a raw array.
        */
        T& operator[](int index) { return m_Data[index]; }
        const T& operator[](int index) const { return m_Data[index]; }
    };
}
