#pragma once

#define threadsafe
#define threadsafe_const const

#include <d3d12.h>
#include <dxgi1_5.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>

#include <cstdint>
#include <string>
#include <wrl.h>
#include <process.h>
#include <shellapi.h>
#include <algorithm>
#include <deque>
#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <queue>

#define MAX_FRAME_LATENCY    1
#define MAX_FRAMES_IN_FLIGHT (MAX_FRAME_LATENCY + 1)    // Current and Last
#define SINGLETHREADED FALSE


#include"System\CLog.h"
#include"System\System.h"

#include"..\Common\FrostDX12.h"
#include"..\Extra\d3dx12.h"

#include"Common\Platform\FrostWindows.h"
#include"Common\Platform\platform.h"
#include"Common\Thread\FrostAtomics_win32.h"
#include"Common\Thread\FrostThread_win32.h"
#include"Common\Thread\FrostThread.h"
#include"Common\Thread\FrostThreadUtil_win32.h"
#include"Common\3rdParty\smartptr.h"

#include"System\SystemThreading.h"
