/* (C) 2003 XDA Developers  itsme@xs4all.nl
 *
 * $Header$
 *
 * utility functions for manipulating ByteVectors
 *
 * the 'BV_' functions are used to add/get values to/from bytevectors
 * in a alignment independent way
 *
 * the bufpack/unpack functions are used to easily construct packets, to be sent to
 * the machine, or over the user data channel.
 *
 */
#include "vectorutils.h"
#include "debug.h"
void DV_AppendPtr(DwordVector& v, void *ptr)
{
    if (sizeof(ptr)<=4) {
        v.push_back(uint32_t(((uint64_t)ptr)&0xFFFFFFFF));
    }
    else if (sizeof(ptr)<=8) {
        v.push_back(uint32_t(((uint64_t)ptr)&0xFFFFFFFF));
        v.push_back(uint32_t(((uint64_t)ptr)>>32));
    }
    else {
        debug("\n\n\nUNKNOWN POINTERSIZE: %d\n\n\n\n", sizeof(ptr));
    }
}
void *DV_GetPtr(DwordVector::const_iterator& i)
{
    void *ptr;
#if __INTPTR_WIDTH__==32
        uint32_t i32;
        i32= *i++;
        ptr= (void*)i32;
#elif __INTPTR_WIDTH__==64
        uint64_t i64;
        i64= *i++;
        i64 |= ((uint64_t)(*i++))<<32;

        ptr= (void*)i64;
#else
#pragma error("__POIINTER_WIDTH__ must be 32 or 64")
#endif
    return ptr;
}

void BV_AppendByte(ByteVector& v, uint8_t value)
{
    v.push_back(value);
}

void BV_AppendBytes(ByteVector& v, const uint8_t *value, int len)
{
	while( len-- )
	    v.push_back(*value++);
}

void BV_AppendWord(ByteVector& v, uint16_t value)
{
    BV_AppendByte(v, uint8_t(value));
    BV_AppendByte(v, uint8_t(value>>8));
}

void BV_AppendDword(ByteVector& v, uint32_t value)
{
    BV_AppendWord(v, uint16_t(value));
    BV_AppendWord(v, uint16_t(value>>16));
}
void BV_AppendQword(ByteVector& v, uint64_t value)
{
    BV_AppendDword(v, uint32_t(value));
    BV_AppendDword(v, uint32_t(value>>32));
}

void BV_AppendNetWord(ByteVector& v, uint16_t value)
{
    BV_AppendByte(v, uint8_t(value>>8));
    BV_AppendByte(v, uint8_t(value));
}

void BV_AppendNetDword(ByteVector& v, uint32_t value)
{
    BV_AppendNetWord(v, uint16_t(value>>16));
    BV_AppendNetWord(v, uint16_t(value));
}

void BV_AppendVector(ByteVector& v1, const ByteVector& v2)
{
    v1.insert(v1.end(), v2.begin(), v2.end());
}
void BV_AppendString(ByteVector& v, const std::string& s)
{
    for (std::string::const_iterator i= s.begin() ; i!=s.end() ; ++i)
        BV_AppendByte(v, uint8_t(*i));
}
void BV_AppendTString(ByteVector& v, const std::tstring& s)
{
#ifdef _UNICODE
    for (std::tstring::const_iterator i= s.begin() ; i!=s.end() ; ++i)
        BV_AppendWord(v, uint16_t(*i));
#else
    for (std::tstring::const_iterator i= s.begin() ; i!=s.end() ; ++i)
        BV_AppendByte(v, uint8_t(*i));
#endif
}
void BV_AppendWString(ByteVector& v, const std::Wstring& s)
{
    for (std::Wstring::const_iterator i= s.begin() ; i!=s.end() ; ++i)
        BV_AppendWord(v, uint16_t(*i));
}

void BV_AppendRange(ByteVector& v, const ByteVector::const_iterator& begin, const ByteVector::const_iterator& end)
{
    v.insert(v.end(), begin, end);
}
inline int hexchar2nyble(char c)
{
    return c<'0' ? -1
        : c<='9' ? c-'0'
        : c<'A' ? -1
        : c<='F' ? c-'A'+10
        : c<'a' ? -1
        : c<='f' ? c-'a'+10
        : -1;
}
void BV_AppendHex(ByteVector& v, const char*hexstr)
{
    char c;
    uint8_t highnyb=0;
    bool bHigh= true;
    while((c=*hexstr++)!=0)
    {
        if (bHigh)
            highnyb = hexchar2nyble(c)<<4;
        else
            v.push_back(highnyb | hexchar2nyble(c));
        bHigh = !bHigh;
    }
}
ByteVector BV_FromBuffer(uint8_t* buf, int len)
{
    return ByteVector(buf, buf+len);
}
ByteVector BV_FromDword(uint32_t value)
{
    return ByteVector((uint8_t*)&value, 4+(uint8_t*)&value);
}
ByteVector BV_FromString(const std::string& str)
{
    if (str.empty())
        return ByteVector();
    return ByteVector((uint8_t*)iteratorptr(str.begin()), (uint8_t*)(iteratorptr(str.end()-1)+1));
}
ByteVector BV_FromWString(const std::Wstring& wstr)
{
    if (wstr.empty())
        return ByteVector();
    return ByteVector((uint8_t*)iteratorptr(wstr.begin()), (uint8_t*)(iteratorptr(wstr.end()-1)+1));
}


uint8_t BV_GetByte(const ByteVector& bv)
{
    ByteVector::const_iterator i= bv.begin();
    return BV_GetByte(i);
}
uint8_t BV_GetByte(ByteVector::const_iterator &i)
{
    return *i++;
}

uint16_t BV_GetNetWord(const ByteVector& bv)
{
    ByteVector::const_iterator i= bv.begin();
    return BV_GetNetWord(i);
}
uint16_t BV_GetNetWord(ByteVector::const_iterator &i)
{
    uint16_t w= BV_GetByte(i)<<8;
    w= w | BV_GetByte(i);

    return w;
}

uint16_t BV_GetWord(const ByteVector& bv)
{
    ByteVector::const_iterator i= bv.begin();
    return BV_GetWord(i);
}
uint16_t BV_GetWord(ByteVector::const_iterator &i)
{
    uint16_t w= BV_GetByte(i);
    w= w | ( BV_GetByte(i)<<8 );

    return w;
}

uint32_t BV_GetNetDword(const ByteVector& bv)
{
    ByteVector::const_iterator i= bv.begin();
    return BV_GetNetDword(i);
}
uint32_t BV_GetNetDword(ByteVector::const_iterator &i)
{
    uint32_t w= BV_GetNetWord(i)<<16;
    w= w | BV_GetNetWord(i);

    return w;
}

uint32_t BV_GetDword(const ByteVector& bv)
{
    ByteVector::const_iterator i= bv.begin();
    return BV_GetDword(i);
}
uint32_t BV_GetDword(ByteVector::const_iterator &i)
{
    uint32_t w= BV_GetWord(i);
    w= w | ( BV_GetWord(i)<<16 );

    return w;
}
uint64_t BV_GetQword(ByteVector::const_iterator &i)
{
    uint64_t w= BV_GetDword(i);
    w= w | ( ( (uint64_t )BV_GetDword(i))<<32 );

    return w;
}
// these create temp objects
std::string BV_GetString(const ByteVector& bv, int len)
{
    ByteVector::const_iterator i= bv.begin();
    return BV_GetString(i, len);
}
std::string BV_GetString(ByteVector::const_iterator &i, int len)
{
    std::string s;
    while (len--)
        s += (char) *i++;

    return s;
}
std::tstring BV_GetTString(ByteVector::const_iterator &i, int len)
{
    std::tstring s;
#ifdef UNICODE
    while (len--) {
        s += BV_GetWord(i);
    }
#else
    while (len--)
        s += (char) *i++;
#endif
    return s;
}

std::Wstring BV_GetWString(const ByteVector& bv, int len)
{
    ByteVector::const_iterator i= bv.begin();
    return BV_GetWString(i, len);
}
std::Wstring BV_GetWString(ByteVector::const_iterator &i, int len)
{
    std::Wstring s;
    while (len--) {
        s += BV_GetWord(i);
    }

    return s;
}


ByteVector BV_GetByteVector(ByteVector::const_iterator &i, ByteVector::const_iterator end)
{
    return ByteVector(i, end);
}



// these create a new object which the caller has to free
std::string* BV_MakeString(const ByteVector& bv, int len)
{
    ByteVector::const_iterator i= bv.begin();
    return BV_MakeString(i, len);
}
std::string* BV_MakeString(ByteVector::const_iterator &i, int len)
{
    std::string *s= new std::string();
    while (len--)
        *s += (char) *i++;

    return s;
}
std::tstring* BV_MakeTString(ByteVector::const_iterator &i, int len)
{
    std::tstring *s= new std::tstring();
#ifdef UNICODE
    while (len--) {
        *s += BV_GetWord(i);
    }
#else
    while (len--)
        *s += (char) *i++;
#endif
    return s;
}

std::Wstring* BV_MakeWString(const ByteVector& bv, int len)
{
    ByteVector::const_iterator i= bv.begin();
    return BV_MakeWString(i, len);
}
std::Wstring* BV_MakeWString(ByteVector::const_iterator &i, int len)
{
    std::Wstring *s= new std::Wstring();
    while (len--) {
        *s += BV_GetWord(i);
    }

    return s;
}


ByteVector* BV_MakeByteVector(ByteVector::const_iterator &i, ByteVector::const_iterator end)
{
    return new ByteVector(i, end);
}

// vaguely based on perl's pack/unpack
//
// 'n'  packs a 16 bit word in network order
// 'N'  packs a 32 bit word in network order
// 'v'  packs a 16 bit word
// 'V'  packs a 32 bit word
// 'C'  packs a 8 bit character
// 'B'  packs a ByteVector*
// 'S'  packs a string* as a (byte)length prefixed
// 'T'  packs a TCHAR string
// 'W'  packs a WCHAR string
// 'Q'  packs a quadword ( 8 bytes )
// 'h'  packs a hex std::string as bytes
// 'H'  packs a hex char* string as bytes
//
// note: originally strings were len prefixed AND nul terminated, now they are only len-prefixed
void vbufpack(ByteVector& buf, const char *fmt, va_list ap)
{
    while (*fmt) {
        switch(*fmt++)
        {
        case 'n': BV_AppendNetWord(buf, uint16_t(va_arg(ap,int))); break;
        case 'N': BV_AppendNetDword(buf, uint32_t(va_arg(ap,int))); break;
        case 'v': BV_AppendWord(buf, uint16_t(va_arg(ap,int))); break;
        case 'V': BV_AppendDword(buf, uint32_t(va_arg(ap,int))); break;
        case 'Q': BV_AppendQword(buf, va_arg(ap,uint64_t)); break;
        case 'C': BV_AppendByte(buf, uint8_t(va_arg(ap,int))); break;
        case 'B': BV_AppendVector(buf, *va_arg(ap,ByteVector*)); break;

        case 'H': BV_AppendHex(buf, va_arg(ap,const char*)); break;
        case 'h': 
                  {
                  std::string* strptr= va_arg(ap,std::string*);
                  BV_AppendHex(buf, strptr->c_str());
                  }
                  break;
        case 'S': 
              {
                  std::string* strptr= va_arg(ap,std::string*);
                  BV_AppendByte(buf, uint8_t(strptr->size()));
                  BV_AppendString(buf, *strptr);
//                  BV_AppendByte(buf, 0);    // add terminating NUL
              }
              break;
        case 'T': 
              {
                  std::tstring* strptr= va_arg(ap,std::tstring*);
                  BV_AppendByte(buf, uint8_t(strptr->size()));
                  BV_AppendTString(buf, *strptr);
//#ifdef UNICODE
//                  BV_AppendWord(buf, 0);    // add terminating NUL
//#else
//                  BV_AppendByte(buf, 0);    // add terminating NUL
//#endif
              }
              break;
        case 'W': 
              {
                  std::Wstring* strptr= va_arg(ap,std::Wstring*);
                  BV_AppendByte(buf, uint8_t(strptr->size()));
                  BV_AppendWString(buf, *strptr);
//                  BV_AppendWord(buf, 0);    // add terminating NUL
              }
              break;

        default:
                  throw "ERROR: unknown pack format character";
        }
    }
}
void bufpack(ByteVector& buf, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vbufpack(buf, fmt, ap);
    va_end(ap);
}
ByteVector bufpack(const char*fmt, ...)
{
    ByteVector bv;
    va_list ap;
    va_start(ap, fmt);
    vbufpack(bv, fmt, ap);
    va_end(ap);
    return bv;
}

// 'B' can only be last in the format string
DwordVector bufunpack(const ByteVector& buf, const char *fmt)
{
    DwordVector items;
    ByteVector::const_iterator i= buf.begin();
    while (*fmt) {
        switch(*fmt++)
        {
            // todo: implement 'h' and 'H'
        case 'n': items.push_back(BV_GetNetWord(i)); break;
        case 'N': items.push_back(BV_GetNetDword(i)); break;
        case 'v': items.push_back(BV_GetWord(i)); break;
        case 'V': items.push_back(BV_GetDword(i)); break;
        case 'Q':
                  {
                  uint64_t v64= BV_GetQword(i);
                  items.push_back(v64&0xFFFFFFFF);
                  items.push_back(v64>>32);
                  }
                  break;
        case 'C': items.push_back(BV_GetByte(i)); break;
        case 'S': DV_AppendPtr(items, BV_MakeString(i, BV_GetByte(i))); break;
        case 'T': DV_AppendPtr(items, BV_MakeTString(i, BV_GetByte(i))); break;
        case 'W': DV_AppendPtr(items, BV_MakeWString(i, BV_GetByte(i))); break;
        case 'B': 
            {
                DV_AppendPtr(items, BV_MakeByteVector(i, buf.end()));
                if (fmt[1])
                    throw "ERROR: bufunpack: B must be last in format";
            }
            break;
        default:
                  throw "ERROR: unknown pack format character";
        }
    }
    return items;
}

void bufunpack2(const ByteVector& bv, const char*fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ByteVector::const_iterator i= bv.begin();
    while (*fmt) {
        switch(*fmt++)
        {
            // todo: implement 'h' and 'H'
        case 'n': *va_arg(ap,uint16_t*)=BV_GetNetWord(i); break;
        case 'N': *va_arg(ap,uint32_t*)=BV_GetNetDword(i); break;
        case 'v': *va_arg(ap,uint16_t*)=BV_GetWord(i); break;
        case 'V': *va_arg(ap,uint32_t*)=BV_GetDword(i); break;
        case 'Q': *va_arg(ap,uint64_t*)=BV_GetQword(i); break;
        case 'C': *va_arg(ap,uint8_t*)=BV_GetByte(i); break;
        case 'S': *va_arg(ap,std::string*)=BV_GetString(i, BV_GetByte(i)); break;
        case 'T': *va_arg(ap,std::tstring*)=BV_GetTString(i, BV_GetByte(i)); break;
        case 'W': *va_arg(ap,std::Wstring*)=BV_GetWString(i, BV_GetByte(i)); break;
        case 'B': 
            {
                *va_arg(ap,ByteVector*)=BV_GetByteVector(i, bv.end());
                if (fmt[1])
                    throw "ERROR: bufunpack2: B must be last in format";
            }
            break;
        default:
                  throw "ERROR: unknown pack format character";
        }
    }
    va_end(ap);
}

#ifndef _WIN32
// this causes:
// fatal error C1001: An internal error has occurred in the compiler.
// (compiler file 'msc1.cpp', line 1393)
template<> std::vector<std::string> MakeVector(int n, ...)
{
    std::vector<std::string> v;

    va_list ap;
    va_start(ap, n);
    while (n--)
        v.push_back(std::string(va_arg(ap, const char*)));
    va_end(ap);

    return v;
}
#endif

