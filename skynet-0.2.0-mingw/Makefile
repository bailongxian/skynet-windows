include platform.mk

LUA_CLIB_PATH ?= luaclib
CSERVICE_PATH ?= cservice

SKYNET_BUILD_PATH ?= .

CFLAGS = -g -O2 -Wall -I$(LUA_INC) $(MYCFLAGS)

# lua

LUA_STATICLIB := 3rd/lua/liblua.a
LUA_LIB ?= $(LUA_STATICLIB)
LUA_INC ?= 3rd/lua

$(LUA_STATICLIB) :
	cd 3rd/lua && $(MAKE) CC=$(CC) $(PLAT) && cp lua52.dll ../../

# jemalloc 

JEMALLOC_STATICLIB := 3rd/jemalloc/lib/libjemalloc_pic.a
JEMALLOC_INC := 3rd/jemalloc/include/jemalloc

all : jemalloc
	
.PHONY : jemalloc

MALLOC_STATICLIB := $(JEMALLOC_STATICLIB)

$(JEMALLOC_STATICLIB) : 3rd/jemalloc/Makefile
	cd 3rd/jemalloc && $(MAKE) CC=$(CC) 

3rd/jemalloc/autogen.sh :
	git submodule update --init

3rd/jemalloc/Makefile : | 3rd/jemalloc/autogen.sh
	cd 3rd/jemalloc && ./autogen.sh --with-jemalloc-prefix=je_ --disable-valgrind

jemalloc : $(MALLOC_STATICLIB)

# skynet

CSERVICE = snlua logger gate master harbor
LUA_CLIB = skynet socketdriver int64 bson mongo md5 netpack cjson clientsocket memory profile multicast

SKYNET_SRC = skynet_handle.c skynet_module.c skynet_mq.c \
  skynet_server.c skynet_start.c skynet_timer.c skynet_error.c \
  skynet_harbor.c skynet_env.c skynet_monitor.c skynet_socket.c socket_server.c \
  malloc_hook.c dlfcn.c socket_buffer_pool.c socket_helper.c

all : \
  $(SKYNET_BUILD_PATH)/skynet.exe skynet_core.dll \
  $(foreach v, $(CSERVICE), $(CSERVICE_PATH)/$(v).dll) \
  $(foreach v, $(LUA_CLIB), $(LUA_CLIB_PATH)/$(v).dll) 

$(SKYNET_BUILD_PATH)/skynet.exe : skynet-src/skynet_main.c $(LUA_LIB) $(SKYNET_BUILD_PATH)/skynet_core.dll
	$(CC) $(CFLAGS) -o $@ $< -Iskynet-src -I$(JEMALLOC_INC) $(SKYNET_LIBS) $(SKYNET_DEFINES) -L$(SKYNET_BUILD_PATH) -llua52  -lskynet_core

$(SKYNET_BUILD_PATH)/skynet_core.dll : $(foreach v, $(SKYNET_SRC), skynet-src/$(v))
	$(CC) $(CFLAGS) $(SHARED) -o $@ $^ -Iskynet-src -I$(JEMALLOC_INC) $(SKYNET_LIBS) $(SKYNET_DEFINES) -L$(SKYNET_BUILD_PATH) -llua52 -lws2_32
	
$(LUA_CLIB_PATH) :
	mkdir $(LUA_CLIB_PATH)

$(CSERVICE_PATH) :
	mkdir $(CSERVICE_PATH)

define CSERVICE_TEMP
  $$(CSERVICE_PATH)/$(1).dll : service-src/service_$(1).c | $$(CSERVICE_PATH)
	$(CC) $$(CFLAGS) $$(SHARED) $$< -o $$@ -Iskynet-src -L$(SKYNET_BUILD_PATH) -llua52  -lskynet_core
endef

$(foreach v, $(CSERVICE), $(eval $(call CSERVICE_TEMP,$(v))))

$(LUA_CLIB_PATH)/skynet.dll : lualib-src/lua-skynet.c lualib-src/lua-seri.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Iskynet-src -Iservice-src -Ilualib-src -L$(SKYNET_BUILD_PATH) -llua52 -lskynet_core

$(LUA_CLIB_PATH)/socketdriver.dll : lualib-src/lua-socket.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Iskynet-src -Iservice-src -L$(SKYNET_BUILD_PATH) -llua52 -lskynet_core

$(LUA_CLIB_PATH)/int64.dll : 3rd/lua-int64/int64.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) -I$(LUA_INC) $^ -o $@ -L$(SKYNET_BUILD_PATH) -llua52

$(LUA_CLIB_PATH)/bson.dll : lualib-src/lua-bson.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Iskynet-src  -L$(SKYNET_BUILD_PATH) -llua52 -lws2_32

$(LUA_CLIB_PATH)/mongo.dll : lualib-src/lua-mongo.c | $(LUA_CLIB_PATH) 
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Iskynet-src -L$(SKYNET_BUILD_PATH) -llua52

$(LUA_CLIB_PATH)/md5.dll : 3rd/lua-md5/md5.c 3rd/lua-md5/md5lib.c 3rd/lua-md5/compat-5.2.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) -I3rd/lua-md5 $^ -o $@ -L$(SKYNET_BUILD_PATH) -llua52

$(LUA_CLIB_PATH)/netpack.dll : lualib-src/lua-netpack.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -Iskynet-src -o $@ -L$(SKYNET_BUILD_PATH) -llua52

$(LUA_CLIB_PATH)/cjson.dll : 3rd/lua-cjson/lua_cjson.c 3rd/lua-cjson/fpconv.c 3rd/lua-cjson/strbuf.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) -I./skynet-src  -I$(LUA_INC) $^ -o $@ -L$(SKYNET_BUILD_PATH) -llua52

$(LUA_CLIB_PATH)/clientsocket.dll : lualib-src/lua-clientsocket.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -lpthread -L$(SKYNET_BUILD_PATH) -llua52 -lws2_32

$(LUA_CLIB_PATH)/memory.dll : lualib-src/lua-memory.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) -Iskynet-src $^ -o $@ -L$(SKYNET_BUILD_PATH) -llua52 -L. -lskynet_core

$(LUA_CLIB_PATH)/profile.dll : lualib-src/lua-profile.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -L$(SKYNET_BUILD_PATH) -llua52

$(LUA_CLIB_PATH)/multicast.dll : lualib-src/lua-multicast.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) -Iskynet-src $^ -o $@ -L$(SKYNET_BUILD_PATH) -llua52

clean:
#	del /Q luaclib\* cservice\* $(SKYNET_BUILD_PATH)\skynet.exe $(SKYNET_BUILD_PATH)\skynet_core.dll
	rm -f luaclib/* cservice/* $(SKYNET_BUILD_PATH)/skynet.exe $(SKYNET_BUILD_PATH)/skynet_core.dll

cleanall: clean
	cd 3rd/lua && $(MAKE) clean
	cd 3rd/lua-cjson && $(MAKE) clean
	cd 3rd/jemalloc && $(MAKE) clean
	rm -f $(LUA_STATICLIB)

