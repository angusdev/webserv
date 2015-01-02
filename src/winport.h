#ifndef _WINPORT_H

#define _WINPORT_H

#if _WIN32

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>

typedef struct {
	int first_header;
	char server_root[MAX_PATH];
} thread_data_type;

int win_init();
int win_exit();
thread_data_type* win_tls_get_value();
int win_service_request(int conn);

#endif

#endif