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
// The body must be a statement:
#pragma once
#ifndef  __PIMPL_MACRO_H__
#define __PIMPL_MACRO_H__


#define PIMP_DO_PRAGMA(text)                      _Pragma(#text)

#if defined(_MSC_VER) && _MSC_VER >= 1500
#undef PIMPL_DO_PRAGMA                           /* not needed */
#define PIMPL_DECL_EXPORT                           __declspec(dllexport)
#define PIMPL_DECL_IMPORT                           __declspec(dllimport)
#define PIMPL_DECL_DEPRECATED                       __declspec(deprecated)
#define PIMPL_WARNING_PUSH                          __pragma(warning(push))
#define PIMPL_WARNING_POP                           __pragma(warning(pop))

# define PIMPL_DECL_OVERRIDE                            override
# define PIMPL_DECL_FINAL                               final

#define PIMPL_WARNING_DISABLE_GCC(text)

#elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 406)
#define PIMPL_DECL_EXPORT                           __attribute__((visibility("default")))  
#define PIMPL_DECL_IMPORT                           __attribute__((visibility("default")))
#  define PIMPL_DECL_DEPRECATED                     __attribute__ ((__deprecated__))
#define PIMPL_WARNING_PUSH                          PIMP_DO_PRAGMA(GCC diagnostic push)
#define PIMPL_WARNING_POP                           PIMP_DO_PRAGMA(GCC diagnostic pop)
#define PIMPL_WARNING_DISABLE_GCC(text)             PIMP_DO_PRAGMA`(GCC diagnostic ignored text)
#endif

#  define PIMPL_FUNC_INFO                           __PRETTY_FUNCTION__
#  define PIMPL_ALIGNOF(type)                       __alignof__(type)
#  define PIMPL_TYPEOF(expr)                        __typeof__(expr)
#  define PIMPL_DECL_ALIGN(n)                       __attribute__((__aligned__(n)))
#  define PIMPL_DECL_PACKED                         __attribute__((__packed__))
#  define PIMPL_DECL_UNUSED                         __attribute__((__unused__))
#  define PIMPL_LIKELY(expr)                        __builtin_expect(!!(expr), true)
#  define PIMPL_UNLIKELY(expr)                      __builtin_expect(!!(expr), false)
#  define PIMPL_NORETURN                            __attribute__((__noreturn__))
#  define PIMPL_REQUIRED_RESULT                     __attribute__ ((__warn_unused_result__))
#  define PIMPL_DECL_PURE_FUNCTION                  __attribute__((pure))
#  define PIMPL_DECL_CONST_FUNCTION                 __attribute__((const))




#define PIMPL_CAST_IGNORE_ALIGN(body) PIMPL_WARNING_PUSH PIMPL_WARNING_DISABLE_GCC("-Wcast-align") body PIMPL_WARNING_POP
template <typename T> inline T *getPtrHelper(T *ptr) { return ptr; }


#define PIMPL_CLAIM_PRIVATE(ClassName) class ClassName##Private;

#define PIMPL_DECLARE_PRIVATE(ClassName) \
    inline ClassName##Private* d_func() \
    { PIMPL_CAST_IGNORE_ALIGN(return reinterpret_cast<ClassName##Private *>(getPtrHelper(d_ptr));) } \
    inline const ClassName##Private* d_func() const \
    { PIMPL_CAST_IGNORE_ALIGN(return reinterpret_cast<const ClassName##Private *>(getPtrHelper(d_ptr));) } \
    friend class ClassName##Private;\
private:\
	ClassName##Private *const d_ptr;

#define PIMPL_CLAIM_PUBLIC(ClassName) class ClassName;
#define PIMPL_DECLARE_PUBLIC(ClassName)                                    \
    inline ClassName* q_func() { return static_cast<ClassName *>(q_ptr); } \
    inline const ClassName* q_func() const { return static_cast<const ClassName *>(q_ptr); } \
    friend class ClassName;\
private:\
	ClassName *const q_ptr;	

#define P_D(ClassName) ClassName##Private * const d = d_func()
#define P_Q(ClassName) ClassName * const q = q_func()
#endif // ! __PIMPL_MACRO_H__