#ifndef _____EXTRA_ALGO_HPP_____
#define _____EXTRA_ALGO_HPP_____
#include <algorithm>

template<class I, class F> I find_last_if (I begin, const I& end, const F& eq) {
I last = end;
for (; begin!=end; ++begin) {
if (eq(*begin)) last = begin;
}
return last;
}

template<class I, class F> std::pair<I,I> find_consecutive (const I& begin, const I& end, const F& pred) {
auto first = std::find_if(begin, end, pred);
if (first==end) return std::make_pair(end, end);
auto last = std::find_if_not(first, end, pred);
return std::make_pair(first, last);
}

template<class T, class C> static inline void insert_n (C& container, int n, const T& val) {
if (n>0) std::fill_n(std::back_inserter(container), n, val);
}


#endif