#include "EvnVariables.h"
#include <stdlib.h>

static int setenv(const char* name, const char* value, int overwrite)
{
    int errcode = 0;
    if (!overwrite) 
    {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name);
        if (errcode || envsize) 
            return errcode;
    }
    return _putenv_s(name, value);
}

EvnVariables::EvnVariables() {

}

EvnVariables::~EvnVariables() {

}

bool EvnVariables::setValue(const string& strKey, const string& strValue)
{
    if (strKey.empty())
        return false;
    return _putenv_s(strKey.c_str(), strValue.c_str()) == 0;
}

string EvnVariables::getValue(const string& strKey)
{
    if (strKey.empty())
        return string();
    return getenv(strKey.c_str());
}

bool EvnVariables::delValue(const string& strKey)
{
    if (strKey.empty())
        return false;
    return setValue(strKey, "");
}