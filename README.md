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

-The reverse connection is executed once the DLL is attached to the process.

-I had an issue where when the reverse shell was executed and the session was opened, the process (notepad.exe) froze in place and didn't work, which is a major giveaway.

-We need to let the user feel that everything is normal as usual.

-I created a mutex to ensure that the once the connection is established, no more connections are made.

-I used the NtCreateThreadEx to stealthly create a thread in the target process where the reverse shell is executed.

-I used my previous obfuscated reverese shell code I wrote before --> https://github.com/BiLLY-J03l/reverse-shell-c



### COMPILATION
    cl loader.c
    cl /LD malicious_dll.c
    


### EXECUTION

-I commented out most the printf statements to make it stealthier, you can uncomment them in the code and see the details.


![image](https://github.com/user-attachments/assets/3df437be-73a3-4ac3-9e38-87f6e50d30a7)

![image](https://github.com/user-attachments/assets/187a909b-5cd0-43e1-b992-175e12c1e6ba)

![image](https://github.com/user-attachments/assets/d48f0da0-8dba-48fc-8fe3-78209e8a895b)

![image](https://github.com/user-attachments/assets/b5f45b02-584c-4056-ae24-06b3ce082fcd)

![image](https://github.com/user-attachments/assets/56c9f9fe-6372-4b10-891b-b7484d707bab)



### VirusTotal Analysis

** Loader.exe **

![image](https://github.com/user-attachments/assets/4546b8ab-23a9-4c79-969b-1fd1f88a9d22)


** malicious_dll.dll **

![image](https://github.com/user-attachments/assets/c22fe722-aee0-450e-bdc4-52c68ce471f5)








