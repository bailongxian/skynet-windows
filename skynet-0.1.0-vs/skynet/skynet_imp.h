#ifndef SKYNET_IMP_H
#define SKYNET_IMP_H

#include "skynet_macro.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct skynet_config {
	int thread;
	int harbor;
	const char * logger;
	const char * module_path;
	const char * master;
	const char * local;
	const char * start;
	const char * standalone;
};

SKYNET_API void skynet_start(struct skynet_config * config);

#ifdef __cplusplus
}
#endif

#endif
