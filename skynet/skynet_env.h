#ifndef SKYNET_ENV_H
#define SKYNET_ENV_H

#include "skynet_macro.h"

#ifdef __cplusplus
extern "C"
{
#endif

SKYNET_API const char * skynet_getenv(const char *key);
SKYNET_API void skynet_setenv(const char *key, const char *value);

SKYNET_API void skynet_env_init();

#ifdef __cplusplus
}
#endif

#endif
