#pragma once

template <typename T, std::size_t Capacity>
struct RingBuffer {
    static constexpr std::size_t kCapacity = Capacity;

    T _data[Capacity];
    size_t _head = 0; // read starts from here
    size_t _tail = 0; // write starts from here
    size_t _count = 0;

    T* Push();
    void Pop();
    void Clear();
    T* operator[](std::size_t i);
    T const* operator[](std::size_t i) const;
};

template<typename T, std::size_t Capacity>
T* RingBuffer<T, Capacity>::Push() {
    if (_count >= Capacity) {
        return nullptr;
    }
    std::size_t nextTail = _tail + 1;
    if (nextTail >= Capacity) {
        nextTail = 0;
    }
    T& d = _data[_tail];
    _tail = nextTail;
    ++_count;
    return &d;
}

template<typename T, std::size_t Capacity>
void RingBuffer<T, Capacity>::Pop() {
    if (_count == 0) {
        return;
    }
    --_count;
    ++_head;
    if (_head >= Capacity) {
        _head = 0;
    }
}

template<typename T, std::size_t Capacity>
void RingBuffer<T, Capacity>::Clear() {
    _tail = _head;
    _count = 0;
}


template<typename T, std::size_t Capacity>
T* RingBuffer<T, Capacity>::operator[](std::size_t i) {
    if (_count == 0) {
        return nullptr;
    }
    if (i >= _count) {
        return nullptr;
    }

    int ix = _head + i;
    if (ix >= Capacity) {
        ix -= Capacity;
    }
    return &_data[ix];
}

template<typename T, std::size_t Capacity>
T const* RingBuffer<T, Capacity>::operator[](std::size_t i) const {
    if (_count == 0) {
        return nullptr;
    }
    if (i >= _count) {
        return nullptr;
    }

    int ix = _head + i;
    if (ix >= Capacity) {
        ix -= Capacity;
    }
    return &_data[ix];
}