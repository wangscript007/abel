//
// -----------------------------------------------------------------------------
// File: fixed_array.h
// -----------------------------------------------------------------------------
//
// A `fixed_array<T>` represents a non-resizable array of `T` where the length of
// the array can be determined at run-time. It is a good replacement for
// non-standard and deprecated uses of `alloca()` and variable length arrays
// within the GCC extension. (See
// https://gcc.gnu.org/onlinedocs/gcc/Variable-Length.html).
//
// `fixed_array` allocates small arrays inline, keeping performance fast by
// avoiding heap operations. It also helps reduce the chances of
// accidentally overflowing your stack if large input is passed to
// your function.

#ifndef ABEL_ASL_FIXED_ARRAY_H_
#define ABEL_ASL_FIXED_ARRAY_H_

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>

#include <abel/algorithm/algorithm.h>
#include <abel/thread/dynamic_annotations.h>
#include <abel/base/throw_delegate.h>
#include <abel/base/profile.h>
#include <abel/container/internal/compressed_tuple.h>
#include <abel/memory/memory.h>

namespace abel {


    constexpr static auto kFixedArrayUseDefault = static_cast<size_t>(-1);

// -----------------------------------------------------------------------------
// fixed_array
// -----------------------------------------------------------------------------
//
// A `fixed_array` provides a run-time fixed-size array, allocating a small array
// inline for efficiency.
//
// Most users should not specify an `inline_elements` argument and let
// `fixed_array` automatically determine the number of elements
// to store inline based on `sizeof(T)`. If `inline_elements` is specified, the
// `fixed_array` implementation will use inline storage for arrays with a
// length <= `inline_elements`.
//
// Note that a `fixed_array` constructed with a `size_type` argument will
// default-initialize its values by leaving trivially constructible types
// uninitialized (e.g. int, int[4], double), and others default-constructed.
// This matches the behavior of c-style arrays and `std::array`, but not
// `std::vector`.
//
// Note that `fixed_array` does not provide a public allocator; if it requires a
// heap allocation, it will do so with global `::operator new[]()` and
// `::operator delete[]()`, even if T provides class-scope overrides for these
// operators.
    template<typename T, size_t N = kFixedArrayUseDefault,
            typename A = std::allocator<T>>
    class fixed_array {
        static_assert(!std::is_array<T>::value || std::extent<T>::value > 0,
                      "Arrays with unknown bounds cannot be used with fixed_array.");

        static constexpr size_t kInlineBytesDefault = 256;

        using AllocatorTraits = std::allocator_traits<A>;
        // std::iterator_traits isn't guaranteed to be SFINAE-friendly until C++17,
        // but this seems to be mostly pedantic.
        template<typename Iterator>
        using EnableIfForwardIterator = abel::enable_if_t<std::is_convertible<
                typename std::iterator_traits<Iterator>::iterator_category,
                std::forward_iterator_tag>::value>;

        static constexpr bool NoexceptCopyable() {
            return std::is_nothrow_copy_constructible<StorageElement>::value &&
                   abel::allocator_is_nothrow<allocator_type>::value;
        }

        static constexpr bool NoexceptMovable() {
            return std::is_nothrow_move_constructible<StorageElement>::value &&
                   abel::allocator_is_nothrow<allocator_type>::value;
        }

        static constexpr bool DefaultConstructorIsNonTrivial() {
            return !abel::is_trivially_default_constructible<StorageElement>::value;
        }

    public:
        using allocator_type = typename AllocatorTraits::allocator_type;
        using value_type = typename allocator_type::value_type;
        using pointer = typename allocator_type::pointer;
        using const_pointer = typename allocator_type::const_pointer;
        using reference = typename allocator_type::reference;
        using const_reference = typename allocator_type::const_reference;
        using size_type = typename allocator_type::size_type;
        using difference_type = typename allocator_type::difference_type;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        static constexpr size_type inline_elements =
                (N == kFixedArrayUseDefault ? kInlineBytesDefault / sizeof(value_type)
                                            : static_cast<size_type>(N));

        fixed_array(
                const fixed_array &other,
                const allocator_type &a = allocator_type()) noexcept(NoexceptCopyable())
                : fixed_array(other.begin(), other.end(), a) {}

        fixed_array(
                fixed_array &&other,
                const allocator_type &a = allocator_type()) noexcept(NoexceptMovable())
                : fixed_array(std::make_move_iterator(other.begin()),
                             std::make_move_iterator(other.end()), a) {}

        // Creates an array object that can store `n` elements.
        // Note that trivially constructible elements will be uninitialized.
        explicit fixed_array(size_type n, const allocator_type &a = allocator_type())
                : storage_(n, a) {
            if (DefaultConstructorIsNonTrivial()) {
                memory_internal::ConstructRange(storage_.alloc(), storage_.begin(),
                                                storage_.end());
            }
        }

        // Creates an array initialized with `n` copies of `val`.
        fixed_array(size_type n, const value_type &val,
                   const allocator_type &a = allocator_type())
                : storage_(n, a) {
            memory_internal::ConstructRange(storage_.alloc(), storage_.begin(),
                                            storage_.end(), val);
        }

        // Creates an array initialized with the size and contents of `init_list`.
        fixed_array(std::initializer_list<value_type> init_list,
                   const allocator_type &a = allocator_type())
                : fixed_array(init_list.begin(), init_list.end(), a) {}

        // Creates an array initialized with the elements from the input
        // range. The array's size will always be `std::distance(first, last)`.
        // REQUIRES: Iterator must be a forward_iterator or better.
        template<typename Iterator, EnableIfForwardIterator<Iterator> * = nullptr>
        fixed_array(Iterator first, Iterator last,
                   const allocator_type &a = allocator_type())
                : storage_(std::distance(first, last), a) {
            memory_internal::CopyRange(storage_.alloc(), storage_.begin(), first, last);
        }

        ~fixed_array() noexcept {
            for (auto *cur = storage_.begin(); cur != storage_.end(); ++cur) {
                AllocatorTraits::destroy(storage_.alloc(), cur);
            }
        }

        // Assignments are deleted because they break the invariant that the size of a
        // `fixed_array` never changes.
        void operator=(fixed_array &&) = delete;

        void operator=(const fixed_array &) = delete;

        // fixed_array::size()
        //
        // Returns the length of the fixed array.
        size_type size() const { return storage_.size(); }

        // fixed_array::max_size()
        //
        // Returns the largest possible value of `std::distance(begin(), end())` for a
        // `fixed_array<T>`. This is equivalent to the most possible addressable bytes
        // over the number of bytes taken by T.
        constexpr size_type max_size() const {
            return (std::numeric_limits<difference_type>::max)() / sizeof(value_type);
        }

        // fixed_array::empty()
        //
        // Returns whether or not the fixed array is empty.
        bool empty() const { return size() == 0; }

        // fixed_array::memsize()
        //
        // Returns the memory size of the fixed array in bytes.
        size_t memsize() const { return size() * sizeof(value_type); }

        // fixed_array::data()
        //
        // Returns a const T* pointer to elements of the `fixed_array`. This pointer
        // can be used to access (but not modify) the contained elements.
        const_pointer data() const { return AsValueType(storage_.begin()); }

        // Overload of fixed_array::data() to return a T* pointer to elements of the
        // fixed array. This pointer can be used to access and modify the contained
        // elements.
        pointer data() { return AsValueType(storage_.begin()); }

        // fixed_array::operator[]
        //
        // Returns a reference the ith element of the fixed array.
        // REQUIRES: 0 <= i < size()
        reference operator[](size_type i) {
            assert(i < size());
            return data()[i];
        }

        // Overload of fixed_array::operator()[] to return a const reference to the
        // ith element of the fixed array.
        // REQUIRES: 0 <= i < size()
        const_reference operator[](size_type i) const {
            assert(i < size());
            return data()[i];
        }

        // fixed_array::at
        //
        // Bounds-checked access.  Returns a reference to the ith element of the
        // fiexed array, or throws std::out_of_range
        reference at(size_type i) {
            if (ABEL_UNLIKELY(i >= size())) {
                throw_std_out_of_range("fixed_array::at failed bounds check");
            }
            return data()[i];
        }

        // Overload of fixed_array::at() to return a const reference to the ith element
        // of the fixed array.
        const_reference at(size_type i) const {
            if (ABEL_UNLIKELY(i >= size())) {
                throw_std_out_of_range("fixed_array::at failed bounds check");
            }
            return data()[i];
        }

        // fixed_array::front()
        //
        // Returns a reference to the first element of the fixed array.
        reference front() { return *begin(); }

        // Overload of fixed_array::front() to return a reference to the first element
        // of a fixed array of const values.
        const_reference front() const { return *begin(); }

        // fixed_array::back()
        //
        // Returns a reference to the last element of the fixed array.
        reference back() { return *(end() - 1); }

        // Overload of fixed_array::back() to return a reference to the last element
        // of a fixed array of const values.
        const_reference back() const { return *(end() - 1); }

        // fixed_array::begin()
        //
        // Returns an iterator to the beginning of the fixed array.
        iterator begin() { return data(); }

        // Overload of fixed_array::begin() to return a const iterator to the
        // beginning of the fixed array.
        const_iterator begin() const { return data(); }

        // fixed_array::cbegin()
        //
        // Returns a const iterator to the beginning of the fixed array.
        const_iterator cbegin() const { return begin(); }

        // fixed_array::end()
        //
        // Returns an iterator to the end of the fixed array.
        iterator end() { return data() + size(); }

        // Overload of fixed_array::end() to return a const iterator to the end of the
        // fixed array.
        const_iterator end() const { return data() + size(); }

        // fixed_array::cend()
        //
        // Returns a const iterator to the end of the fixed array.
        const_iterator cend() const { return end(); }

        // fixed_array::rbegin()
        //
        // Returns a reverse iterator from the end of the fixed array.
        reverse_iterator rbegin() { return reverse_iterator(end()); }

        // Overload of fixed_array::rbegin() to return a const reverse iterator from
        // the end of the fixed array.
        const_reverse_iterator rbegin() const {
            return const_reverse_iterator(end());
        }

        // fixed_array::crbegin()
        //
        // Returns a const reverse iterator from the end of the fixed array.
        const_reverse_iterator crbegin() const { return rbegin(); }

        // fixed_array::rend()
        //
        // Returns a reverse iterator from the beginning of the fixed array.
        reverse_iterator rend() { return reverse_iterator(begin()); }

        // Overload of fixed_array::rend() for returning a const reverse iterator
        // from the beginning of the fixed array.
        const_reverse_iterator rend() const {
            return const_reverse_iterator(begin());
        }

        // fixed_array::crend()
        //
        // Returns a reverse iterator from the beginning of the fixed array.
        const_reverse_iterator crend() const { return rend(); }

        // fixed_array::fill()
        //
        // Assigns the given `value` to all elements in the fixed array.
        void fill(const value_type &val) { std::fill(begin(), end(), val); }

        // Relational operators. Equality operators are elementwise using
        // `operator==`, while order operators order FixedArrays lexicographically.
        friend bool operator==(const fixed_array &lhs, const fixed_array &rhs) {
            return abel::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        }

        friend bool operator!=(const fixed_array &lhs, const fixed_array &rhs) {
            return !(lhs == rhs);
        }

        friend bool operator<(const fixed_array &lhs, const fixed_array &rhs) {
            return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
                                                rhs.end());
        }

        friend bool operator>(const fixed_array &lhs, const fixed_array &rhs) {
            return rhs < lhs;
        }

        friend bool operator<=(const fixed_array &lhs, const fixed_array &rhs) {
            return !(rhs < lhs);
        }

        friend bool operator>=(const fixed_array &lhs, const fixed_array &rhs) {
            return !(lhs < rhs);
        }

        template<typename H>
        friend H abel_hash_value(H h, const fixed_array &v) {
            return H::combine(H::combine_contiguous(std::move(h), v.data(), v.size()),
                              v.size());
        }

    private:
        // StorageElement
        //
        // For FixedArrays with a C-style-array value_type, StorageElement is a POD
        // wrapper struct called StorageElementWrapper that holds the value_type
        // instance inside. This is needed for construction and destruction of the
        // entire array regardless of how many dimensions it has. For all other cases,
        // StorageElement is just an alias of value_type.
        //
        // Maintainer's Note: The simpler solution would be to simply wrap value_type
        // in a struct whether it's an array or not. That causes some paranoid
        // diagnostics to misfire, believing that 'data()' returns a pointer to a
        // single element, rather than the packed array that it really is.
        // e.g.:
        //
        //     fixed_array<char> buf(1);
        //     sprintf(buf.data(), "foo");
        //
        //     error: call to int __builtin___sprintf_chk(etc...)
        //     will always overflow destination buffer [-Werror]
        //
        template<typename OuterT, typename InnerT = abel::remove_extent_t<OuterT>,
                size_t InnerN = std::extent<OuterT>::value>
        struct StorageElementWrapper {
            InnerT array[InnerN];
        };

        using StorageElement =
        abel::conditional_t<std::is_array<value_type>::value,
                StorageElementWrapper<value_type>, value_type>;

        static pointer AsValueType(pointer ptr) { return ptr; }

        static pointer AsValueType(StorageElementWrapper<value_type> *ptr) {
            return std::addressof(ptr->array);
        }

        static_assert(sizeof(StorageElement) == sizeof(value_type), "");
        static_assert(alignof(StorageElement) == alignof(value_type), "");

        class NonEmptyInlinedStorage {
        public:
            StorageElement *data() { return reinterpret_cast<StorageElement *>(buff_); }

            void AnnotateConstruct(size_type n);

            void AnnotateDestruct(size_type n);

#ifdef ADDRESS_SANITIZER
            void* RedzoneBegin() { return &redzone_begin_; }
            void* RedzoneEnd() { return &redzone_end_ + 1; }
#endif  // ADDRESS_SANITIZER

        private:
            ADDRESS_SANITIZER_REDZONE(redzone_begin_);
            alignas(StorageElement) char buff_[sizeof(StorageElement[inline_elements])];
            ADDRESS_SANITIZER_REDZONE(redzone_end_);
        };

        class EmptyInlinedStorage {
        public:
            StorageElement *data() { return nullptr; }

            void AnnotateConstruct(size_type) {}

            void AnnotateDestruct(size_type) {}
        };

        using InlinedStorage =
        abel::conditional_t<inline_elements == 0, EmptyInlinedStorage,
                NonEmptyInlinedStorage>;

        // Storage
        //
        // An instance of Storage manages the inline and out-of-line memory for
        // instances of fixed_array. This guarantees that even when construction of
        // individual elements fails in the fixed_array constructor body, the
        // destructor for Storage will still be called and out-of-line memory will be
        // properly deallocated.
        //
        class Storage : public InlinedStorage {
        public:
            Storage(size_type n, const allocator_type &a)
                    : size_alloc_(n, a), data_(InitializeData()) {}

            ~Storage() noexcept {
                if (UsingInlinedStorage(size())) {
                    InlinedStorage::AnnotateDestruct(size());
                } else {
                    AllocatorTraits::deallocate(alloc(), AsValueType(begin()), size());
                }
            }

            size_type size() const { return size_alloc_.template get<0>(); }

            StorageElement *begin() const { return data_; }

            StorageElement *end() const { return begin() + size(); }

            allocator_type &alloc() { return size_alloc_.template get<1>(); }

        private:
            static bool UsingInlinedStorage(size_type n) {
                return n <= inline_elements;
            }

            StorageElement *InitializeData() {
                if (UsingInlinedStorage(size())) {
                    InlinedStorage::AnnotateConstruct(size());
                    return InlinedStorage::data();
                } else {
                    return reinterpret_cast<StorageElement *>(
                            AllocatorTraits::allocate(alloc(), size()));
                }
            }

            // `CompressedTuple` takes advantage of EBCO for stateless `allocator_type`s
            container_internal::CompressedTuple<size_type, allocator_type> size_alloc_;
            StorageElement *data_;
        };

        Storage storage_;
    };

    template<typename T, size_t N, typename A>
    constexpr size_t fixed_array<T, N, A>::kInlineBytesDefault;

    template<typename T, size_t N, typename A>
    constexpr typename fixed_array<T, N, A>::size_type
            fixed_array<T, N, A>::inline_elements;

    template<typename T, size_t N, typename A>
    void fixed_array<T, N, A>::NonEmptyInlinedStorage::AnnotateConstruct(
            typename fixed_array<T, N, A>::size_type n) {
#ifdef ADDRESS_SANITIZER
        if (!n) return;
        ANNOTATE_CONTIGUOUS_CONTAINER(data(), RedzoneEnd(), RedzoneEnd(), data() + n);
        ANNOTATE_CONTIGUOUS_CONTAINER(RedzoneBegin(), data(), data(), RedzoneBegin());
#endif                   // ADDRESS_SANITIZER
        static_cast<void>(n);  // Mark used when not in asan mode
    }

    template<typename T, size_t N, typename A>
    void fixed_array<T, N, A>::NonEmptyInlinedStorage::AnnotateDestruct(
            typename fixed_array<T, N, A>::size_type n) {
#ifdef ADDRESS_SANITIZER
        if (!n) return;
        ANNOTATE_CONTIGUOUS_CONTAINER(data(), RedzoneEnd(), data() + n, RedzoneEnd());
        ANNOTATE_CONTIGUOUS_CONTAINER(RedzoneBegin(), data(), RedzoneBegin(), data());
#endif                   // ADDRESS_SANITIZER
        static_cast<void>(n);  // Mark used when not in asan mode
    }

}  // namespace abel

#endif  // ABEL_ASL_FIXED_ARRAY_H_
