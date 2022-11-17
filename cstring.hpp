#pragma once

#include <cstdint>
#include <climits>
#include <cassert>
#include <utility>
#include <memory>
#include <string>
#include <limits>
#include <atomic>
#include <stdexcept>
#include <string_view>

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
 *       ----|----|-----------0
 *        ^   ^    ^
 *        |   |    |-- string content (0-ended)
 *        |   |-- string length (4-bytes)
 *        |-- ref counter (4-bytes)
 * 
 *   mStr (cstring pointer) points to the string content (to allow view content on debug).
 *   Allocated memory is aligned to ref counter type size.
 *   Allocated memory is a multiple of ref counter type size.
 * @todo
 *   - Check that processor assumes memory alignment or we need to add __builtin_assume_aligned(a)) or __attribute((aligned(4)))
 *   - Check that std::atomic is enough to grant integrity in a multi-threaded usage
 *   - Explore cache invalidation impact on multi-threaded code
 *   - Performance tests
 * @details This class is immutable.
 * @details This class is not thread-safe.
 * @see https://en.cppreference.com/w/cpp/string/basic_string
 */
template<typename Char,
         typename Traits = std::char_traits<Char>,
         typename Alloc = std::allocator<Char>>
class basic_cstring
{

  public: // declarations

    typedef Traits traits_type;
    typedef Alloc allocator_type;
    typedef Char value_type;
    typedef const Char & const_reference;
    typedef const Char * const_pointer;
    typedef typename Alloc::difference_type difference_type;
    typedef typename Alloc::size_type size_type;
    typedef const Char * const_iterator;
    typedef typename std::reverse_iterator<const Char *> const_reverse_iterator;
    typedef std::basic_string_view<Char, Traits> basic_cstring_view;
    typedef std::atomic_uint32_t ref_counter_type;
    using CounterAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ref_counter_type>;

  private: // declarations

    template<unsigned int N>
    struct MemLayout {
      static_assert(N != 0);
      ref_counter_type counter{0};
      std::uint32_t length{N-1};
      std::array<Char, N> str{};
      explicit MemLayout(const char (&x)[N]) { for (unsigned int i=0; i<N; i++) str[i] = x[i]; }
    };

  public: // static members

    static constexpr size_type npos = static_cast<size_type>(-1);

  private: // static members

    static CounterAlloc alloc;
    static constexpr ref_counter_type mEmpty[3] = {0};

  private: // members

    //! Memory buffer with ref_counter_type alignment.
    const Char *mStr = nullptr;

  private: // static methods

    //! Sanitize a char array pointer avoiding nulls.
    static inline constexpr const Char * sanitize(const Char *str) {
      return (str == nullptr ? "" : str);
    }

    //! Return pointer to counter from pointer to string.
    static inline constexpr ref_counter_type * getPtrToCounter(const Char *str) {
      assert(str != nullptr);
      Char *ptr = const_cast<Char *>(str) - 2*sizeof(ref_counter_type);
      return reinterpret_cast<ref_counter_type *>(ptr);
    }

    //! Return pointer to string length from pointer to counter.
    static inline constexpr std::uint32_t * getPtrToLength(const ref_counter_type *ptr) {
      assert(ptr != nullptr);
      return reinterpret_cast<std::uint32_t *>(const_cast<ref_counter_type *>(ptr) + 1);
    }

    //! Return pointer to string length from pointer to string.
    static inline constexpr std::uint32_t * getPtrToLength(const Char *str) {
      return getPtrToLength(getPtrToCounter(str));
    }

    //! Return pointer to string from pointer to counter.
    static inline constexpr const Char * getPtrToString(const ref_counter_type *ptr) {
      assert(ptr != nullptr);
      return reinterpret_cast<Char *>(const_cast<ref_counter_type *>(ptr) + 2);
    }

    //! Allocate memory for the counter + length + string + eof. Returns a pointer to string.
    static const Char * allocate(std::size_t len) {
      assert(len > 0);
      assert(len <= std::numeric_limits<std::uint32_t>::max());
      std::size_t n = 3 + (len * sizeof(Char)) / sizeof(ref_counter_type);
      ref_counter_type *ptr = std::allocator_traits<CounterAlloc>::allocate(alloc, n);
      std::allocator_traits<CounterAlloc>::construct(alloc, ptr, 1);
      assert(reinterpret_cast<std::size_t>(ptr)%alignof(ref_counter_type) == 0);
      getPtrToLength(ptr)[0] = static_cast<std::uint32_t>(len);
      return getPtrToString(ptr);
    }

    //! Deallocate string memory if no more references.
    static void deallocate(const Char *str) {
      ref_counter_type *ptr = getPtrToCounter(str);
      switch(ptr[0]) {
        case 0: // constant
          break;
        case 1: // no more references
          std::allocator_traits<CounterAlloc>::destroy(alloc, ptr);
          std::allocator_traits<CounterAlloc>::deallocate(alloc, ptr, 1);
          break;
        default:
          ptr[0]--;
      }
    }

    //! Increment the reference counter (except for constants).
    static void incrementRefCounter(const Char *str) {
      ref_counter_type *ptr = getPtrToCounter(str);
      if (ptr[0] > 0) {
        ptr[0]++;
      }
    }

  private: // methods

    //! Constructor (used to create static cstrings).
    template<unsigned int N>
    explicit basic_cstring(const MemLayout<N> &layout) {
      assert(layout.counter == 0);
      assert(layout.length != 0);
      assert(layout.str[layout.length] == 0);
      mStr = &(layout.str[0]);
    }

  public: // static methods

    //! Create a static cstring instance (no mem alloc).
    template<unsigned int N>
    static basic_cstring make_static(const Char (&x)[N]) {
      return basic_cstring(MemLayout<N>(x));
    }

  public: // methods

    //! Default constructor.
    basic_cstring() : basic_cstring(nullptr) {}
    //! Constructor (from Char *).
    basic_cstring(const Char *str) : basic_cstring(str, (str == nullptr ? 0 : traits_type::length(str))) {}
    //! Constructor (from Char *).
    basic_cstring(const Char *str, std::size_t len) {
      if (str == nullptr || len == 0) {
        mStr = getPtrToString(mEmpty);
        return;
      }
      mStr = allocate(len);
      Char *content = const_cast<Char *>(mStr);
      traits_type::copy(content, str, len);
      content[len] = Char();
    }
    //! Destructor.
    ~basic_cstring() { deallocate(mStr); }

    //! Copy constructor.
    basic_cstring(const basic_cstring &other) noexcept : mStr(other.mStr) { incrementRefCounter(mStr); }
    //! Move constructor.
    basic_cstring(basic_cstring &&other) noexcept : mStr(std::exchange(other.mStr, getPtrToString(mEmpty))) {}

    //! Copy assignment.
    basic_cstring & operator=(const basic_cstring &other) { 
      if (mStr == other.mStr) return *this;
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
    bool empty() const noexcept { return (mStr[0] == Char()); }

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
    void swap(basic_cstring<Char,Traits,Alloc> &other) noexcept { std::swap(mStr, other.mStr); }

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
    int compare(const Char *str) const {
      return view().compare(sanitize(str));
    }
    int compare(size_type pos, size_type len, const Char *str) const {
      return substr(pos, len).compare(sanitize(str));
    }
    int compare(size_type pos, size_type len, const Char *str, size_type len2) const {
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
      size_type len = sv.length();
      return (compare(0, len, sv.data()) == 0);
    }
    bool starts_with(const Char *str) const noexcept {
      return starts_with(basic_cstring_view(sanitize(str)));
    }

    //! Checks if the string ends with the given suffix.
    bool ends_with(const basic_cstring &other) const noexcept {
      size_type len1 = length();
      size_type len2 = other.length();
      return (len1 >= len2 && compare(len1-len2, len2, other) == 0);
    }
    bool ends_with(const basic_cstring_view sv) const noexcept {
      size_type len1 = length();
      size_type len2 = sv.length();
      return (len1 >= len2 && compare(len1-len2, len2, sv.data()) == 0);
    }
    bool ends_with(const Char *str) const noexcept {
      return ends_with(basic_cstring_view(sanitize(str)));
    }

    //! Find the first ocurrence of a substring.
    size_type find(const basic_cstring &other, size_type pos=0) const noexcept{
      return view().find(other.view(), pos);
    }
    size_type find(const Char *str, size_type pos, size_type len) const {
      return view().find(sanitize(str), pos, len);
    }
    size_type find(const Char *str, size_type pos=0) const {
      return view().find(sanitize(str), pos);
    }
    size_type find(Char c, size_type pos=0) const noexcept {
      return view().find(c, pos);
    }

    //! Find the last occurrence of a substring.
    size_type rfind(const basic_cstring &other, size_type pos=npos) const noexcept{
      return view().rfind(other.view(), pos);
    }
    size_type rfind(const Char *str, size_type pos, size_type len) const {
      return view().rfind(sanitize(str), pos, len);
    }
    size_type rfind(const Char *str, size_type pos=npos) const {
      return view().rfind(sanitize(str), pos);
    }
    size_type rfind(Char c, size_type pos=npos) const noexcept {
      return view().rfind(c, pos);
    }

    //! Finds the first character equal to one of the given characters.
    size_type find_first_of(const basic_cstring &other, size_type pos=0) const noexcept {
      return view().find_first_of(other.view(), pos);
    }
    size_type find_first_of(const Char *str, size_type pos, size_type len) const {
      return view().find_first_of(sanitize(str), pos, len);
    }
    size_type find_first_of(const Char *str, size_type pos=0) const {
      return view().find_first_of(sanitize(str), pos);
    }
    size_type find_first_of(Char c, size_type pos=0) const noexcept {
      return view().find_first_of(c, pos);
    }

    //! Finds the first character equal to none of the given characters.
    size_type find_first_not_of(const basic_cstring &other, size_type pos=0) const noexcept {
      return view().find_first_not_of(other.view(), pos);
    }
    size_type find_first_not_of(const Char *str, size_type pos, size_type len) const {
      return view().find_first_not_of(sanitize(str), pos, len);
    }
    size_type find_first_not_of(const Char *str, size_type pos=0) const {
      return view().find_first_not_of(sanitize(str), pos);
    }
    size_type find_first_not_of(Char c, size_type pos=0) const noexcept {
      return view().find_first_not_of(c, pos);
    }

    //! Finds the last character equal to one of given characters.
    size_type find_last_of(const basic_cstring &other, size_type pos=npos) const noexcept {
      return view().find_last_of(other.view(), pos);
    }
    size_type find_last_of(const Char *str, size_type pos, size_type len) const {
      return view().find_last_of(sanitize(str), pos, len);
    }
    size_type find_last_of(const Char *str, size_type pos=npos) const {
      return view().find_last_of(sanitize(str), pos);
    }
    size_type find_last_of(Char c, size_type pos=npos) const noexcept {
      return view().find_last_of(c, pos);
    }

    //! Finds the last character equal to none of the given characters.
    size_type find_last_not_of(const basic_cstring &other, size_type pos=npos) const noexcept {
      return view().find_last_not_of(other.view(), pos);
    }
    size_type find_last_not_of(const Char *str, size_type pos, size_type len) const {
      return view().find_last_not_of(sanitize(str), pos, len);
    }
    size_type find_last_not_of(const Char *str, size_type pos=npos) const {
      return view().find_last_not_of(sanitize(str), pos);
    }
    size_type find_last_not_of(Char c, size_type pos=npos) const noexcept {
      return view().find_last_not_of(c, pos);
    }

    //! Checks if the string contains the given substring.
    bool contains(basic_cstring_view sv) const noexcept {
      return (view().find(sv) != npos);
    }
    bool contains(Char c) const noexcept {
      return (find(c) != npos);
    }
    bool contains(const Char *str) const noexcept {
      return (find(str) != npos);
    }

    //! Left trim spaces.
    basic_cstring_view ltrim() const {
      const Char *ptr = mStr;
      while (std::isspace(*ptr)) ptr++;
      return basic_cstring_view(ptr);
    }

    //! Right trim spaces.
    basic_cstring_view rtrim() const {
      const Char *ptr = mStr + length() - 1;
      while (ptr >= mStr && std::isspace(*ptr)) ptr--;
      ptr++;
      return basic_cstring_view(mStr, static_cast<size_type>(ptr - mStr));
    }

    //! Trim spaces.
    basic_cstring_view trim() const {
      const Char *ptr1 = mStr;
      const Char *ptr2 = mStr + length() - 1;
      while (std::isspace(*ptr1)) ptr1++;
      while (ptr2 >= ptr1 && std::isspace(*ptr2)) ptr2--;
      ptr2++;
      return basic_cstring_view(ptr1, static_cast<size_type>(ptr2 - ptr1));
    }

};

//! Static variable declaration
template<typename Char, typename Traits, typename Alloc>
typename gto::basic_cstring<Char, Traits, Alloc>::CounterAlloc gto::basic_cstring<Char, Traits, Alloc>::alloc{};

//! Comparison operators (between basic_cstring)
template<typename Char, typename Traits, typename Alloc>
inline bool operator==(const basic_cstring<Char,Traits,Alloc> &lhs, const basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
  return (lhs.compare(rhs) == 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator!=(const basic_cstring<Char,Traits,Alloc> &lhs, const basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
  return (lhs.compare(rhs) != 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator<(const basic_cstring<Char,Traits,Alloc> &lhs, const basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
  return (lhs.compare(rhs) < 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator<=(const basic_cstring<Char,Traits,Alloc> &lhs, const basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
  return (lhs.compare(rhs) <= 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator>(const basic_cstring<Char,Traits,Alloc> &lhs, const basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
  return (lhs.compare(rhs) > 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator>=(const basic_cstring<Char,Traits,Alloc> &lhs, const basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
  return (lhs.compare(rhs) >= 0);
}

//! Comparison operators (between basic_cstring and Char*)
template<typename Char, typename Traits, typename Alloc>
inline bool operator==(const basic_cstring<Char,Traits,Alloc> &lhs, const Char *rhs) noexcept {
  return (lhs.compare(rhs) == 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator!=(const basic_cstring<Char,Traits,Alloc> &lhs, const Char *rhs) noexcept {
  return (lhs.compare(rhs) != 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator<(const basic_cstring<Char,Traits,Alloc> &lhs, const Char *rhs) noexcept {
  return (lhs.compare(rhs) < 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator<=(const basic_cstring<Char,Traits,Alloc> &lhs, const Char *rhs) noexcept {
  return (lhs.compare(rhs) <= 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator>(const basic_cstring<Char,Traits,Alloc> &lhs, const Char *rhs) noexcept {
  return (lhs.compare(rhs) > 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator>=(const basic_cstring<Char,Traits,Alloc> &lhs, const Char *rhs) noexcept {
  return (lhs.compare(rhs) >= 0);
}

//! Comparison operators (between Char * and basic_cstring)
template<typename Char, typename Traits, typename Alloc>
inline bool operator==(const Char *lhs, const basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
  return (rhs.compare(lhs) == 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator!=(const Char *lhs, const basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
  return (rhs.compare(lhs) != 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator<(const Char *lhs, const basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
  return (rhs.compare(lhs) > 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator<=(const Char *lhs, const basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
  return (rhs.compare(lhs) >= 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator>(const Char *lhs, const basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
  return (rhs.compare(lhs) < 0);
}
template<typename Char, typename Traits, typename Alloc>
inline bool operator>=(const Char *lhs, const basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
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
template<typename Char, typename Traits, typename Alloc>
inline void swap(gto::basic_cstring<Char,Traits,Alloc> &lhs, gto::basic_cstring<Char,Traits,Alloc> &rhs) noexcept {
  lhs.swap(rhs);
}

//! Performs stream output on basic_cstring.
template<typename Char, typename Traits, typename Alloc>
inline basic_ostream<Char,Traits> & operator<<(std::basic_ostream<Char,Traits> &os, const gto::basic_cstring<Char,Traits,Alloc> &str) {
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
