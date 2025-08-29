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
#pragma once
#if defined(__clang__) || defined(__GNUC__)
#define CPP_STANDARD __cplusplus
#elif defined(_MSC_VER)
#define CPP_STANDARD _MSVC_LANG
#endif
#if CPP_STANDARD >=  202002L 
#define HAS_CPP_VER 20
#elif CPP_STANDARD >= 201703L 
#define HAS_CPP_VER 17
#elif CPP_STANDARD >= 201402L
#define HAS_CPP_VER 14
#elif CPP_STANDARD >= 201103L
#define HAS_CPP_VER 11
#elif CPP_STANDARD >= 199711L
#define HAS_CPP_VER 09
#endif