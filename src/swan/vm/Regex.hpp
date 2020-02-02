#ifndef NO_REGEX
#ifndef _____SWAN_REGEX_HPP_____
#define _____SWAN_REGEX_HPP_____
#include "Iterable.hpp"
#include "String.hpp"
#include "VM.hpp"

#ifdef USE_BOOST_REGEX
#include<boost/regex.hpp>
using boost::regex;
namespace regex_constants = boost::regex_constants;
using boost::cmatch;
using boost::regex_iterator;
using boost::regex_token_iterator;
using boost::regex_match;
using boost::regex_search;
using boost::regex_replace;
#define REGEX_TEST_MATCH_FLAGS regex_constants::match_nosubs  | regex_constants::match_any
#define REGEX_SEARCH_NOT_FULL_DEFAULT_OPTIONS regex_constants::match_nosubs
#else
#include<regex>
using std::regex;
namespace regex_constants = std::regex_constants;
using std::cmatch;
using std::regex_iterator;
using std::regex_token_iterator;
using std::regex_match;
using std::regex_search;
using std::regex_replace;
#define REGEX_TEST_MATCH_FLAGS regex_constants::match_any
#define REGEX_SEARCH_NOT_FULL_DEFAULT_OPTIONS regex_constants::match_default
#endif


struct QRegex: QObject {
regex regex;
regex_constants::match_flag_type matchOptions;
static std::pair<regex_constants::syntax_option_type, regex_constants::match_flag_type>  parseOptions (const char* options);
QRegex (QVM& vm, const char* begin, const char* end, regex_constants::syntax_option_type regexOptions, regex_constants::match_flag_type matchOptions);
~QRegex () = default;
inline size_t getMemSize () { return sizeof(*this); }
};

struct QRegexMatchResult: QObject {
cmatch match;
QRegexMatchResult (QVM& vm): QObject(vm.regexMatchResultClass) {}
QRegexMatchResult (QVM& vm, const cmatch& m): QObject(vm.regexMatchResultClass), match(m)  {}
~QRegexMatchResult () = default;
inline size_t getMemSize () { return sizeof(*this); }
};

struct QRegexIterator: QSequence {
regex_iterator<const char*> it, end;
QString& str;
QRegex& regex;
QRegexIterator (QVM& vm, QString& s, QRegex& r, regex_constants::match_flag_type options);
bool gcVisit ();
~QRegexIterator () = default;
inline size_t getMemSize () { return sizeof(*this); }
};

struct QRegexTokenIterator: QSequence {
regex_token_iterator<const char*> it, end;
QString& str;
QRegex& regex;
QRegexTokenIterator (QVM& vm, QString& s, QRegex& r, regex_constants::match_flag_type options, int g);
bool gcVisit ();
~QRegexTokenIterator () = default;
inline size_t getMemSize () { return sizeof(*this); }
};
#endif

#endif