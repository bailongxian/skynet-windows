#include "socket_server.h"
#include <assert.h>
#include <stdio.h>
#include <malloc.h>

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
		break;
	case SOCKET_SENT:
		printf("sent %d bytes\n", result->ud);
		break;
	case SOCKET_CLOSE:
		printf("close(%lu) [id=%d]\n",result->opaque,result->id);
		break;
	case SOCKET_OPEN:
		printf("open(%lu) [id=%d]\n",result->opaque,result->id);
		break;
	case SOCKET_ERROR:
		printf("error(%lu) [id=%d]\n",result->opaque,result->id);
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
	ss = socket_server_create(socket_callback,1);

	int conn_id = socket_server_connect(ss, 100, "127.0.0.1", 8888);
	if (conn_id == -1)
	{
		socket_server_exit(ss);
		socket_server_wait_for_exit(ss);
		return 0;
	}

	char buf[1024] = {0};
	while(fgets(buf, sizeof(buf), stdin) != NULL)
	{
		if (strncmp(buf, "quit", 4) == 0)
			break;
		buf[strlen(buf)-1] = '\0';
		char* sendbuf = (char*)malloc(strlen(buf)+1);
		memcpy(sendbuf, buf, strlen(buf)+1);
		socket_server_send(ss, conn_id, sendbuf, strlen(sendbuf)+1);
		memset(buf, 0, sizeof(buf));
	}

	socket_server_close(ss, 100, conn_id);
	socket_server_exit(ss);
	socket_server_wait_for_exit(ss);
	socket_server_release(ss);

	return 0;
}
