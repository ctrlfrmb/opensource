/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2021 leiwei. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* This is free software: you are free to use, modify and distribute,
* but must retain the author's copyright notice and license terms.
*
* Author: leiwei E-mail: ctrlfrmb@gmail.com
* Version: v1.0.0
* Date: 2021-08-21
*----------------------------------------------------------------------------*/

/**
 * @file circular_buffer.h
 * @brief A C++17 compatible, fixed-size circular buffer with random access iterators.
 *
 * This header provides a template-based circular buffer (also known as a ring buffer)
 * that offers a fixed-capacity container. When the buffer is full, adding a new
 * element overwrites the oldest one. It is implemented using std::array for
 * cache-friendly, contiguous storage and provides a full suite of random access
 * iterators, making it compatible with C++ Standard Library algorithms.
 *
 * This implementation is NOT thread-safe. External synchronization is required
 * for concurrent access.
 */

#ifndef COMMON_CIRCULAR_BUFFER_H
#define COMMON_CIRCULAR_BUFFER_H

#include <cstddef>      // for std::size_t, std::ptrdiff_t
#include <array>        // for std::array
#include <algorithm>    // for std::copy, std::fill
#include <stdexcept>    // for std::out_of_range, std::logic_error
#include <iterator>     // for std::random_access_iterator_tag
#include <utility>      // for std::move
#include <type_traits>  // for std::conditional_t, std::is_const_v, std::remove_const_t

namespace Common {

// Forward declaration of the iterator class
template<typename T, std::size_t N>
class circular_buffer_iterator;

/**
 * @class circular_buffer
 * @brief A fixed-size circular buffer container.
 * @tparam T The type of elements stored in the buffer.
 * @tparam N The maximum capacity of the buffer. Must be greater than 0.
 */
template<typename T, std::size_t N>
class circular_buffer {
public:
    // --- Member Types ---
    static_assert(N > 0, "circular_buffer capacity must be greater than 0");

    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = circular_buffer_iterator<T, N>;
    using const_iterator = circular_buffer_iterator<const T, N>;

public:
    // --- Constructors ---

    /**
     * @brief Default constructor. Creates an empty buffer.
     */
    constexpr circular_buffer() = default;

    /**
     * @brief Constructs the buffer by copying elements from a C-style array.
     * The buffer will be full after construction.
     * @param values A C-style array of N elements to initialize the buffer with.
     */
    constexpr circular_buffer(const value_type(&values)[N])
        : size_(N), tail_(N > 0 ? N - 1 : 0) {
        std::copy(std::begin(values), std::end(values), data_.begin());
    }

    /**
     * @brief Constructs the buffer by filling it with N copies of a given value.
     * The buffer will be full after construction.
     * @param v The value to fill the buffer with.
     */
    constexpr circular_buffer(const_reference v)
        : size_(N), tail_(N > 0 ? N - 1 : 0) {
        std::fill(data_.begin(), data_.end(), v);
    }

    // --- Capacity ---

    /**
     * @brief Returns the number of elements currently in the buffer.
     * @return The number of elements.
     */
    constexpr size_type size() const noexcept { return size_; }

    /**
     * @brief Returns the maximum number of elements the buffer can hold.
     * @return The capacity N.
     */
    constexpr size_type capacity() const noexcept { return N; }

    /**
     * @brief Checks if the buffer is empty.
     * @return true if the buffer is empty, false otherwise.
     */
    constexpr bool empty() const noexcept { return size_ == 0; }

    /**
     * @brief Checks if the buffer is full.
     * @return true if the buffer is full, false otherwise.
     */
    constexpr bool full() const noexcept { return size_ == N; }

    // --- Modifiers ---

    /**
     * @brief Clears the contents of the buffer.
     */
    constexpr void clear() noexcept { size_ = 0; head_ = 0; tail_ = 0; }

    /**
     * @brief Adds an element to the back of the buffer.
     * If the buffer is full, the oldest element is overwritten.
     * @param value The value to be added (lvalue).
     */
    constexpr void push_back(const T& value) {
        if (empty()) {
            data_[tail_] = value;
            ++size_;
        } else {
            if (full()) {
                head_ = (head_ + 1) % N;
            } else {
                ++size_;
            }
            tail_ = (tail_ + 1) % N;
            data_[tail_] = value;
        }
    }

    /**
     * @brief Adds an element to the back of the buffer.
     * If the buffer is full, the oldest element is overwritten.
     * @param value The value to be added (rvalue).
     */
    constexpr void push_back(T&& value) {
        if (empty()) {
            data_[tail_] = std::move(value);
            ++size_;
        } else {
            if (full()) {
                head_ = (head_ + 1) % N;
            } else {
                ++size_;
            }
            tail_ = (tail_ + 1) % N;
            data_[tail_] = std::move(value);
        }
    }

    /**
     * @brief Adds a range of elements to the back of the buffer.
     * If the buffer overflows, older elements are overwritten.
     * @tparam InputIt The type of the input iterator.
     * @param first Iterator to the first element to add.
     * @param last Iterator to the element past the last element to add.
     */
    template<typename InputIt>
    void push_back_range(InputIt first, InputIt last) {
        for (; first != last; ++first) {
            push_back(*first);
        }
    }

    /**
     * @brief Removes and returns the element at the front of the buffer.
     * @return The removed element.
     * @throws std::logic_error if the buffer is empty.
     */
    constexpr T pop_front() {
        if (empty()) throw std::logic_error("Buffer is empty");
        size_type index = head_;
        head_ = (head_ + 1) % N;
        --size_;
        // If the buffer becomes empty, reset pointers to the start
        if (empty()) {
            head_ = 0;
            tail_ = 0;
        }
        return std::move(data_[index]);
    }

    /**
     * @brief Removes a range of elements from the front of the buffer.
     * @tparam OutputIt The type of the output iterator.
     * @param dest Iterator to the beginning of the destination range.
     * @param count The number of elements to pop.
     * @return An iterator to the end of the destination range.
     */
    template<typename OutputIt>
    OutputIt pop_front_range(OutputIt dest, size_type count) {
        size_type num_to_pop = std::min(count, size_);
        for (size_type i = 0; i < num_to_pop; ++i) {
            *(dest++) = pop_front();
        }
        return dest;
    }


    // --- Element access ---

    /**
     * @brief Accesses the element at a specific logical position.
     * No bounds checking is performed.
     * @param pos The logical index of the element to access (0 is the front).
     * @return A reference to the element.
     */
    constexpr reference operator[](const size_type pos) {
        return data_[(head_ + pos) % N];
    }

    /**
     * @brief Accesses the element at a specific logical position (const version).
     * No bounds checking is performed.
     * @param pos The logical index of the element to access (0 is the front).
     * @return A const reference to the element.
     */
    constexpr const_reference operator[](const size_type pos) const {
        return data_[(head_ + pos) % N];
    }

    /**
     * @brief Accesses the element at a specific logical position with bounds checking.
     * @param pos The logical index of the element to access.
     * @return A reference to the element.
     * @throws std::out_of_range if pos is not within the range of the buffer.
     */
    constexpr reference at(const size_type pos) {
        if (pos >= size_) throw std::out_of_range("Index is out of range!");
        return data_[(head_ + pos) % N];
    }

    /**
     * @brief Accesses the element at a specific logical position with bounds checking (const version).
     * @param pos The logical index of the element to access.
     * @return A const reference to the element.
     * @throws std::out_of_range if pos is not within the range of the buffer.
     */
    constexpr const_reference at(const size_type pos) const {
        if (pos >= size_) throw std::out_of_range("Index is out of range!");
        return data_[(head_ + pos) % N];
    }

    /**
     * @brief Returns a reference to the first element in the buffer.
     * @return A reference to the front element.
     * @throws std::logic_error if the buffer is empty.
     */
    constexpr reference front() {
        if (empty()) throw std::logic_error("Buffer is empty");
        return data_[head_];
    }

    /**
     * @brief Returns a const reference to the first element in the buffer.
     * @return A const reference to the front element.
     * @throws std::logic_error if the buffer is empty.
     */
    constexpr const_reference front() const {
        if (empty()) throw std::logic_error("Buffer is empty");
        return data_[head_];
    }

    /**
     * @brief Returns a reference to the last element in the buffer.
     * @return A reference to the back element.
     * @throws std::logic_error if the buffer is empty.
     */
    constexpr reference back() {
        if (empty()) throw std::logic_error("Buffer is empty");
        return data_[tail_];
    }

    /**
     * @brief Returns a const reference to the last element in the buffer.
     * @return A const reference to the back element.
     * @throws std::logic_error if the buffer is empty.
     */
    constexpr const_reference back() const {
        if (empty()) throw std::logic_error("Buffer is empty");
        return data_[tail_];
    }

    // --- Iterators ---

    /**
     * @brief Returns an iterator to the beginning of the buffer.
     * @return An iterator to the first element.
     */
    iterator begin() { return iterator(*this, 0); }

    /**
     * @brief Returns an iterator to the end of the buffer.
     * @return An iterator to the element following the last element.
     */
    iterator end() { return iterator(*this, size_); }

    /**
     * @brief Returns a const iterator to the beginning of the buffer.
     * @return A const iterator to the first element.
     */
    const_iterator begin() const { return const_iterator(*this, 0); }

    /**
     * @brief Returns a const iterator to the end of the buffer.
     * @return A const iterator to the element following the last element.
     */
    const_iterator end() const { return const_iterator(*this, size_); }

private:
    // Befriend the iterator template to allow access to private members
    template<typename U, std::size_t M>
    friend class circular_buffer_iterator;

    std::array<value_type, N> data_{};
    size_type head_ = 0;
    size_type tail_ = 0;
    size_type size_ = 0;
};

/**
 * @class circular_buffer_iterator
 * @brief A random access iterator for the circular_buffer.
 * @tparam T The type of elements in the buffer (can be const).
 * @tparam N The capacity of the buffer.
 */
template<typename T, std::size_t N>
class circular_buffer_iterator {
public:
    static_assert(N > 0, "circular_buffer_iterator capacity must be greater than 0");

    // --- Member Types ---
    using BufferType = std::conditional_t<std::is_const_v<T>,
                                          const circular_buffer<std::remove_const_t<T>, N>,
                                          circular_buffer<std::remove_const_t<T>, N>>;

    using self_type = circular_buffer_iterator<T, N>;
    using value_type = std::remove_const_t<T>;
    using reference = T&;
    using pointer = T*;
    using iterator_category = std::random_access_iterator_tag;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

public:
    // --- Constructor ---
    explicit circular_buffer_iterator(BufferType& buffer, const size_type index)
        : buffer_(&buffer), index_(index) {}

    // --- Dereferencing ---
    reference operator*() const {
        if (index_ >= buffer_->size()) throw std::logic_error("Cannot dereference end iterator");
        return (*buffer_)[index_];
    }
    pointer operator->() const { return std::addressof(operator*()); }

    // --- Increment/Decrement ---
    self_type& operator++() {
        if (index_ >= buffer_->size()) throw std::out_of_range("Iterator cannot be incremented past the end");
        ++index_;
        return *this;
    }
    self_type operator++(int) { self_type temp = *this; ++*this; return temp; }
    self_type& operator--() {
        if (index_ == 0) throw std::out_of_range("Iterator cannot be decremented before the beginning");
        --index_;
        return *this;
    }
    self_type operator--(int) { self_type temp = *this; --*this; return temp; }

    // --- Arithmetic ---
    self_type& operator+=(const difference_type offset) {
        difference_type next_index = static_cast<difference_type>(index_) + offset;
        if (next_index < 0 || static_cast<size_type>(next_index) > buffer_->size()) {
            throw std::out_of_range("Iterator offset is out of bounds");
        }
        index_ = static_cast<size_type>(next_index);
        return *this;
    }
    self_type& operator-=(const difference_type offset) { return *this += -offset; }
    self_type operator+(difference_type offset) const { self_type temp = *this; return temp += offset; }
    self_type operator-(difference_type offset) const { self_type temp = *this; return temp -= offset; }
    difference_type operator-(const self_type& other) const {
        // It's undefined behavior to subtract iterators from different containers
        return static_cast<difference_type>(index_) - static_cast<difference_type>(other.index_);
    }
    reference operator[](const difference_type offset) const { return *(*this + offset); }

    // --- Comparison ---
    bool operator==(const self_type& other) const { return buffer_ == other.buffer_ && index_ == other.index_; }
    bool operator!=(const self_type& other) const { return !(*this == other); }
    bool operator<(const self_type& other) const { return index_ < other.index_; }
    bool operator>(const self_type& other) const { return other < *this; }
    bool operator<=(const self_type& other) const { return !(other < *this); }
    bool operator>=(const self_type& other) const { return !(*this < other); }

private:
    BufferType* buffer_;
    size_type index_ = 0;
};

} // namespace Common

#endif // COMMON_CIRCULAR_BUFFER_H
