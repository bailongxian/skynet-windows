#include "socket_server.h"
#include <assert.h>
#include <stdio.h>

#pragma comment(lib, "skynet_core.lib")

struct socket_server* ss = NULL;

int socket_callback(int type, struct socket_message* result)
{
	switch (type)
	{
	case SOCKET_EXIT:
		return 0;
	case SOCKET_DATA:
		printf("received %d bytes\n",result->ud);
		socket_server_send(ss, result->id, result->data, result->ud);
		break;
	case SOCKET_SENT:
		printf("sent %d bytes\n", result->ud);
		break;
	case SOCKET_CLOSE:
		printf("close(%lu) [id=%d]\n",result->opaque,result->id);
		break;
	case SOCKET_OPEN:
		printf("open(%lu) [id=%d] %s\n",result->opaque,result->id,result->data);
		break;
	case SOCKET_ERROR2:
		printf("error(%lu) [id=%d] [%s]\n",result->opaque,result->id,result->data);
		break;
	case SOCKET_ACCEPT:
		printf("accept(%lu) [id=%d %s] from [%d]\n",result->opaque, result->ud, result->data, result->id);
		break;
	default:
		//skynet_error(NULL, "Unknown socket message type %d.",type);
		return -1;
	}

	return 0;
}

int main(void)
{
	ss = socket_server_create(socket_callback,10);
	int l = socket_server_listen(ss, 100, "", 8888, 1024);
	socket_server_start(ss, 200, l);

	socket_server_wait_for_exit(ss);
	socket_server_release(ss);
	return 0;
}
