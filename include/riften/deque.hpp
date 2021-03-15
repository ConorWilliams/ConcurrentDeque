#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

// This (standalone) file implements the deque described in the papers, "Correct and Efficient
// Work-Stealing for Weak Memory Models," and "Dynamic Circular Work-Stealing Deque". Both are avaliable
// in 'reference/'.

namespace riften {

namespace detail {

// Basic wrapper around a c-style array of atomic objects that provides modulo load/stores. Capacity
// must be a power of 2.
template <typename T> struct RingBuff {
  public:
    explicit RingBuff(std::int64_t cap) : _cap{cap}, _mask{cap - 1} {
        assert(cap && (!(cap & (cap - 1))) && "Capacity must be buf power of 2!");
    }

    std::int64_t capacity() const noexcept { return _cap; }

    // Relaxed store at modulo index
    void store(std::int64_t i, T x) noexcept { _buff[i & _mask].store(x, std::memory_order_relaxed); }

    // Relaxed load at modulo index
    T load(std::int64_t i) const noexcept { return _buff[i & _mask].load(std::memory_order_relaxed); }

    // Allocates and returns a new ring buffer, copies elements in range [b, t) into the new buffer.
    RingBuff<T>* resize(std::int64_t b, std::int64_t t) const {
        RingBuff<T>* ptr = new RingBuff{2 * _cap};
        for (std::int64_t i = t; i != b; ++i) {
            ptr->store(i, load(i));
        }
        return ptr;
    }

  private:
    std::int64_t _cap;   // Capacity of the buffer
    std::int64_t _mask;  // Bitmask to perform modulo capacity operations

#if !__cpp_lib_smart_ptr_for_overwrite
    std::unique_ptr<std::atomic<T>[]> _buff = std::make_unique<std::atomic<T>[]>(_cap);
#else
    std::unique_ptr<std::atomic<T>[]> _buff = std::make_unique_for_overwrite<std::atomic<T>[]>(_cap);
#endif
};

template <typename T> struct is_always_lock_free {
    static constexpr bool value = std::atomic<T>::is_always_lock_free;
};

template <typename T> static constexpr bool lock_free_v
    = std::conjunction_v<std::is_trivially_copyable<T>,
                         std::is_copy_constructible<T>,
                         std::is_move_constructible<T>,
                         std::is_copy_assignable<T>,
                         std::is_move_assignable<T>,
                         is_always_lock_free<T>>;

}  // namespace detail

// Lock-free single-producer multiple-consumer deque. There are no constraints on the type `T` that can
// be stored. Only the deque owner can perform pop and push operations where the deque behaves like a
// stack. Others can (only) steal data from the deque, they see a FIFO queue. All threads must have
// finished using the deque before it is destructed.
template <typename T> class Deque {
  public:
    // Constructs the deque with a given capacity the capacity of the deque (must be power of 2)
    explicit Deque(std::int64_t cap = 1024);

    // Move/Copy is not supported
    Deque(Deque const& other) = delete;
    Deque& operator=(Deque const& other) = delete;

    //  Query the size at instance of call
    std::size_t size() const noexcept;

    // Query the capacity at instance of call
    int64_t capacity() const noexcept;

    // Test if empty at instance of call
    bool empty() const noexcept;

    // Emplace an item to the deque. Only the owner thread can insert an item to the deque. The operation
    // can trigger the deque to resize its cap if more space is required. Provides the strong exception
    // garantee.
    template <typename... Args> void emplace(Args&&... args);

    // Pops out an item from the deque. Only the owner thread can pop out an item from the deque. The
    // return can be a std::nullopt if this operation failed (empty deque).
    std::optional<T> pop() noexcept;

    // Steals an item from the deque Any threads can try to steal an item from the deque. The return can
    // be a std::nullopt if this operation failed (not necessary empty).
    std::optional<T> steal() noexcept;

    // Destruct the deque, all threads must have finished using the deque.
    ~Deque() noexcept;

    // If true elements of type `T` are stored directly in the ring buffer.
    static constexpr bool no_alloc = std::is_trivially_destructible_v<T> && detail::lock_free_v<T>;

  private:
    using buffer_t = detail::RingBuff<std::conditional_t<no_alloc, T, T*>>;

    std::atomic<std::int64_t> _top;                   // Top of deque
    std::atomic<std::int64_t> _bottom;                // Bottom of deque.
    std::atomic<buffer_t*> _buffer;                   // Current buffer.
    std::vector<std::unique_ptr<buffer_t>> _garbage;  // Store old buffers here.

    // Convinience aliases.
    static constexpr std::memory_order relaxed = std::memory_order_relaxed;
    static constexpr std::memory_order consume = std::memory_order_consume;
    static constexpr std::memory_order acquire = std::memory_order_acquire;
    static constexpr std::memory_order release = std::memory_order_release;
    static constexpr std::memory_order seq_cst = std::memory_order_seq_cst;
};

template <typename T> Deque<T>::Deque(std::int64_t cap)
    : _top(0), _bottom(0), _buffer(new buffer_t{cap}) {
    _garbage.reserve(32);
}

template <typename T> std::size_t Deque<T>::size() const noexcept {
    int64_t b = _bottom.load(relaxed);
    int64_t t = _top.load(relaxed);
    return static_cast<std::size_t>(b >= t ? b - t : 0);
}

template <typename T> int64_t Deque<T>::capacity() const noexcept {
    return _buffer.load(relaxed)->capacity();
}

template <typename T> bool Deque<T>::empty() const noexcept { return !size(); }

template <typename T> template <typename... Args> void Deque<T>::emplace(Args&&... args) {
    std::int64_t b = _bottom.load(relaxed);
    std::int64_t t = _top.load(acquire);
    buffer_t* buf = _buffer.load(relaxed);

    if (buf->capacity() < (b - t) + 1) {
        // Queue is full, build a new one
        _garbage.emplace_back(std::exchange(buf, buf->resize(b, t)));
        _buffer.store(buf, relaxed);
    }

    // Construct new object
    if constexpr (no_alloc) {
        buf->store(b, {std::forward<Args>(args)...});
    } else {
        buf->store(b, new T{std::forward<Args>(args)...});
    }

    std::atomic_thread_fence(release);
    _bottom.store(b + 1, relaxed);
}

template <typename T> std::optional<T> Deque<T>::pop() noexcept {
    std::int64_t b = _bottom.load(relaxed) - 1;
    buffer_t* buf = _buffer.load(relaxed);
    _bottom.store(b, relaxed);
    std::atomic_thread_fence(seq_cst);
    std::int64_t t = _top.load(relaxed);

    if (t <= b) {
        // Non-empty deque
        if (t == b) {
            // The last item could get stolen
            if (!_top.compare_exchange_strong(t, t + 1, seq_cst, relaxed)) {
                // Failed race.
                _bottom.store(b + 1, relaxed);
                return std::nullopt;
            }
            _bottom.store(b + 1, relaxed);
        }

        // Can delay load until after aquiring slot as only this thread can push()
        auto x = buf->load(b);

        if constexpr (no_alloc) {
            return x;
        } else {
            std::optional tmp{std::move(*x)};
            delete x;
            return tmp;
        }

    } else {
        _bottom.store(b + 1, relaxed);
        return std::nullopt;
    }
}

template <typename T> std::optional<T> Deque<T>::steal() noexcept {
    std::int64_t t = _top.load(acquire);
    std::atomic_thread_fence(seq_cst);
    std::int64_t b = _bottom.load(acquire);

    if (t < b) {
        // Must load *before* aquiring the slot as slot may be overwritten immidiatly after aquiring.
        auto x = _buffer.load(consume)->load(t);

        if (!_top.compare_exchange_strong(t, t + 1, seq_cst, relaxed)) {
            // Failed race.
            return std::nullopt;
        }

        if constexpr (no_alloc) {
            return x;
        } else {
            std::optional tmp{std::move(*x)};
            delete x;
            return tmp;
        }

    } else {
        // Empty deque.
        return std::nullopt;
    }
}

template <typename T> Deque<T>::~Deque() noexcept {
    if constexpr (!no_alloc) {
        // Clean up all remaining items in the deque.
        while (!empty()) {
            pop();
        }

        assert(empty() && "Busy during destruction");  // Check for interupts.
    }

    delete _buffer.load();
}

}  // namespace riften
