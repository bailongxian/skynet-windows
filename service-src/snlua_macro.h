#ifndef _SNLUA_MACRO_H_
#define _SNLUA_MACRO_H_

#ifdef SNLUA_EXPORTS
#define SNLUA_API __declspec(dllexport)
#else
#define SNLUA_API __declspec(dllimport)
#endif

#endif // _SKYNET_MACRO_H_
