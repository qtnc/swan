#ifndef NO_REGEX
#include "Regex.hpp"

std::pair<regex_constants::syntax_option_type, regex_constants::match_flag_type>  QRegex::parseOptions (const char* opt) {
regex_constants::syntax_option_type options = regex::ECMAScript | regex::optimize;
regex_constants::match_flag_type flags = regex_constants::match_default | regex_constants::format_default;
if (opt) while(*opt) switch(*opt++){
case 'c': options |= regex::collate; break;
case 'f': flags |= regex_constants::format_first_only; break;
case 'i': options |= regex::icase; break;
case 'y': flags |= regex_constants::match_continuous; break;
#ifdef USE_BOOST_REGEX
case 'E': options |= regex::no_empty_expressions; break;
case 'M': options |= regex::no_mod_m; break;
case 'S': 
options |= regex::no_mod_s; 
flags |= regex_constants::match_not_dot_newline;
break;
case 's': options |= regex::mod_s; break;
case 'x': options |= regex::mod_x; break;
case 'z': flags |= regex_constants::format_all; break;
#else
case 'E': flags |= regex_constants::match_not_null; break;
#endif
}
return std::make_pair(options, flags);
}

QRegex::QRegex (QVM& vm, const char* begin, const char* end, regex_constants::syntax_option_type regexOptions, regex_constants::match_flag_type matchOptions0):
QObject(vm.regexClass), regex(begin, end, regexOptions), matchOptions(matchOptions0)
{}

QRegexIterator::QRegexIterator (QVM& vm, QString& s, QRegex& r, regex_constants::match_flag_type options):
QSequence(vm.regexIteratorClass), str(s), regex(r),
it(s.begin(), s.end(), r.regex, options | r.matchOptions)
{}

QRegexTokenIterator::QRegexTokenIterator (QVM& vm, QString& s, QRegex& r, regex_constants::match_flag_type options, int g):
QSequence(vm.regexTokenIteratorClass), str(s), regex(r), 
it(s.begin(), s.end(), r.regex, g, options | r.matchOptions)
{}
#endif

