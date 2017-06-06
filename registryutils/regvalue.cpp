#include "regvalue.h"

#ifndef _WIN32_WCE

#define SQUOTEESC  "[^'\\\\]|\\\\'"
#define DQUOTEESC  "[^\"\\\\]|\\\\\""
#define SDQUOTEESC  "[^\"'\\\\]|\\\\'|\\\\\""
#define STRESCAPES "\\\\[\\\\0rtn]|\\\\x[0-9a-fA-F][0-9a-fA-F]"

const CHARREGEX RegistryValue::reDwordPattern= CHARREGEX("(dword|bit|bin|bitmask|dec|oct):\\s*(\\w+)", REGEX_CONSTANTS::icase);
const CHARREGEX RegistryValue::reStringPattern_s=         CHARREGEX("^\\s*(?:(string|sz|mui_sz|expand\\w+):)?'((?:" SQUOTEESC "|" STRESCAPES ")*)'", REGEX_CONSTANTS::icase);
const CHARREGEX RegistryValue::reStringPattern_d=        CHARREGEX("^\\s*(?:(string|sz|mui_sz|expand\\w+):)?\"((?:" DQUOTEESC "|" STRESCAPES ")*)\"", REGEX_CONSTANTS::icase);

// unquoted stringtypes:
const CHARREGEX RegistryValue::reStringPattern_n= CHARREGEX("^\\s*(string|sz|mui_sz|expand\\w*):((?:" SDQUOTEESC "|" STRESCAPES ")*?)", REGEX_CONSTANTS::icase);

const CHARREGEX RegistryValue::reMultiPartStringPattern_s=    CHARREGEX("'((?:" SQUOTEESC "|" STRESCAPES ")*)'", REGEX_CONSTANTS::icase);
const CHARREGEX RegistryValue::reMultiPartStringPattern_d= CHARREGEX("\"((?:" DQUOTEESC "|" STRESCAPES ")*)\"", REGEX_CONSTANTS::icase);
const CHARREGEX RegistryValue::reMultiStringPattern= CHARREGEX("^\\s*multi_sz:\\s*(.*?)", REGEX_CONSTANTS::icase);
const CHARREGEX RegistryValue::reBinaryPattern= CHARREGEX("^\\s*hex(?:\\((\\w+)\\))?:\\s*(.*?)", REGEX_CONSTANTS::icase);
const CHARREGEX RegistryValue::reFilePattern= CHARREGEX("^\\s*file(?:\\((\\w+)\\))?:\\s*(.*?)", REGEX_CONSTANTS::icase);
#endif
