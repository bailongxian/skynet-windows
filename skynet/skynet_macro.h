#ifndef _SKYNET_MACRO_H_
#define _SKYNET_MACRO_H_

#ifdef SKYNET_EXPORTS
#define SKYNET_API __declspec(dllexport)
#else
#define SKYNET_API __declspec(dllimport)
#endif

#endif // _SKYNET_MACRO_H_
