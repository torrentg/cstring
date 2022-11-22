#pragma once

#include <memory>
#include <string>
#include <limits>
#include <atomic>
#include <utility>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace gto {

/**
 * @brief Immutable string based on a plain C-string (char *) with ref-counting.
 * @details
 *   - Shared content between multiple instances (using ref counting).
 *   - Automatic mem dealloc (when no refs point to content).
 *   - Same sizeof than a 'char *'.
 *   - Null not allowed (equals to empty string).
 *   - Empty string don't require alloc.
 *   - String content available on debug.
 *   - Mimics the STL basic_string class.
 * @details Memory layout:
 * 
 *       ----|----|-----------NUL
 *        ^   ^    ^
 *        |   |    |-- string content (NUL-terminated)
 *        |   |-- string length (4-bytes)
 *        |-- ref counter (4-bytes)
 * 
 *   mStr (cstring pointer) points to the string content (to allow view content on debug).
 *   Allocated memory is aligned to ref counter type size.
 *   Allocated memory is a multiple of ref counter type size.
 * @see https://en.cppreference.com/w/cpp/string/basic_string
 * @see https://github.com/torrentg/cstring
 * @note This class is immutable.
 * @version 1.0.0
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

    static constexpr size_type npos = std::numeric_limits<size_type>::max();

    //! Returns the maximum number of elements the string is able to hold.
    static constexpr size_type max_size() noexcept {
      return std::numeric_limits<prefix_type>::max() - sizeof(prefix_type) / sizeof(value_type);
    }

  private: // static members

    struct EmptyCString { 
      atomic_prefix_type counter{0};
      prefix_type len{0};
      value_type str[2] = {value_type(), value_type()};
    };

    static allocator_type alloc;
    static constexpr EmptyCString mEmpty{};

  private: // members

    //! Memory buffer with prefix_type alignment.
    const_pointer mStr = nullptr;

  private: // static methods

    //! Sanitize a char array pointer avoiding nulls.
    static inline constexpr const_pointer sanitize(const_pointer str) {
      return ((str == nullptr || str[0] == value_type()) ? mEmpty.str : str);
    }

    //! Return pointer to counter from pointer to string.
    static inline constexpr atomic_prefix_type * getPtrToCounter(const_pointer str) {
      static_assert(sizeof(atomic_prefix_type) == sizeof(prefix_type));
      static_assert(sizeof(value_type) <= sizeof(prefix_type));
      assert(str != nullptr);
      pointer ptr = const_cast<pointer>(str) - (2 * sizeof(prefix_type)) / sizeof(value_type);
      return reinterpret_cast<atomic_prefix_type *>(ptr);
    }

    //! Return pointer to string length from pointer to string.
    static inline constexpr prefix_type * getPtrToLength(const_pointer str) {
      static_assert(sizeof(value_type) <= sizeof(prefix_type));
      assert(str != nullptr);
      pointer ptr = const_cast<pointer>(str) - (sizeof(prefix_type) / sizeof(value_type));
      return reinterpret_cast<prefix_type *>(ptr);
    }

    //! Return pointer to string from pointer to counter.
    static inline constexpr const_pointer getPtrToString(const prefix_type *ptr) {
      static_assert(sizeof(atomic_prefix_type) == sizeof(prefix_type));
      assert(ptr != nullptr);
      return reinterpret_cast<const_pointer>(ptr + 2);
    }

    //! Returns the allocated array length (of prefix_type values).
    //! @details It is granted that there is place for the ending NULL.
    static size_type getAllocatedLength(size_type len) {
      return (3 + (len * sizeof(value_type)) / sizeof(prefix_type));
    }

    //! Allocate memory for the counter + length + string + eof. Returns a pointer to string.
    static pointer allocate(size_type len) {
      static_assert(sizeof(atomic_prefix_type) == sizeof(prefix_type));
      static_assert(alignof(value_type) <= alignof(prefix_type));
      static_assert(sizeof(value_type) <= sizeof(prefix_type));
      assert(len > 0 && len <= max_size());
      size_type n = getAllocatedLength(len);
      prefix_type *ptr = allocator_traits::allocate(alloc, n);
      assert(reinterpret_cast<std::size_t>(ptr) % alignof(prefix_type) == 0);
      allocator_traits::construct(alloc, ptr, 1);
      ptr[1] = static_cast<prefix_type>(len);
      return const_cast<pointer>(getPtrToString(ptr));
    }

    //! Deallocate string memory if no more references.
    static void deallocate(const_pointer str) {
      atomic_prefix_type *ptr = getPtrToCounter(str);
      prefix_type counts = ptr[0].load(std::memory_order_relaxed);

      if (counts == 0) { // constant (eg. mEmpty)
        return;
      } 
      
      if (counts > 1) {
        counts = ptr[0].fetch_sub(1, std::memory_order_relaxed);
      }

      if (counts == 1) {
        prefix_type len = *getPtrToLength(str);
        size_type n = getAllocatedLength(len);
        allocator_traits::destroy(alloc, ptr);
        allocator_traits::deallocate(alloc, reinterpret_cast<prefix_type *>(ptr), n);
      }
    }

    //! Increment the reference counter (except for constants).
    static void incrementRefCounter(const_pointer str) {
      atomic_prefix_type *ptr = getPtrToCounter(str);
      if (ptr[0].load(std::memory_order_relaxed) > 0) {
        ptr[0].fetch_add(1, std::memory_order_relaxed);
      }
    }

  public: // methods

    //! Default constructor.
    basic_cstring() : basic_cstring(nullptr) {}
    //! Constructor.
    basic_cstring(const_pointer str) : basic_cstring(str, (str == nullptr ? 0 : traits_type::length(str))) {}
    //! Constructor.
    basic_cstring(const_pointer str, size_type len) {
      if (len > max_size()) {
        throw std::length_error("invalid cstring length");
      } else if (str == nullptr || len == 0) {
        mStr = mEmpty.str;
      } else {
        pointer content = allocate(len);
        traits_type::copy(content, str, len);
        content[len] = value_type();
        mStr = content;
      }
    }
    //! Destructor.
    ~basic_cstring() { deallocate(mStr); }

    //! Copy constructor.
    basic_cstring(const basic_cstring &other) noexcept : mStr(other.mStr) { incrementRefCounter(mStr); }
    //! Move constructor.
    basic_cstring(basic_cstring &&other) noexcept : mStr(std::exchange(other.mStr, mEmpty.str)) {}

    //! Copy assignment.
    basic_cstring & operator=(const basic_cstring &other) { 
      if (mStr == other.mStr) {
        return *this;
      }
      deallocate(mStr);
      mStr = other.mStr;
      incrementRefCounter(mStr);
      return *this;
    }
    //! Move assignment.
    basic_cstring & operator=(basic_cstring &&other) noexcept { std::swap(mStr, other.mStr); return *this; }

    //! Return length of string.
    size_type size() const noexcept { return *(getPtrToLength(mStr)); }
    //! Return length of string.
    size_type length() const noexcept { return *(getPtrToLength(mStr)); }
    //! Test if string is empty.
    bool empty() const noexcept { return (length() == 0); }

    //! Get character of string.
    const_reference operator[](size_type pos) const { return mStr[pos]; }
    //! Get character of string checking for out_of_range.
    const_reference at(size_type pos) const { return (empty() || pos >= length() ? throw std::out_of_range("cstring::at") : mStr[pos]); }
    //! Get last character of the string.
    const_reference back() const { return (empty() ? throw std::out_of_range("cstring::back") : mStr[length()-1]); }
    //! Get first character of the string.
    const_reference front() const { return (empty() ? throw std::out_of_range("cstring::front") : mStr[0]); }

    //! Returns a non-null pointer to a null-terminated character array.
    inline const_pointer data() const noexcept { assert(mStr != nullptr); return mStr; }
    //! Returns a non-null pointer to a null-terminated character array.
    inline const_pointer c_str() const noexcept { return data(); }
    //! Returns a string_view of content.
    inline basic_cstring_view view() const { return basic_cstring_view(mStr, length()); }

    // Const iterator to the begin.
    const_iterator cbegin() const noexcept { return view().cbegin(); }
    // Const iterator to the end.
    const_iterator cend() const noexcept { return view().cend(); }
    // Const reverse iterator to the begin.
    const_reverse_iterator crbegin() const noexcept { return view().crbegin(); }
    // Const reverse iterator to the end.
    const_reverse_iterator crend() const noexcept { return view().crend(); }

    //! Exchanges the contents of the string with those of other.
    void swap(basic_cstring &other) noexcept { std::swap(mStr, other.mStr); }

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
      return (compare(0, len, sv.data()) == 0);
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
      return (len1 >= len2 && compare(len1-len2, len2, sv.data()) == 0);
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
    basic_cstring_view ltrim() const {
      const_pointer ptr = mStr;
      while (std::isspace(*ptr)) ptr++;
      return basic_cstring_view(ptr);
    }

    //! Right trim spaces.
    basic_cstring_view rtrim() const {
      const_pointer ptr = mStr + length() - 1;
      while (ptr >= mStr && std::isspace(*ptr)) ptr--;
      ptr++;
      return basic_cstring_view(mStr, static_cast<size_type>(ptr - mStr));
    }

    //! Trim spaces.
    basic_cstring_view trim() const {
      const_pointer ptr1 = mStr;
      const_pointer ptr2 = mStr + length() - 1;
      while (std::isspace(*ptr1)) ptr1++;
      while (ptr2 >= ptr1 && std::isspace(*ptr2)) ptr2--;
      ptr2++;
      return basic_cstring_view(ptr1, static_cast<size_type>(ptr2 - ptr1));
    }

}; // namespace gto

//! Static variable declaration
template<typename Char, typename Traits, typename Allocator>
typename gto::basic_cstring<Char, Traits, Allocator>::allocator_type gto::basic_cstring<Char, Traits, Allocator>::alloc{};

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

// template incarnations
typedef basic_cstring<char> cstring;
typedef basic_cstring<wchar_t> wcstring;
typedef basic_cstring<char>::basic_cstring_view cstring_view;
typedef basic_cstring<wchar_t>::basic_cstring_view wcstring_view;

} // namespace gto

namespace std {

//! Specializes the std::swap algorithm for std::basic_cstring.
template<typename Char, typename Traits, typename Allocator>
inline void swap(gto::basic_cstring<Char,Traits,Allocator> &lhs, gto::basic_cstring<Char,Traits,Allocator> &rhs) noexcept {
  lhs.swap(rhs);
}

//! Performs stream output on basic_cstring.
template<typename Char, typename Traits, typename Allocator>
inline basic_ostream<Char,Traits> & operator<<(std::basic_ostream<Char,Traits> &os, const gto::basic_cstring<Char,Traits,Allocator> &str) {
  return operator<<(os, str.view());
}

//! The template specializations of std::hash for gto::cstring.
template<>
struct hash<gto::cstring> {
  std::size_t operator()(const gto::cstring &str) const {
    return hash<std::string_view>()(str.view());
  }
};

//! The template specializations of std::hash for gto::wcstring.
template<>
struct hash<gto::wcstring> {
  std::size_t operator()(const gto::wcstring &str) const {
    return hash<std::wstring_view>()(str.view());
  }
};

} // namespace std
