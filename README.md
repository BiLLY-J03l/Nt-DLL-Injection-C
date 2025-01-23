# Nt-DLL-Injection-C
Nt-DLL-Injection-C

## DLL injection with obfuscation Techniques including the Native API and offsets

### STAGE 1 --> loader.exe

-The Loader loads kernel32.dll and NTDLL.dll to the address space of the process.

-Then it explicitly gets the functions it needs from the loaded DLLs implementing offsets obfuscation.

-It creates a mutex to ensure that the instance is ran once and exits if the exe is executed again.

-Then it enumerates all the processes in the system and searches for notepad.exe (you can change that in the enum_processes() function.

-After that it tries to get a handle of that process.

-It then allocates memory for the size of the malicious DLL and sets Read-Write permissions only as it won't be executed.

-Then it writes the malicious DLL data into that allocated memory.

-Lastly the malicious DLL is loaded in the address space of the targeted process.



### STAGE 2 --> malicious_dll.dll
