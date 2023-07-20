#ifndef FIRMWARE_MODULE_H
#define FIRMWARE_MODULE_H

#include <Uefi.h>

typedef struct _MODULE
{
	UINT64 Base;
	UINT64 Size;
}MODULE, *PMODULE;

typedef struct _SECTION
{
	UINT64 Start;
	UINT64 Size;
}SECTION, *PSECTION;

BOOLEAN GetModuleFromFunction(IN CONST VOID* Function, OUT PMODULE Module);
BOOLEAN GetModuleFromBase(IN UINT64 Base, OUT PMODULE Module);
BOOLEAN DumpModuleToDisk(IN CONST PMODULE Module);
BOOLEAN GetModuleSection(IN CONST PMODULE Module, IN CONST CHAR8* Section, OUT PSECTION Out);
BOOLEAN ScanModule(IN CONST SECTION* Section, IN CONST CHAR8* Pattern, IN CONST CHAR8* Mask, OUT UINT64* Address);



#endif