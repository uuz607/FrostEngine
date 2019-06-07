#pragma once


#ifdef _DEBUG
#define DX12_LOG(cond, ...) \
		do { if (cond) { FrostLog("DX12Log:"__VA_ARGS__); } } while(0)
#define DX12_ERROR(...) \
		do { FrostLog("DX12 Error:"__VA_ARGS__); } while (0)
#define DX12_ASSERT(cond, ...) \
		do { if (!(cond)) { DX12_ERROR(__VA_ARGS__); FROST_ASSERT(0); __debugbreak(); } } while (0)
#define DX12_WARNING(cond, ...) \
		do { if (!(cond)) { DX12_LOG(__VA_ARGS__); } } while (0)
#else
#define DX12_LOG(cond, ...) do {} while (0)
#define DX12_ERROR(...)     do {} while (0)
#define DX12_ASSERT(cond, ...)
#define DX12_WARNING(cond, ...)
#endif

#define DX12_NOT_IMPLEMENTED DX12_ASSERT(0, "Not implemented!");