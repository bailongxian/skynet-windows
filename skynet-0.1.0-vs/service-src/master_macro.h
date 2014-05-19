#ifndef _MASTER_MACRO_H_
#define _MASTER_MACRO_H_

#ifdef MASTER_EXPORTS
#define MASTER_API __declspec(dllexport)
#else
#define MASTER_API __declspec(dllimport)
#endif

#endif // _SKYNET_MACRO_H_
