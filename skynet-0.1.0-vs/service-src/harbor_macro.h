#ifndef _HARBOR_MACRO_H_
#define _HARBOR_MACRO_H_

#ifdef HARBOR_EXPORTS
#define HARBOR_API __declspec(dllexport)
#else
#define HARBOR_API __declspec(dllimport)
#endif

#endif // _HARBOR_MACRO_H_
