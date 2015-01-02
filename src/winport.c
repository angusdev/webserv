#include <windows.h>
#include <userenv.h>

#include "winport.h"
#include "helper.h"
#include "servreq.h"

/* the server root will be under %USERPROFILE% */
#define WIN_SERVER_ROOT "\\httpd\\html"

DWORD dwTlsIndex;
static char server_root[MAX_PATH];

BOOL getCurrentUserDir(wchar_t* buf, DWORD buflen){
	HANDLE hToken;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken))
		return FALSE;

	if (!GetUserProfileDirectory(hToken, buf, &buflen))
		return FALSE;

	CloseHandle(hToken);

	return TRUE;
}

int win_init() {
	int rc;
	WSADATA wsadata;
	if ((rc = WSAStartup(MAKEWORD(2, 2), &wsadata)) != 0) {
		return rc;
	}

	if ((dwTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
		Error_Quit("TlsAlloc failed");
	}

	/* Set the actual server_root to under %USERPROFILE% */
	wchar_t wpath[MAX_PATH];
	if (!getCurrentUserDir(wpath, MAX_PATH)){
		Error_Quit("Init fail.  Cannot get user profile directory");
	}

	size_t wcstombs_ret;
	if (wcstombs_s(&wcstombs_ret, server_root, (size_t)MAX_PATH, wpath, (size_t)MAX_PATH) != 0) {
		Error_Quit("Init fail.  Cannot get user profile directory");
	}

	strcat_s(server_root, _countof(server_root), WIN_SERVER_ROOT);

	return 0;
}

int win_exit() {
	TlsFree(dwTlsIndex);

	return 0;
}

thread_data_type* win_tls_get_value() {
	return (thread_data_type*)TlsGetValue(dwTlsIndex);
}

int win_init_thread() {
	thread_data_type* tls = (thread_data_type*)LocalAlloc(LPTR, sizeof(thread_data_type));
	memset(tls, 0, sizeof(tls));
	tls->first_header = 1;

	strcpy_s(tls->server_root, _countof(tls->server_root), server_root);
	if (!TlsSetValue(dwTlsIndex, tls)) {
		return -1;
	}
	
	return 0;
}

int win_free_thread() {
	LPVOID lpvData = TlsGetValue(dwTlsIndex);
	if (lpvData != 0) {
		LocalFree((HLOCAL)lpvData);
	}

	return 0;
}

DWORD WINAPI win_service_request_thread(LPVOID lpParam) {
	int conn = (int)lpParam;

	win_init_thread();

	Service_Request(conn);

	closesocket(conn);

	win_free_thread();

	return 0;
}

int win_service_request(int conn) {
	DWORD threadId;

	HANDLE thread = CreateThread(NULL, 0, win_service_request_thread, (void *)conn, 0, &threadId);

	if (thread != NULL) {
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
		return 0;
	}
	else {
		return -1;
	}
}