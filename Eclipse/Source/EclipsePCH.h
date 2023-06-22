#pragma once

#include <iostream>
#include <chrono>
#include <functional>
#include <memory>
#include <algorithm>
#include <fstream>
#include <cstdint>
#include <limits>
#include <utility>

#include <string>
#include <string_view>
#include <sstream>

#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <vector>
#include <array>
#include <queue>

#include <thread>
#include <mutex>

#ifdef ELS_PLATFORM_WINDOWS
	#define NOMINMAX  
	#include <Windows.h>
#endif

#include <Eclipse/Core/Log.h>