#include "common.h"

ID3D11DeviceContext2 *lastUpdate = (ID3D11DeviceContext2 *)0xDEADBEEFCAFEDEAD;
std::unordered_map<uint32_t, ID3D11DeviceContext2 **> ContextAddrs;

thread_local char BSGraphics_TLSGlob[0x4000];