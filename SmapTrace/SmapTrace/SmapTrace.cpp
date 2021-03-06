/*
 *  Intel provides this code “as-is” and disclaims all express and implied warranties, including without 
 *  limitation, the implied warranties of merchantability, fitness for a particular purpose, and non-infringement, 
 *  as well as any warranty arising from course of performance, course of dealing, or usage in trade. No license 
 *  (express or implied, by estoppel or otherwise) to any intellectual property rights is granted by Intel providing 
 *  this code.
 *  This code is preliminary, may contain errors and is subject to change without notice. 
 *  Intel technologies' features and benefits depend on system configuration and may require enabled hardware, 
 *  software or service activation. Performance varies depending on system configuration.  Any differences in your 
 *  system hardware, software or configuration may affect your actual performance.  No product or component can be 
 *  absolutely secure.
 *  Intel and the Intel logo are trademarks of Intel Corporation in the United States and other countries. 
 *  *Other names and brands may be claimed as the property of others.
 *  © Intel Corporation
 */

//
//  Licensed under the GPL v2
//

//
//  SmapTrace.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

WINDBG_EXTENSION_APIS   ExtensionApis = { 0 };

IDebugAdvanced4*        g_AdvancedDebug4 = NULL;
IDebugClient7*          g_DebugClient7 = NULL;
IDebugControl7*         g_DebugControl7 = NULL;
IDebugDataSpaces4*      g_DataSpaces4 = NULL;
IDebugRegisters2*       g_Registers2 = NULL;
IDebugSymbols5*         g_DebugSymbols5 = NULL;
IDebugSystemObjects4*   g_DebugSystemObjects4 = NULL;

PATCH_INFO              g_PatchInfo = { 0 };
char                    g_LogFmtString[KUSER_MAX_FMT_STRING_LENGTH] = "%016llX %016llX %016llX %016llX\n";

//
//  Compile *.asm files with FASM and copy-paste the payload here
//

uint8_t                 g_KiPageFaultPatch[0x1000] = { 0 };
uint8_t                 g_KiPageFaultPatchTemplate[] =
{
	0x90, 0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53, 0x44, 0x0F, 0x20, 0xC0,
	0x50, 0x48, 0xC7, 0xC0, 0x02, 0x00, 0x00, 0x00, 0x44, 0x0F, 0x22, 0xC0, 0x48, 0x8B, 0x4C, 0x24,
	0x40, 0x0F, 0x20, 0xD2, 0x4C, 0x8B, 0x44, 0x24, 0x48, 0x4C, 0x8B, 0x4C, 0x24, 0x60, 0x48, 0x0F,
	0xBA, 0xE2, 0x38, 0x0F, 0x82, 0xFD, 0x01, 0x00, 0x00, 0x49, 0x0F, 0xBA, 0xE0, 0x38, 0x0F, 0x83,
	0xF2, 0x01, 0x00, 0x00, 0x48, 0x83, 0xF9, 0x03, 0x74, 0x0C, 0x48, 0x83, 0xF9, 0x01, 0x0F, 0x85,
	0xE2, 0x01, 0x00, 0x00, 0xEB, 0x33, 0x51, 0x50, 0x0F, 0x20, 0xD1, 0x48, 0xC1, 0xE9, 0x09, 0x48,
	0xB8, 0xF8, 0xFF, 0xFF, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0x48, 0x21, 0xC1, 0x48, 0xB8, 0xBE, 0xBA,
	0xFE, 0xCA, 0xEF, 0xBE, 0xAD, 0xDE, 0x48, 0x01, 0xC8, 0x48, 0x8B, 0x00, 0x48, 0x0F, 0xBA, 0xE0,
	0x01, 0x58, 0x59, 0x0F, 0x83, 0xAD, 0x01, 0x00, 0x00, 0x50, 0x48, 0xB8, 0xBE, 0xBA, 0xFE, 0xCA,
	0xEF, 0xBE, 0xAD, 0xDE, 0x49, 0x39, 0xC0, 0x58, 0x0F, 0x82, 0x72, 0x01, 0x00, 0x00, 0x50, 0x48,
	0xB8, 0xBE, 0xBA, 0xFE, 0xCA, 0xEF, 0xBE, 0xAD, 0xDE, 0x49, 0x39, 0xC0, 0x58, 0x0F, 0x87, 0x5D,
	0x01, 0x00, 0x00, 0x0F, 0x01, 0xCB, 0x90, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x65, 0x67, 0x48,
	0xA1, 0xEA, 0xEA, 0x00, 0x00, 0x48, 0x25, 0xFF, 0x00, 0x00, 0x00, 0x49, 0x89, 0xC0, 0x48, 0xB8,
	0xBE, 0xBA, 0xFE, 0xCA, 0xEF, 0xBE, 0xAD, 0xDE, 0x49, 0xF7, 0xE0, 0x49, 0x89, 0xC0, 0x48, 0xB8,
	0xBE, 0xBA, 0xFE, 0xCA, 0xEF, 0xBE, 0xAD, 0xDE, 0x4C, 0x01, 0xC0, 0x41, 0x59, 0x41, 0x58, 0x5A,
	0x59, 0x48, 0x89, 0x88, 0xEA, 0xEA, 0x00, 0x00, 0x4C, 0x89, 0x80, 0xEA, 0xEA, 0x00, 0x00, 0x48,
	0x89, 0x90, 0xEA, 0xEA, 0x00, 0x00, 0x4C, 0x89, 0x88, 0xEA, 0xEA, 0x00, 0x00, 0x41, 0x0F, 0x20,
	0xD9, 0x4C, 0x89, 0x88, 0xEA, 0xEA, 0x00, 0x00, 0x48, 0x89, 0xC1, 0x0F, 0x31, 0x89, 0x81, 0xEA,
	0xEA, 0x00, 0x00, 0x89, 0x91, 0xEA, 0xEA, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x10, 0xF3, 0x0F, 0x7F,
	0x04, 0x24, 0x48, 0x83, 0xEC, 0x10, 0xF3, 0x0F, 0x7F, 0x0C, 0x24, 0x48, 0x83, 0xEC, 0x10, 0xF3,
	0x0F, 0x7F, 0x14, 0x24, 0x48, 0x83, 0xEC, 0x10, 0xF3, 0x0F, 0x7F, 0x1C, 0x24, 0x48, 0x83, 0xEC,
	0x10, 0xF3, 0x0F, 0x7F, 0x24, 0x24, 0x48, 0x83, 0xEC, 0x10, 0xF3, 0x0F, 0x7F, 0x2C, 0x24, 0x51,
	0x52, 0x41, 0x50, 0x41, 0x51, 0x51, 0x48, 0x81, 0xC1, 0xDA, 0xDA, 0xDA, 0x0A, 0x48, 0xC7, 0xC2,
	0xDA, 0xDA, 0xDA, 0x0A, 0x49, 0xC7, 0xC0, 0xDA, 0xDA, 0xDA, 0x0A, 0x48, 0xB8, 0xBE, 0xBA, 0xFE,
	0xCA, 0xEF, 0xBE, 0xAD, 0xDE, 0x49, 0x89, 0xC1, 0x48, 0x83, 0xEC, 0x20, 0x48, 0xB8, 0xBE, 0xBA,
	0xFE, 0xCA, 0xEF, 0xBE, 0xAD, 0xDE, 0xFF, 0xD0, 0x48, 0x83, 0xC4, 0x28, 0x41, 0x59, 0x41, 0x58,
	0x5A, 0x59, 0x48, 0x81, 0xC1, 0xDA, 0xDA, 0xDA, 0x0A, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x49,
	0x89, 0xC8, 0x48, 0xC7, 0xC1, 0x4E, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC2, 0x00, 0x00, 0x00, 0x00,
	0x48, 0xB8, 0xBE, 0xBA, 0xFE, 0xCA, 0xEF, 0xBE, 0xAD, 0xDE, 0x48, 0x83, 0xEC, 0x30, 0xFF, 0xD0,
	0x48, 0x83, 0xC4, 0x30, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0xF3, 0x0F, 0x6F, 0x2C, 0x24, 0x48,
	0x83, 0xC4, 0x10, 0xF3, 0x0F, 0x6F, 0x24, 0x24, 0x48, 0x83, 0xC4, 0x10, 0xF3, 0x0F, 0x6F, 0x1C,
	0x24, 0x48, 0x83, 0xC4, 0x10, 0xF3, 0x0F, 0x6F, 0x14, 0x24, 0x48, 0x83, 0xC4, 0x10, 0xF3, 0x0F,
	0x6F, 0x0C, 0x24, 0x48, 0x83, 0xC4, 0x10, 0xF3, 0x0F, 0x6F, 0x04, 0x24, 0x48, 0x83, 0xC4, 0x10,
	0x48, 0x8B, 0x44, 0x24, 0x58, 0x48, 0x0D, 0x00, 0x01, 0x24, 0x00, 0x48, 0x89, 0x44, 0x24, 0x58,
	0x58, 0x44, 0x0F, 0x22, 0xC0, 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0x58,
	0x48, 0x83, 0xC4, 0x08, 0x48, 0xCF, 0x58, 0x44, 0x0F, 0x22, 0xC0, 0x41, 0x5B, 0x41, 0x5A, 0x41,
	0x59, 0x41, 0x58, 0x5A, 0x59, 0x48, 0xB8, 0xBE, 0xBA, 0xFE, 0xCA, 0xEF, 0xBE, 0xAD, 0xDE, 0x48,
	0x87, 0x04, 0x24, 0xC3
};

uint8_t                 g_KiDebugTrapOrFaultPatch[] =
{
	0x90, 0x0F, 0x01, 0xCB, 0x50, 0x48, 0x8B, 0x44, 0x24, 0x18, 0x48, 0x0F, 0xBA, 0xE0, 0x15, 0x73,
	0x0E, 0x48, 0x25, 0xFF, 0xFE, 0xDB, 0x7F, 0x48, 0x89, 0x44, 0x24, 0x18, 0x58, 0x48, 0xCF, 0x48,
	0xB8, 0xBE, 0xBA, 0xFE, 0xCA, 0xEF, 0xBE, 0xAD, 0xDE, 0x48, 0x87, 0x04, 0x24, 0xC3
};

//
//  Patch related stuff
//

HRESULT GetFieldOffset(LPCSTR lpTypeName, LPCSTR lpFieldName, PULONG64 pAddress)
{
	HRESULT  hResult = S_OK;

	ULONG    fieldOffset;
	ULONG    typeId;
	ULONG64  module;

	hResult = g_DebugSymbols5->GetSymbolTypeId(lpTypeName, &typeId, &module);
	if (hResult != S_OK)
		return hResult;

	hResult = g_DebugSymbols5->GetFieldOffset(module, typeId, lpFieldName, &fieldOffset);
	if (hResult != S_OK)
		return hResult;

	if (pAddress != NULL)
		*pAddress = fieldOffset;

	return hResult;
}

HRESULT GetTypeSize(LPCSTR lpTypeName, PULONG64 pSize)
{
	HRESULT  hResult = S_OK;

	ULONG    typeSize;
	ULONG    typeId;
	ULONG64  module;

	hResult = g_DebugSymbols5->GetSymbolTypeId(lpTypeName, &typeId, &module);
	if (hResult != S_OK)
		return hResult;

	hResult = g_DebugSymbols5->GetTypeSize(module, typeId, &typeSize);
	if (hResult != S_OK)
		return hResult;

	if (pSize != NULL)
		*pSize = typeSize;

	return hResult;
}

HRESULT FillPatchInfo()
{
	HRESULT hResult = S_OK;
	ULONG64 TempOffset = 0;
	ULONG   TempOffset32 = 0;

	//
	//  Check the number of cores
	//

	hResult = g_DebugControl7->GetNumberProcessors(&TempOffset32);
	if (hResult != S_OK)
		return hResult;

	g_PatchInfo.CpuCount = TempOffset32;
	if (g_PatchInfo.CpuCount > 8)
	{
		// SMAP_EVENT_INFO_ENTRY is per CPU, so we are limited to the amount of free space at KUSER_SHARED_DATA page
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Too many cores\n");
		return hResult;
	}

	//
	//  Active addresses and offsets
	//

	hResult = GetTypeSize("nt!_KUSER_SHARED_DATA", &TempOffset);
	if (hResult != S_OK)
		return hResult;

	TempOffset = (TempOffset + 0x10 - (TempOffset % 0x10));
	g_PatchInfo.HooksAddress = KUSER_SHARED_PAGE + TempOffset;
	g_PatchInfo.HooksSize = sizeof(g_KiPageFaultPatchTemplate) + sizeof(g_KiDebugTrapOrFaultPatch);
	g_PatchInfo.KiPageFaultHookAddress = g_PatchInfo.HooksAddress;
	g_PatchInfo.KiPageFaultHookSize = sizeof(g_KiPageFaultPatchTemplate);
	g_PatchInfo.KiDebugTrapOrFaultHookAddress = g_PatchInfo.KiPageFaultHookAddress + g_PatchInfo.KiPageFaultHookSize;
	g_PatchInfo.KiDebugTrapOrFaultHookSize = sizeof(g_KiDebugTrapOrFaultPatch);

	g_PatchInfo.AreaAddress = g_PatchInfo.HooksAddress;

	TempOffset = g_PatchInfo.AreaAddress + g_PatchInfo.HooksSize;
	TempOffset = (TempOffset + 0x10 - (TempOffset % 0x10));
	g_PatchInfo.InfoAddress = TempOffset;
	g_PatchInfo.FmtStringAddress = g_PatchInfo.InfoAddress + (sizeof(SMAP_EVENT_INFO_ENTRY) * g_PatchInfo.CpuCount);

	hResult = g_DebugSymbols5->GetOffsetByName("nt!KiPageFault", &g_PatchInfo.KiPageFault);
	if (hResult != S_OK)
		return hResult;

	hResult = g_DebugSymbols5->GetOffsetByName("nt!KiDebugTrapOrFault", &g_PatchInfo.KiDebugTrapOrFault);
	if (hResult != S_OK)
		return hResult;

	hResult = g_DebugSymbols5->GetOffsetByName("nt!vsnprintf_s", &g_PatchInfo.vsnprintf_s);
	if (hResult != S_OK)
		return hResult;

	hResult = g_DebugSymbols5->GetOffsetByName("nt!MmPteBase", &g_PatchInfo.MmPteBase);
	if (hResult != S_OK)
		return hResult;

	ULONG BytesRead = 0;
	hResult = g_DataSpaces4->ReadVirtual(g_PatchInfo.MmPteBase, &g_PatchInfo.MmPteBase, sizeof(g_PatchInfo.MmPteBase), &BytesRead);
	if (hResult != S_OK)
		return hResult;

	hResult = g_DebugSymbols5->GetOffsetByName("nt!DbgPrintEx", &g_PatchInfo.DbgPrintEx);
	if (hResult != S_OK)
		return hResult;

	hResult = GetFieldOffset("nt!_KPCR", "Prcb", &TempOffset);
	if (hResult != S_OK)
		return hResult;

	g_PatchInfo.CurrentCpuOffset = TempOffset;

	hResult = GetFieldOffset("nt!_KPRCB", "LegacyNumber", &TempOffset);
	if (hResult != S_OK)
		return hResult;

	g_PatchInfo.CurrentCpuOffset += TempOffset;

	//
	//  Filtered module info
	//

	hResult = g_DebugSymbols5->GetModuleByModuleName("nt", 0, NULL, &g_PatchInfo.FilteredModuleBase);
	if (hResult != S_OK)
		return hResult;

	DEBUG_MODULE_PARAMETERS dmp = { 0 };
	hResult = g_DebugSymbols5->GetModuleParameters(1, &g_PatchInfo.FilteredModuleBase, 0, &dmp);
	if (hResult != S_OK)
		return hResult;

	g_PatchInfo.FilteredModuleSize = dmp.Size;
	g_PatchInfo.FilteredModuleEnd = g_PatchInfo.FilteredModuleBase + g_PatchInfo.FilteredModuleSize;

	return hResult;
}

HRESULT ApplyFixupsForKiPageFault()
{
	uint64_t QwordFixups[9];
	uint32_t DwordFixups[4];
	uint16_t WordFixups[8];

	uint32_t QwordFixupIndex = 0;
	uint32_t DwordFixupIndex = 0;
	uint32_t WordFixupIndex = 0;

	RtlCopyMemory(g_KiPageFaultPatch, g_KiPageFaultPatchTemplate, g_PatchInfo.KiPageFaultHookSize);

	QwordFixups[0] = g_PatchInfo.MmPteBase;
	QwordFixups[1] = g_PatchInfo.FilteredModuleBase;
	QwordFixups[2] = g_PatchInfo.FilteredModuleEnd;
	QwordFixups[3] = sizeof(SMAP_EVENT_INFO_ENTRY);
	QwordFixups[4] = g_PatchInfo.InfoAddress;
	QwordFixups[5] = g_PatchInfo.FmtStringAddress;
	QwordFixups[6] = g_PatchInfo.vsnprintf_s;
	QwordFixups[7] = g_PatchInfo.DbgPrintEx;
	QwordFixups[8] = g_PatchInfo.KiPageFault;

	DwordFixups[0] = offsetof(SMAP_EVENT_INFO_ENTRY, OutputBuffer);
	DwordFixups[1] = KUSER_OUT_BUF_LENGTH;
	DwordFixups[2] = KUSER_OUT_BUF_LENGTH;
	DwordFixups[3] = offsetof(SMAP_EVENT_INFO_ENTRY, OutputBuffer);

	WordFixups[0] = g_PatchInfo.CurrentCpuOffset;
	WordFixups[1] = offsetof(SMAP_EVENT_INFO_ENTRY, ErrorCode);
	WordFixups[2] = offsetof(SMAP_EVENT_INFO_ENTRY, TrapRip);
	WordFixups[3] = offsetof(SMAP_EVENT_INFO_ENTRY, FaultingAddress);
	WordFixups[4] = offsetof(SMAP_EVENT_INFO_ENTRY, TrapRsp);
	WordFixups[5] = offsetof(SMAP_EVENT_INFO_ENTRY, TrapCr3);
	WordFixups[6] = offsetof(SMAP_EVENT_INFO_ENTRY, Tsc);
	WordFixups[7] = offsetof(SMAP_EVENT_INFO_ENTRY, Tsc) + sizeof(DWORD);

	// Apply shellcode fixups
	for (uint64_t iByte = 0; iByte < sizeof(g_KiPageFaultPatch); iByte++)
	{
		if (QwordFixupIndex != sizeof(QwordFixups) / sizeof(uint64_t))
		{
			if (*(uint64_t*)(g_KiPageFaultPatch + iByte) == 0xDEADBEEFCAFEBABE)
			{
				*(uint64_t*)(g_KiPageFaultPatch + iByte) = QwordFixups[QwordFixupIndex];
				++QwordFixupIndex;
			}
		}

		if (DwordFixupIndex != sizeof(DwordFixups) / sizeof(uint32_t))
		{
			if (*(uint32_t*)(g_KiPageFaultPatch + iByte) == 0x0ADADADA)
			{
				*(uint32_t*)(g_KiPageFaultPatch + iByte) = DwordFixups[DwordFixupIndex];
				++DwordFixupIndex;
			}
		}

		if (WordFixupIndex != sizeof(WordFixups) / sizeof(uint16_t))
		{
			if (*(uint16_t*)(g_KiPageFaultPatch + iByte) == 0xEAEA)
			{
				*(uint16_t*)(g_KiPageFaultPatch + iByte) = WordFixups[WordFixupIndex];
				++WordFixupIndex;
			}
		}
	}

	return S_OK;
}

HRESULT ApplyFixupsForKiDebugTrapOrFault()
{
	uint64_t QwordFixups[1];
	uint32_t QwordFixupIndex = 0;

	QwordFixups[0] = g_PatchInfo.KiDebugTrapOrFault;

	// Apply shellcode fixups
	for (uint64_t iByte = 0; iByte < sizeof(g_KiDebugTrapOrFaultPatch); iByte++)
	{
		if (QwordFixupIndex != sizeof(QwordFixups))
		{
			if (*(uint64_t*)(g_KiDebugTrapOrFaultPatch + iByte) == 0xDEADBEEFCAFEBABE)
			{
				*(uint64_t*)(g_KiDebugTrapOrFaultPatch + iByte) = QwordFixups[QwordFixupIndex];
				++QwordFixupIndex;
			}
		}
	}

	return S_OK;
}

HRESULT InitializePayload()
{
	HRESULT hResult = S_OK;

	hResult = ApplyFixupsForKiPageFault();
	if (hResult != S_OK)
		return hResult;

	hResult = ApplyFixupsForKiDebugTrapOrFault();
	if (hResult != S_OK)
		return hResult;

	return hResult;
}

HRESULT GetPtePhysAddress(ULONGLONG Address, PULONG64 pPtePhysAddress)
{
	HRESULT hResult = S_OK;

	ULONG64 PageTableRoot = 0;

	DEBUG_VALUE  dbgVal = { 0 };
	ULONG        RegIndex = 0;
	ULONG        BytesRead = 0;

	hResult = g_Registers2->GetIndexByName("cr3", &RegIndex);
	if (hResult != S_OK)
		return hResult;

	hResult = g_Registers2->GetValue(RegIndex, &dbgVal);
	if (hResult != S_OK)
		return hResult;

	ULONG64 pml4_idx = (Address & 0xFF8000000000) >> 39;
	ULONG64 pdpte_idx = (Address & 0x7FC0000000) >> 30;
	ULONG64 pde_idx = (Address & 0x3FE00000) >> 21;
	ULONG64 pte_idx = (Address & 0x1FF000) >> 12;
	ULONG64 offset = (Address & 0xFFF);

	ULONG64 pml4e = (Address & 0xFF8000000000) >> 39;
	ULONG64 pdpte = (Address & 0x7FC0000000) >> 30;
	ULONG64 pde = (Address & 0x3FE00000) >> 21;
	ULONG64 pte = (Address & 0x1FF000) >> 12;

	PageTableRoot = dbgVal.I64;

	pml4e = (PageTableRoot & 0xFFFFFF000) + pml4_idx * 8;
	hResult = g_DataSpaces4->ReadPhysical2(pml4e, DEBUG_PHYSICAL_UNCACHED, &pml4e, sizeof(pml4e), &BytesRead);
	if (hResult != S_OK)
		return hResult;

	if (!(pml4e & 1))
		return STG_E_INVALIDPARAMETER;

	pdpte = (pml4e & 0xFFFFFF000) + 8 * pdpte_idx;
	hResult = g_DataSpaces4->ReadPhysical2(pdpte, DEBUG_PHYSICAL_UNCACHED, &pdpte, sizeof(pdpte), &BytesRead);
	if (hResult != S_OK)
		return hResult;

	if (!(pdpte & 1))
		return STG_E_INVALIDPARAMETER;

	if (pdpte & 0x80)
	{
		// Gig page
		if (pPtePhysAddress != NULL)
			*pPtePhysAddress = (pml4e & 0xFFFFFF000) + 8 * pdpte_idx;

		return hResult;
	}

	pde = (pdpte & 0xFFFFFF000) + 8 * pde_idx;
	hResult = g_DataSpaces4->ReadPhysical2(pde, DEBUG_PHYSICAL_UNCACHED, &pde, sizeof(pde), &BytesRead);
	if (hResult != S_OK)
		return hResult;

	if (!(pde & 1))
		return STG_E_INVALIDPARAMETER;

	if (pde & 0x80)
	{
		// Large page
		if (pPtePhysAddress != NULL)
			*pPtePhysAddress = (pdpte & 0xFFFFFF000) + 8 * pde_idx;

		return hResult;
	}

	pte = (pde & 0xFFFFFF000) + 8 * pte_idx;
	hResult = g_DataSpaces4->ReadPhysical2(pte, DEBUG_PHYSICAL_UNCACHED, &pte, sizeof(pte), &BytesRead);
	if (hResult != S_OK)
		return hResult;

	if (!(pte & 1))
		return STG_E_INVALIDPARAMETER;

	// Success
	if (pPtePhysAddress != NULL)
		*pPtePhysAddress = (pde & 0xFFFFFF000) + 8 * pte_idx;

	return hResult;
}

HRESULT MakeExecutable(ULONGLONG Address)
{
	HRESULT hResult = S_OK;

	ULONG64 PteAddress = 0;
	hResult = GetPtePhysAddress(Address, &PteAddress);
	if (hResult != S_OK)
		return hResult;

	ULONG   OpBytes = 0;

	// Read PTE
	ULONG64 Pte = 0;
	hResult = g_DataSpaces4->ReadPhysical2(PteAddress, DEBUG_PHYSICAL_UNCACHED, &Pte, sizeof(Pte), &OpBytes);
	if (hResult != S_OK)
		return hResult;

	// Modify PTE
	Pte = Pte & 0x7FFFFFFFFFFFFFFF;
	hResult = g_DataSpaces4->WritePhysical2(PteAddress, DEBUG_PHYSICAL_UNCACHED, &Pte, sizeof(Pte), &OpBytes);
	if (hResult != S_OK)
		return hResult;

	return hResult;
}

HRESULT MakeNonExecutable(ULONGLONG Address)
{
	HRESULT hResult = S_OK;

	ULONG64 PteAddress = 0;
	hResult = GetPtePhysAddress(Address, &PteAddress);
	if (hResult != S_OK)
		return hResult;

	ULONG   OpBytes = 0;

	// Read PTE
	ULONG64 Pte = 0;
	hResult = g_DataSpaces4->ReadPhysical2(PteAddress, DEBUG_PHYSICAL_UNCACHED, &Pte, sizeof(Pte), &OpBytes);
	if (hResult != S_OK)
		return hResult;

	// Modify PTE
	Pte = Pte | 0x8000000000000000;
	hResult = g_DataSpaces4->WritePhysical2(PteAddress, DEBUG_PHYSICAL_UNCACHED, &Pte, sizeof(Pte), &OpBytes);
	if (hResult != S_OK)
		return hResult;

	return hResult;
}

HRESULT PatchIDTEntry(ULONG EntryIdx, ULONG64 NewHandlerAddress)
{
	HRESULT hResult = S_OK;

	ULONG64      idtr = 0;
	DEBUG_VALUE  dbgVal = { 0 };
	ULONG        RegIndex = 0;
	ULONG        BytesRW = 0;

	hResult = g_Registers2->GetIndexByName("idtr", &RegIndex);
	if (hResult != S_OK)
		return hResult;

	hResult = g_Registers2->GetValue(RegIndex, &dbgVal);
	if (hResult != S_OK)
		return hResult;

	idtr = dbgVal.I64;

	// Target IDT entry
	idtr += sizeof(KIDTENTRY64) * EntryIdx;

	KIDTENTRY64 entry = { 0 };

	hResult = g_DataSpaces4->ReadVirtualUncached(idtr, &entry, sizeof(KIDTENTRY64), &BytesRW);
	if (hResult != S_OK)
		return hResult;

	entry.OffsetLow = NewHandlerAddress;
	entry.OffsetMiddle = NewHandlerAddress >> 16;
	entry.OffsetHigh = NewHandlerAddress >> 32;

	hResult = g_DataSpaces4->WriteVirtualUncached(idtr, &entry, sizeof(KIDTENTRY64), &BytesRW);
	if (hResult != S_OK)
		return hResult;

	return hResult;
}

HRESULT Stac()
{
	HRESULT hResult = S_OK;

	LONG64       rflags = 0;
	DEBUG_VALUE  dbgVal = { 0 };
	ULONG        RegIndex = 0;

	//
	//  Reset AC flag
	//

	hResult = g_Registers2->GetIndexByName("efl", &RegIndex);
	if (hResult != S_OK)
		return hResult;

	hResult = g_Registers2->GetValue(RegIndex, &dbgVal);
	if (hResult != S_OK)
		return hResult;

	rflags = dbgVal.I64;
	_bittestandreset64(&rflags, 18);
	dbgVal.I64 = rflags;

	hResult = g_Registers2->SetValue(RegIndex, &dbgVal);
	if (hResult != S_OK)
		return hResult;

	return hResult;
}

HRESULT EnableSMAP()
{
	HRESULT hResult = S_OK;

	LONG64       cr4 = 0;
	DEBUG_VALUE  dbgVal = { 0 };
	ULONG        RegIndex = 0;

	//
	//  Set SMAP
	//

	hResult = g_Registers2->GetIndexByName("cr4", &RegIndex);
	if (hResult != S_OK)
		return hResult;

	hResult = g_Registers2->GetValue(RegIndex, &dbgVal);
	if (hResult != S_OK)
		return hResult;

	cr4 = dbgVal.I64;
	_bittestandset64(&cr4, 21);
	dbgVal.I64 = cr4;

	hResult = g_Registers2->SetValue(RegIndex, &dbgVal);
	if (hResult != S_OK)
		return hResult;

	return hResult;
}

HRESULT DisableSMAP()
{
	HRESULT hResult = S_OK;

	LONG64       cr4 = 0;
	DEBUG_VALUE  dbgVal = { 0 };
	ULONG        RegIndex = 0;

	//
	//  Reset SMAP
	//

	hResult = g_Registers2->GetIndexByName("cr4", &RegIndex);
	if (hResult != S_OK)
		return hResult;

	hResult = g_Registers2->GetValue(RegIndex, &dbgVal);
	if (hResult != S_OK)
		return hResult;

	cr4 = dbgVal.I64;
	_bittestandreset64(&cr4, 21);
	dbgVal.I64 = cr4;

	hResult = g_Registers2->SetValue(RegIndex, &dbgVal);
	if (hResult != S_OK)
		return hResult;

	return hResult;
}

HRESULT WritePayload()
{
	HRESULT hResult = S_OK;

	ULONG Written = 0;

	// Write a format string
	hResult = g_DataSpaces4->WriteVirtualUncached(g_PatchInfo.FmtStringAddress, g_LogFmtString, KUSER_MAX_FMT_STRING_LENGTH, &Written);
	if (hResult != S_OK)
		return hResult;

	// Apply KiPageFault patch
	hResult = g_DataSpaces4->WriteVirtualUncached(g_PatchInfo.KiPageFaultHookAddress, g_KiPageFaultPatch, g_PatchInfo.KiPageFaultHookSize, &Written);
	if (hResult != S_OK)
		return hResult;

	// Apply KiDebugTrapOrFault patch
	hResult = g_DataSpaces4->WriteVirtualUncached(g_PatchInfo.KiDebugTrapOrFaultHookAddress, g_KiDebugTrapOrFaultPatch, g_PatchInfo.KiDebugTrapOrFaultHookSize, &Written);
	if (hResult != S_OK)
		return hResult;

	return hResult;
}

//
//  DbgEng extension implementation
//

EXTERN_C HRESULT CALLBACK DebugExtensionInitialize(_Out_ PULONG Version, _Out_ PULONG Flags)
{
	HRESULT hResult = S_OK;

	if (g_DebugClient7 == NULL)
	{
		if (hResult = DebugCreate(__uuidof(IDebugClient7), (void**)&g_DebugClient7) != S_OK)
		{
			dprintf("Acquiring IDebugClient7* Failled\n\n");
			return hResult;
		}
	}

	if (g_AdvancedDebug4 == NULL)
	{
		if (hResult = g_DebugClient7->QueryInterface(__uuidof(IDebugAdvanced4), (void**)&g_AdvancedDebug4) != S_OK)
		{
			dprintf("Acquiring IDebugAdvanced4* Failled\n\n");
			return hResult;
		}
	}

	if (g_DebugControl7 == NULL)
	{
		if (hResult = g_DebugClient7->QueryInterface(__uuidof(IDebugControl7), (void**)&g_DebugControl7) != S_OK)
		{
			dprintf("Acquiring IDebugControl7* Failled\n\n");
			return hResult;
		}
	}

	if (g_DataSpaces4 == NULL)
	{
		if (hResult = g_DebugClient7->QueryInterface(__uuidof(IDebugDataSpaces4), (void**)&g_DataSpaces4) != S_OK)
		{
			dprintf("Acquiring IDebugDataSpaces4* Failled\n\n");
			return hResult;
		}
	}

	if (g_Registers2 == NULL)
	{
		if (hResult = g_DebugClient7->QueryInterface(__uuidof(IDebugRegisters2), (void**)&g_Registers2) != S_OK)
		{
			dprintf("Acquiring IDebugRegisters2* Failled\n\n");
			return hResult;
		}
	}

	if (g_DebugSymbols5 == NULL)
	{
		if (hResult = g_DebugClient7->QueryInterface(__uuidof(IDebugSymbols5), (void**)&g_DebugSymbols5) != S_OK)
		{
			dprintf("Acquiring IDebugSymbols5* Failled\n\n");
			return hResult;
		}
	}

	if (g_DebugSystemObjects4 == NULL)
	{
		if (hResult = g_DebugClient7->QueryInterface(__uuidof(IDebugSystemObjects4), (void**)&g_DebugSystemObjects4) != S_OK)
		{
			dprintf("Acquiring IDebugSystemObjects4* Failled\n\n");
			return hResult;
		}
	}

	hResult = FillPatchInfo();
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Symbols missing\n");
		return hResult;
	}

	hResult = InitializePayload();
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't initialize payload\n");
		return hResult;
	}

	hResult = MakeExecutable(KUSER_SHARED_PAGE);
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't delete XD bit\n");
		return hResult;
	}

	hResult = WritePayload();
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't write patches\n");
		return hResult;
	}

	return S_OK;
}

EXTERN_C void CALLBACK DebugExtensionUninitialize(void)
{
	HRESULT hResult = S_OK;

	// Save current CPU context
	ULONG CurrentCpu = 0;
	hResult = g_DebugSystemObjects4->GetCurrentThreadId(&CurrentCpu);
	if (hResult != S_OK)
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't get the current CPU id\n");

	char SwitchCmd[] = "~0\n";
	for (ULONG iCpu = 0; iCpu < g_PatchInfo.CpuCount; iCpu++)
	{
		SwitchCmd[1] = '0' + iCpu;
		hResult = g_DebugControl7->Execute(DEBUG_OUTCTL_IGNORE, SwitchCmd, DEBUG_EXECUTE_NOT_LOGGED | DEBUG_EXECUTE_NO_REPEAT);
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't switch CPU\n");
			break;
		}

		hResult = DisableSMAP();
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't disable SMAP\n");
			break;
		}

		hResult = PatchIDTEntry(IDT_PAGE_FAULT_IDX, g_PatchInfo.KiPageFault);
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't patch IDT\n");
			break;
		}

		hResult = PatchIDTEntry(IDT_DBG_TRAPFAULT_IDX, g_PatchInfo.KiDebugTrapOrFault);
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't patch IDT\n");
			break;
		}
	}

	// Restore original CPU context
	SwitchCmd[1] = '0' + CurrentCpu;
	hResult = g_DebugControl7->Execute(DEBUG_OUTCTL_IGNORE, SwitchCmd, DEBUG_EXECUTE_NOT_LOGGED | DEBUG_EXECUTE_NO_REPEAT);
	if (hResult != S_OK)
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't switch CPU\n");

	if (g_AdvancedDebug4 != NULL)
	{
		g_AdvancedDebug4->Release();
	}

	if (g_DebugControl7 != NULL)
	{
		g_DebugControl7->Release();
	}

	if (g_DataSpaces4 != NULL)
	{
		g_DataSpaces4->Release();
	}

	if (g_Registers2 != NULL)
	{
		g_Registers2->Release();
	}

	if (g_DebugSymbols5 != NULL)
	{
		g_DebugSymbols5->Release();
	}

	if (g_DebugSystemObjects4 != NULL)
	{
		g_DebugSystemObjects4->Release();
	}

	if (g_DebugClient7 != NULL)
	{
		g_DebugClient7->Release();
	}
}

EXTERN_C void CALLBACK DebugExtensionNotify(_In_ ULONG Notify, _In_ ULONG64 Argument)
{
	switch (Notify)
	{
		case DEBUG_NOTIFY_SESSION_ACTIVE:
			//Inst->OnSessionActive(Argument);
			break;
		case DEBUG_NOTIFY_SESSION_INACTIVE:
			//Inst->OnSessionInactive(Argument);
			break;
		case DEBUG_NOTIFY_SESSION_ACCESSIBLE:
			//Inst->OnSessionAccessible(Argument);
			break;
		case DEBUG_NOTIFY_SESSION_INACCESSIBLE:
			//Inst->OnSessionInaccessible(Argument);
			break;
	}
}

HRESULT CALLBACK trauto(PDEBUG_CLIENT pDebugClient, PCSTR args)
{
	HRESULT hResult = S_OK;

	// Save current CPU context
	ULONG CurrentCpu = 0;
	hResult = g_DebugSystemObjects4->GetCurrentThreadId(&CurrentCpu);
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't get the current CPU id\n");
		return hResult;
	}

	char SwitchCmd[] = "~0\n";
	for (ULONG iCpu = 0; iCpu < g_PatchInfo.CpuCount; iCpu++)
	{
		SwitchCmd[1] = '0' + iCpu;
		hResult = g_DebugControl7->Execute(DEBUG_OUTCTL_IGNORE, SwitchCmd, DEBUG_EXECUTE_NOT_LOGGED | DEBUG_EXECUTE_NO_REPEAT);
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't switch CPU\n");
			return hResult;
		}

		hResult = PatchIDTEntry(IDT_PAGE_FAULT_IDX, g_PatchInfo.KiPageFaultHookAddress);
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't patch IDT\n");
			return hResult;
		}

		hResult = PatchIDTEntry(IDT_DBG_TRAPFAULT_IDX, g_PatchInfo.KiDebugTrapOrFaultHookAddress);
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't patch IDT\n");
			return hResult;
		}

		hResult = EnableSMAP();
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't enable SMAP\n");
			return hResult;
		}
	}

	// Restore original CPU context
	SwitchCmd[1] = '0' + CurrentCpu;
	hResult = g_DebugControl7->Execute(DEBUG_OUTCTL_IGNORE, SwitchCmd, DEBUG_EXECUTE_NOT_LOGGED | DEBUG_EXECUTE_NO_REPEAT);
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't switch CPU\n");
		return hResult;
	}

	return hResult;
}

HRESULT CALLBACK trman(PDEBUG_CLIENT pDebugClient, PCSTR args)
{
	HRESULT hResult = S_OK;

	// Save current CPU context
	ULONG CurrentCpu = 0;
	hResult = g_DebugSystemObjects4->GetCurrentThreadId(&CurrentCpu);
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't get the current CPU id\n");
		return hResult;
	}

	char SwitchCmd[] = "~0\n";
	for (ULONG iCpu = 0; iCpu < g_PatchInfo.CpuCount; iCpu++)
	{
		SwitchCmd[1] = '0' + iCpu;
		hResult = g_DebugControl7->Execute(DEBUG_OUTCTL_IGNORE, SwitchCmd, DEBUG_EXECUTE_NOT_LOGGED | DEBUG_EXECUTE_NO_REPEAT);
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't switch CPU\n");
			return hResult;
		}

		hResult = PatchIDTEntry(IDT_PAGE_FAULT_IDX, g_PatchInfo.KiPageFaultHookAddress);
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't patch IDT\n");
			return hResult;
		}

		hResult = EnableSMAP();
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't enable SMAP\n");
			return hResult;
		}
	}

	// Restore original CPU context
	SwitchCmd[1] = '0' + CurrentCpu;
	hResult = g_DebugControl7->Execute(DEBUG_OUTCTL_IGNORE, SwitchCmd, DEBUG_EXECUTE_NOT_LOGGED | DEBUG_EXECUTE_NO_REPEAT);
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't switch CPU\n");
		return hResult;
	}

	return hResult;
}

HRESULT CALLBACK trnext(PDEBUG_CLIENT pDebugClient, PCSTR args)
{
	HRESULT hResult = S_OK;

	hResult = Stac();
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't set AC\n");
		return hResult;
	}

	hResult = g_DebugControl7->SetExecutionStatus(DEBUG_STATUS_GO);
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't resume\n");
		return hResult;
	}

	return hResult;
}

HRESULT CALLBACK trstop(PDEBUG_CLIENT pDebugClient, PCSTR args)
{
	HRESULT hResult = S_OK;

	// Save current CPU context
	ULONG CurrentCpu = 0;
	hResult = g_DebugSystemObjects4->GetCurrentThreadId(&CurrentCpu);
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't get the current CPU id\n");
		return hResult;
	}

	char SwitchCmd[] = "~0\n";
	for (ULONG iCpu = 0; iCpu < g_PatchInfo.CpuCount; iCpu++)
	{
		SwitchCmd[1] = '0' + iCpu;
		hResult = g_DebugControl7->Execute(DEBUG_OUTCTL_IGNORE, SwitchCmd, DEBUG_EXECUTE_NOT_LOGGED | DEBUG_EXECUTE_NO_REPEAT);
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't switch CPU\n");
			return hResult;
		}

		hResult = DisableSMAP();
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't disable SMAP\n");
			return hResult;
		}

		hResult = PatchIDTEntry(IDT_PAGE_FAULT_IDX, g_PatchInfo.KiPageFault);
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't patch IDT\n");
			return hResult;
		}

		hResult = PatchIDTEntry(IDT_DBG_TRAPFAULT_IDX, g_PatchInfo.KiDebugTrapOrFault);
		if (hResult != S_OK)
		{
			g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't patch IDT\n");
			return hResult;
		}
	}

	// Restore original CPU context
	SwitchCmd[1] = '0' + CurrentCpu;
	hResult = g_DebugControl7->Execute(DEBUG_OUTCTL_IGNORE, SwitchCmd, DEBUG_EXECUTE_NOT_LOGGED | DEBUG_EXECUTE_NO_REPEAT);
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't switch CPU\n");
		return hResult;
	}

	return hResult;
}

HRESULT CALLBACK trfilter(PDEBUG_CLIENT pDebugClient, PCSTR args)
{
	HRESULT hResult = S_OK;

	//
	//  Filtered module info
	//

	hResult = g_DebugSymbols5->GetModuleByModuleName(args, 0, NULL, &g_PatchInfo.FilteredModuleBase);
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't find the requested module\n");
		return hResult;
	}

	DEBUG_MODULE_PARAMETERS dmp = { 0 };
	hResult = g_DebugSymbols5->GetModuleParameters(1, &g_PatchInfo.FilteredModuleBase, 0, &dmp);
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't retrieve module parameters\n");
		return hResult;
	}

	g_PatchInfo.FilteredModuleSize = dmp.Size;
	g_PatchInfo.FilteredModuleEnd = g_PatchInfo.FilteredModuleBase + g_PatchInfo.FilteredModuleSize;

	hResult = ApplyFixupsForKiPageFault();
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't apply the filter\n");
		return hResult;
	}

	hResult = WritePayload();
	if (hResult != S_OK)
	{
		g_DebugControl7->Output(DEBUG_OUTCTL_ALL_CLIENTS, "Can't write patches\n");
		return hResult;
	}

	return hResult;
}
