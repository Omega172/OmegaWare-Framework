#pragma once
#include "Logging/Logging.h"
#include "Console/Console.h"

namespace Utils
{
	bool IsReadableMemory(void* ptr, size_t byteCount);
	bool IsReadableMemory(const void* ptr, size_t byteCount);
}