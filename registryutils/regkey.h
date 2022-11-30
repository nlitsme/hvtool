#ifndef _REG_KEY_H_
#define _REG_KEY_H_

#ifdef _WIN32
#include <windows.h>
#endif
#include "regfileparser.h"

#ifdef WINCEREGUTL
#ifndef _WIN32_WCE
// include only for win32 version
#include <util/rapitypes.h>
#include "dllversion.h"
#endif
#endif

#include "regvalue.h"
#include "regpath.h"

#define STD_NAME_BUF_SIZE   256
#define STD_VALUE_BUF_SIZE  32768

// this registry value type is specific to windows ce
#ifndef REG_MUI_SZ 
#define REG_MUI_SZ   (21)
#endif

#if defined(WIN32REGUTL) || defined(_WIN32_WCE)
#define __RegCloseKey        RegCloseKey
#define __RegCreateKeyEx     RegCreateKeyEx
#define __RegDeleteKey       RegDeleteKey
#define __RegDeleteValue     RegDeleteValue
#define __RegEnumKeyEx       RegEnumKeyEx
#define __RegEnumValue       RegEnumValue
#define __RegOpenKeyEx       RegOpenKeyEx
#define __RegQueryInfoKey    RegQueryInfoKey
#define __RegQueryValueEx    RegQueryValueEx
#define __RegSetValueEx      RegSetValueEx
#endif

#ifdef WINCEREGUTL
#define __RegCloseKey        CeRegCloseKey
#define __RegCreateKeyEx     CeRegCreateKeyEx
#define __RegDeleteKey       CeRegDeleteKey
#define __RegDeleteValue     CeRegDeleteValue
#define __RegEnumKeyEx       CeRegEnumKeyEx
#define __RegEnumValue       CeRegEnumValue
#define __RegOpenKeyEx       CeRegOpenKeyEx
#define __RegQueryInfoKey    CeRegQueryInfoKey
#define __RegQueryValueEx    CeRegQueryValueEx
#define __RegSetValueEx      CeRegSetValueEx
#endif



class RegistryKey {
    static int& level() { 
        static int l;
        return l;
    }
public:
    typedef std::vector<RegistryKey> List;

    RegistryKey() : m_hKey(0), m_suppress_errors(false)
    {
    }

    RegistryKey(HKEY hRoot) : m_path(hRoot), m_hKey(0)
    {
    }

    RegistryKey(const RegistryKey& hKey) : m_path(hKey.m_path), m_hKey(0)
    {
    }
    RegistryKey(const RegistryPath& path) : m_path(path), m_hKey(0)
    {
    }

    RegistryKey& operator=(const RegistryKey& rk)
    {
        m_path= rk.m_path;
        m_hKey= 0;
        return *this;
    }
    RegistryKey(const RegistryKey& hRoot, const std::string& path) : m_hKey(0), m_path(hRoot.m_path, path)
    {
    }
    ~RegistryKey()
    {
        Close();
    }

    void Close()
    {
        __RegCloseKey(m_hKey);
        m_hKey= 0;
    }
    bool Open()
    {
        if (m_hKey)
            return true;

        DWORD dwAccessTypes[5]= {0xf003f, 0x20019, KEY_ALL_ACCESS|DELETE, KEY_ALL_ACCESS, KEY_READ};

        LONG res=-1;
        for (int i=0 ; i<5 ; i++) {
            res= __RegOpenKeyEx(GetRoot(), ToWString(GetPath()).c_str(), 0, dwAccessTypes[i], &m_hKey);
            if (res==ERROR_SUCCESS)
                break;
        }
        if (res!=ERROR_SUCCESS)
        {
            if (!m_suppress_errors)
                error(res, "Failed to open key %hs:'%hs'", GetRootName().c_str(), GetPath().c_str());
            m_hKey= 0;
            return false;
        }
        m_dwMaxSubKeyNameLength= STD_NAME_BUF_SIZE;
        m_dwMaxValueNameLength= STD_NAME_BUF_SIZE;
        m_dwMaxValueLength= STD_VALUE_BUF_SIZE;
/*
 *
 * this does not seem to work on ce/rapi
 *
        __RegQueryInfoKey(m_hKey, NULL, NULL, NULL, NULL, &m_dwMaxSubKeyNameLength,
                NULL, NULL, &m_dwMaxValueNameLength, &m_dwMaxValueLength, NULL, NULL);

        if ( m_dwMaxSubKeyNameLength==0 && m_dwMaxValueNameLength==0 && m_dwMaxValueLength==0)
        {
            debug("ERROR: noinfo\n");
            m_dwMaxSubKeyNameLength= STD_NAME_BUF_SIZE;
            m_dwMaxValueNameLength= STD_NAME_BUF_SIZE;
            m_dwMaxValueLength= STD_VALUE_BUF_SIZE;
        }
*/
        return true;
    }
    HKEY GetHKey()
    {
        Open();

        return m_hKey;
    }

    void info()
    {
        std::Wstring regclass; regclass.resize(256);
        DWORD cbClass= regclass.size();

        DWORD nSubKeys=0;
        DWORD maxSubKeyNameLength=0;
        DWORD maxClassLength=0;
        DWORD nValues=0;
        DWORD maxValueNameLength=0;
        DWORD maxValueLength=0;

        Open();

        // BUG: only the key and value counts are returned
        LONG res= __RegQueryInfoKey(m_hKey, stringptr(regclass), &cbClass, NULL, &nSubKeys,
                &maxSubKeyNameLength, &maxClassLength, &nValues,
                &maxValueNameLength, &maxValueLength, NULL, NULL);
        if (res != ERROR_SUCCESS)
        {
            error(res, "__RegQueryInfoKey(%hs:'%hs')", GetRootName().c_str(), GetPath().c_str());
            return ;
        }
        regclass.resize(cbClass);

        debug("regclass='%hs'  nkeys=%d nvals=%d  maxkey=%d maxcls=%d maxname=%d maxval=%d\n",
                regclass.c_str(), nSubKeys, maxSubKeyNameLength,
                maxClassLength, nValues, maxValueNameLength,&maxValueLength);
    }

    // creates a new subkey
    bool CreateSubKey(const std::string& keyname)
    {
        if (!Open())
            return false;

        HKEY hk;
        DWORD disp;
        LONG res= __RegCreateKeyEx(m_hKey, ToWString(keyname).c_str(), 0, NULL, 0, 0, NULL, &hk, &disp);
        if (res != ERROR_SUCCESS)
        {
            error(res, "Failed to create subkey %hs:'%hs'\\'%hs'", GetRootName().c_str(), GetPath().c_str(), keyname.c_str());
            return false;
        }
        __RegCloseKey(hk);
        return true;
    }


    // deletes a childless subkey from this key
    bool DeleteSubKey(const std::string& keyname)
    {
        if (!Open())
            return false;
        LONG res= __RegDeleteKey(m_hKey, ToWString(keyname).c_str());
        if (res != ERROR_SUCCESS)
        {
            error(res, "Failed to delete subkey %hs:'%hs'\\'%hs'", GetRootName().c_str(), GetPath().c_str(), keyname.c_str());
            return false;
        }
        return true;
    }

    // delete all children
    bool DeleteChildren()
    {
        bool bRes= true;
        StringList children= GetSubKeyNames();
        for (StringList::iterator i= children.begin() ; i!=children.end() ; ++i)
        {
            bRes = bRes && DeleteSubKey(*i);
        }
        return bRes;
    }

    bool DeleteValue(const std::string& valuename)
    {
        if (!Open())
            return false;
        LONG res= __RegDeleteValue(m_hKey, ToWString(valuename).c_str());
        if (res != ERROR_SUCCESS)
        {
            error(res, "Failed to delete value %hs:'%hs'\\'%hs'", GetRootName().c_str(), GetPath().c_str(), valuename.c_str());
            return false;
        }
        return true;
    }

    //-------------------------------------------------------------------
    // deletes this key and all it's children
    bool DeleteKey()
    {
        if (!DeleteChildren())
            return false;

        Close();

        if (!GetParent().DeleteSubKey(m_path.GetName()))
            return false;

        return true;
    }

    //-------------------------------------------------------------------
    // creates registry key
    bool CreateKey()
    {
        m_suppress_errors= true;
        if (Open()) {
            m_suppress_errors= false;
            Close();
            return true;
        }
        m_suppress_errors= false;

		if (GetPath().empty())
			return false;

        if (!GetParent().CreateKey())
            return false;

        if (!GetParent().CreateSubKey(m_path.GetName()))
            return false;

        debug("created key %hs\\%hs\n", GetRootName().c_str(), GetPath().c_str());
        return true;
    }


    void DumpKey(int hex, int maxdepth)
    {
        debug("[%hs\\%hs]\n", GetRootName().c_str(), GetPath().c_str());
        try {
            DumpValues(hex);
        } catch(std::string &e) { debug("ERROR: %s\n", e.c_str()); }
        debug("\n");

        level()++;
        if (maxdepth==-1 || maxdepth>=level()) {
            try {
                DumpChildren(hex, maxdepth);
            } catch(std::string &e) { debug("ERROR: %s\n", e.c_str()); }
        }
        level()--;
    }

    //-------------------------------------------------------------------
    void DumpValues(int hex)
    {
        RegistryValue::StringMap map= GetValues();

        for (RegistryValue::StringMap::iterator i= map.begin() ; i!=map.end() ; ++i)
        {
            if (hex>2) {
                std::wstring wkey= ToWString((*i).first);
                debug(";%hs\n", hexstring((BYTE*)stringptr(wkey), 2*wkey.size(), ',').c_str());
            }
            debug("%hs=%hs\n", (*i).first.c_str(), (*i).second.AsString(hex).c_str());
            RegistryValue::DumpWarnings();
        }
    }

    void DumpChildren(int hex, int maxdepth)
    {
        List list= GetSubKeys();

        for (List::iterator i= list.begin() ; i!=list.end() ; ++i)
        {
            (*i).DumpKey(hex, maxdepth);
        }
    }

    //-------------------------------------------------------------------
    // returns parent to this key
    RegistryKey GetParent()
    {
        return RegistryKey(m_path.GetParent());
    }
    StringList GetSubKeyNames()
    {
        StringList list;
        if (!Open())
            throw stringformat("GetSubKeyNames(%hs:'%hs'): could not open", GetRootName().c_str(), GetPath().c_str());

        for (DWORD i=0 ; true ; i++)
        {
            DWORD cbName = m_dwMaxSubKeyNameLength+1;
            std::Wstring wname; wname.resize(cbName);
            LONG res =__RegEnumKeyEx(m_hKey, i, stringptr(wname), &cbName, NULL, NULL, NULL, NULL);
            if (res==ERROR_NO_MORE_ITEMS)
                break;
            if (res!=ERROR_SUCCESS)
            {
                error(res, "__RegEnumKeyEx(%hs:'%hs', %d)", GetRootName().c_str(), GetPath().c_str(), i);
                continue;
            }
            wname.resize(cbName);
            list.push_back(ToString(wname));
        }
        return list;
    }
    List GetSubKeys()
    {
        List list;
        if (!Open())
            throw stringformat("GetSubKeys(%hs:'%hs'): could not open", GetRootName().c_str(), GetPath().c_str());

        for (DWORD i=0 ; true ; i++)
        {
            DWORD cbName = m_dwMaxSubKeyNameLength+1;
            std::Wstring wname; wname.resize(cbName);
            LONG res =__RegEnumKeyEx(m_hKey, i, stringptr(wname), &cbName, NULL, NULL, NULL, NULL);
            if (res==ERROR_NO_MORE_ITEMS)
                break;
            if (res!=ERROR_SUCCESS)
            {
                error(res, "__RegEnumKeyEx(%hs:'%hs', %d)", GetRootName().c_str(), GetPath().c_str(), i);
                continue;
            }
            wname.resize(cbName);
            std::string aname= ToString(wname);
            if (aname.empty())
                debug("ERROR converting subkey %d of %s : %s\n", i, GetPath().c_str(), hexdump((BYTE*)wname.c_str(), wname.size(), 2).c_str());
            else
                list.push_back(RegistryKey(*this, aname));
        }
        return list;
    }
    StringList GetValueNames()
    {
        StringList list;
        if (!Open())
            throw stringformat("GetValueNames(%hs:'%hs'): could not open", GetRootName().c_str(), GetPath().c_str());

        for (DWORD i=0 ; true ; i++)
        {
            DWORD cbName = m_dwMaxValueNameLength+1;
            std::Wstring wname; wname.resize(cbName);
            LONG res= __RegEnumValue(m_hKey, i, stringptr(wname), &cbName, NULL, NULL, NULL, NULL);
            if (res==ERROR_NO_MORE_ITEMS)
                break;
            if (res==ERROR_MORE_DATA)
            {
                wname.resize(cbName);
                res= __RegEnumValue(m_hKey, i, stringptr(wname), &cbName, NULL, NULL, NULL, NULL);
            }
            if (res!=ERROR_SUCCESS)
            {
                error(res, "GetValueNames: __RegEnumValue(%hs:'%hs', %d)", GetRootName().c_str(), GetPath().c_str(), i);
                continue;
            }
            wname.resize(cbName);
            list.push_back(ToString(wname));
        }
        return list;
    }
    RegistryValue::StringMap GetValues()
    {
        RegistryValue::StringMap map;
        if (!Open())
            throw stringformat("GetValues(%hs:'%hs'): could not open", GetRootName().c_str(), GetPath().c_str());

        for (DWORD i=0 ; true ; i++)
        {
            DWORD cbName = m_dwMaxValueNameLength+1;
            std::Wstring wname; wname.resize(cbName);
            DWORD cbData = m_dwMaxValueLength;
            ByteVector data; data.resize(cbData);
            DWORD dwType;
            LONG res= __RegEnumValue(m_hKey, i, stringptr(wname), &cbName, NULL, &dwType, vectorptr(data), &cbData);
            if (res==ERROR_NO_MORE_ITEMS)
                break;
            if (res==ERROR_MORE_DATA)
            {
                wname.resize(cbName);
                data.resize(cbData);
                res= __RegEnumValue(m_hKey, i, stringptr(wname), &cbName, NULL, &dwType, vectorptr(data), &cbData);
            }
            if (res!=ERROR_SUCCESS)
            {
                error(res, "GetValues: __RegEnumValue(%hs:'%hs', %d)", GetRootName().c_str(), GetPath().c_str(), i);
                continue;
            }
            wname.resize(cbName);
            data.resize(cbData);

            map[ToString(wname)]= RegistryValue(dwType, data);
        }
        return map;
    }
    RegistryValue GetValue(const std::string& valuename)
    {
        if (!Open())
            throw stringformat("GetValue(%hs:'%hs', '%hs'): could not open", GetRootName().c_str(), GetPath().c_str(), valuename.c_str());
        DWORD cbData = m_dwMaxValueLength;
        ByteVector data; data.resize(cbData);
        DWORD dwType;
        LONG res= __RegQueryValueEx(m_hKey, ToWString(valuename).c_str(), NULL, &dwType, vectorptr(data), &cbData);
        if (res==ERROR_MORE_DATA)
        {
            data.resize(cbData);
            res= __RegQueryValueEx(m_hKey, ToWString(valuename).c_str(), NULL, &dwType, vectorptr(data), &cbData);
        }
        if (res!=ERROR_SUCCESS)
            throw stringformat("GetValue(%hs:'%hs', '%hs'): __RegQueryValueEx:%08lx", GetRootName().c_str(), GetPath().c_str(), valuename.c_str(), res);
        data.resize(cbData);
        return RegistryValue(dwType, data);
    }
    bool SetValue(const std::string& valuename, const RegistryValue& value)
    {
        if (!Open())
            throw stringformat("SetValue(%hs:'%hs', '%hs'): could not open", GetRootName().c_str(), GetPath().c_str(), valuename.c_str());

        //debug("setting [%s] %s to %s\n", GetPath().c_str(), valuename.c_str(), value.AsString(0).c_str());
        ByteVector data= value.GetData();
        LONG res= __RegSetValueEx(m_hKey, ToWString(valuename).c_str(), NULL, value.GetType(), vectorptr(data), data.size());
        if (res!=ERROR_SUCCESS)
            throw stringformat("SetValue(%hs:'%hs', '%hs'): __RegSetValueEx:%08lx", GetRootName().c_str(), GetPath().c_str(), valuename.c_str(), res);
        debug("set value %hs\\%hs  %hs : %hs\n", GetRootName().c_str(), GetPath().c_str(), valuename.c_str(), value.AsString(1).c_str());
        return true;
    }

    std::string GetPath() const { return m_path.GetPath(); }
    HKEY GetRoot() const { return m_path.GetRoot(); }
    std::string GetRootName() const { return m_path.GetRootName(); }
    static RegistryKey FromKeySpec(const std::string& keyspec) { return RegistryKey(RegistryPath::FromKeySpec(keyspec)); }
private:
        RegistryPath m_path;

        HKEY m_hKey;
        DWORD m_dwMaxSubKeyNameLength;
        DWORD m_dwMaxValueNameLength;
        DWORD m_dwMaxValueLength;

        bool m_suppress_errors;
};



#endif
