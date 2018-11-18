#ifndef ____Q_BASE_EXT_H_1___
#define ____Q_BASE_EXT_H_1___
#include "QValue.hpp"
#include<cstdlib>
#include<vector>
#include<unordered_map>
#include<unordered_set>
#ifndef NO_OPTIONAL_COLLECTIONS
#include<map>
#include<list>
#endif
#ifndef NO_RANDOM
#include<random>
#endif
#ifndef NO_REGEX
#include<boost/regex.hpp>
#endif

//https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
#define FNV_OFFSET 0x811c9dc5
#define FNV_PRIME 16777619

size_t hashBytes (const uint8_t* start, const uint8_t* end);
void appendToString (QFiber& f, QV x, std::string& out);

inline bool isDigit (uint32_t c) {
return c>='0' && c<='9';
}

template<class T> int strnatcmp (const T* L, const T* R) {
bool alphaMode=true;
while(*L&&*R) {
if (alphaMode) {
while(*L&&*R) {
bool lDigit = isDigit(*L), rDigit = isDigit(*R);
if (lDigit&&rDigit) { alphaMode=false; break; }
else if (lDigit) return -1;
else if (rDigit) return 1;
else if (*L!=*R) return *L-*R;
++L; ++R;
}} else { // numeric mode
T *lEnd=0, *rEnd=0;
unsigned long long int l = strtoull(L, &lEnd, 0), r = strtoull(R, &rEnd, 0);
if (lEnd) L=lEnd; 
if (rEnd) R=rEnd;
if (l!=r) return l-r;
alphaMode=true;
}}
if (*R) return -1;
else if (*L) return 1;
return 0;
}

template<class T> struct nat_less {
inline bool operator() (const T* l, const T* r) { return strnatcmp(l,r)<0; }
inline bool operator() (const std::basic_string<T>& l, const std::basic_string<T>& r) {  return strnatcmp(l.data(), r.data() )<0;  }
};

struct QVHasher {
size_t operator() (const QV& qv) const;
};

struct QVEqualler {
bool operator() (const QV& a, const QV& b) const;
};

struct QVBinaryPredicate  {
QV func;
QVBinaryPredicate (const QV& f): func(f) {}
bool operator() (const QV& a, const QV& b) const;
};

struct QVUnaryPredicate  {
QV func;
QVUnaryPredicate (const QV& f): func(f) {}
bool operator() (const QV& a) const;
};

struct QVLess {
bool operator() (const QV& a, const QV& b) const;
};

struct QBuffer: QSequence {
uint32_t length;
uint8_t data[];
static QBuffer* create (QVM& vm, const void* buf, int length);
template <class T> static inline QBuffer* create (QVM& vm, const T* start, const T* end) { return create(vm, start, (end-start)*sizeof(T)); }
static QBuffer* create (QBuffer*);
QBuffer (QVM& vm, uint32_t len);
inline uint8_t* begin () { return data; }
inline uint8_t* end () { return data+length; }
virtual ~QBuffer () = default;
};

struct QSet: QSequence {
typedef std::unordered_set<QV, QVHasher, QVEqualler> set_type;
typedef set_type::iterator iterator;
set_type set;
QSet (QVM& vm): QSequence(vm.setClass) {}
virtual void insertIntoVector (QFiber& f, std::vector<QV>& list, int start) override { list.insert(list.begin()+start, set.begin(), set.end()); }
virtual void insertIntoSet (QFiber& f, QSet& s) override { s.set.insert(set.begin(), set.end()); }
virtual void join (QFiber& f, const std::string& delim, std::string& out) override;
virtual ~QSet () = default;
virtual bool gcVisit () override;
};

struct QSetIterator: QObject {
QSet& set;
QSet::iterator iterator;
QSetIterator (QVM& vm, QSet& m): QObject(vm.objectClass), set(m), iterator(m.set.begin()) {}
virtual bool gcVisit () override;
virtual ~QSetIterator() = default;
};

struct QTuple: QSequence {
uint32_t length;
QV data[];
QTuple (QVM& vm, uint32_t len): QSequence(vm.tupleClass), length(len) {}
inline QV& at (int n) {
if (n<0) n+=length;
return data[n];
}
static QTuple* create (QVM& vm, uint32_t length, const QV* data);
virtual void insertIntoVector (QFiber& f, std::vector<QV>& list, int start) override { list.insert(list.begin()+start, data, data+length); }
virtual void insertIntoSet (QFiber& f, QSet& set) override { set.set.insert(data, data+length); }
virtual void join (QFiber& f, const std::string& delim, std::string& out) override;
virtual ~QTuple () = default;
virtual bool gcVisit () override;
};

struct QList: QSequence {
std::vector<QV> data;
QList (QVM& vm): QSequence(vm.listClass) {}
inline QV& at (int n) {
if (n<0) n+=data.size();
return data[n];
}
virtual void insertIntoVector (QFiber& f, std::vector<QV>& list, int start) override { list.insert(list.begin()+start, data.begin(), data.end()); }
virtual void insertIntoSet (QFiber& f, QSet& s) override { s.set.insert(data.begin(), data.end()); }
virtual void join (QFiber& f, const std::string& delim, std::string& out) override;
virtual ~QList () = default;
virtual bool gcVisit () override;
};

struct QMap: QSequence {
typedef std::unordered_map<QV, QV, QVHasher, QVEqualler> map_type;
typedef map_type::iterator iterator;
map_type map;
QMap (QVM& vm): QSequence(vm.mapClass) {}
inline QV get (const QV& key) {
auto it = map.find(key);
if (it==map.end()) return QV();
else return it->second;
}
inline QV& set (const QV& key, const QV& value) { return map[key] = value; }
virtual ~QMap () = default;
virtual bool gcVisit () override;
};

struct QMapIterator: QObject {
QMap& map;
QMap::iterator iterator;
QMapIterator (QVM& vm, QMap& m): QObject(vm.objectClass), map(m), iterator(m.map.begin()) {}
virtual bool gcVisit () override;
virtual ~QMapIterator() = default;
};

struct QRange: QSequence, QS::Range  {
QRange (QVM& vm, double s, double e, double p, bool i): QSequence(vm.rangeClass), QS::Range(s, e, p, i) {}
QRange (QVM& vm, const QS::Range& r): QSequence(vm.rangeClass), QS::Range(r) {}
QV iterate (const QV& x) {
if (x.isNull()) return start;
double val = x.asNum() + step;
if (inclusive) return (end-start) * (end -val) >= 0 ? val : QV();
else return (end-start) * (end -val) > 0 ? val : QV();
}
virtual ~QRange () = default;
};

#ifndef NO_OPTIONAL_COLLECTIONS
struct QDictionary: QSequence {
typedef std::map<QV, QV, QVBinaryPredicate> map_type; 
typedef map_type::iterator iterator;
map_type map;
QV sorter;
QDictionary (QVM& vm, QV& sorter0): QSequence(vm.dictionaryClass), map(sorter0), sorter(sorter0) {}
inline QV get (const QV& key) {
auto it = map.find(key);
if (it==map.end()) return QV();
else return it->second;
}
inline QV& set (const QV& key, const QV& value) { return map[key] = value; }
virtual ~QDictionary () = default;
virtual bool gcVisit () override;
};

struct QDictionaryIterator: QObject {
QDictionary& map;
QDictionary::iterator iterator;
QDictionaryIterator (QVM& vm, QDictionary& m): QObject(vm.objectClass), map(m), iterator(m.map.begin()) {}
virtual bool gcVisit () override;
virtual ~QDictionaryIterator() = default;
};

struct QLinkedList: QSequence {
typedef std::list<QV> list_type;
typedef list_type::iterator iterator;
list_type data;
QLinkedList (QVM& vm): QSequence(vm.linkedListClass) {}
inline QV& at (int n) {
int size = data.size();
iterator origin = data.begin();
if (n<0) origin = data.end();
else if (n>=size/2) { origin=data.end(); n-=size; }
std::advance(origin, n);
return *origin;
}
virtual void insertIntoVector (QFiber& f, std::vector<QV>& list, int start) override { list.insert(list.begin()+start, data.begin(), data.end()); }
virtual void insertIntoSet (QFiber& f, QSet& s) override { s.set.insert(data.begin(), data.end()); }
virtual void join (QFiber& f, const std::string& delim, std::string& out) override;
virtual ~QLinkedList () = default;
virtual bool gcVisit () override;
};

struct QLinkedListIterator: QObject {
QLinkedList& list;
QLinkedList::iterator iterator;
QLinkedListIterator (QVM& vm, QLinkedList& m): QObject(vm.objectClass), list(m), iterator(m.data.begin()) {}
virtual bool gcVisit () override;
virtual ~QLinkedListIterator() = default;
};
#endif

#ifndef NO_RANDOM
struct QRandom: QObject {
std::mt19937 rand;
QRandom (QVM& vm): QObject(vm.objectClass) {}
};
#endif

#ifndef NO_REGEX
struct QRegex: QObject {
boost::regex regex;
boost::regex_constants::match_flag_type matchOptions;
static std::pair<boost::regex_constants::syntax_option_type, boost::regex_constants::match_flag_type>  parseOptions (const char* options);
QRegex (QVM& vm, const char* begin, const char* end, boost::regex_constants::syntax_option_type regexOptions, boost::regex_constants::match_flag_type matchOptions);
virtual ~QRegex () = default;
};

struct QRegexMatchResult: QObject {
boost::cmatch match;
QRegexMatchResult (QVM& vm): QObject(vm.regexMatchResultClass) {}
QRegexMatchResult (QVM& vm, const boost::cmatch& m): QObject(vm.regexMatchResultClass), match(m)  {}
virtual ~QRegexMatchResult () = default;
};

struct QRegexIterator: QSequence {
boost::regex_iterator<const char*> it, end;
QString& str;
QRegex& regex;
QRegexIterator (QVM& vm, QString& s, QRegex& r, boost::regex_constants::match_flag_type options);
virtual bool gcVisit () override;
virtual ~QRegexIterator () = default;
};

struct QRegexTokenIterator: QSequence {
boost::regex_token_iterator<const char*> it, end;
QString& str;
QRegex& regex;
QRegexTokenIterator (QVM& vm, QString& s, QRegex& r, boost::regex_constants::match_flag_type options, int g);
virtual bool gcVisit () override;
virtual ~QRegexTokenIterator () = default;
};
#endif

#endif