#ifndef _REG_PATH_H_
#define _REG_PATH_H_

#include "stringutils.h"

#ifdef _WIN32
#include <windows.h>
#else
typedef uint32_t HKEY;

#define HKEY_CLASSES_ROOT       ( (HKEY) 0x80000000 )
#define HKEY_CURRENT_USER       ( (HKEY) 0x80000001 )
#define HKEY_LOCAL_MACHINE       ( (HKEY) 0x80000002 )
#define HKEY_USERS       ( (HKEY) 0x80000003 )
#define HKEY_PERFORMANCE_DATA       ( (HKEY) 0x80000004 )
#define HKEY_CURRENT_CONFIG       ( (HKEY) 0x80000005 )
#define HKEY_DYN_DATA       ( (HKEY) 0x80000006 )
#define HKEY_PERFORMANCE_TEXT       ( (HKEY) 0x80000050 )
#define HKEY_PERFORMANCE_NLSTEXT       ( (HKEY) 0x80000060 )

#endif
class RegistryPath {
public:
    static RegistryPath FromKeySpec(const std::string& keyspec)
    {
        HKEY hRoot;
        std::string path;

        size_t slashpos= keyspec.find_first_of("/\\");

        // 0x80000000 HKEY_CLASSES_ROOT        hkcr
        // 0x80000001 HKEY_CURRENT_USER        hkcu
        // 0x80000002 HKEY_LOCAL_MACHINE       hklm  hkpr
        // 0x80000003 HKEY_USERS               hku
        // 0x80000004 HKEY_PERFORMANCE_DATA    hkpd
        // 0x80000005 HKEY_CURRENT_CONFIG      hkcc
        // 0x80000006 HKEY_DYN_DATA            hkdd
        // 0x80000050 HKEY_PERFORMANCE_TEXT    hkpt
        // 0x80000060 HKEY_PERFORMANCE_NLSTEXT hkpn
        //
        if (stringicompare(keyspec.substr(0,slashpos), std::string("hkcr"))==0)
            hRoot= HKEY_CLASSES_ROOT;
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkey_classes_root"))==0)
            hRoot= HKEY_CLASSES_ROOT;
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkcu"))==0)
            hRoot= HKEY_CURRENT_USER;
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkey_current_user"))==0)
            hRoot= HKEY_CURRENT_USER;
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hklm"))==0)
            hRoot= HKEY_LOCAL_MACHINE;
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkey_local_machine"))==0)
            hRoot= HKEY_LOCAL_MACHINE;
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hku"))==0)
            hRoot= HKEY_USERS;
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkey_users"))==0)
            hRoot= HKEY_USERS;
#ifdef HKEY_PERFORMANCE_DATA
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkpd"))==0)
            hRoot= HKEY_PERFORMANCE_DATA;
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkey_performance_data"))==0)
            hRoot= HKEY_PERFORMANCE_DATA;
#endif
#ifdef HKEY_CURRENT_CONFIG
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkcc"))==0)
            hRoot= HKEY_CURRENT_CONFIG;
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkey_current_config"))==0)
            hRoot= HKEY_CURRENT_CONFIG;
#endif
#ifdef HKEY_DYN_DATA
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkdd"))==0)
            hRoot= HKEY_DYN_DATA;
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkey_dyn_data"))==0)
            hRoot= HKEY_DYN_DATA;
#endif
#ifdef HKEY_PERFORMANCE_TEXT
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkpt"))==0)
            hRoot= HKEY_PERFORMANCE_TEXT;
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkey_performance_text"))==0)
            hRoot= HKEY_PERFORMANCE_TEXT;
#endif
#ifdef HKEY_PERFORMANCE_NLSTEXT
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkpn"))==0)
            hRoot= HKEY_PERFORMANCE_NLSTEXT;
        else if (stringicompare(keyspec.substr(0,slashpos), std::string("hkey_performance_nlstext"))==0)
            hRoot= HKEY_PERFORMANCE_NLSTEXT;
#endif
        else if (isdigit(keyspec[0])) {
            hRoot= (HKEY)(0x80000000+strtoul(stringptr(keyspec), 0, 0));
        }
        else
            hRoot= HKEY_LOCAL_MACHINE;

        if (slashpos==keyspec.npos)
            path= "";
        else
            path= keyspec.substr(slashpos+1);

        //debug("FromKeySpec: %08lx - %s\n", hRoot, path.c_str());

        return RegistryPath(hRoot, path);
    }

    RegistryPath() : m_hRoot(0), m_path()
    {
    }

    RegistryPath(HKEY hRoot) : m_hRoot(hRoot), m_path()
    {
    }

    RegistryPath(const RegistryPath& hKey) : m_hRoot(hKey.GetRoot()), m_path(hKey.GetPath())
    {
    }
    RegistryPath& operator=(const RegistryPath& rk)
    {
        m_hRoot=rk.GetRoot();
        m_path= rk.GetPath();
        return *this;
    }
    RegistryPath(const RegistryPath& hRoot, const std::string& path) : m_hRoot(hRoot.GetRoot())
    {
        m_path= hRoot.GetPath();
        if (!m_path.empty())
            m_path += "\\";
        m_path += path;
    }
    ~RegistryPath()
    {
    }



    // returns the roothive of this key
    HKEY GetRoot() const
    {
        return m_hRoot;
    }

    // returns the full path to this key
    std::string GetPath() const
    {
        return m_path;
    }

    // returns the name of this key
    std::string GetName() const
    {
        size_t pos= m_path.find_last_of("\\/");
        if (pos!=m_path.npos)
            return m_path.substr(pos+1);
        else
            return m_path;
    }

    // returns the name of the hive of this key
    std::string GetRootName() const
    {
        if (m_hRoot==HKEY_CLASSES_ROOT       ) return "HKCR";
        if (m_hRoot==HKEY_CURRENT_USER       ) return "HKCU";
        if (m_hRoot==HKEY_LOCAL_MACHINE      ) return "HKLM";
        if (m_hRoot==HKEY_USERS              ) return "HKU";
#ifdef HKEY_PERFORMANCE_DATA
        if (m_hRoot==HKEY_PERFORMANCE_DATA   ) return "HKPD";
#endif
#ifdef HKEY_CURRENT_CONFIG
        if (m_hRoot==HKEY_CURRENT_CONFIG     ) return "HKCC";
#endif
#ifdef HKEY_DYN_DATA
        if (m_hRoot==HKEY_DYN_DATA           ) return "HKDD";
#endif
#ifdef HKEY_PERFORMANCE_TEXT
        if (m_hRoot==HKEY_PERFORMANCE_TEXT   ) return "HKPT";
#endif
#ifdef HKEY_PERFORMANCE_NLSTEXT
        if (m_hRoot==HKEY_PERFORMANCE_NLSTEXT) return "HKPN";
#endif

        return "unknown";
    }
    RegistryPath GetParent()
    {
        size_t pos= m_path.find_last_of("\\/");
        if (pos!=m_path.npos)
            return RegistryPath(m_hRoot, m_path.substr(0, pos));
        else
            return RegistryPath(m_hRoot);
    }
private:
        HKEY m_hRoot;
        std::string m_path;
};

typedef std::vector<RegistryPath> RegistryPathList;


#endif
