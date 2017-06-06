#include <string>
#include "vectorutils.h"
#include "stringutils.h"
#include "regfileparser.h"


size_t findendquote(const std::string& str, size_t pos, char quotechar)
{
    bool bEscaped= false;
    while (pos<str.size())
    {
        if (bEscaped)
        {
            bEscaped= false;
        }
        else if (str[pos]==quotechar)
        {
            return pos+1;
        }
        else if (str[pos]=='\\')
        {
            bEscaped= true;
        }
        ++pos;
    }
    return std::string::npos;
}
bool ReadLine(FILE *f, std::string& line)
{
    line.erase();
    line.resize(65536);

    char *p= fgets(&line[0], line.size(), f);
    if (p==NULL) {
        line.erase();
        return false;
    }
    line.resize(stringlength(p));
    // todo: handle long lines ( > 65536 )
    //  - read until line has EOLN

    while (line.size() && (line[line.size()-1]=='\r' || line[line.size()-1]=='\n')) {
        line.resize(line.size()-1);
    }
    return true;
}
bool IsSetSpec(const std::string& spec)
{
    if (spec[0]!=':')
        return false;
    if (spec.find('=')==std::string::npos)
        return false;

    if (spec[1]=='\'' || spec[1]=='\"')
    {
        size_t endquote= findendquote(spec, 2, spec[1]);

        if (endquote==std::string::npos)
        {
            throw stringformat("IsSetSpec: missing endquote in '%hs'", spec.c_str());
        }
        //debug("isset: endq=%d size=%d c=%c\n", endquote, spec.size(), spec[endquote]);
        if (endquote==spec.size())
            return false;

        return (spec[endquote]=='=');
    }
    return true;
}
std::string GetNameFromSetSpec(const std::string& spec, size_t start)
{
    if (spec[start]=='\'' || spec[start]=='\"')
    {
        size_t endquote= findendquote(spec, start+1, spec[start]);
        //debug("GetNameFromSetSpec:%s\n", spec.substr(1, endquote-3).c_str());

        //todo: check for missing quote.
        return cstrunescape(spec.substr(start+1, endquote-start-2));
    }
    size_t eqpos= spec.find('=');

    //debug("GetNameFromSetSpec:%s\n", spec.substr(1,eqpos-1).c_str());
    return spec.substr(start,eqpos-start);
}

std::string GetValueSpecFromSetSpec(const std::string& spec, size_t start)
{
    if (spec[start]=='\'' || spec[start]=='\"')
    {
        size_t endquote= findendquote(spec, start+1, spec[start]);
        //debug("GetValueSpecFromSetSpec:%s\n", spec.substr(endquote).c_str());

        //todo: check for missing quote.
        size_t eqpos= spec.find('=', endquote);
        return spec.substr(eqpos+1);
    }
    size_t eqpos= spec.find('=');

    //debug("GetValueSpecFromSetSpec:%s\n", spec.substr(eqpos+1).c_str());
    return spec.substr(eqpos+1);
}

// removed utf-8 ByteOrderMarker
void stripbom(std::string& line)
{
    while (!line.empty()) {
        size_t ixbom= line.find("\xef\xbb\xbf");
        if (ixbom==line.npos)
            return;
        line.erase(ixbom, 3);
    }
}

#ifndef _WIN32_WCE
bool ProcessRegFile(const std::string& filename, regkeymaker& mk)
{
    FILE *f= fopen(filename.c_str(), "r");
    if (f==NULL) {
        perror(filename.c_str());
        return false;
    }

    std::string line;

// note: not yet handling linecontinuations
// note: not yet handling utf-16LE encoded files
    while (ReadLine(f, line)) {
        // remove trailing whitespace
        while (line.size() && isspace(line[line.size()-1])) {
            line.resize(line.size()-1);
        }

        stripbom(line);

        if (line.size()==0)
            continue;
        else if (line=="REGEDIT4")
            continue;
        else if (line[0]=='[' && line[line.size()-1]==']') {
            mk.newkey(RegistryPath::FromKeySpec(line.substr(1, line.size()-2)));
        }
        else if (line[0]==';') {
            // skip comments.
        }
        else {
            // remove leading whitespace
            line.erase(0, line.find_first_not_of(" \t"));

            std::string valuename= GetNameFromSetSpec(line, 0);

            // note: not handling comments here: comment ending in ',' will confuse us.

            // handle line continuation
            while (line.at(line.size()-1)=='\\' || line.at(line.size()-1)==',' ) {
                // strip  backslash + whitespace
                if (line.at(line.size()-1)=='\\') {
                    line.resize(line.size()-1);
                    while (line.size() && isspace(line[line.size()-1])) {
                        line.resize(line.size()-1);
                    }
                }
                std::string continuedline;
                if (!ReadLine(f, continuedline))
                    break;
                // trim whitespace
                continuedline.erase(0, continuedline.find_first_not_of(" \t"));
                while (continuedline.size() && isspace(continuedline[continuedline.size()-1])) {
                    continuedline.resize(continuedline.size()-1);
                }

                line += continuedline;
            }
            std::string valuespec= GetValueSpecFromSetSpec(line, 0);

            //debug("valline: %hs  = %hs\n", valuename.c_str(), valuespec.c_str());

            mk.setval(valuename, RegistryValue::FromValueSpec(valuespec));
        }
    }

    fclose(f);
    return true;
}
#endif

