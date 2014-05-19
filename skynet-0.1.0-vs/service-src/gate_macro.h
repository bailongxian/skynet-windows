#ifndef _GATE_MACRO_H_
#define _GATE_MACRO_H_

#ifdef GATE_EXPORTS
#define GATE_API __declspec(dllexport)
#else
#define GATE_API __declspec(dllimport)
#endif

#endif // _GATE_MACRO_H_
