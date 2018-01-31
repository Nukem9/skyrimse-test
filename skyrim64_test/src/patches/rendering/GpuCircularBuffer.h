#pragma once

#include "../../common.h"

//
// Idea implemented from http://gamedevs.org/uploads/efficient-buffer-management.pdf
// "Don’t Throw it all Away: Efficient Buffer Management"
//
class GpuCircularBuffer
{
public:
	D3D11_MAPPED_SUBRESOURCE Map;
	D3D11_BUFFER_DESC Description;
	ID3D11Buffer *D3DBuffer = nullptr;

	uint32_t *FrameUtilizedAmounts = nullptr;	// History tracker for how many bytes each frame is using
	uint32_t CurrentOffset = 0;					// Byte offset into the buffer
	uint32_t CurrentUtilized = 0;				// Number of bytes possibly in use by GPU (cannot be written)
	uint32_t CurrentAvailable = 0;				// Number of unused bytes (as guaranteed by Initialize()/FreeOldFrame())

	GpuCircularBuffer(ID3D11Device *Device, uint32_t Type, uint32_t BufferSize, uint32_t MaxFrames);
	~GpuCircularBuffer();

	void *MapData(ID3D11DeviceContext *Context, uint32_t AllocationSize, uint32_t *AllocationOffset, bool ForceRemap);
	void UnmapData(ID3D11DeviceContext *Context);
	void SwapFrame(uint32_t FrameIndex);
	void FreeOldFrame(uint32_t FrameIndex);
};