#pragma once

#include "../../common.h"
#include "../TES/BSTArray.h"

struct z_stream_s;
class TESForm_CK;

struct PerkRankEntry
{
	union
	{
		struct
		{
			uint32_t FormId;	// 0x0
			uint8_t Rank;		// 0x4
		};

		struct
		{
			uint64_t FormIdOrPointer;	// 0x0
			uint8_t NewRank;			// 0x8
		};
	};
};
static_assert(sizeof(PerkRankEntry) == 0x10);

int hk_inflateInit(z_stream_s *Stream, const char *Version, int Mode);
int hk_inflate(z_stream_s *Stream, int Flush);
bool OpenPluginSaveDialog(HWND ParentWindow, const char *BasePath, bool IsESM, char *Buffer, uint32_t BufferSize, const char *Directory);
bool IsBSAVersionCurrent(class BSFile *File);

void CreateLipGenProcess(__int64 a1);
bool IsLipDataPresent(void *Thisptr);
bool WriteLipData(void *Thisptr, const char *Path, int Unkown1, bool Unknown2, bool Unknown3);
int IsWavDataPresent(const char *Path, __int64 a2, __int64 a3, __int64 a4);

uint32_t BSGameDataSystemUtility__GetCCFileCount();
const char *BSGameDataSystemUtility__GetCCFile(uint32_t Index);
bool BSGameDataSystemUtility__IsCCFile(const char *Name);

uint32_t sub_1414974E0(BSTArray<void *>& Array, const void *&Target, uint32_t StartIndex, __int64 Unused);
uint32_t sub_1414974E0_SSE41(BSTArray<void *>& Array, const void *&Target, uint32_t StartIndex, __int64 Unused);
bool sub_1415D5640(__int64 a1, uint32_t *a2);
void UpdateLoadProgressBar();
void SortDialogueInfo(__int64 TESDataHandler, uint32_t FormType, int(*SortFunction)(const void *, const void *));
void QuitHandler();
void hk_sub_141047AB2(__int64 FileHandle, __int64 *Value);
void hk_call_14158589F(__int64 Buffer, __int64 *Value);
bool InitItemPerkRankDataVisitor(PerkRankEntry *Entry, uint32_t *FormId, __int64 UnknownArray);
void PerkRankData__LoadFrom(__int64 ArrayHandle, PerkRankEntry *&Entry);
void FaceGenOverflowWarning(__int64 Texture);
void ExportFaceGenForSelectedNPCs(__int64 a1, __int64 a2);
void hk_call_141C68FA6(TESForm_CK *DialogForm, __int64 Unused);
void *hk_call_141C26F3A(void *a1);
void hk_sub_141032ED7(__int64 a1, __int64 a2, __int64 a3);
void *hk_call_1417E42BF(void *a1);
HRESULT DirectX__LoadFromDDSFile(__int64 a1, __int64 a2, __int64 a3, __int64 a4, unsigned int a5, int a6);
void hk_call_141C410A1(__int64 a1, class BSShaderProperty *Property);
void TESObjectWEAP__Data__ConvertCriticalData(__int64 DiskCRDT, __int64 SourceCRDT);
void TESObjectWEAP__Data__LoadCriticalData(__int64 TESFile, __int64 SourceCRDT);
const char *hk_call_1417F4A04(int ActorValueIndex);
uint32_t BSSystemDir__NextEntry(__int64 a1, bool *IsComplete);
bool BSResource__LooseFileLocation__FileExists(const char *CanonicalFullPath, uint32_t *TotalSize);
void hk_call_1412DD706(HWND WindowHandle, uint32_t *ControlId);
std::vector<class TESObjectREFR_CK *> CreateCellPersistentMapCopy(__int64 List);
int sub_141BBF220(__int64 a1, __int64 a2);
int sub_141BBF320(__int64 a1, __int64 a2);
void hk_call_141CF03C9(__int64 a1, bool Enable);
void hk_call_141CE8269(__int64 a1);
const char *hk_call_1416B849E(__int64 a1);
void hk_call_14135CDD3(__int64 RenderWindowInstance, uint32_t *UntypedPointerHandle, bool Select);
int hk_call_1412D1541(__int64 ObjectListInsertData, TESForm_CK *Form);
void hk_call_14147FB57(HWND ListViewHandle, TESForm_CK *Form, bool UseImage, int ItemIndex);
float hk_call_14202E0E8(float Delta);
void BSUtilities__SetLocalAppDataPath(const char *Path);
void hk_call_14130F9E8(uintptr_t a1, bool a2);
void hk_call_14267B359(__int64 SourceNode, __int64 DestNode, __int64 CloningProcess);
size_t hk_call_141A0808C(const char *String);
void TESClass__InitializeEditDialog(TESForm_CK *Class, HWND WindowHandle);
void hk_call_142D12196();
void hk_sub_141481390(HWND ControlHandle);
void hk_call_141434458(__int64 a1, uint32_t Color);
BOOL hk_call_1420AD5C9(HWND RichEditControl, const char *Text);
void NiSkinInstance__LinkObject(__int64 SkinInstance, __int64 Stream);
void hk_call_141299CF5();
void *hk_call_1427D0AC0(__int64 ResourceManager, uint32_t LineCount, __int64 a3, __int64 a4, __int64 a5, __int64 a6, __int64 a7);

template<typename T, bool Stable = false>
void ArrayQuickSortRecursive(BSTArray<T>& Array, int(*SortFunction)(const void *, const void *))
{
	auto compare = [SortFunction](const T& A, const T& B)
	{
		return SortFunction(A, B) == -1;
	};

	if constexpr (Stable)
		std::stable_sort(&Array[0], &Array[Array.QSize()], compare);
	else
		std::sort(&Array[0], &Array[Array.QSize()], compare);
}