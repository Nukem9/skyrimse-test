#pragma once

namespace Experimental
{
	struct NullsubPatch
	{
		std::initializer_list<uint8_t> Signature;
		uint8_t JumpPatch[5];
		uint8_t CallPatch[5];
	};

	void RunOptimizations();

	uint64_t PatchEditAndContinue();
	uint64_t PatchMemInit();
	uint64_t PatchLinkedList();
	uint64_t PatchTemplatedFormIterator();

	const NullsubPatch *FindNullsubPatch(uintptr_t SourceAddress, uintptr_t TargetFunction);
	bool PatchNullsub(uintptr_t SourceAddress, uintptr_t TargetFunction, const NullsubPatch *Patch = nullptr);
}