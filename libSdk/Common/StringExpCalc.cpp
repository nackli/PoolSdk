#include<iostream>
#include<sstream>
#include<stack>
#include<vector>
#include<map>
#include <algorithm>
#include <functional>
#include "StringExpCalc.h"
#include <cmath>
#include <cctype>
using namespace std;
/********************************************************************************************************************/
/********************************************************************************************************************/
static map<string, function<double(double, double)>>  g_mapDoubleParam =
{
    {"+",           [](double leftNum, double rightNum)->double { return leftNum + rightNum; } },
    {"-",           [](double leftNum, double rightNum)->double { return leftNum - rightNum; } },
    {"*",           [](double leftNum, double rightNum)->double { return leftNum * rightNum; } },
    {"/",           [](double leftNum, double rightNum)->double { return leftNum / rightNum; } },
    {"%",           [](double leftNum, double rightNum)->double { return (int)leftNum % (int)rightNum; } },
    {"&",           [](double leftNum, double rightNum)->double { return (int)leftNum & (int)rightNum; } },
    {"^",           [](double leftNum, double rightNum)->double { return (int)leftNum ^ (int)rightNum; } },
    {"|",           [](double leftNum, double rightNum)->double { return (int)leftNum | (int)rightNum; } },
    {"pow",         [](double leftNum, double rightNum)->double { return pow(leftNum ,rightNum); } },//power
    {"remainder",   [](double leftNum, double rightNum)->double { return remainder(leftNum , rightNum); } },//remainder
    {"nexttoward",  [](double leftNum, double rightNum)->double { return nexttoward(leftNum , rightNum); } },//nexttoward
    {"nextafter",   [](double leftNum, double rightNum)->double { return nextafter(leftNum , rightNum); } },//nextafter
    {"hypot",       [](double leftNum, double rightNum)->double { return hypot(leftNum , rightNum); } },//hypot
    {"max",         [](double leftNum, double rightNum)->double { return fmax(leftNum , rightNum); } },//max
    {"min",         [](double leftNum, double rightNum)->double { return fmin(leftNum , rightNum); } },//min
    {"ldexp",       [](double leftNum, double rightNum)->double { return ldexp(leftNum , (int)rightNum); } },//ldexp 
    {"fmod",        [](double leftNum, double rightNum)->double { return fmod(leftNum , rightNum); } },//fmod 
    {"fdim",        [](double leftNum, double rightNum)->double { return fdim(leftNum , rightNum); } },//fdim 
    {"scalbn",      [](double leftNum, double rightNum)->double { return scalbn(leftNum , (int)rightNum); } },//scalbn 
    {"atan2",       [](double dataNum, double rightNum)->double { return atan2(dataNum,rightNum); } },//atan2
#ifdef _WIN32     
    {"jn",       [](double dataNum, double rightNum)->double { return _jn((int)dataNum,rightNum); } },//atan2
    {"yn",       [](double dataNum, double rightNum)->double { return _yn((int)dataNum,rightNum); } },//atan2
#endif
};

static map<string, function<double(double)>> g_mapSingleParam =
{
   {"sin",      [](double dataNum)->double { return sin(dataNum); } },//sin
   {"cos",      [](double dataNum)->double { return cos(dataNum); } },//cos
   {"cosh",     [](double dataNum)->double { return cosh(dataNum ); } },//cosh
   {"sinh",     [](double dataNum)->double { return sinh(dataNum); } },//sinh
   {"tan",      [](double dataNum)->double { return tan(dataNum); } },//tanf
   {"tanh",     [](double dataNum)->double { return tanh(dataNum); } },//tanf
   {"asin",     [](double dataNum)->double { return asin(dataNum); } },//asin
   {"acos",     [](double dataNum)->double { return acos(dataNum); } },//acos
   {"atan",     [](double dataNum)->double { return atan(dataNum); } },//atan
   {"acosh",    [](double dataNum)->double { return acosh(dataNum); } },//acosh
   {"asinh",    [](double dataNum)->double { return asinh(dataNum); } },//asinh
   {"atanh",    [](double dataNum)->double { return acosh(dataNum); } },//atanh
   {"fabs",     [](double dataNum)->double { return fabs(dataNum); } },//fabs
   {"abs",      [](double dataNum)->double { return abs(dataNum); } },//fabs
   {"sqrt",     [](double dataNum)->double { return sqrt(dataNum); } },//sqrt
   {"exp",      [](double dataNum)->double { return exp(dataNum); } },//exp
   {"exp2",     [](double dataNum)->double { return exp2(dataNum); } },//exp2
   {"log",      [](double dataNum)->double { return log(dataNum); } },//log
   {"log10",    [](double dataNum)->double { return log10(dataNum); } },//log10
   {"log1p",    [](double dataNum)->double { return log1p(dataNum); } },//log1p
   {"log2",     [](double dataNum)->double { return log2(dataNum); } },//log2
   {"logb",     [](double dataNum)->double { return logb(dataNum); } },//logb
   {"cbrt",     [](double dataNum)->double { return cbrt(dataNum); } },//cbrt
   {"expm1",    [](double dataNum)->double { return expm1(dataNum); }},//expm1
   {"floor",    [](double dataNum)->double { return floor(dataNum); } },//floor
   {"round",    [](double dataNum)->double { return round(dataNum); } },//round
   {"ceil",     [](double dataNum)->double { return ceil(dataNum); } },//ceil
   {"rint",     [](double dataNum)->double { return rint(dataNum); } },//rint
   {"tgamma",   [](double dataNum)->double { return tgamma(dataNum); } },//tgamma
   {"trunc",    [](double dataNum)->double { return trunc(dataNum); } },//trunc
   {"erf",      [](double dataNum)->double { return erf(dataNum); } },//erf
#ifdef _WIN32 
   {"j0",       [](double dataNum)->double { return _j0(dataNum); } },//erf
   {"j1",       [](double dataNum)->double { return _j1(dataNum); } },//erf
   {"y0",       [](double dataNum)->double { return _y0(dataNum); } },//erf
   {"y1",       [](double dataNum)->double { return _y1(dataNum); } },//erf
#endif
};

static map<string, int> g_mapOperInfo;
/********************************************************************************************************************/

//is nega
static bool isNega(const std::string& s, int i)
{
    return (s[i] == '-' && s.length() > 1) && (i == 0 || s[i - 1] == '(');
}

static double OnCalcExp(vector<string>& tokens) 
{
    if (tokens.empty())
        return 0;

    stack<double> numStack;
    //scalc
    for (size_t pos = 0; pos != tokens.size(); pos++)
    {
        if (tokens[pos].empty())
            return 0;
        if (isNega(tokens[pos],0) || isdigit(tokens[pos][0]))//token is digit
            numStack.push(stod(tokens[pos]));
        else//clac 
        { 
            if (numStack.empty())
                return 0;
           
            double rightNum = numStack.top();
            numStack.pop();

            double dResultVal = rightNum;

            string oper = tokens[pos];   //opertion
            if (!oper.empty())
            {
                auto funOper = g_mapDoubleParam[oper];
                if (funOper)
                {
                    if (numStack.empty())
                        return 0;
                    double leftNum = numStack.top();
                    numStack.pop();
                    dResultVal = funOper(leftNum, rightNum);
                } 
                else
                {
                    auto funOper = g_mapSingleParam[oper];
                    if(funOper)
                        dResultVal = funOper(rightNum);
                }                                          
            }     
            //push result from stack
            numStack.push(dResultVal);
        }
    }
    //pop result from stack
    return numStack.top();
}

static void OnInitOperPriorityMap()
{
    g_mapOperInfo["("] = 20;
    g_mapOperInfo[")"] = 20;
    g_mapOperInfo["*"] = 16;
    g_mapOperInfo["/"] = 16;
    g_mapOperInfo["%"] = 16;
    g_mapOperInfo["+"] = 15;
    g_mapOperInfo["-"] = 15;
    g_mapOperInfo["&"] = 10;
    g_mapOperInfo["^"] = 9;
    g_mapOperInfo["|"] = 8;
    g_mapOperInfo[","] = 0;
    for (auto it = g_mapDoubleParam.begin(); it != g_mapDoubleParam.end(); it++)
        g_mapOperInfo.insert(make_pair<string, int>(string(it->first), 18));

    for (auto it = g_mapSingleParam.begin(); it != g_mapSingleParam.end(); it++)
        g_mapOperInfo.insert(make_pair<string, int>(string(it->first), 18));

}

static int OnGetLagerRand(string strFirst, string strSec)
{
    if (g_mapOperInfo.empty())
        return -1;
    return g_mapOperInfo[strFirst] - g_mapOperInfo[strSec];
}

//remove space
static int OnRemoveSpace(std::string& strExp)
{
    int iBegin = 0;
    int iEnd = 0;
    int iAlphaNum = 0;

    while (iEnd != strExp.size())
    {
        if (strExp[iEnd] != ' ')
        {
            if (strExp[iEnd] != '(' && strExp[iEnd] != ')')
                iAlphaNum++;
            if(iEnd >= 1 && (strExp[iEnd] == 'x' || strExp[iEnd] == 'X') && strExp[iEnd - 1] == '0')
                strExp[iBegin] = '#';
            else
                strExp[iBegin] = strExp[iEnd];
            iBegin++;
            iEnd++;
        }
        else
            iEnd++;
    }
    strExp.resize(iBegin);
    std::transform(strExp.begin(), strExp.end(), strExp.begin(), ::tolower);
    return iAlphaNum;
}

static bool IsOper(string strOper)
{
    if(g_mapOperInfo.find(strOper)!= g_mapOperInfo.end())
        return true;
    return false;
}

static void OnExchargeCmp(stack<string>& sOper, stack<string>& sData, string  strCurOpt = {})
{
    while (!sOper.empty())
    {
        if (!strCurOpt.empty())
        {
            if (OnGetLagerRand(strCurOpt, sOper.top()) <= 0 && sOper.top() != "(")
            {
                sData.push(sOper.top());
                sOper.pop();
            }
            else
                break;
        }
        else
        {
            sData.push(sOper.top());
            sOper.pop();
        }
    }
}

static void OnExchargeStack(stack<string>& stackOper, stack<string>& stackData)
{
    string str = stackOper.top();
    while (!stackOper.empty() && stackOper.top() != "(")
    {
        stackData.push(stackOper.top());
        stackOper.pop();
    }
    if (stackOper.empty())
        return;
    stackOper.pop();
}

static void OnStack2Vector(stack<string> stackRes, vector<string>& tokens)
{
    if (stackRes.empty())
        return;
    tokens.clear();
    while (!stackRes.empty())
    {
        string val = stackRes.top();
        stackRes.pop();
        tokens.emplace_back(val);
    }
    std::reverse(tokens.begin(), tokens.end());
}

static void OnParse2Polish(string expression ,vector<string>& tokens)
{
    stack<string> stackData;
    stack<string> stackOper;
    
    for (size_t i = 0; i < expression.size(); i++)
    {
        char szChar = expression[i];
        //int ret = IsOper(string(1,szChar));
        if (isdigit(szChar) || isNega(expression, i) ||  szChar == '.')
        {
            string strTemp;
            while (isdigit(szChar) || szChar == '.'|| szChar == '-')
            {
                strTemp += szChar;
                if ((i + 1) < expression.size() && (isdigit(expression[i + 1]) || expression[i + 1] == '.'))
                {
                    i++;
                    szChar = expression[i];
                }
                else
                    break;
            }
            if(!strTemp.empty())
                stackData.push(strTemp); 
        }
        else
        {
            string strOperType;
            while (isalnum(szChar))//req string
            {
                strOperType += szChar;
                if ((i + 1) < expression.size() && isalnum(expression[i + 1]))
                {
                    i++;
                    szChar = expression[i];
                }
                else  
                    break;                    
            }
            if (strOperType.empty())
                strOperType = szChar;

            if (!IsOper(strOperType))
                continue;

            if (stackOper.empty() || (stackOper.top() == "(" 
                && strOperType != ")" && strOperType != ","))
                stackOper.push(strOperType);
            else if (strOperType == ")")
                OnExchargeStack(stackOper, stackData);
            else if (OnGetLagerRand(strOperType, stackOper.top())> 0)
                stackOper.push(strOperType);
    /*        else if (OnGetLagerRand(szChar, stackOper.top()[0]) == 0)
            {
                OnExchargeCmp(stackOper, stackData, szChar);
                stackOper.push(string(1, szChar));
            }   */
            else
            {   
                OnExchargeCmp(stackOper, stackData, strOperType);
                stackOper.push(strOperType);   // 栈为空，直接压入s2              
            }
        }
    }
    OnExchargeCmp(stackOper, stackData);
    OnStack2Vector(stackData, tokens);
}
/********************************************************************************************************************/

StringExpCalc::StringExpCalc()
{
    OnInitOperPriorityMap();
}

StringExpCalc::~StringExpCalc()
{
}

double StringExpCalc::CalcExpValue(string strExpInt)
{
    std::vector<std::string> tokens;
    OnRemoveSpace(strExpInt);
    OnParse2Polish(strExpInt, tokens);
    return OnCalcExp(tokens);  //复用之前写的代码
}
