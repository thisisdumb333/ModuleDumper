#include "module.h"
#include <PiDxe.h>

#include <Uefi.h>
#include <Library/UefiLib.h>


#include <Library/FileHandleLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Guid/FileInfo.h>
#include <Library/DevicePathLib.h>

BOOLEAN GetModuleFromFunction(IN const VOID* Function, OUT PMODULE Module)
{
	if (!Function || !Module)
		return FALSE;

	UINT64 PageStart = ((UINTN)Function & ~(SIZE_4KB - 1));
	UINT64 CurrentAddress = (UINT64)PageStart;

	do
	{
		EFI_IMAGE_DOS_HEADER* DosHeader = (EFI_IMAGE_DOS_HEADER*)CurrentAddress;

		if (DosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE)
		{
			Module->Base = CurrentAddress;
			

			EFI_IMAGE_NT_HEADERS64* NTHeader = (EFI_IMAGE_NT_HEADERS64*)(DosHeader->e_lfanew + CurrentAddress);
			
			Module->Size = NTHeader->OptionalHeader.SizeOfImage;
			
			return TRUE;

		}
		CurrentAddress -= SIZE_4KB; // Move to the previous 4KB page
	} while (TRUE);

	return FALSE;

}

BOOLEAN GetModuleFromBase(IN UINT64 Base, OUT PMODULE Module)
{
	if (!Module || !Base)
		return FALSE;

	Module->Base = Base;

	EFI_IMAGE_DOS_HEADER* DosHeader = (EFI_IMAGE_DOS_HEADER*)Base;

	EFI_IMAGE_NT_HEADERS64* NTHeader = (EFI_IMAGE_NT_HEADERS64*)(DosHeader->e_lfanew + Base);

	Module->Size = NTHeader->OptionalHeader.SizeOfImage;

	return TRUE;
}

BOOLEAN DumpModuleToDisk(IN const PMODULE Module)
{
    if (Module == NULL)
        return FALSE;

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* gFS;
    EFI_FILE_PROTOCOL* RootDir;
    EFI_FILE_PROTOCOL* File;
    EFI_STATUS Status;

    Status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&gFS);
    if (EFI_ERROR(Status))
        return FALSE;

    Status = gFS->OpenVolume(gFS, &RootDir);
    if (EFI_ERROR(Status))
        return FALSE;

    CHAR16 FileName[] = L"ModuleName.bin"; // Adjust the file name as needed
    Status = RootDir->Open(
        RootDir,
        &File,
        FileName,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
        EFI_FILE_ARCHIVE
    );
    if (EFI_ERROR(Status))
    {
        RootDir->Close(RootDir);
        return FALSE;
    }

    UINTN DataSize = Module->Size;
    Status = File->Write(File, &DataSize, (VOID*)Module->Base);
    if (EFI_ERROR(Status))
    {
        File->Close(File);
        RootDir->Close(RootDir);
        return FALSE;
    }

    File->Close(File);
    RootDir->Close(RootDir);

    return TRUE;
}

#define IMAGE_FIRST_SECTION( NtHeader )                \
  (( EFI_IMAGE_SECTION_HEADER*) ((UINT64)(NtHeader) +    \
   OFFSET_OF( EFI_IMAGE_NT_HEADERS64, OptionalHeader ) +  \
   ((NtHeader))->FileHeader.SizeOfOptionalHeader))

BOOLEAN GetModuleSection(IN const PMODULE Module, IN const CHAR8* Section, OUT PSECTION Out)
{
    if (!Module || !Section || !Out)
        return FALSE;

    EFI_IMAGE_DOS_HEADER* ImageDosHeader = (EFI_IMAGE_DOS_HEADER*)Module->Base;
    EFI_IMAGE_NT_HEADERS64* ImageNtHeader = (EFI_IMAGE_NT_HEADERS64*)(Module->Base + ImageDosHeader->e_lfanew);
    EFI_IMAGE_SECTION_HEADER* SectionHeader = IMAGE_FIRST_SECTION(ImageNtHeader);

    for (UINT32 i = 0; i < ImageNtHeader->FileHeader.NumberOfSections; i++, ++Section)
    {
        if (AsciiStrCmp(Section, SectionHeader->Name) == 0)
        {
            Out->Start = Module->Base + SectionHeader->VirtualAddress;
            Out->Size = SectionHeader->Misc.VirtualSize;

            return TRUE;
        }

    }

    return FALSE;

}

static UINT64 Scan(IN CONST CHAR8* pattern, IN CONST CHAR8* mask, UINT64 begin, UINT64 size)
{
    INT64 patternLen = AsciiStrLen(mask);

    for (int i = 0; i < size; i++)
    {
        BOOLEAN found = TRUE;
        for (int j = 0; j < patternLen; j++)
        {
            if (mask[j] != '?' && pattern[j] != *(char*)((INT64)begin + i + j))
            {
                found = FALSE;
                break;
            }
        }
        if (found)
        {
            return (begin + i);
        }
    }
    return 0;
}

BOOLEAN ScanModule(IN CONST SECTION* Section, IN CONST CHAR8* Pattern, IN CONST CHAR8* Mask, OUT UINT64* Address)
{
    UINT64 Found =  Scan(Pattern, Mask, Section->Start, Section->Size);

    if (!Found)
        return FALSE;

    *Address = Found;

    return TRUE;
}
