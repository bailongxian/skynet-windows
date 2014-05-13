#include "skynet.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "logger_macro.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct logger {
	FILE * handle;
	int close;
};

LOGGER_API struct logger *
logger_create(void) {
	struct logger * inst = (struct logger *)skynet_malloc(sizeof(*inst));
	inst->handle = NULL;
	inst->close = 0;
	return inst;
}

LOGGER_API void
logger_release(struct logger * inst) {
	if (inst->close) {
		fclose(inst->handle);
	}
	skynet_free(inst);
}

static int
_logger(struct skynet_context * context, void *ud, int type, int session, uint32_t source, const void * msg, size_t sz) {
	struct logger * inst = (struct logger *)ud;
	fprintf(inst->handle, "[:%x] ",source);
	fwrite(msg, sz , 1, inst->handle);
	fprintf(inst->handle, "\n");
	fflush(inst->handle);

	return 0;
}

LOGGER_API int
logger_init(struct logger * inst, struct skynet_context *ctx, const char * parm) {
	if (parm) {
		inst->handle = fopen(parm,"w");
		if (inst->handle == NULL) {
			return 1;
		}
		inst->close = 1;
	} else {
		inst->handle = stdout;
	}
	if (inst->handle) {
		skynet_callback(ctx, inst, _logger);
		skynet_command(ctx, "REG", ".logger");
		return 0;
	}
	return 1;
}

#ifdef __cplusplus
}
#endif

