#ifndef _REG_VALUE_H_
#define _REG_VALUE_H_

#include "stringutils.h"
#include "debug.h"
#include "FileFunctions.h"
#include "util/endianutil.h"


#ifndef _WIN32_WCE
#if !defined(USE_BOOST_REGEX) && !defined(USE_STD_REGEX)
#error "defined either USE_BOOST_REGEX or USE_STD_REGEX"
#endif
#ifdef USE_BOOST_REGEX
#include <boost/regex.hpp>
#define BASIC_REGEX boost::basic_regex
#define CHARREGEX   boost::regex
#define REGEX_ITER  boost::regex_iterator
#define REGEX_MATCH boost::regex_match
#define REGEX_SEARCH boost::regex_search
#define REGEX_CONSTANTS boost::regex_constants
#define SMATCH      boost::smatch
#define PARTIALARG  , boost::match_partial
#endif

#ifdef USE_STD_REGEX
#include <regex>
#define BASIC_REGEX std::basic_regex
#define CHARREGEX   std::regex
#define REGEX_ITER  std::regex_iterator
#define REGEX_MATCH std::regex_match
#define REGEX_SEARCH std::regex_search
#define REGEX_CONSTANTS std::regex_constants
#define SMATCH      std::smatch
#define PARTIALARG
#endif


#endif

#include <map>

#ifndef REG_NONE

#define REG_NONE                    ( 0 )   // No value type
#define REG_SZ                      ( 1 )   // Unicode nul terminated string
#define REG_EXPAND_SZ               ( 2 )   // Unicode nul terminated string
#define REG_BINARY                  ( 3 )   // Free form binary
#define REG_DWORD                   ( 4 )   // 32-bit number
#define REG_DWORD_LITTLE_ENDIAN     ( 4 )   // 32-bit number (same as REG_DWORD)
#define REG_DWORD_BIG_ENDIAN        ( 5 )   // 32-bit number
#define REG_LINK                    ( 6 )   // Symbolic Link (unicode)
#define REG_MULTI_SZ                ( 7 )   // Multiple Unicode strings
#define REG_RESOURCE_LIST           ( 8 )   // Resource list in the resource map
#define REG_FULL_RESOURCE_DESCRIPTOR ( 9 )  // Resource list in the hardware description
#define REG_RESOURCE_REQUIREMENTS_LIST ( 10 )
#define REG_QWORD                   ( 11 )  // 64-bit number
#define REG_QWORD_LITTLE_ENDIAN     ( 11 )  // 64-bit number (same as REG_QWORD)
#define REG_MUI_SZ                  ( 21 )  // MUI sz string (pointer to resource dll & resource #) 
#endif

// this registry value type is specific to windows ce
#ifndef REG_MUI_SZ 
#define REG_MUI_SZ   (21)
#endif
typedef uint32_t ValueType_t;

class RegistryValue {
    static StringList& W() {
        static StringList g_warnings;
        return g_warnings;
    }
    static void warn(const std::string& msg)
    {
        W().push_back(msg);
    }
public:
    typedef std::map<std::string,RegistryValue> StringMap;

    static void DumpWarnings()
    {
        for (size_t i= 0 ; i<W().size() ; i++)
            debug("WARNING: %hs\n", W()[i].c_str());

        ClearWarnings();
    }
    static void ClearWarnings()
    {
        W().clear();
    }



#ifndef _WIN32_WCE
private:
    const static CHARREGEX reDwordPattern;
    const static CHARREGEX reStringPattern_s;  // single quoted string
    const static CHARREGEX reStringPattern_d;  // double quoted string
    const static CHARREGEX reStringPattern_n;  // tagged string unquoted
    const static CHARREGEX reMultiPartStringPattern_s;
    const static CHARREGEX reMultiPartStringPattern_d;
    const static CHARREGEX reMultiStringPattern;
    const static CHARREGEX reBinaryPattern;
    const static CHARREGEX reFilePattern;

    enum DwordType  {
        DWTYPE_HEX=16,
        DWTYPE_BITS=2,
        DWTYPE_DEC=10,
        DWTYPE_OCT=8,
    };

public:
    // this function takes a hexadecimal string value,  like '81f01234'
    // and converts it to a REG_DWORD : { 0x34, 0x12, 0xf0, 0x81 } value
    static RegistryValue FromDwordValue(DwordType dwType, const std::string& valstr) {
        return RegistryValue(strtoul(stringptr(valstr), 0, dwType));
    }
    // this function takes an escaped string value like ab\'cd\"ef\"\n
    // and converts it to a REG_SZ, or REG_EXPAND_SZ : { "ab'cd\"ef\"\n" }
    static RegistryValue FromStringValue(ValueType_t dwValType, const std::string& escstr) {
        return RegistryValue(dwValType, cstrunescape(escstr));
    }
    // this function takes an escaped string value like ab\'cd\"ef\"\n
    // and converts it to a REG_SZ : { "ab'cd\"ef\"\n" }
    static RegistryValue FromMultiStringValue(const std::string& multistr) {
        StringList list;

        bool bDoubleQuotes= multistr[0]=='\"';

        std::string::const_iterator i= multistr.begin();
        std::string::const_iterator iend= multistr.end();
        SMATCH what;
        while (REGEX_SEARCH(i, iend, what, bDoubleQuotes?reMultiPartStringPattern_d:reMultiPartStringPattern_s)) {
            std::string escstr(what[1].first, what[1].second);
            list.push_back(cstrunescape(escstr));

            i= what[0].second;
        }

        return RegistryValue(list);
    }
    static RegistryValue FromFile(ValueType_t dwValueType, const std::string& filename) {
        ByteVector bv;
        if (!LoadFileData(filename, bv))
            throw stringformat("ERROR reading data from file '%hs'", filename.c_str());

        return RegistryValue(dwValueType, bv);
    }
    // this function takes a string of format 00,11,22,33,44
    // and converts it to { 0x00, 0x11, 0x22, 0x33, 0x44 }
    static RegistryValue FromBinaryValue(ValueType_t dwValueType, const std::string& hexdata) {
        ByteVector bv;
        size_t i=0;
        while (i<hexdata.size()) {
            bv.push_back((uint8_t)strtoul(&hexdata[i], 0, 16));

            i= hexdata.find(",", i);
            if (i==hexdata.npos)
                break;

            i++;
        }
        return RegistryValue(dwValueType, bv);
    }
    static ValueType_t typestr_to_valuetype(const std::string& typestr)
    {
        if (typestr=="none")                       return REG_NONE;
        if (typestr=="sz")                         return REG_SZ;
        if (typestr=="string")                     return REG_SZ;
        if (typestr=="expand_sz")                  return REG_EXPAND_SZ;
        if (typestr=="expandsz")                   return REG_EXPAND_SZ;
        if (typestr=="expand")                     return REG_EXPAND_SZ;
        if (typestr=="binary")                     return REG_BINARY;
        if (typestr=="hex")                        return REG_BINARY;
        if (typestr=="dword")                      return REG_DWORD;
        if (typestr=="dwordle")                    return REG_DWORD_LITTLE_ENDIAN;
        if (typestr=="qword")                      return REG_QWORD;
        if (typestr=="qwordle")                    return REG_QWORD_LITTLE_ENDIAN;
        if (typestr=="qword")                      return REG_QWORD;
        if (typestr=="dwordbe")                    return REG_DWORD_BIG_ENDIAN;
        if (typestr=="multisz")                    return REG_MULTI_SZ;
        if (typestr=="multi_sz")                   return REG_MULTI_SZ;
        if (typestr=="mui_sz")                     return REG_MUI_SZ;
        if (typestr=="link")                       return REG_LINK;
        if (typestr=="resourcelist")               return REG_RESOURCE_LIST;
        if (typestr=="resource_list")              return REG_RESOURCE_LIST;
        if (typestr=="rl")                         return REG_RESOURCE_LIST;
        if (typestr=="full_resource_descriptor")   return REG_FULL_RESOURCE_DESCRIPTOR;
        if (typestr=="frd")                        return REG_FULL_RESOURCE_DESCRIPTOR;
        if (typestr=="resource_requirements_list") return REG_RESOURCE_REQUIREMENTS_LIST;
        if (typestr=="rrl")                        return REG_RESOURCE_REQUIREMENTS_LIST;

        if (REGEX_MATCH(typestr, CHARREGEX("^\\s*[0-9]+\\s*$")))
            return strtoul(typestr.c_str(), 0, 0);

        throw stringformat("unknown value type: %hs", typestr.c_str());

    }
    static DwordType xlat_dword_type_string(const std::string& dword_type)
    {
        if (dword_type=="dword")   return DWTYPE_HEX;
        if (dword_type=="bitmask") return DWTYPE_BITS;
        if (dword_type=="bit")     return DWTYPE_BITS;
        if (dword_type=="bin")     return DWTYPE_BITS;
        if (dword_type=="dec")     return DWTYPE_DEC;
        if (dword_type=="oct")     return DWTYPE_OCT;

        throw stringformat("unknown dword format type: %hs", dword_type.c_str());
    }
    static ValueType_t xlat_string_type_string(const std::string& string_type)
    {
        if (string_type.empty())
            throw "invalid stringtype: empty";
        if (string_type[0]=='s')     return REG_SZ;
        if (string_type[0]=='e')     return REG_EXPAND_SZ;
        if (string_type[0]=='m')     return REG_MUI_SZ;

        throw stringformat("unknown string format type: %hs", string_type.c_str());
    }

    static RegistryValue FromValueSpec(const std::string& spec)
    {
        SMATCH what;
        if (REGEX_MATCH(spec, what, reDwordPattern)) {
            std::string dword_type(what[1].first, what[1].second);
            std::string valstr(what[2].first, what[2].second);
            DwordType dwType= xlat_dword_type_string(dword_type);
            return FromDwordValue(dwType, valstr);
        }
        else if (REGEX_MATCH(spec, what, reStringPattern_s)) {
            std::string str_type= (what[1].matched)
                ? std::string(what[1].first, what[1].second)
                : "sz";
            ValueType_t dwStrType= xlat_string_type_string(str_type);

            std::string escstr(what[2].first, what[2].second);
            //printf("squoted string: type=%d / '%s'  val='%s'\n", dwStrType, str_type.c_str(), escstr.c_str());
            return FromStringValue(dwStrType, escstr);
        }
        else if (REGEX_MATCH(spec, what, reStringPattern_d)) {
            std::string str_type= (what[1].matched)
                ? std::string(what[1].first, what[1].second)
                : "sz";
            ValueType_t dwStrType= xlat_string_type_string(str_type);

            std::string escstr(what[2].first, what[2].second);
            //printf("dquoted string: type=%d / '%s'  val=%s\n", dwStrType, str_type.c_str(), vhexdump(escstr).c_str());
            return FromStringValue(dwStrType, escstr);
        }
        else if (REGEX_MATCH(spec, what, reStringPattern_n)) {
            std::string str_type(what[1].first, what[1].second);
            std::string escstr(what[2].first, what[2].second);
            ValueType_t dwStrType= xlat_string_type_string(str_type);
            //printf("unquoted string: type=%d / '%s'  val='%s'\n", dwStrType, str_type.c_str(), escstr.c_str());
            return FromStringValue(dwStrType, escstr);
        }
        else if (REGEX_MATCH(spec, what, reMultiStringPattern)) {
            std::string multistr(what[1].first, what[1].second);
            return FromMultiStringValue(multistr);
        }
        else if (REGEX_MATCH(spec, what, reBinaryPattern)) {
            std::string typestr= (what[1].matched)
                ? std::string(what[1].first, what[1].second)
                : "3";
            std::string hexdata(what[2].first, what[2].second);
            ValueType_t dwValueType= typestr_to_valuetype(typestr);
            return FromBinaryValue(dwValueType, hexdata);
        }
        else if (REGEX_MATCH(spec, what, reFilePattern)) {
            std::string typestr= (what[1].matched)
                ? std::string(what[1].first, what[1].second)
                : "3";
            std::string filename(what[2].first, what[2].second);
            ValueType_t dwValueType= typestr_to_valuetype(typestr);
            return FromFile(dwValueType, filename);
        }
        throw stringformat("unimplemented valuespec: '%hs'\ndid you escape all backslashes?", spec.c_str());
    }
#endif

public:
    RegistryValue()
        : m_dwType(REG_NONE)
    {
        //debug("new regvalue none\n");
    }
    RegistryValue(ValueType_t dwType, const ByteVector& data)
        : m_dwType(dwType), m_data(data)
    {
        //debug("new regvalue type %d\n", m_dwType);
    }
    RegistryValue(const RegistryValue& rv)
        : m_dwType(rv.GetType()), m_data(rv.GetData())
    {
        //debug("copy of regvalue type %d\n", m_dwType);
    }

    ValueType_t GetType() const
    {
        return m_dwType;
    }

    // binary value
    RegistryValue(const ByteVector& data)
        : m_dwType(REG_BINARY), m_data(data)
    {
    }
    ByteVector GetData() const
    {
        return m_data;
    }

    // dword value
    RegistryValue(uint32_t dwValue)
        : m_dwType(REG_DWORD), m_data(BV_FromDword(dwValue))
    {
    }
    uint32_t GetDword() const
    {
        if (m_dwType!=REG_DWORD) throw stringformat("GetDword: not a dword: %d", m_dwType);
        if (m_data.size()!=4) throw stringformat("GetDword: incorrect size: %d", m_data.size());

        return BV_GetDword(m_data);
    }

    // string value
    RegistryValue(ValueType_t dwValType, const std::string& strValue)
        : m_dwType(dwValType), m_data(BV_FromWString(ToWString(strValue)))
    {
        BV_AppendWord(m_data, 0); // add terminating (WCHAR)NUL
    }
    std::string GetString() const
    {
        if (m_dwType!=REG_MUI_SZ && m_dwType!=REG_SZ && m_dwType!=REG_EXPAND_SZ) throw stringformat("GetString: not a string: %d", m_dwType);
        if (m_data.size()&1) {
            warn(stringformat("GetString: uneven size: %d", m_data.size()));
        }
        if (m_data.empty())
            return "";

        std::string str= ToString((WCHAR*)&m_data[0]);

        return str;
    }

    // string list value
    RegistryValue(const StringList& listValue)
        : m_dwType(REG_MULTI_SZ)
    {
        m_data.clear();
        for (StringList::const_iterator i= listValue.begin() ; i!=listValue.end() ; ++i)
        {
            BV_AppendWString(m_data, ToWString(*i));
            BV_AppendWord(m_data, 0); // add terminating (WCHAR)NUL
        }
        BV_AppendWord(m_data, 0); // add terminating (WCHAR)NUL
    }
    StringList GetStringList() const
    {
        if (m_dwType!=REG_MULTI_SZ) throw stringformat("GetStringList: not a multisz: %d", m_dwType);
		if (m_data.size()&1) {
            warn(stringformat("GetStringList: uneven size: %d", m_data.size()));
        }

        bool bFoundUnicode= false;
        StringList list;
        std::string str;
        // todo: make this convert properly to utf8
        for (size_t i= 0 ; i<m_data.size() ; ++i)
        {
            if ((i&1)==1) {
                if (m_data[i]!=0) {
                    bFoundUnicode= true;
                    str += (char)m_data[i];
                }
            }
            else {
                if (m_data[i]==0)
                {
                    list.push_back(str);
                    str.erase();
                }
                else
                {
                    str += (char)m_data[i];
                }
            }
        }
        if (bFoundUnicode)
            warn(stringformat("GetStringList: with unicode chars"));
        if (!str.empty()) {
            warn(stringformat("GetStringList: no terminating NUL"));
        }
		if (list.empty()) {
			// ... nop ... empty multisz
		}
        else if (!list.back().empty()) {
            warn(stringformat("GetStringList: no closing element"));
        }
        else
            list.pop_back();
        return list;
    }
    std::string quotestring(const std::string& str) const
    {
        std::string qstr= "\"";
        for (size_t i=0 ; i<str.size() ; i++)
        {
            if (str[i]=='\r') qstr += "\\r";
            else if (str[i]=='\n') qstr += "\\n";
            else if (str[i]=='\t') qstr += "\\t";
            else if (str[i]=='\0') qstr += "\\0";
            else if (str[i]<' ' || str[i]>'~')
                qstr+=stringformat("\\x%02x", (unsigned char)str[i]);
            else
                qstr+=str[i];
        }
        return qstr+"\"";
    }
    std::string MultiszAsString() const
    {
        StringList list= GetStringList();

        for (size_t i=0 ; i<list.size() ; ++i)
            list[i]= quotestring(list[i]);
        return std::string("multi_sz:")+JoinStringList(list, ",");
    }
    std::string DwordAsString() const
    {
            if (m_dwType!=REG_DWORD) throw stringformat("DwordAsString: not a dword: %d", m_dwType);
            if (m_data.size()==0)
                    return stringformat("dword:");
            else if (m_data.size()!=sizeof(uint32_t)) {
        warn(stringformat("DwordAsString: invalid sized dword: %d", m_data.size()));
                    return stringformat("hex(%d):", m_dwType)+ascdump(m_data);
            }
            return stringformat("dword:%08lx", get32le(m_data.begin()));
    }
    std::string AsString(int hex) const
    {
        if (hex>1) {
            return stringformat("hex(%d):", m_dwType)+hexstring(&m_data.front(), m_data.size(), ',');
        }
        switch(m_dwType) {
case REG_NONE:		if (m_data.size()) { 
                        warn(stringformat("AsString: none with data: %d bytes", m_data.size()));
                        return stringformat("hex(%d):", m_dwType)+ascdump(m_data);
                    } 
                    else
                        return "none";
case REG_SZ:		return quotestring(GetString());
case REG_EXPAND_SZ: return std::string("expand:")+quotestring(GetString());
case REG_BINARY:	if (hex>0)
                                                return std::string("hex:")+hexstring(&m_data.front(), m_data.size(), ',');
					else 
						return std::string("hex:")+ascdump(m_data);
case REG_DWORD:		return DwordAsString();
//case REG_DWORD_BE: return stringformat("dwordbe:%08lx", get32le(m_data.begin()));
case REG_MULTI_SZ:	return MultiszAsString();
case REG_LINK:		return "link:"+ascdump(m_data);
case REG_RESOURCE_LIST: return "rsclist:"+ascdump(m_data);
case REG_FULL_RESOURCE_DESCRIPTOR: return "rscdesc:"+ascdump(m_data);
case REG_RESOURCE_REQUIREMENTS_LIST: return "rscreqslist:"+ascdump(m_data);
case REG_QWORD:		return "qword:"+ascdump(m_data);
default:			return stringformat("hex(%d):", m_dwType)+ascdump(m_data);
        }
    }

private:
    ValueType_t m_dwType;
    ByteVector m_data;
    //Value* m_value;
};


#endif
