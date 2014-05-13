/*#include "skynet_timer.h"
#include "rwlock.h"

#include <windows.h>
#include <process.h>


static unsigned __stdcall _timer(void* p)
{
	struct monitor * m = (struct monitor*)p;
	for (;;) {
		skynet_updatetime();
		CHECK_ABORT
		//	wakeup(m,m->count-1);

		SleepEx(2, true);
		//usleep(2500);
	}
	// wakeup socket thread
	//skynet_socket_exit();
	// wakeup all worker thread
	//pthread_cond_broadcast(&m->cond);
}

int main(void)
{
	skynet_timer_init();

	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, _timer, NULL, 0, NULL);

	skynet_timeout(10, 100, 10);
	skynet_timeout(20, 300, 10);
	skynet_timeout(30, 500, 10);
	WaitForSingleObject(hThread, INFINITE);
	return 0;
}
*/

#include "skynet.h"

#include "skynet_imp.h"
#include "skynet_env.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
};


static int
optint(const char *key, int opt) {
	const char * str = skynet_getenv(key);
	if (str == NULL) {
		char tmp[20];
		sprintf(tmp,"%d",opt);
		skynet_setenv(key, tmp);
		return opt;
	}
	return strtol(str, NULL, 10);
}

/*
static int
optboolean(const char *key, int opt) {
	const char * str = skynet_getenv(key);
	if (str == NULL) {
		skynet_setenv(key, opt ? "true" : "false");
		return opt;
	}
	return strcmp(str,"true")==0;
}
*/
static const char *
optstring(const char *key,const char * opt) {
	const char * str = skynet_getenv(key);
	if (str == NULL) {
		if (opt) {
			skynet_setenv(key, opt);
			opt = skynet_getenv(key);
		}
		return opt;
	}
	return str;
}

static void
_init_env(lua_State *L) {
	lua_pushglobaltable(L);
	lua_pushnil(L);  /* first key */
	while (lua_next(L, -2) != 0) {
		int keyt = lua_type(L, -2);
		if (keyt != LUA_TSTRING) {
			fprintf(stderr, "Invalid config table\n");
			exit(1);
		}
		const char * key = lua_tostring(L,-2);
		if (lua_type(L,-1) == LUA_TBOOLEAN) {
			int b = lua_toboolean(L,-1);
			skynet_setenv(key,b ? "true" : "false" );
		} else {
			const char * value = lua_tostring(L,-1);
			if (value == NULL) {
				fprintf(stderr, "Invalid config table key = %s\n", key);
				exit(1);
			}
			skynet_setenv(key,value);
		}
		lua_pop(L,1);
	}
	lua_pop(L,1);
}


int
main(int argc, char *argv[]) {
	const char * config_file = "config";
	if (argc > 1) {
		config_file = argv[1];
	}
	skynet_env_init();

	struct skynet_config config;

	//struct lua_State *L = lua_newstate(skynet_lalloc, NULL);
	struct lua_State *L = luaL_newstate();
	luaL_openlibs(L);	// link lua lib
	lua_close(L);

	L = luaL_newstate();
	//luaL_openlibs(L);	// link lua lib
	int err = luaL_dofile(L, config_file);
	if (err) {
		fprintf(stderr,"%s\n",lua_tostring(L,-1));
		lua_close(L);
		return 1;
	} 
	_init_env(L);

#ifdef LUA_CACHELIB
  printf("Skynet lua code cache enable\n");
#endif

	const char *path = optstring("lua_path","./lualib/?.lua;./lualib/?/init.lua");
	//SetEnvironmentVariable("LUA_PATH", path);
	const char *cpath = optstring("lua_cpath","./luaclib/?.dll");
	//SetEnvironmentVariable("LUA_CPATH", cpath);
	optstring("luaservice","./service/?.lua");

	config.thread =  optint("thread",8);
	config.module_path = optstring("cpath","./cservice/?.dll");
	config.logger = optstring("logger",NULL);
	config.harbor = optint("harbor", 1);
	config.master = optstring("master","127.0.0.1:2012");
	config.start = optstring("start","main.lua");
	config.local = optstring("address","127.0.0.1:2525");
	config.standalone = optstring("standalone",NULL);

	lua_close(L);

	skynet_start(&config);

	printf("skynet exit\n");
	system("pause");

	return 0;
}

