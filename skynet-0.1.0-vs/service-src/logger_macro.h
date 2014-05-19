#ifndef _LOGGER_MACRO_H_
#define _LOGGER_MACRO_H_

#ifdef LOGGER_EXPORTS
#define LOGGER_API __declspec(dllexport)
#else
#define LOGGER_API __declspec(dllimport)
#endif

#endif // _LOGGER_MACRO_H_
