# TODO

* ~~Change the comment 0-ended to NUL-terminated~~
* Review public/private blocks
* T~~he prefix_type and atomic_prefix_type have no reason to be public~~
* ~~Add static_assert(alignof(Char) <= alignof(prefix_type)), etc...~~
  * ~~to validate (and document) each assumption that is made.~~
  * ~~I recommend putting those static_assert where the assumptions are used, such as in the getPtrToCounter and getPtrToLength.~~
* ~~Since you have no synchronization with another piece of memory, you can instead use the Relaxed memory ordering.~~
* ~~Your definition of mEmpty violates strict-aliasing.~~
  * ~~struct EmptyString { atomic_prefix_type r; prefix_type s; value_type z; };~~
  * ~~static constexpr EmptyString mEmpty = {};~~
* The getAllocatedLength function could benefit from a comment explaining what is going on, because that's quite unclear.
* ~~In allocate, you never check that n > len~~
* It would be better for deallocate just to deallocate, and to have a decrementRefCounter function instead.
* Review comments
* Mark noexcept functions that cannot throw an exception, such as your default constructor, operator[], etc...
* ~~Review if-else~~
  * ~~If an if block ends with return, there is no need for an else~~
  * ~~Also, even when an if has a single statement in its block, do use {} around it.~~
* Your operator== and friends are declared in the global namespace, instead of being declared in the gto namespace.
* It is better to specialize std algorithms in the global namespace, rather than open the std namespace.
* The definitions of swap and operator<< are NOT specializations, they're overloading. They should be in the gto namespace, instead.
* Add credits to readme
  * https://codereview.stackexchange.com/questions/281365/an-immutable-c-string-with-ref-counting/281392#281392
* Test using Char = char16_6 and char32_t
