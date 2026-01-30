#pragma once

#include <memory>
#include <string>
#include <limits>
#include <atomic>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <iterator>
#include <stdexcept>
#include <string_view>

namespace gto {

/**
 * @brief Immutable string based on a plain C-string (char *) with ref-counting.
 * 
 * @details
 *   - Shared content between multiple instances (using ref counting).
 *   - Automatic mem dealloc (when no refs point to content).
 *   - Same sizeof as a 'char *'.
 *   - Null not allowed (equals to an empty string).
 *   - Empty string do not require memory allocation.
 *   - String content is available in debug.
 *   - Mimics the STL basic_string class.
 * 
 * @details Memory layout:
 * 
 *       ----|----|-----------NUL
 *        ^   ^    ^
 *        |   |    |-- string content (NUL-terminated)
 *        |   |-- string length (4-bytes)
 *        |-- reference counter (4-bytes)
 * 
 *   m_str (cstring pointer) points to the string content (to allow viewing content in debug).
 *   Allocated memory is aligned to ref-counter type size.
 *   Allocated memory is a multiple of ref-counter type size.
 * 
 * @see https://en.cppreference.com/w/cpp/string/basic_string
 * @see https://github.com/torrentg/cstring
 * 
 * @note This class is immutable.
 * @version 1.0.6
 */
template<typename Char,
         typename Traits = std::char_traits<Char>,
         typename Allocator = std::allocator<Char>>
class basic_cstring
{
  private: // declarations

    using prefix_type = std::uint32_t;
    using atomic_prefix_type = std::atomic<prefix_type>;
    using pointer = typename std::allocator_traits<Allocator>::pointer;

  public: // declarations

    using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<prefix_type>;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using traits_type = Traits;
    using size_type = typename std::allocator_traits<Allocator>::size_type;
    using difference_type = typename std::allocator_traits<Allocator>::difference_type;
    using value_type = Char;
    using const_reference = const value_type &;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
    using const_iterator = const_pointer;
    using const_reverse_iterator = typename std::reverse_iterator<const_iterator>;
    using basic_cstring_view = std::basic_string_view<value_type, traits_type>;

  public: // static members

    //! Special value indicating the maximum achievable index + 1.
    static constexpr size_type npos = std::numeric_limits<size_type>::max();

    //! Returns the maximum number of elements the string is able to hold.
    static constexpr size_type max_size() noexcept {
        return std::numeric_limits<prefix_type>::max() - 1;
    }

  private: // static members

    struct empty_cstring_t { 
        atomic_prefix_type counter{0};
        prefix_type len{0};
        value_type str[2] = {value_type(), value_type()};
    };

    static allocator_type m_allocator;
    static constexpr empty_cstring_t m_empty{};

  private: // members

    //! String content (NUL-terminated, prefix_type alignment).
    const_pointer m_str = nullptr;

  private: // static methods

    //! Sanitize a char array pointer avoiding nulls.
    static constexpr const_pointer sanitize(const_pointer str) noexcept {
        return ((str == nullptr || str[0] == value_type()) ? m_empty.str : str);
    }

    //! Return pointer to counter from pointer to string.
    static constexpr atomic_prefix_type * get_ptr_to_counter(const_pointer str) noexcept
    {
        static_assert(sizeof(atomic_prefix_type) == sizeof(prefix_type));
        static_assert(sizeof(value_type) <= sizeof(prefix_type));
        assert(str != nullptr);

        if (str == m_empty.str)
            return const_cast<atomic_prefix_type *>(&m_empty.counter);

        pointer ptr = const_cast<pointer>(str) - (2 * sizeof(prefix_type)) / sizeof(value_type);
        return reinterpret_cast<atomic_prefix_type *>(ptr);
    }

    //! Return pointer to string length from pointer to string.
    static constexpr prefix_type * get_ptr_to_length(const_pointer str) noexcept
    {
        static_assert(sizeof(value_type) <= sizeof(prefix_type));
        assert(str != nullptr);

        if (str == m_empty.str)
            return const_cast<prefix_type *>(&m_empty.len);

        pointer ptr = const_cast<pointer>(str) - (sizeof(prefix_type) / sizeof(value_type));
        return reinterpret_cast<prefix_type *>(ptr);
    }

    /** 
     * Returns the length of the prefix_type array to allocate (in prefix_type sizeof units).
     * 
     * @details Allocator allocates an array of prefix_type (not Char) to grant the memory alignment.
     * @details The returned length considere the place for the ending NUL.
     * 
     * @example Let len=6, sizeof(Char)=2, sizeof(prefix_type)=4
     *    We need to reserve:
     *      - 4-bytes for the counter (uint32_t)
     *      - 4-bytes for the length (uint32_t)
     *      - 6 x 2-bytes = 12-bytes for the string content
     *      - 1 x 2-bytes = 2-bytes for the terminating NUL
     *    Total to reserve = 22-bytes
     *    Expresed in 4-bytes (uint32_t) units = 6 (upper rounding)
     *    In this case there are 2 ending bytes acting as padding.
     * 
     * @param[in] len Length of the string (without ending NUL).
     * 
     * @return Length of the prefix_type array (in prefix_type sizeof units).
     */
    static size_type get_allocated_length(size_type len) noexcept
    {
        static_assert(sizeof(value_type) <= sizeof(prefix_type));

        return 3 + (len * sizeof(value_type)) / sizeof(prefix_type);
    }

    /** 
     * Allocate memory for the ref-counter + length + string + NUL.
     * 
     * @details We allocate prefix_types to grant memory alignment.
     * 
     * @param[in] len Length to reserve (of prefix_types).
     * 
     * @return A pointer to the allocated memory.
     */
    [[nodiscard]] 
    static prefix_type * allocate(size_type len)
    {
        static_assert(sizeof(atomic_prefix_type) == sizeof(prefix_type));
        static_assert(alignof(value_type) <= alignof(prefix_type));
        static_assert(sizeof(value_type) <= sizeof(prefix_type));
        assert(len > 0);

        prefix_type *ptr = allocator_traits::allocate(m_allocator, len);
        assert(reinterpret_cast<std::size_t>(ptr) % alignof(prefix_type) == 0);

        return ptr;
    }

    //! Deallocates the memory allocated by the object.
    static void deallocate(const_pointer str) noexcept
    {
        assert(str != nullptr);
        assert(str != m_empty.str);

        atomic_prefix_type *ptr = get_ptr_to_counter(str);
        prefix_type len = *get_ptr_to_length(str);
        size_type n = get_allocated_length(len);

        ptr->~atomic_prefix_type();
        allocator_traits::deallocate(m_allocator, reinterpret_cast<prefix_type *>(ptr), n);
    }

    /**
     * Decrements the ref-counter.
     * If no more references then deallocate the memory.
     * The empty string is never deallocated.
     * 
     * @param[in] str Memory to release.
     */
    static void release(const_pointer str) noexcept
    {
        atomic_prefix_type *ptr = get_ptr_to_counter(str);
        prefix_type counts = ptr[0].load(std::memory_order_relaxed);

        if (counts == 0) // constant (eg. m_empty)
            return;

        if (counts > 1)
            counts = ptr[0].fetch_sub(1, std::memory_order_relaxed);

        if (counts == 1)
            deallocate(str);
    }

    //! Increment the reference counter (except for constants).
    static void increment_ref_counter(const_pointer str) noexcept
    {
        atomic_prefix_type *ptr = get_ptr_to_counter(str);

        if (ptr[0].load(std::memory_order_relaxed) > 0)
            ptr[0].fetch_add(1, std::memory_order_relaxed);
    }

  public: // methods

    /**
     * Constructs a default cstring (empty string).
     */
    basic_cstring() noexcept : basic_cstring(nullptr) {}

    /** 
     * Constructs a cstring with a copy of the NULL-terminated str.
     * The length is determined by the first NULL character.
     * 
     * @param[in] str The NULL-terminated string to copy (NULL means the empty string).
     */
    basic_cstring(const_pointer str) : basic_cstring(str, (str == nullptr ? 0 : traits_type::length(str))) {}

    /** 
     * Constructs a cstring with the first len characters pointed by str.
     * 
     * A terminating NUL character will be added.
     * str can contain null characters.
     * 
     * @param[in] str The string to copy (NULL means the empty string, not necessarily NUL-terminated).
     * @param[in] len The number of characters to copy (0 means the empty string).
     */
    basic_cstring(const_pointer str, size_type len)
    {
        if (len > max_size())
            throw std::length_error("invalid cstring length");

        if (str == nullptr || len == 0) {
            m_str = m_empty.str;
        } else {
            size_type n = get_allocated_length(len);
            prefix_type *ptr = allocate(n);
            std::atomic_init(reinterpret_cast<atomic_prefix_type*>(ptr), 1); // ref-counter = 1
            ptr[1] = static_cast<prefix_type>(len); // length
            pointer content = reinterpret_cast<pointer>(ptr + 2);
            traits_type::copy(content, str, len);
            content[len] = value_type();
            m_str = content;
        }
    }

    /**
     * Destructor.
     * Decrements the ref-counter if other instances exists.
     * Otherwise deallocates memory.
     */
    ~basic_cstring() { release(m_str); }

    //! Copy constructor.
    basic_cstring(const basic_cstring &other) noexcept : m_str(other.m_str) { increment_ref_counter(m_str); }

    //! Move constructor.
    basic_cstring(basic_cstring &&other) noexcept : m_str(std::exchange(other.m_str, m_empty.str)) {}

    //! Copy assignment.
    basic_cstring & operator=(const basic_cstring &other) noexcept
    {
        if (m_str == other.m_str)
            return *this;

        release(m_str);
        m_str = other.m_str;
        increment_ref_counter(m_str);

        return *this;
    }

    //! Move assignment.
    basic_cstring & operator=(basic_cstring &&other) noexcept { std::swap(m_str, other.m_str); return *this; }

    //! Conversion to 'const char *'
    operator const_pointer() const { return m_str; }

    //! Returns the reference-counter value (0 if empty). 
    prefix_type use_count() const noexcept { return get_ptr_to_counter(m_str)[0].load(std::memory_order_relaxed); }

    //! Return length of string.
    size_type size() const noexcept { return *(get_ptr_to_length(m_str)); }

    //! Return length of string.
    size_type length() const noexcept { return *(get_ptr_to_length(m_str)); }

    //! Test if string is empty.
    bool empty() const noexcept { return (length() == 0); }

    //! Returns a reference to the character at specified location pos in range [0, length()].
    const_reference operator[](size_type pos) const noexcept { return m_str[pos]; }

    //! Returns a reference to the character at specified location pos in range [0, length()].
    const_reference at(size_type pos) const { return (pos > length() ? throw std::out_of_range("cstring::at") : m_str[pos]); }

    //! Get last character of the string.
    const_reference back() const { return (empty() ? throw std::out_of_range("cstring::back") : m_str[length()-1]); }

    //! Get first character of the string.
    const_reference front() const { return (empty() ? throw std::out_of_range("cstring::front") : m_str[0]); }

    //! Returns a non-null pointer to a null-terminated character array.
    const_pointer data() const noexcept { assert(m_str != nullptr); return m_str; }

    //! Returns a non-null pointer to a null-terminated character array.
    const_pointer c_str() const noexcept { return data(); }

    //! Returns a string_view of content.
    basic_cstring_view view() const noexcept { return basic_cstring_view(m_str, length()); }

    // Const iterator to the begin.
    const_iterator cbegin() const noexcept { return view().cbegin(); }

    // Const iterator to the end.
    const_iterator cend() const noexcept { return view().cend(); }

    // Const reverse iterator to the begin.
    const_reverse_iterator crbegin() const noexcept { return view().crbegin(); }

    // Const reverse iterator to the end.
    const_reverse_iterator crend() const noexcept { return view().crend(); }

    //! Exchanges the contents of the string with those of other.
    void swap(basic_cstring &other) noexcept { std::swap(m_str, other.m_str); }

    //! Returns the substring [pos, pos+len).
    basic_cstring_view substr(size_type pos=0, size_type len=npos) const { return view().substr(pos, len); }

    //! Compare contents.
    int compare(const basic_cstring &other) const noexcept {
        return view().compare(other.view());
    }
    int compare(size_type pos, size_type len, const basic_cstring &other) const noexcept { 
        return substr(pos, len).compare(other.view());
    }
    int compare(size_type pos1, size_type len1, const basic_cstring &other, size_type pos2, size_type len2=npos) const {
        return substr(pos1, len1).compare(other.substr(pos2, len2));
    }
    int compare(const_pointer str) const {
        return view().compare(sanitize(str));
    }
    int compare(size_type pos, size_type len, const_pointer str) const {
        return substr(pos, len).compare(sanitize(str));
    }
    int compare(size_type pos, size_type len, const_pointer str, size_type len2) const {
        return substr(pos, len).compare(basic_cstring_view(sanitize(str), len2));
    }
    int compare(const basic_cstring_view other) const noexcept {
        return view().compare(other);
    }

    //! Checks if the string view begins with the given prefix.
    bool starts_with(const basic_cstring &other) const noexcept {
        size_type len = other.length();
        return (compare(0, len, other) == 0);
    }
    bool starts_with(const basic_cstring_view sv) const noexcept {
        auto len = sv.length();
        return (compare(0, len, sv.data(), len) == 0);
    }
    bool starts_with(const_pointer str) const noexcept {
        return starts_with(basic_cstring_view(sanitize(str)));
    }

    //! Checks if the string ends with the given suffix.
    bool ends_with(const basic_cstring &other) const noexcept {
        auto len1 = length();
        auto len2 = other.length();
        return (len1 >= len2 && compare(len1-len2, len2, other) == 0);
    }
    bool ends_with(const basic_cstring_view sv) const noexcept {
        size_type len1 = length();
        size_type len2 = sv.length();
        return (len1 >= len2 && compare(len1-len2, len2, sv.data(), len2) == 0);
    }
    bool ends_with(const_pointer str) const noexcept {
        return ends_with(basic_cstring_view(sanitize(str)));
    }

    //! Find the first ocurrence of a substring.
    auto find(const basic_cstring &other, size_type pos=0) const noexcept{
        return view().find(other.view(), pos);
    }
    auto find(const_pointer str, size_type pos, size_type len) const {
        return view().find(sanitize(str), pos, len);
    }
    auto find(const_pointer str, size_type pos=0) const {
        return view().find(sanitize(str), pos);
    }
    auto find(value_type c, size_type pos=0) const noexcept {
        return view().find(c, pos);
    }

    //! Find the last occurrence of a substring.
    auto rfind(const basic_cstring &other, size_type pos=npos) const noexcept{
      return view().rfind(other.view(), pos);
    }
    auto rfind(const_pointer str, size_type pos, size_type len) const {
        return view().rfind(sanitize(str), pos, len);
    }
    auto rfind(const_pointer str, size_type pos=npos) const {
        return view().rfind(sanitize(str), pos);
    }
    auto rfind(value_type c, size_type pos=npos) const noexcept {
        return view().rfind(c, pos);
    }

    //! Finds the first character equal to one of the given characters.
    auto find_first_of(const basic_cstring &other, size_type pos=0) const noexcept {
        return view().find_first_of(other.view(), pos);
    }
    auto find_first_of(const_pointer str, size_type pos, size_type len) const {
        return view().find_first_of(sanitize(str), pos, len);
    }
    auto find_first_of(const_pointer str, size_type pos=0) const {
        return view().find_first_of(sanitize(str), pos);
    }
    auto find_first_of(value_type c, size_type pos=0) const noexcept {
        return view().find_first_of(c, pos);
    }

    //! Finds the first character equal to none of the given characters.
    auto find_first_not_of(const basic_cstring &other, size_type pos=0) const noexcept {
        return view().find_first_not_of(other.view(), pos);
    }
    auto find_first_not_of(const_pointer str, size_type pos, size_type len) const {
        return view().find_first_not_of(sanitize(str), pos, len);
    }
    auto find_first_not_of(const_pointer str, size_type pos=0) const {
        return view().find_first_not_of(sanitize(str), pos);
    }
    auto find_first_not_of(value_type c, size_type pos=0) const noexcept {
        return view().find_first_not_of(c, pos);
    }

    //! Finds the last character equal to one of given characters.
    auto find_last_of(const basic_cstring &other, size_type pos=npos) const noexcept {
        return view().find_last_of(other.view(), pos);
    }
    auto find_last_of(const_pointer str, size_type pos, size_type len) const {
        return view().find_last_of(sanitize(str), pos, len);
    }
    auto find_last_of(const_pointer str, size_type pos=npos) const {
        return view().find_last_of(sanitize(str), pos);
    }
    auto find_last_of(value_type c, size_type pos=npos) const noexcept {
        return view().find_last_of(c, pos);
    }

    //! Finds the last character equal to none of the given characters.
    auto find_last_not_of(const basic_cstring &other, size_type pos=npos) const noexcept {
        return view().find_last_not_of(other.view(), pos);
    }
    auto find_last_not_of(const_pointer str, size_type pos, size_type len) const {
        return view().find_last_not_of(sanitize(str), pos, len);
    }
    auto find_last_not_of(const_pointer str, size_type pos=npos) const {
        return view().find_last_not_of(sanitize(str), pos);
    }
    auto find_last_not_of(value_type c, size_type pos=npos) const noexcept {
        return view().find_last_not_of(c, pos);
    }

    //! Checks if the string contains the given substring.
    bool contains(basic_cstring_view sv) const noexcept {
        return (view().find(sv) != npos);
    }
    bool contains(value_type c) const noexcept {
        return (find(c) != npos);
    }
    bool contains(const_pointer str) const noexcept {
        return (find(str) != npos);
    }

    //! Left trim spaces.
    basic_cstring_view ltrim() const
    {
        const_pointer ptr = m_str;
        while (std::isspace(*ptr)) ptr++;
        return basic_cstring_view(ptr);
    }

    //! Right trim spaces.
    basic_cstring_view rtrim() const
    {
        const_pointer ptr = m_str + length() - 1;
        while (ptr >= m_str && std::isspace(*ptr)) ptr--;
        ptr++;
        return basic_cstring_view(m_str, static_cast<size_type>(ptr - m_str));
    }

    //! Trim spaces.
    basic_cstring_view trim() const
    {
        const_pointer ptr1 = m_str;
        const_pointer ptr2 = m_str + length() - 1;
        while (std::isspace(*ptr1)) ptr1++;
        while (ptr2 >= ptr1 && std::isspace(*ptr2)) ptr2--;
        ptr2++;
        return basic_cstring_view(ptr1, static_cast<size_type>(ptr2 - ptr1));
    }

}; // class basic_cstring

//! Static variable declaration
template<typename Char, typename Traits, typename Allocator>
typename gto::basic_cstring<Char, Traits, Allocator>::allocator_type gto::basic_cstring<Char, Traits, Allocator>::m_allocator{};

//! Comparison operators (between basic_cstring)
template<typename Char, typename Traits, typename Allocator>
inline bool operator==(const basic_cstring<Char,Traits,Allocator> &lhs, const basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    return (lhs.compare(rhs) == 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator!=(const basic_cstring<Char,Traits,Allocator> &lhs, const basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    return (lhs.compare(rhs) != 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator<(const basic_cstring<Char,Traits,Allocator> &lhs, const basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    return (lhs.compare(rhs) < 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator<=(const basic_cstring<Char,Traits,Allocator> &lhs, const basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    return (lhs.compare(rhs) <= 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator>(const basic_cstring<Char,Traits,Allocator> &lhs, const basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    return (lhs.compare(rhs) > 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator>=(const basic_cstring<Char,Traits,Allocator> &lhs, const basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    return (lhs.compare(rhs) >= 0);
}

//! Comparison operators (between basic_cstring and Char*)
template<typename Char, typename Traits, typename Allocator>
inline bool operator==(const basic_cstring<Char,Traits,Allocator> &lhs, const Char *rhs) noexcept {
    return (lhs.compare(rhs) == 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator!=(const basic_cstring<Char,Traits,Allocator> &lhs, const Char *rhs) noexcept {
    return (lhs.compare(rhs) != 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator<(const basic_cstring<Char,Traits,Allocator> &lhs, const Char *rhs) noexcept {
    return (lhs.compare(rhs) < 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator<=(const basic_cstring<Char,Traits,Allocator> &lhs, const Char *rhs) noexcept {
    return (lhs.compare(rhs) <= 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator>(const basic_cstring<Char,Traits,Allocator> &lhs, const Char *rhs) noexcept {
    return (lhs.compare(rhs) > 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator>=(const basic_cstring<Char,Traits,Allocator> &lhs, const Char *rhs) noexcept {
    return (lhs.compare(rhs) >= 0);
}

//! Comparison operators (between Char * and basic_cstring)
template<typename Char, typename Traits, typename Allocator>
inline bool operator==(const Char *lhs, const basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    return (rhs.compare(lhs) == 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator!=(const Char *lhs, const basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    return (rhs.compare(lhs) != 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator<(const Char *lhs, const basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    return (rhs.compare(lhs) > 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator<=(const Char *lhs, const basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    return (rhs.compare(lhs) >= 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator>(const Char *lhs, const basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    return (rhs.compare(lhs) < 0);
}
template<typename Char, typename Traits, typename Allocator>
inline bool operator>=(const Char *lhs, const basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    return (rhs.compare(lhs) <= 0);
}

//! Overloading the std::swap algorithm for std::basic_cstring.
template<typename Char, typename Traits, typename Allocator>
inline void swap(gto::basic_cstring<Char,Traits,Allocator> &lhs, gto::basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
    lhs.swap(rhs);
}

//! Performs stream output on basic_cstring.
template<typename Char, typename Traits, typename Allocator>
inline std::basic_ostream<Char,Traits> & operator<<(std::basic_ostream<Char,Traits> &os, const gto::basic_cstring<Char,Traits,Allocator> &str) {
    return operator<<(os, str.view());
}

//! Utility class to compare basic_cstring to other types.
template<typename Char,
         typename Traits = std::char_traits<Char>,
         typename Allocator = std::allocator<Char>>
struct basic_cstring_compare
{
    using is_transparent = std::true_type;

    // basic_cstring vs basic_cstring
    bool operator()(const basic_cstring<Char, Traits, Allocator>& lhs, const basic_cstring<Char, Traits, Allocator>& rhs) const noexcept {
        return lhs.compare(rhs) < 0;
    }

    // basic_cstring vs const Char *
    bool operator()(const basic_cstring<Char, Traits, Allocator>& lhs, const Char* rhs) const noexcept {
        return lhs.compare(rhs) < 0;
    }
    bool operator()(const Char* lhs, const basic_cstring<Char, Traits, Allocator>& rhs) const noexcept {
        return rhs.compare(lhs) > 0;
    }

    // basic_cstring vs std::basic:string
    bool operator()(const basic_cstring<Char, Traits, Allocator>& lhs, const std::basic_string<Char, Traits, Allocator>& rhs) const noexcept {
        return lhs.compare(rhs.c_str()) < 0;
    }
    bool operator()(const std::basic_string<Char, Traits, Allocator>& lhs, const basic_cstring<Char, Traits, Allocator>& rhs) const noexcept {
        return rhs.compare(lhs.c_str()) > 0;
    }

    // basic_cstring vs std::basic:string_view
    bool operator()(const basic_cstring<Char, Traits, Allocator>& lhs, const std::basic_string_view<Char, Traits>& rhs) const noexcept {
        return lhs.compare(rhs) < 0;
    }
    bool operator()(const std::basic_string_view<Char, Traits>& lhs, const basic_cstring<Char, Traits, Allocator>& rhs) const noexcept {
        return rhs.compare(lhs) > 0;
    }
};

// templates incarnations
using cstring = basic_cstring<char>;
using wcstring = basic_cstring<wchar_t> ;
using cstring_compare = basic_cstring_compare<char>;
using wcstring_compare = basic_cstring_compare<wchar_t>;

} // namespace gto

//! The template specializations of std::hash for gto::cstring.
template<>
struct std::hash<gto::cstring> {
    std::size_t operator()(const gto::cstring &str) const {
        return std::hash<std::string_view>()(str.view());
    }
};

//! The template specializations of std::hash for gto::wcstring.
template<>
struct std::hash<gto::wcstring> {
    std::size_t operator()(const gto::wcstring &str) const {
        return std::hash<std::wstring_view>()(str.view());
    }
};
