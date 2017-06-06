#ifndef _REG_FILEPARSER_H_
#define _REG_FILEPARSER_H_
#include <string>
#include <vectorutils.h>

#include "regvalue.h"
#include "regpath.h"

size_t findendquote(const std::string& str, size_t pos, char quotechar);
bool IsSetSpec(const std::string& spec);
std::string GetNameFromSetSpec(const std::string& spec, size_t start);
std::string GetValueSpecFromSetSpec(const std::string& spec, size_t start);

struct regkeymaker {
    virtual void newkey(const RegistryPath& path)= 0;
    virtual void setval(const std::string& name, const RegistryValue& value)= 0;
};
bool ProcessRegFile(const std::string& filename, regkeymaker& mk);
#endif
