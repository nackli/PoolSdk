/***************************************************************************************************************************************************/
/*
* @Author: Nack Li
* @version 1.0
* @copyright 2025 nackli. All rights reserved.
* @License: MIT (https://opensource.org/licenses/MIT).
* @Date: 2025-08-29
* @LastEditTime: 2025-08-29
*/
/***************************************************************************************************************************************************/
#include "EvnVariables.h"
#include <stdlib.h>
using namespace std;
#ifdef _WIN32   
static int OnSetEnv(const char* name, const char* value, int overwrite)
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
#else
static int OnSetEnv(const char* name, const char* value, int overwrite)
{
 
    if (!overwrite) 
    {
        char *pValue = getenv(name);
        if (pValue) 
            return -1;
    }
    char szEnvValue[256] = {0};
    sprintf(szEnvValue,"%s=%s",name,value);
    return putenv(szEnvValue);
}
#endif 

EvnVariables::EvnVariables() {

}

EvnVariables::~EvnVariables() {

}

bool EvnVariables::setValue(const string& strKey, const string& strValue)
{
    if (strKey.empty())
        return false;         
    return OnSetEnv(strKey.c_str(), strValue.c_str(),false) == 0;
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