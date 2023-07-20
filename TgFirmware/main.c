#include <Library/UefiLib.h>

#include "Firmware/Module.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

const UINT8 _gDriverUnloadImageCount = 1;
const UINT32 _gUefiDriverRevision = 0x200;
const UINT32 _gDxeRevision = 0x200;
CHAR8* gEfiCallerBaseName = "Google UEFI";

EFI_STATUS EFIAPI UefiUnload ( IN EFI_HANDLE ImageHandle )
{
    
    return EFI_ACCESS_DENIED;
}

EFI_INPUT_KEY
WaitForKey(
)
{
    EFI_INPUT_KEY key;

    while (gST->ConIn->ReadKeyStroke(gST->ConIn, &key) == EFI_NOT_READY);

    return key;
}

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE * SystemTable)
{
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    Print(L"Starting...\n");

    Print(L"Dumping...\n");

    MODULE RTModule;
    RTModule.Base = 0;
    RTModule.Size = 0;

    if (!GetModuleFromFunction((VOID*)SystemTable->RuntimeServices->SetVariable, &RTModule))
    {
        Print(L"Failed to find Runtime Service Provider\n");

        return FALSE;
    }


    if (!DumpModuleToDisk(&RTModule))
    {
        Print(L"Failed to dump Runtime Service Provider to disk\n");

        return FALSE;
    }

    Print(L"Runtime Provider Data: %llx, %llx\n", RTModule.Base, SystemTable->RuntimeServices->SetVariable);

    WaitForKey();

    return EFI_SUCCESS;
}

