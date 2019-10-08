#pragma once

bool PatchNullsub(uintptr_t SourceAddress, uintptr_t TargetFunction, bool Extended);
void ExperimentalPatchOptimizations();