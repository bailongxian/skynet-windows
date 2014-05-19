#include "socket_server.h"
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <process.h>

#pragma comment(lib, "skynet_core.lib")

struct socket_server* ss = NULL;

#define MAX_CLIENTS		10000
#define MAX_MSGS		100
#define DEFAULT_MSG_SIZE	4


int* clients;
char data[DEFAULT_MSG_SIZE];

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


static unsigned __stdcall test(void* p)
{
	int i;
	for (i=0; i<MAX_CLIENTS; ++i)
	{
		clients[i] = socket_server_connect(ss, 100+i, "127.0.0.1", 8888);
	}
	int j;
	for (i=0; i<MAX_MSGS; ++i)
	{
		for (j=0; j<MAX_CLIENTS; ++j)
		{
			socket_server_send(ss, clients[i], data, 5000);
			Sleep(1);
		}
	
		
	}

	return 0;
}


int main(void)
{
	clients = (int*)malloc(sizeof(int)*MAX_CLIENTS);
	ss = socket_server_create(socket_callback);


	HANDLE h = (HANDLE)_beginthreadex(NULL, 0, test, 0, 0, NULL);

	WaitForSingleObject(h, INFINITE);
	int i;
	for (i=0; i<MAX_CLIENTS; ++i)
	{
		socket_server_close(ss, 100+i, clients[i]);
	}

	socket_server_exit(ss);
	socket_server_wait_for_exit(ss);
	socket_server_release(ss);

	printf("test end\n");
	free(clients);
	system("pause");
	return 0;
}
