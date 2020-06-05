#include "Parser.hpp"
#include "Token.hpp"
#include "../vm/VM.hpp"
#include<boost/algorithm/string.hpp>
#include<utf8.h>
#include<unordered_map>
using namespace std;

extern std::unordered_map<std::string,QTokenType> KEYWORDS;

double strtod_c  (const char*, char** = nullptr);

template<class T> static inline uint32_t utf8inc (T& it, T end) {
if (it!=end) utf8::next(it, end);
return it==end? 0 : utf8::peek_next(it, end);
}

template<class T> static inline uint32_t utf8peek (T it, T end) {
return it==end? 0 : utf8::peek_next(it, end);
}

static inline ostream& operator<< (ostream& out, const QToken& token) {
return out << string(token.start, token.length);
}

bool isName (uint32_t c) {
return (c>='a' && c<='z') || (c>='A' && c<='Z') || c=='_' 
||(c>=0xC0 && c<0x2000 && c!=0xD7 && c!=0xF7)
|| (c>=0x2C00 && c<0x2E00)
|| (c>=0x2E80 && c<0xFFF0)
|| c>=0x10000;
}

bool isUpper (uint32_t c) {
return c>='A' && c<='Z';
}

static bool isSpaceOrIgnorableLine (uint32_t c, const char* in, const char* end) {
if (isSpace(c)) return true;
else if (!isLine(c)) return false;
while (in<end && (c=utf8::next(in, end)) && (isSpace(c) || isLine(c)));
return string(".+-/*&|").find(c)!=string::npos;
}

bool isDigit (uint32_t c) {
return c>='0' && c<='9';
}

static inline double parseNumber (const char*& in) {
if (*in=='0') {
switch(in[1]){
case 'x': case 'X': return strtoll(in+2, const_cast<char**>(&in), 16);
case 'o': case 'O': return strtoll(in+2, const_cast<char**>(&in), 8);
case 'b': case 'B': return strtoll(in+2, const_cast<char**>(&in), 2);
case '1': case '2': case '3': case '4': case '5': case '6': case '7': return strtoll(in+1, const_cast<char**>(&in), 8);
default: break;
}}
double d = strtod_c(in, const_cast<char**>(&in));
if (in[-1]=='.') in--;
return d;
}

static void skipComment (const char*& in, const char* end, char delim) {
int c = utf8peek(in, end);
if (isSpace(c) || isName(c) || c==delim) {
while(c && in<end && !isLine(c)) c=utf8::next(in, end);
return;
}
else if (!c || isLine(c)) return;
int opening = c, closing = c, nesting = 1;
if (opening=='[' || opening=='{' || opening=='<') closing = opening+2; 
else if (opening=='(') closing=')';
while(c = utf8inc(in, end)){
if (c==delim) {
if (utf8inc(in, end)==opening) nesting++;
}
else if (c==closing) {
if (utf8inc(in, end)==delim && --nesting<=0) { utf8::next(in, end); break; }
}}}

static int parseCodePointValue (QParser& parser, const char*& in, const char* end, int n, int base) {
char buf[n+1];
memcpy(buf, in, n);
buf[n]=0;
in += n;
auto c = strtoul(buf, nullptr, base);
if (c>=0x110000) {
parser.cur = { T_STRING, in -n -2, static_cast<size_t>(n+2), QV::UNDEFINED };
parser.parseError("Invalid code point");
return 0xFFFD;
}
return c;
}

static int getStringEndingChar (int c) {
switch(c){
case 147: return 148;
case 171: return 187;
case 8220: return 8221;
default:  return c;
}}

static QV parseString (QParser& parser, QVM& vm, const char*& in, const char* end, int ending) {
string re;
auto out = back_inserter(re);
int c=0;
auto begin = in;
while(in<end && (c=utf8::next(in, end))!=ending && c) {
if (c=='\n' && !vm.multilineStrings) break;
if (c=='\\') {
const char* ebegin = in -1;
c = utf8::next(in, end);
switch(c){
case 'b': c='\b'; break;
case 'e': c='\x1B'; break;
case 'f': c='\f'; break;
case 'n': c='\n'; break;
case 'r': c='\r'; break;
case 't': c='\t'; break;
case 'u': c = parseCodePointValue(parser, in, end, 4, 16); break;
case 'U': c = parseCodePointValue(parser, in, end, 8, 16); break;
case 'v': c = '\v'; break;
case 'x': c = parseCodePointValue(parser, in, end, 2, 16); break;
case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': 
c = strtoul(--in, const_cast<char**>(&in), 0); 
if (c>=0x110000) { parser.cur = { T_STRING, ebegin, static_cast<size_t>(in-ebegin), QV::UNDEFINED }; parser.parseError("Invalid code point"); c=0xFFFD; }
break;
default:
if ((c>='a' && c<='z') || (c>='a' && c<='Z')) {
parser.cur = { T_STRING, ebegin, static_cast<size_t>(in-ebegin), QV::UNDEFINED };
parser.parseError("Invalid escape sequence");
}
break;
}}
utf8::append(c, out);
}
if (c!=ending) {
parser.cur = { T_STRING, begin, static_cast<size_t>(in-begin), QV::UNDEFINED };
parser.parseError("Unterminated string");
}
return QString::create(vm, re);
}

const QToken& QParser::prevToken () {
stackedTokens.push_back(cur);
return cur=prev;
}

const QToken& QParser::nextNameToken (bool eatEq) {
prev=cur;
if (!stackedTokens.empty()) {
cur = stackedTokens.front();
stackedTokens.clear();
in = cur.start;
}
#define RET0(X) { cur = { X, start, static_cast<size_t>(in-start), QV::UNDEFINED}; return cur; }
#define RET RET0(T_NAME)
#define RET2(C) if (utf8peek(in, end)==C) utf8::next(in, end); RET
#define RET3(C1,C2) if(utf8peek(in, end)==C1 || utf8peek(in, end)==C2) utf8::next(in, end); RET
#define RET4(C1,C2,C3) if (utf8peek(in, end)==C1 || utf8peek(in, end)==C2 || utf8peek(in, end)==C3) utf8::next(in, end); RET
#define RET22(C1,C2) if (utf8peek(in, end)==C1) utf8::next(in, end); RET2(C2)
const char *start = in;
if (in>=end || !*in) RET0(T_END)
int c;
do {
c = utf8::next(in, end);
} while((isSpace(c) || isLine(c)) && *(start=in) && in<end);
switch(c){
case '\0': RET0(T_END)
case '(':
if (utf8peek(in, end)==')') {
utf8::next(in, end);
RET2('=')
}break;
case '[': 
if (utf8peek(in, end)==']') {
utf8::next(in, end);
RET2('=')
}break;
case '/': 
switch (utf8peek(in, end)) {
case '/': case '*': skipComment(in, end, '/'); return nextNameToken(eatEq);
default: RET
}
case '+': RET2('+')
case '-': RET2('-')
case '*': RET2('*')
case '\\': RET
case '%': RET
case '|': RET
case '&': RET
case '^': RET
case '~': RET
case '@': RET
case '!': RET2('=')
case '=': RET2('=')
case '>': RET3('>', '=')
//case '<': RET3('<', '=')
case '<':
if (utf8peek(in, end)=='=') { utf8::next(in, end); RET2('>') }
else { RET2('<') }
case '#': skipComment(in, end, '#'); return nextNameToken(eatEq);
}
if (isName(c)) {
while(in<end && (c=utf8::peek_next(in, end)) && (isName(c) || isDigit(c))) utf8::next(in, end);
if (in<end && utf8::peek_next(in, end)=='=' && eatEq) utf8::next(in, end);
RET
}
cur = { T_END, in, 1, QV::UNDEFINED };
parseError("Unexpected character (%#0$2X)", c);
RET0(T_END)
#undef RET
#undef RET0
#undef RET2
#undef RET22
#undef RET3
#undef RET4
}

const QToken& QParser::nextToken () {
prev=cur;
if (!stackedTokens.empty()) {
cur = stackedTokens.back();
stackedTokens.pop_back();
return cur;
}
#define RET(X) { cur = { X, start, static_cast<size_t>(in-start), QV::UNDEFINED}; return cur; }
#define RETV(X,V) { cur = { X, start, static_cast<size_t>(in-start), V}; return cur; }
#define RET2(C,A,B) if (utf8peek(in, end)==C) { utf8::next(in, end); RET(A) } else RET(B)
#define RET3(C1,A,C2,B,C) if(utf8peek(in, end)==C1) { utf8::next(in, end); RET(A) } else if (utf8peek(in, end)==C2) { utf8::next(in, end); RET(B) } else RET(C)
#define RET4(C1,R1,C2,R2,C3,R3,C) if(utf8peek(in, end)==C1) { utf8::next(in, end); RET(R1) } else if (utf8peek(in, end)==C2) { utf8::next(in, end); RET(R2) } else if (utf8peek(in, end)==C3) { utf8::next(in, end); RET(R3) }  else RET(C)
#define RET22(C1,C2,R11,R12,R21,R22) if (utf8peek(in, end)==C1) { utf8::next(in, end); RET2(C2,R11,R12) } else RET2(C2,R21,R22)
const char *start = in;
if (in>=end || !*in) RET(T_END)
uint32_t c;
do {
c = utf8::next(in, end);
} while(isSpaceOrIgnorableLine(c, in, end) && *(start=in) && in<end);
switch(c){
case '\0': RET(T_END)
case '\n': RET(T_LINE)
case '(': RET(T_LEFT_PAREN)
case ')': RET(T_RIGHT_PAREN)
case '[': RET(T_LEFT_BRACKET)
case ']': RET(T_RIGHT_BRACKET)
case '{': RET(T_LEFT_BRACE)
case '}': RET(T_RIGHT_BRACE)
case ',': RET(T_COMMA)
case ';': RET(T_SEMICOLON)
case ':': RET2(':', T_COLONCOLON, T_COLON)
case '_': RET2('_', T_UNDUND, T_UND)
case '$': RET(T_DOLLAR)
case '+': RET3('+', T_PLUSPLUS, '=', T_PLUSEQ, T_PLUS)
case '-': RET4('-', T_MINUSMINUS, '=', T_MINUSEQ, '>', T_MINUSGT, T_MINUS)
case '*': RET22('*', '=', T_STARSTAREQ, T_STARSTAR, T_STAREQ, T_STAR)
case '\\': RET2('=', T_BACKSLASHEQ, T_BACKSLASH)
case '%': RET2('=', T_PERCENTEQ, T_PERCENT)
case '|': RET22('|', '=', T_BARBAREQ, T_BARBAR, T_BAREQ, T_BAR)
case '&': RET22('&', '=', T_AMPAMPEQ, T_AMPAMP, T_AMPEQ, T_AMP)
case '^': RET2('=', T_CIRCEQ, T_CIRC)
case '@': RET2('=', T_ATEQ, T_AT)
case '~': RET(T_TILDE)
case '!': RET2('=', T_EXCLEQ, T_EXCL)
case '=': RET3('=', T_EQEQ, '>', T_EQGT, T_EQ) 
case '>': RET22('>', '=', T_GTGTEQ, T_GTGT, T_GTE, T_GT) 
//case '<': RET22('<', '=', T_LTLTEQ, T_LTLT, T_LTE, T_LT) 
case '<':
if (utf8peek(in, end)=='<') { utf8::next(in, end); RET2('=', T_LTLTEQ, T_LTLT) }
else { RET22('=', '>', T_LTEQGT, T_LTE, T_EXCLEQ, T_LT) }
case '?': 
if (utf8peek(in, end)=='.') { utf8::next(in, end); RET(T_DOTQUEST) } 
else { RET22('?', '=', T_QUESTQUESTEQ, T_QUESTQUEST, T_QUESTQUESTEQ, T_QUEST) }
case '/': 
switch (utf8peek(in, end)) {
case '/': case '*': skipComment(in, end, '/'); return nextToken();
default: RET2('=', T_SLASHEQ, T_SLASH)
}
case '.':
if (utf8peek(in, end)=='?') { utf8::next(in, end); RET(T_DOTQUEST) } 
else if (utf8peek(in, end)=='.') { utf8::next(in, end); RET2('.', T_DOTDOTDOT, T_DOTDOT) }
else RET(T_DOT)
case '"': case '\'': case '`':
case 146: case 147: case 171: case 8216: case 8217: case 8220: 
{
QV str = parseString(*this, vm, in, end, getStringEndingChar(c));
RETV(T_STRING, str)
}
case '#': skipComment(in, end, '#'); return nextToken();
case 183: case 215: case 8901: RET(T_STAR)
case 247: RET(T_SLASH)
case 8722: RET(T_MINUS)
case 8734: RETV(T_NUM, 1.0/0.0)
case 8800: RET(T_EXCLEQ)
case 8801: RET(T_EQEQ)
case 8804: RET(T_LTE)
case 8805: RET(T_GTE)
case 8712: RET(T_IN)
case 8743: RET(T_AMPAMP)
case 8744: RET(T_BARBAR)
case 8745: RET(T_AMP)
case 8746: RET(T_BAR)
case 8891: RET(T_CIRC)
}
if (isDigit(c)) {
double d = parseNumber(--in);
RETV(T_NUM, d)
}
else if (isName(c)) {
while(in<end && (c=utf8::peek_next(in, end)) && (isName(c) || isDigit(c))) utf8::next(in, end);
QTokenType type = T_NAME;
auto it = KEYWORDS.find(string(start, in-start));
if (it!=KEYWORDS.end()) type = it->second;
switch(type){
case T_TRUE: RETV(type, true)
case T_FALSE: RETV(type, false)
case T_NULL: RETV(type, QV::Null)
case T_UNDEFINED: RETV(type, QV::UNDEFINED)
default: RET(type)
}}
else if (isSpace(c)) RET(T_END)
cur = { T_END, in, 1, QV::UNDEFINED };
parseError("Unexpected character (%#0$2X)", c);
RET(T_END)
#undef RET
#undef RETV
#undef RET2
#undef RET22
#undef RET3
#undef RET4
}

pair<int,int> QParser::getPositionOf (const char* pos) {
if (pos<start || pos>=end) return { -1, -1 };
int line=1, column=1;
for (const char* c=start; c<pos && *c; c++) {
if (isLine(*c)) { line++; column=1; }
else column++;
}
return { line, column };
}

bool QParser::match (QTokenType type) {
if (nextToken().type==type) return true;
prevToken();
return false;
}

bool QParser::consume (QTokenType type, const char* msg) {
if (nextToken().type==type) return true;
parseError(msg);
return false;
}

void QParser::skipNewlines () {
while(matchOneOf(T_LINE, T_SEMICOLON));
}

void QParser::printMessage (const QToken& tok, Swan::CompilationMessage::Kind msgtype, const string& msg) {
auto p = getPositionOf(tok.start);
int line = p.first, column = p.second;
Swan::CompilationMessage z = { msgtype, msg, std::string(tok.start, tok.length), displayName, line, column };
vm.messageReceiver(z);
}
