#ifndef SKYNET_H
#define SKYNET_H

#include "skynet_macro.h"
#ifdef __cplusplus
extern "C"
{
#endif

//#include "skynet_malloc.h"

#include <stddef.h>
#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <process.h>

#define PTYPE_TEXT 0
#define PTYPE_RESPONSE 1
#define PTYPE_MULTICAST_DEPRECATED 2
#define PTYPE_CLIENT 3
#define PTYPE_SYSTEM 4
#define PTYPE_HARBOR 5
#define PTYPE_SOCKET 6
// read lualib/skynet.lua lualib/simplemonitor.lua
#define PTYPE_RESERVED_ERROR 7	
// read lualib/skynet.lua lualib/mqueue.lua
#define PTYPE_RESERVED_QUEUE 8
#define PTYPE_RESERVED_DEBUG 9
#define PTYPE_RESERVED_LUA 10

#define PTYPE_TAG_DONTCOPY 0x10000
#define PTYPE_TAG_ALLOCSESSION 0x20000

struct skynet_context;

SKYNET_API void skynet_error(struct skynet_context * context, const char *msg, ...);
SKYNET_API const char * skynet_command(struct skynet_context * context, const char * cmd , const char * parm);
SKYNET_API uint32_t skynet_queryname(struct skynet_context * context, const char * name);
SKYNET_API int skynet_send(struct skynet_context * context, uint32_t source, uint32_t destination , int type, int session, void * msg, size_t sz);
SKYNET_API int skynet_sendname(struct skynet_context * context, const char * destination , int type, int session, void * msg, size_t sz);

SKYNET_API int skynet_isremote(struct skynet_context *, uint32_t handle, int * harbor);

SKYNET_API typedef int (*skynet_cb)(struct skynet_context * context, void *ud, int type, int session, uint32_t source , const void * msg, size_t sz);
SKYNET_API void skynet_callback(struct skynet_context * context, void *ud, skynet_cb cb);

SKYNET_API uint32_t skynet_current_handle(void);

#define skynet_malloc malloc
#define skynet_free free
#define skynet_strdup _strdup

#ifdef __cplusplus
}
#endif



static char *
strsep (char **stringp, const char *delim)
{
  char *begin, *end;

  begin = *stringp;
  if (begin == NULL)
    return NULL;

  /** A frequent case is when the delimiter string contains only one
     character.  Here we don't need to call the expensive `strpbrk'
     function and instead work using `strchr'.  */
  if (delim[0] == '\0' || delim[1] == '\0')
    {
      char ch = delim[0];

      if (ch == '\0')
	end = NULL;
      else
	{
	  if (*begin == ch)
	    end = begin;
	  else if (*begin == '\0')
	    end = NULL;
	  else
	    end = strchr (begin + 1, ch);
	}
    }
  else
    /** Find the end of the token.  */
    end = strpbrk (begin, delim);

  if (end)
    {
      /** Terminate the token and set *STRINGP past NUL character.  */
      *end++ = '\0';
      *stringp = end;
    }
  else
    /** No more delimiters; this is the last token.  */
    *stringp = NULL;

  return begin;
}

#endif
