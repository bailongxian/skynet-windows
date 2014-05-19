#include "skynet.h"

#include "skynet_module.h"

#include <assert.h>
#include <string.h>
//#include <dlfcn.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_MODULE_TYPE 32

struct modules {
	int count;
	volatile long lock;
	const char * path;
	struct skynet_module m[MAX_MODULE_TYPE];
};

static struct modules * M = NULL;

static void *
_try_open(struct modules *m, const char * name) {
	const char *l;
	const char * path = m->path;
	size_t path_size = strlen(path);
	size_t name_size = strlen(name);

	int sz = path_size + name_size;
	//search path
	void * dl = NULL;
	//char tmp[sz];
	char tmp[256];
	do
	{
		memset(tmp,0,sz);
		while (*path == ';') path++;
		if (*path == '\0') break;
		l = strchr(path, ';');
		if (l == NULL) l = path + strlen(path);
		int len = l - path;
		int i;
		for (i=0;path[i]!='?' && i < len ;i++) {
			tmp[i] = path[i];
		}
		memcpy(tmp+i,name,name_size);
		if (path[i] == '?') {
			strncpy_s(tmp+i+name_size,256-i-name_size, path+i+1,len - i - 1);
		} else {
			fprintf(stderr,"Invalid C service path\n");
			exit(1);
		}
		//dl = dlopen(tmp, RTLD_NOW | RTLD_GLOBAL);
		dl =  ::LoadLibrary(tmp);
		path = l;
	}while(dl == NULL);

	if (dl == NULL) {
		//fprintf(stderr, "try open %s failed : %s\n",name,dlerror());
		fprintf(stderr, "try open %s failed : %d\n",name, GetLastError());
	}

	return dl;
}

static struct skynet_module * 
_query(const char * name) {
	int i;
	for (i=0;i<M->count;i++) {
		if (strcmp(M->m[i].name,name)==0) {
			return &M->m[i];
		}
	}
	return NULL;
}

static int
_open_sym(struct skynet_module *mod) {
	size_t name_size = strlen(mod->name);
	//char tmp[name_size + 9]; // create/init/release , longest name is release (7)
	char tmp[256];
	memcpy(tmp, mod->name, name_size);
	strcpy_s(tmp+name_size, strlen("_create")+1, "_create");
	//mod->create = dlsym(mod->module, tmp);
	mod->create = (skynet_dl_create)GetProcAddress((HMODULE)mod->module, tmp);
	int n = GetLastError();
	strcpy_s(tmp+name_size, strlen("_init")+1, "_init");
	//mod->init = dlsym(mod->module, tmp);
	mod->init = (skynet_dl_init)GetProcAddress((HMODULE)mod->module, tmp);
	strcpy_s(tmp+name_size, strlen("_release")+1, "_release");
	//mod->release = dlsym(mod->module, tmp);
	mod->release = (skynet_dl_release)GetProcAddress((HMODULE)mod->module, tmp);

	return mod->init == NULL;
}

struct skynet_module * 
skynet_module_query(const char * name) {
	struct skynet_module * result = _query(name);
	if (result)
		return result;

	//while(__sync_lock_test_and_set(&M->lock,1)) {}
	while(InterlockedCompareExchange(&M->lock, 1, 1)) {}

	result = _query(name); // double check

	if (result == NULL && M->count < MAX_MODULE_TYPE) {
		int index = M->count;
		void * dl = _try_open(M,name);
		if (dl) {
			M->m[index].name = name;
			M->m[index].module = dl;

			if (_open_sym(&M->m[index]) == 0) {
				M->m[index].name = skynet_strdup(name);
				M->count ++;
				result = &M->m[index];
			}
		}
	}

	//__sync_lock_release(&M->lock);
	InterlockedExchange(&M->lock, 0);

	return result;
}

void 
skynet_module_insert(struct skynet_module *mod) {
	//while(__sync_lock_test_and_set(&M->lock,1)) {}
	while(InterlockedCompareExchange(&M->lock, 1, 1)) {}

	struct skynet_module * m = _query(mod->name);
	assert(m == NULL && M->count < MAX_MODULE_TYPE);
	int index = M->count;
	M->m[index] = *mod;
	++M->count;
	//__sync_lock_release(&M->lock);
	InterlockedExchange(&M->lock, 0);
}

void * 
skynet_module_instance_create(struct skynet_module *m) {
	if (m->create) {
		return m->create();
	} else {
		return (void *)(intptr_t)(~0);
	}
}

int
skynet_module_instance_init(struct skynet_module *m, void * inst, struct skynet_context *ctx, const char * parm) {
	return m->init(inst, ctx, parm);
}

void 
skynet_module_instance_release(struct skynet_module *m, void *inst) {
	if (m->release) {
		m->release(inst);
	}
}

void 
skynet_module_init(const char *path) {
	struct modules *m = (struct modules*)skynet_malloc(sizeof(*m));
	m->count = 0;
	m->path = skynet_strdup(path);
	m->lock = 0;

	M = m;
}
