#include "util/ReadWriter.h"
#include "util/rw/MmapReader.h"
#include "util/rw/MemoryReader.h"
#include "crypto/hash.h"
#include <stdio.h>
#include "vectorutils.h"
#include "util/chariterators.h"
#include <memory>
#include <functional>
#include <map>
#include <regpath.h>
#include <regvalue.h>
#include <regfileparser.h>

#include "args.h"

#ifndef _WIN32
typedef uint32_t HKEY;
#endif

int g_verbose;

template<typename PTR>
size_t vectorread32le(PTR rd, DwordVector& v, size_t n)
{
    v.resize(n);

    size_t nr= rd->read((uint8_t*)&v[0], n*sizeof(uint32_t));
    if (nr%sizeof(uint32_t))
        throw "read partial uint32_t";
    v.resize(nr/sizeof(uint32_t));
#if __BYTE_ORDER == __BIG_ENDIAN
#ifdef __GXX_EXPERIMENTAL_CXX0X__
    std::for_each(v.begin(), v.end(), [](uint32_t& x) { x= swab32(x);});
#else
    throw "need c++0x";
#endif
#endif
    return v.size();
}

template<typename PTR>
size_t vectorread8(PTR rd, ByteVector& v, size_t n)
{
	v.resize(n);
	size_t nr= rd->read((uint8_t*)&v[0], n);
	v.resize(nr);
	return v.size();
}
template<typename PTR>
size_t readutf16le(PTR rd, std::Wstring& v, size_t n)
{
    v.resize(n);
    size_t nr= rd->read((uint8_t*)&v[0], n*sizeof(uint16_t));
    if (nr%sizeof(uint16_t))
        throw "read partial uint16_t";
    v.resize(nr/sizeof(uint16_t));
    v.resize(stringlength(&v[0]));
#if __BYTE_ORDER == __BIG_ENDIAN
#ifdef __GXX_EXPERIMENTAL_CXX0X__
    std::for_each(v.begin(), v.end(), [](uint16_t& x) { x= swab16(x);});
#else
    throw "need c++0x";
#endif
#endif

    return v.size();
}
template<typename PTR>
    void vectorwrite32le(PTR wr, const DwordVector& v)
    {
        for (DwordVector::const_iterator i= v.begin() ; i!=v.end() ; ++i)
            wr->write32le(*i);
    }
template<typename PTR>
    void writeutf16le(PTR wr, const std::Wstring& v)
    {
#if __BYTE_ORDER == __BIG_ENDIAN
#ifdef __GXX_EXPERIMENTAL_CXX0X__
        std::for_each(v.begin(), v.end(), [this](uint16_t x) { wr->write16le(x); });
#else
        throw "need c++0x";
#endif
#else  // __LITTLE_ENDIAN
        wr->write((const uint8_t*)v.c_str(), v.size()*sizeof(uint16_t));
#endif
    }


namespace ent {

class base;
typedef std::shared_ptr<base> entry_ptr;
    class roots;
    class key;
    class value;
    class database;
    class record;
    class recordmore;
    class index;
    class volume;

    class stringvalue;
    class binaryvalue;
    class dwordvalue;
    class stringlistvalue;
    class muistringvalue;


typedef std::map<uint32_t,entry_ptr> entrymap_t;
class base : public MemoryReader {
    uint32_t _id;  // note: this id is not orred with 0x20000000
protected:
    ByteVector _data;
public:
    base(uint32_t id)
        : _id(id)
    {
    }
    base(uint32_t id, const ByteVector& data)
        : _id(id), _data(data)
    {
        setbuf(&_data.front(), _data.size());
    }
    virtual uint16_t entrytype()= 0;
    virtual const char*typestr()=0;
    static entry_ptr readentry(ReadWriter_ptr r, uint32_t ofs, uint8_t flag);

    uint32_t id() { return _id&0x0fffffff; }

    std::string readwstr(size_t n)
    {
        std::Wstring w;
        readutf16le(this, w, n);
        return ToString(w);
    }
    virtual void save(ReadWriter_ptr w)= 0;

    roots* asroots();
    key* askey();
    value* asvalue();

    void savehead(ReadWriter_ptr w, uint32_t savesize)
    {
        w->write32le( (entrytype()<<28) | savesize);
        w->write32le(0);
        w->write32le(_id);
    }
};
    enum { HKCR, HKCU, HKLM, HKU };
    enum {
        ET_DATABASE=7,
        ET_RECORD,  // 8
        ET_RECMORE, // 9
        ET_VOLUME,  // a
        ET_ROOTS,   // b
        ET_KEY,     // c
        ET_VALUE,   // d
        ET_INDEX    // e
    };

// type 0xb000  - have ptrs, contains pointers to start fo HKCR, HKCU, HKLM
class roots : public base {
    std::vector<uint32_t> _roots;
    std::vector<uint32_t> _lasts;
public:
    roots(uint32_t id)
        : base(id)
    {
        _roots.resize(8);
        _lasts.resize(8);
    }
    roots(uint32_t id, const ByteVector& data)
        : base(id, data)
    {
        vectorread32le(this, _roots, 8);
        _lasts.resize(8); // note: not updated
        auto i= std::find_if(_roots.begin()+3, _roots.end(), [](uint32_t x) { return x!=0; });
        if (i!=_roots.end())
            printf("WARNING: more roots: %s\n", hexdump(&_roots[3], 5).c_str());

    }
    virtual uint16_t entrytype() { return ET_ROOTS; }

    uint32_t hiveid(HKEY root)
    {
        return _roots[int(root)&255]&0x0fffffff;
    }
    void hiveid(HKEY root, uint32_t id)
    {
        _roots[int(root)&255]= id|0x20000000;
    }
    uint32_t lasthivekey(HKEY root)
    {
        return _lasts[int(root)&255]&0x0fffffff;
    }
    void lasthivekey(HKEY root, uint32_t id)
    {
        _lasts[int(root)&255]= id|0x20000000;
    }


    virtual const char*typestr() { return "roots"; }

    virtual void save(ReadWriter_ptr w)
    {
        savehead(w, _roots.size()*sizeof(uint32_t));
        vectorwrite32le(w, _roots);
    }
};

// type 0xc000  - ptr to next sibling, first child, first value, name
class key : public base {
    uint32_t _nextsibling;
    uint32_t _firstchild;
    uint32_t _firstvalue;
    uint32_t _lastvalue;   // not stored, just for easy tree building
    uint32_t _lastchild;   // not stored, just for easy tree building
    std::string _name;
public:
    key(uint32_t id, const std::string& name)
        : base(id), _nextsibling(0), _firstchild(0), _firstvalue(0), _lastvalue(0), _lastchild(0), _name(name)
    {
    }
    key(uint32_t id, const ByteVector& data)
        : base(id, data)
    {
        _nextsibling= read32le();
        _firstchild= read32le();
        _firstvalue= read32le();

        // note: when reading these are not updated!!
        _lastvalue= 0;
        _lastchild= 0;

        uint8_t namlen= read8();  // max 0x44
        /*uint8_t unusedlen=*/ read8();  // max 0x61
        uint16_t flags= read16le();
        if (flags && g_verbose)
            printf("WARNING: key flags=%04x\n", flags);

        _name= readwstr(namlen);
    }
    virtual uint16_t entrytype() { return ET_KEY; }
    virtual const char*typestr() { return "key"; }
    std::string name() { return _name; }
    uint32_t nextsibling() { return _nextsibling&0x0fffffff; }
    void nextsibling(uint32_t id) { _nextsibling= id ? (id|0x20000000) : 0; }
    uint32_t firstchild() { return _firstchild&0x0fffffff; }
    void firstchild(uint32_t id) { _firstchild= id ? (id|0x20000000) : 0; }
    uint32_t firstvalue() { return _firstvalue&0x0fffffff; }
    void firstvalue(uint32_t id) { _firstvalue= id ? (id|0x20000000) : 0; }

    uint32_t lastvalue() { return _lastvalue&0x0fffffff; }
    void lastvalue(uint32_t id) { _lastvalue= id ? (id|0x20000000) : 0; }
    uint32_t lastchild() { return _lastchild&0x0fffffff; }
    void lastchild(uint32_t id) { _lastchild= id ? (id|0x20000000) : 0; }


    virtual void save(ReadWriter_ptr w)
    {
        std::Wstring wstr= ToWString(_name);
        size_t padding= (wstr.size()&1) ? 2 : 0;
        savehead(w, 16 + wstr.size()*sizeof(WCHAR)+padding);
        w->write32le(_nextsibling);
        w->write32le(_firstchild);
        w->write32le(_firstvalue);
        w->write8(wstr.size());
        w->write8(0);
        w->write16le(0);

        writeutf16le(w, wstr);

        if (padding)
            w->write16le(0);
    }

};

//=============================================================================
// type 0xd000 - value
class value;
typedef std::shared_ptr<value> value_ptr;

enum { VT_STRING=1, VT_BINARY=3, VT_DWORD=4, VT_STRINGLIST=7, VT_MUI=21 };

class value : public base {
    uint32_t _nextvalue;
    std::string _name;
public:
    value(uint32_t id, const std::string& name)
        : base(id), _nextvalue(0), _name(name)
    {
    }
    value(uint32_t id, uint32_t next, const std::string& name, const ByteVector& data)
        : base(id, data), _nextvalue(next), _name(name)
    {
    }
    uint32_t nextvalue() { return _nextvalue&0x0fffffff; }
    void nextvalue(uint32_t id) { _nextvalue= id ? (id|0x20000000) : 0; }

    virtual uint16_t entrytype() { return ET_VALUE; }
    virtual uint16_t valuetype()= 0;
    static value_ptr readvalue(uint32_t id, const ByteVector& data);
    virtual const char*typestr() { return "value"; }
    virtual std::string asstring()= 0;
    std::string name() const { return _name; }

    virtual void encodeasbinary(ByteVector& bin)= 0;

    virtual void save(ReadWriter_ptr w)
    {
        std::Wstring wstr= ToWString(_name);
        ByteVector bin;
        encodeasbinary(bin);

        size_t datasize= 10 + wstr.size()*sizeof(WCHAR)+bin.size();
        size_t padding= (datasize&3) ? 4-(datasize&3) : 0;
        savehead(w, 10 + wstr.size()*sizeof(WCHAR)+bin.size()+padding);

        w->write32le(_nextvalue);
        w->write16le(valuetype());
        w->write16le(bin.size());
        w->write16le(wstr.size());

        writeutf16le(w, wstr);
        w->write(&bin.front(), bin.size());

        for (unsigned i=0 ; i<padding ; i++)
            w->write8(0);
    }
};
class stringvalue : public value {
    std::string _value;
public:
    stringvalue(uint32_t id, const std::string& name, const std::string& data)
        : value(id, name), _value(data)
    {
    }
    stringvalue(uint32_t id, uint32_t next, const std::string& name, const ByteVector& data)
        : value(id, next, name, data)
    {
        _value= readwstr(data.size()/2);
    }
    virtual uint16_t valuetype() { return VT_STRING; }
    std::string str()
    {
        return _value;
    }
    virtual std::string asstring()
    {
        return "\""+cstrescape(_value)+"\"";
    }
    virtual void encodeasbinary(ByteVector& bin)
    {
        std::for_each(utf8adaptor(_value.begin()), utf8adaptor(_value.end()), [&bin](uint32_t v) { BV_AppendWord(bin, v); });
        BV_AppendWord(bin, 0);
    }
};
class binaryvalue : public value {
    ByteVector _value;
public:
    binaryvalue(uint32_t id, const std::string& name, const ByteVector& data)
        : value(id, name), _value(data)
    {
    }
    binaryvalue(uint32_t id, uint32_t next, const std::string& name, const ByteVector& data)
        : value(id, next, name, data)
    {
        _value= data;
    }
    virtual uint16_t valuetype() { return VT_BINARY; }
    ByteVector bin()
    {
        return _value;
    }
    virtual std::string asstring()
    {
        return "hex:"+hexstring(&_value.front(), _value.size(),',');
    }
    virtual void encodeasbinary(ByteVector& bin)
    {
        bin= _value;
    }
};
class dwordvalue : public value {
    uint32_t _value;
public:
    dwordvalue(uint32_t id, const std::string& name, uint32_t data)
        : value(id, name), _value(data)
    {
    }
    dwordvalue(uint32_t id, uint32_t next, const std::string& name, const ByteVector& data)
        : value(id, next, name, data)
    {
        _value= read32le();
    }
    virtual uint16_t valuetype() { return VT_DWORD; }
    uint32_t dword()
    {
        return _value;
    }
    virtual std::string asstring()
    {
        return stringformat("dword:%08x", _value);
    }
    virtual void encodeasbinary(ByteVector& bin)
    {
        BV_AppendDword(bin, _value);
    }
};
class stringlistvalue : public value {
    StringList _value;
public:
    stringlistvalue(uint32_t id, const std::string& name, const StringList& data)
        : value(id, name), _value(data)
    {
    }
    stringlistvalue(uint32_t id, uint32_t next, const std::string& name, const ByteVector& data)
        : value(id, next, name, data)
    {
        std::Wstring  wstr;
        while (!eof())
        {
            uint16_t w= read16le();
            if (w)
                wstr.push_back(w);
            else {
                _value.push_back(ToString(wstr));
                wstr.clear();
            }
        }
        if (_value.back().empty())
            _value.resize(_value.size()-1);
    }
    virtual uint16_t valuetype() { return VT_STRINGLIST; }
    StringList list()
    {
        return _value;
    }
    virtual std::string asstring()
    {
        std::string str;
        for (StringList::const_iterator i=_value.begin() ; i!=_value.end() ; ++i)
        {
            if (!str.empty())
                str += ", ";
            str += "\"" + cstrescape(*i) + "\"";
        }
        return "multi_sz:"+str;
    }
    virtual void encodeasbinary(ByteVector& bin)
    {
        for (StringList::const_iterator i= _value.begin() ; i!=_value.end() ; ++i)
        {
            BV_AppendWString(bin, ToWString(*i));
            BV_AppendWord(bin, 0); // add terminating (WCHAR)NUL
        }
        BV_AppendWord(bin, 0); // add terminating (WCHAR)NUL
    }
};
class muistringvalue : public value {
    std::string _value;
public:
    muistringvalue(uint32_t id, const std::string& name, const std::string& data)
        : value(id, name), _value(data)
    {
    }
    muistringvalue(uint32_t id, uint32_t next, const std::string& name, const ByteVector& data)
        : value(id, next, name, data)
    {
        _value= readwstr(data.size()/2);
    }
    virtual uint16_t valuetype() { return VT_MUI; }
    std::string muistr()
    {
        return _value;
    }
    virtual std::string asstring()
    {
        return "mui_sz:\""+cstrescape(_value)+"\"";
    }
    virtual void encodeasbinary(ByteVector& bin)
    {
         BV_AppendWString(bin, ToWString(_value));
    }
};
value_ptr value::readvalue(uint32_t id, const ByteVector& data)
{
    MemoryReader r(&data.front(), data.size());
    uint32_t nextvalue= r.read32le();
    uint16_t type= r.read16le();   // 1, 3, 4, 7, 21
    uint16_t vallen= r.read16le();  // max 0xc5a
    uint16_t namlen= r.read16le();  // max 0x75

    std::Wstring w;
    readutf16le(&r, w, namlen);
    std::string name= ToString(w);

    ByteVector valdata;
    vectorread8(&r, valdata, vallen);

    switch(type)
    {
        case VT_STRING: return value_ptr(new stringvalue(id, nextvalue, name, valdata));
        case VT_BINARY: return value_ptr(new binaryvalue(id, nextvalue, name, valdata));
        case VT_DWORD:  return value_ptr(new dwordvalue(id, nextvalue, name, valdata));
        case VT_STRINGLIST: return value_ptr(new stringlistvalue(id, nextvalue, name, valdata));
        case VT_MUI:    return value_ptr(new muistringvalue(id, nextvalue, name, valdata));
        default:
                        printf("WARNING: unsupported value type %d ( next:%08x name:%s, val:%s )\n", type, nextvalue, name.c_str(), vhexdump(valdata).c_str());
    }
    return value_ptr();
}

//=============================================================================

class database : public base {
public:
    database(uint32_t id, const ByteVector& data)
        : base(id, data)
    {
        // todo
        printf("WARNING: database not implemented\n");
    }
    virtual uint16_t entrytype() { return ET_DATABASE; }
    virtual const char*typestr() { return "database"; }
    virtual void save(ReadWriter_ptr w) { throw "not implemented"; }
};
class record : public base {
public:
    record(uint32_t id, const ByteVector& data)
        : base(id, data)
    {
        printf("WARNING: record not implemented\n");
    }
    virtual uint16_t entrytype() { return ET_RECORD; }
    virtual const char*typestr() { return "record"; }
    virtual void save(ReadWriter_ptr w) { throw "not implemented"; }
};
class recordmore : public base {
public:
    recordmore(uint32_t id, const ByteVector& data)
        : base(id, data)
    {
        printf("WARNING: recordmore not implemented\n");
    }
    virtual uint16_t entrytype() { return ET_RECMORE; }
    virtual const char*typestr() { return "recmore"; }
    virtual void save(ReadWriter_ptr w) { throw "not implemented"; }
};
class index : public base {
public:
    index(uint32_t id, const ByteVector& data)
        : base(id, data)
    {
        printf("WARNING: index not implemented\n");
    }
    virtual uint16_t entrytype() { return ET_INDEX; }
    virtual const char*typestr() { return "index"; }
    virtual void save(ReadWriter_ptr w) { throw "not implemented"; }
};
class volume : public base {
public:
    volume(uint32_t id, const ByteVector& data)
        : base(id, data)
    {
        printf("WARNING: volume not implemented\n");
    }
    virtual uint16_t entrytype() { return ET_VOLUME; }
    virtual const char*typestr() { return "volume"; }
    virtual void save(ReadWriter_ptr w) { throw "not implemented"; }
};


roots* base::asroots() { return dynamic_cast<roots*>(this); }
key* base::askey() { return dynamic_cast<key*>(this); }
value* base::asvalue() { return dynamic_cast<value*>(this); }

entry_ptr base::readentry(ReadWriter_ptr r, uint32_t ofs, uint8_t flag)
{
    uint32_t size= r->read32le();
    uint8_t type= size>>28;
    size &= ~0xf0000000;
    uint32_t nul_0004= r->read32le();
    uint32_t id= r->read32le();
    if (nul_0004 && g_verbose)
        printf("WARNING: entry +4=%08x\n", nul_0004);
    ByteVector data;
    vectorread8(r, data, size);

    if (g_verbose>1)
        printf("%08x-%08x:[%02x] %06x %x [%08x] ", ofs, ofs+12+size, flag, size, type, id);
    switch(type) {
        case ET_DATABASE: return entry_ptr(new database(id, data));
        case ET_RECORD  : return entry_ptr(new record(id, data));
        case ET_RECMORE : return entry_ptr(new recordmore(id, data));
        case ET_VOLUME  : return entry_ptr(new volume(id, data));
        case ET_ROOTS   : return entry_ptr(new roots(id, data));
        case ET_KEY     : return entry_ptr(new key(id, data));
        case ET_VALUE   : return value::readvalue(id, data);
        case ET_INDEX   : return entry_ptr(new index(id, data));
        default:
                     printf("WARNING: unknown entry type %d, id=[%08x], data: %s\n", type, id, vhexdump(data).c_str());
    }
    return entry_ptr();
}

} // namespace

class HvFile {
    ReadWriter_ptr _r;
    DwordVector _offsets;
    ByteVector _bootmd5;

    struct unkitem {
        uint32_t type;
        uint32_t ptr;
        uint32_t unk;
        uint32_t flag;


        unkitem(uint32_t type, uint32_t ptr, uint32_t unk, uint32_t flag)
            : type(type), ptr(ptr), unk(unk), flag(flag)
        {
        }
    };
    template<typename V>
    static bool is_all_zero(const V& v)
    {
        typedef typename V::value_type VALTYPE;
        return v.end()==std::find_if(v.begin(), v.end(), [](const VALTYPE& x) { return x!=VALTYPE(); });
    }
    std::vector<unkitem> _unkitems;
    void readheader()
    {
        _r->setpos(0);
        uint32_t hdrsize= _r->read32le();       // +0000
        uint32_t nul_0004= _r->read32le();      // +0004
        
        uint32_t magic= _r->read32le();         // +0008
        if (magic!=0x4d494b45)                  // 'ELIM'
            throw "not a hv/vol file";

        ByteVector filemd5;
        vectorread8(_r, filemd5, 16);           // +000c

        uint32_t nul_001c= _r->read32le();      // +001c
        uint32_t filesize= _r->read32le();      // +0020
        if (filesize>_r->size()) {
            printf("WARN: stored filesize > real filesize\n");
        }
        if (filesize<_r->size()) {
            printf("WARN: stored filesize < real filesize\n");
        }
        uint32_t filetype= _r->read32le();      // +0024  - 0x1000 for db files

        vectorread8(_r, _bootmd5, 16);           // +0028

        DwordVector usuallynul_0038;
        vectorread32le(_r, usuallynul_0038, (0xE4-0x38)/4);     // +0038
        if (!is_all_zero(usuallynul_0038))
            printf("WARN: +0038: %s\n", vhexdump(usuallynul_0038).c_str());

        uint32_t base= _r->read32le();           // +00e4  .. 0x01025000
        uint32_t nul_00e8= _r->read32le();      // +00e8  recoverylog size
        uint32_t isreghive= _r->read32le();     // +00ec
        uint32_t isdbvol= _r->read32le();       // +00f0

        if (isreghive && filetype!=0)
            printf("WARNING, unknown flag combination: +0024=%08x, +00ec=%08x\n", filetype, isreghive);
        if (isdbvol && filetype==0)
            printf("WARNING, unknown flag combination: +0024=%08x, +00ec=%08x\n", filetype, isdbvol);

        if (hdrsize!=0x400)
            printf("WARN: unusual hdrsize : %08x\n", hdrsize);
        if (nul_0004)
            printf("WARN: +0004: %08x\n", nul_0004);
        if (nul_001c)
            printf("WARN: +001c: %08x\n", nul_001c);
        if (nul_00e8)
            printf("WARN: +00e8: %08x\n", nul_00e8);

        if (g_verbose) {
            _r->setpos(0);
            DwordVector filehdr1;
            vectorread32le(_r, filehdr1, 0x38/4);
            DwordVector filehdr2;
            _r->setpos(0xe4);
            vectorread32le(_r, filehdr2, (0xf4-0xe4)/4);
            printf("          hdrsize           magic    --filemd5--------------------------          filesize filetype --bootmd5-------------------------- ... base              isreghv  isdbvol\n");
            printf("filehdr: %s ...%s\n", vhexdump(filehdr1).c_str(), vhexdump(filehdr2).c_str());
        }


        DwordVector usuallynul_00f4;
        vectorread32le(_r, usuallynul_00f4, 6); // +00f4
        if (!is_all_zero(usuallynul_00f4))
            printf("WARN: +00f4: %s\n", vhexdump(usuallynul_00f4).c_str());

        if (g_verbose) {
            // read unknown items --- probably ptrs used when mounted
            printf("base=%08x\n", base);
            uint32_t ofs= 0x10c;
            while (ofs<hdrsize) {
                uint32_t type= _r->read32le();      // +010c + 0x10*i
                uint32_t ptr= _r->read32le();       // +0110 + 0x10*i
                uint32_t unk= _r->read32le();       // +0114 + 0x10*i
                uint32_t flag= _r->read32le();      // +0118 + 0x10*i
                if (type==0)
                    break;

                _unkitems.push_back(unkitem(type, ptr, unk, flag));

                printf("%d %08x[+%8x]  %08x %8x\n", type, ptr, ptr-base, unk, flag);
            }
        }


        // read section ptrs
        _r->setpos(0x1000);

        _offsets.push_back(_r->read32le());     // +1000
        while (1)
        {
            uint32_t sofs= _r->read32le();      // +1000 + 4*i
            if (sofs==0)
                break;
            _offsets.push_back(sofs);
        }
        if (g_verbose)
            printf("hdrptrs: %s\n", vhexdump(_offsets).c_str());
        _offsets.push_back(filesize);

        // read section headers
        for (unsigned i=0 ; i<_offsets.size()-1 ; i++)
        {
            _r->setpos(0x5000+_offsets[i]);

            uint32_t smagic= _r->read32le();    // +5000
            uint32_t snul_0004= _r->read32le(); // +5004
            uint32_t idx= _r->read32le();       // +5008
            if (smagic!=0x20001004)
                throw "invalid section magic";
            if (snul_0004)
                printf("WARN: section%d @%08x : +4=%08x\n", i, _offsets[i], snul_0004);
            if (idx!=i)
                printf("WARN: section%d @%08x : +8=%08x\n", i, _offsets[i], idx);
        }
    }
    void writeheader(ReadWriter_ptr w)
    {
        w->write32le(0x400);                            // +0000 : filehdr size
        w->write32le(0);                                // +0004 : 
        w->write32le(0x4d494b45);                       // +0008 : file magic 'MIKE'
        // +000c : filemd5 later
        w->setpos(0x20);
        w->write32le(w->size());                        // +0020 : filesize
        w->write32le(0);                                // +0024 : filetype : 0 = hv
        w->write(&_bootmd5.front(), _bootmd5.size());   // +0028 : bootmd5

        w->setpos(0xe4);
        w->write32le(0x01025000);                       // +00e4 : base ??

        w->setpos(0xec);
        w->write32le(-1);                               // +00ec : isreghive

        ByteVector digest(16);
        calcfilemd5(w, &digest.front());
        w->setpos(0x0c);
        w->write(&digest.front(), digest.size());
    }
    void calcfilemd5(ReadWriter_ptr w, uint8_t *digest)
    {
        w->setpos(0xfc);
        
        Md5 m;
        while (!w->eof())
        {
            ByteVector data(16384);
            size_t n= w->read(&data.front(), data.size());
            m.add(&data.front(), n);
        }
        m.final(digest);
    }

    void writesectionheader(ReadWriter_ptr w, int n)
    {
        w->write32le(0x20001004);
        w->write32le(0);
        w->write32le(n);
    }
public:
    HvFile()
        : _rootid(0)
    {
        _items.push_back(ent::entry_ptr(new ent::roots(_items.size())));
        _rootid= _items.back()->id();
    }
    HvFile(ReadWriter_ptr r)
        : _r(r), _rootid(0)
    {
        readheader();
    }
    void setbootmd5(const ByteVector& md5)
    {
        _bootmd5= md5;
    }
    void save(ReadWriter_ptr w)
    {
        w->setpos(0x5000);
        std::vector<uint32_t> sectionoffsets;
        uint64_t sectionendpos;
        for (unsigned i=0 ; i<_items.size() ; i+=0x400)
        {
            sectionoffsets.push_back(w->getpos()-0x5000);
            writesectionheader(w, i/0x400);
            uint64_t offsetblockpos= w->getpos();

            std::vector<uint32_t> itemoffsets;

            // skip item offset block - write later
            w->setpos(offsetblockpos+0x1000);

            // write item count
            w->write32le(_items.size()-i<0x400 ? _items.size()-i : 0);

            // write item data
            for (unsigned j= 0 ; j<0x400 && j+i<_items.size() ; j++)
            {
                itemoffsets.push_back(w->getpos()-0x5000);

                _items[i+j]->save(w);
            }
            sectionendpos= w->getpos();
            w->setpos(offsetblockpos);
            for (unsigned j= 0 ; j<0x400 ; j++)
                w->write32le(j<itemoffsets.size() ? itemoffsets[j]+1 : j<0x3ff ? (j+1)*0x40000 : 0);
            w->setpos(sectionendpos);
        }
        if (sectionendpos&0xfff) {
            size_t padding= 0x1000-(sectionendpos&0xfff);
            w->truncate(sectionendpos+padding);
        }
        w->setpos(0x1000);
        for (unsigned j= 0 ; j<sectionoffsets.size() ; j++)
            w->write32le(sectionoffsets[j]);
        w->setpos(0);
        writeheader(w);
    }
    void enumfileentries(std::function<void(ent::entry_ptr)> cb)
    {
        for (unsigned i=0 ; i<_offsets.size()-1 ; i++)
            enumsectionentries(_offsets[i], _r->size()-0x5000, cb);
    }
    void enumsectionentries(uint32_t startofs, uint32_t maxofs, std::function<void(ent::entry_ptr)> cb)
    {
        uint32_t ofs= startofs;
        if (g_verbose>1)
            printf("%08x-%08x: sectionhdr\n", ofs, ofs+12);
        DwordVector hdrvalues;
        _r->setpos(0x5000 + ofs);
        vectorread32le(_r, hdrvalues, 3);
        ofs += 12;

        if (g_verbose>1)
            printf("%08x-%08x: entryptrs\n", ofs, ofs+0x1000);
        DwordVector iofs;
        _r->setpos(0x5000 + ofs);
        vectorread32le(_r, iofs, 0x400);
        ofs += 0x1000;

        if (g_verbose>1)
            printf("%08x-%08x: entrycount\n", ofs, ofs+4);
        // todo:  is this really a count, or something else? it points to the entry with value 0x10000000
        uint32_t count= _r->read32le();
        if (g_verbose) {
            printf("hdr: %s [%08x] %s\n", vhexdump(hdrvalues).c_str(), count, vhexdump(iofs).c_str());
        }

        ofs += 4;

        for (unsigned i=0 ; i<1024; i++)
        {
            uint32_t entryofs= iofs[i]&0x0ffffffc;
            if ((iofs[i]&3)==1 && entryofs<maxofs) {
                _r->setpos(0x5000 + entryofs );

                cb(ent::base::readentry(_r, entryofs, (iofs[i]>>28)|((iofs[i]&3)<<4)));
            }
            else if (iofs[i]!=(i+1)*0x40000 && iofs[i]!=0) {
                printf("WARN: @%08x: entry %03x: %08x\n", startofs+12+i*4, i, iofs[i]);
            }
        }
    }


    typedef std::map<std::string, uint32_t> pathmap_t;
    typedef std::map<HKEY,pathmap_t> rootmap_t;
    uint32_t _rootid;
    std::vector<ent::entry_ptr> _items;
    rootmap_t _roots;

    uint32_t allocpath(HKEY root, uint32_t parent, const std::string& path)
    {
        _items.push_back(ent::entry_ptr(new ent::key(_items.size(), path)));
        uint32_t id= _items.back()->id();

        if (parent) {
            uint32_t lkey= _items[parent]->askey()->lastchild();
            if (!lkey)
                _items[parent]->askey()->firstchild(id);
            else
                _items[lkey]->askey()->nextsibling(id);
            _items[parent]->askey()->lastchild(id);
        }
        else {
            uint32_t rkey= _items[_rootid]->asroots()->lasthivekey(root);
            if (!rkey)
                _items[_rootid]->asroots()->hiveid(root, id);
            else 
                _items[rkey]->askey()->nextsibling(id);
            _items[_rootid]->asroots()->lasthivekey(root, id);
        }

        return id;
    }
    void dumpmap(const pathmap_t& m)
    {
        printf("M: ");
        for (auto i= m.begin() ; i!=m.end() ; ++i)
            printf(" %08x:'%s'", i->second, i->first.c_str());
        printf("\n");
    }
    uint32_t createpath(HKEY root, pathmap_t& pmap, const std::string& path)
    {
        auto p= pmap.find(path);
        if (p!=pmap.end())
            return p->second;

        size_t slash= path.find_last_of("\\");
        uint32_t parentid= slash==path.npos ? 0 : createpath(root, pmap, path.substr(0,slash));

        uint32_t id= allocpath(root, parentid, slash==path.npos ? path : path.substr(slash+1));

        auto ins= pmap.insert(pathmap_t::value_type(path, id));
        if (!ins.second)
            throw "insert failed unexpectedly";
        return ins.first->second;
    }
    uint32_t CreateKey(const RegistryPath& regpath)
    {
        // lookup path in cache or alloc new path, then return id
        auto r= _roots.find(regpath.GetRoot());
        if (r==_roots.end())
        {
            auto ins= _roots.insert(rootmap_t::value_type(regpath.GetRoot(), pathmap_t()));
            if (!ins.second)
                throw "insert failed unexpectedly";
            r= ins.first;
        }
        pathmap_t &root= r->second;
        return createpath(regpath.GetRoot(), root, regpath.GetPath());
    }
    void SetValue(uint32_t keyid, const std::string& valuename, const RegistryValue& value)
    {
        ent::value_ptr v;
        switch(value.GetType())
        {
            case REG_SZ:       v= ent::value_ptr(new ent::stringvalue(_items.size(), valuename, value.GetString())); break;
            case REG_BINARY:   v= ent::value_ptr(new ent::binaryvalue(_items.size(), valuename, value.GetData())); break;
            case REG_DWORD:    v= ent::value_ptr(new ent::dwordvalue(_items.size(), valuename, value.GetDword())); break;
            case REG_MULTI_SZ: v= ent::value_ptr(new ent::stringlistvalue(_items.size(), valuename, value.GetStringList())); break;
            case REG_MUI_SZ:   v= ent::value_ptr(new ent::muistringvalue(_items.size(), valuename, value.GetString())); break;
        }
        if (!v) {
            printf("WARN: unsupported: %s\n", value.AsString(0).c_str());
            throw "unsupported registryvalue type";
        }

        _items.push_back(v);

        uint32_t lastv= _items[keyid]->askey()->lastvalue();
        if (lastv==0)
            _items[keyid]->askey()->firstvalue(v->id());
        else
            _items[lastv]->asvalue()->nextvalue(v->id());
        _items[keyid]->askey()->lastvalue(v->id());

    }
};
struct hvmaker : regkeymaker {
    HvFile hv;
    uint32_t curkey;
    hvmaker()
        : curkey(0)
    {
    }
    void newkey(const RegistryPath& path)
    {
        curkey= hv.CreateKey(path);
    }
    void setval(const std::string& valuename, const RegistryValue& value)
    {
        hv.SetValue(curkey, valuename=="@" ? "Default" : valuename, value);
    }

    void setbootmd5(const ByteVector& md5)
    {
        hv.setbootmd5(md5);
    }
    void save(ReadWriter_ptr w)
    {
        hv.save(w);
    }
}; 

class dumper {
    const ent::entrymap_t& items;
public:
    dumper(const ent::entrymap_t& items) : items(items) { }
    virtual ~dumper() { }

    void dumpvalues(uint32_t id)
    {
        while (id)
        {
            ent::value *v= getval(id);
            dumpvalue(v);
            id= v->nextvalue();
        }
    }
    virtual void dumpvalue(ent::value *v)= 0;
    void dumpkeys(uint32_t id, const std::string& path)
    {
        while (id)
        {
            ent::key *k= getkey(id);
            dumpkey(k, path);
            dumpvalues(k->firstvalue());
            dumpkeys(k->firstchild(), path+"\\"+k->name());
            id= k->nextsibling();
        }
    }
    virtual void dumpkey(ent::key *k, const std::string& path)= 0;
    void dumproot()            
    {
        ent::roots *r= getroot(0);
        if (!r) {
            printf("could not find root\n");
            return;
        }
        dumproots(r);

        dumpkeys(r->hiveid((HKEY)ent::HKCR), "HKCR");
        dumpkeys(r->hiveid((HKEY)ent::HKCU), "HKCU");
        dumpkeys(r->hiveid((HKEY)ent::HKLM), "HKLM");
    }
    virtual void dumproots(ent::roots *r)= 0;
    ent::entry_ptr get(uint32_t id)
    {
        auto i= items.find(id&0x0fffffff);
        if (i==items.end())
            return ent::entry_ptr();

        return i->second;
    }
    ent::roots* getroot(uint32_t id)
    {
        auto p= get(id);
        if (p)
            return p->asroots();

        return NULL;
    }
    ent::key* getkey(uint32_t id)
    {
        auto p= get(id);
        if (p)
            return p->askey();

        return NULL;
    }
    ent::value* getval(uint32_t id)
    {
        auto p= get(id);
        if (p)
            return p->asvalue();

        return NULL;
    }
};
class rawdumper : public dumper {
public:
    rawdumper(const ent::entrymap_t& items) : dumper(items) { }
    virtual void dumpvalue(ent::value *v)
    {
        printf("[%08x]         n:%08x %-64s  %s\n", v->id(), v->nextvalue(), v->name().c_str(), v->asstring().c_str());
    }
    virtual void dumpkey(ent::key *k, const std::string& path)
    {
        printf("[%08x] KEY S:%08x C:%08x V:%08x %s\n", k->id(), k->nextsibling(), k->firstchild(), k->firstvalue(), k->name().c_str());
    }
    virtual void dumproots(ent::roots* r)
    {
        printf("[%08d]   hkcr:[%08x], hkcu:[%08x], hklm:[%08x]\n", r->id(), r->hiveid((HKEY)ent::HKCR), r->hiveid((HKEY)ent::HKCU), r->hiveid((HKEY)ent::HKLM));
    }

};
class regdumper : public dumper {
public:
    regdumper(const ent::entrymap_t& items) : dumper(items) { }

    virtual void dumpvalue(ent::value *v)
    {
        if (v->name() == "Default")
            printf(" @=%s\n", v->asstring().c_str());
        else
            printf(" \"%s\"=%s\n", v->name().c_str(), v->asstring().c_str());
    }
    virtual void dumpkey(ent::key *k, const std::string& path)
    {
        if (k->firstvalue() || !k->firstchild())
            printf("\n[%s\\%s]\n", path.c_str(), k->name().c_str());
    }
    virtual void dumproots(ent::roots* r)
    {
        printf("REGEDIT4\n");
    }
};
void usage()
{
    printf("Usage: hvtool [-v] [-r] [-o OUTFILE] [-b bootmd5hex]  regfiles...\n");
}
int main(int argc, char**argv)
{
    StringList files;
    std::string outfile;
    std::string bootmd5arg;
    ByteVector bootmd5;
    bool fDumpAsRaw= false;


    try {
    for (int i=1 ; i<argc ; i++)
    {
        if (argv[i][0]=='-') switch(argv[i][1])
        {
            case 'o': getarg(argv, i, argc, outfile); break;
            case 'b': bootmd5arg = getstrarg(argv, i, argc); break;
            case 'v': g_verbose+=countoptionmultiplicity(argv, i, argc); break;
            case 'r': fDumpAsRaw= true;; break;
            default:
                      usage();
                      return 1;
        }
        else {
            files.push_back(argv[i]);
        }
    }
    if (!bootmd5arg.empty())
        hex2binary(bootmd5arg, bootmd5);

    if (files.empty()) {
        usage();
        return 1;
    }
    if (!outfile.empty()) {
        hvmaker mk;
        for (unsigned i=0 ; i<files.size() ; i++) {
            if (!ProcessRegFile(files[i], mk))
                return 1;
        }

        if (!bootmd5.empty())
            mk.setbootmd5(bootmd5);
        mk.save(ReadWriter_ptr(new FileReader(outfile, FileReader::createnew)));
    }
    else {
        for (unsigned i=0 ; i<files.size() ; i++) {
            if (files.size()>1)
            printf(";=============== processing %s\n", files[i].c_str());

            ent::entrymap_t items;
            HvFile hv(ReadWriter_ptr(new MmapReader(files[i], MmapReader::readonly)));

            hv.enumfileentries([&items](ent::entry_ptr p) {
                items.insert(ent::entrymap_t::value_type(p->id(), p));
            });
            std::shared_ptr<dumper> d;
            if (fDumpAsRaw)
                d.reset(new rawdumper(items));
            else
                d.reset(new regdumper(items));
            d->dumproot();
        }
    }
    }
    catch(const char*msg) { printf("ERROR: %s\n", msg); return 1; }
    catch(const std::string& msg) { printf("ERROR: %s\n", msg.c_str()); return 1; }
    catch(...) { printf("unknown error\n"); return 1; }
    return 0;
}
