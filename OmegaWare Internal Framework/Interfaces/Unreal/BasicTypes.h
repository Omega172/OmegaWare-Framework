#pragma once

#include "SDK.h"

// Usually not enabled, mostly for debug purposes
#ifndef WITH_CASE_PRESERVING_NAME
#define WITH_CASE_PRESERVING_NAME 0
#endif

#if WITH_CASE_PRESERVING_NAME
static_assert(sizeof(CG::FName) != 0x000008, "FName size does not reflect WITH_CASE_PRESERVING_NAME additions.");
#endif
// Aquired code from unknown sources
namespace CG {
	using FNameEntryId = uint32_t;

	struct FNameEntryHeader {
		uint16_t bIsWide : 1;
#if WITH_CASE_PRESERVING_NAME
		uint16_t Len : 15;
#else
		static const constexpr uint32_t ProbeHashBits = 5;
		uint16_t LowercaseProbeHash : ProbeHashBits;
		uint16_t Len : 10;
#endif
	};

	struct FNameEntryHandle {
		uint32_t Block = 0;
		uint32_t Offset = 0;

	};

	class FNameEntry {
	public:
		enum { NAME_SIZE = 1024 };

#if WITH_CASE_PRESERVING_NAME
		FNameEntryId ComparisonId;
#endif
		FNameEntryHeader Header;
		union {
			char AnsiName[NAME_SIZE];
			wchar_t WideName[NAME_SIZE];
		};

		int32_t GetLength() const {
			if (Header.Len > 1024) [[unlikely]] {
				return 0;
			}

			return Header.Len;
		};

		bool IsWide() const {
			return Header.bIsWide;
		};

		std::string GetAnsiName() const {
			return std::string(const_cast<const char*>(AnsiName), GetLength());
		};

		std::wstring GetWideName() const {
			return std::wstring(const_cast<const wchar_t*>(WideName), GetLength());
		};
	};

	class FNameEntryAllocator {
	private:
		uint8_t FrwLock[0x8];
	public:
		enum { Stride = alignof(FNameEntry) };
		enum { BlockSizeBytes = Stride * (1 << 16) }; // Stride * FNameBlockOffsets

		uint32_t CurrentBlock = 0;
		uint32_t CurrentByteCursor = 0;
		uint8_t* Blocks[1 << 13] = {}; // Blocks[FNameMaxBlocks]

		// Returns the number of blocks that have been allocated so far for names.
		uint32_t NumBlocks() const {
			return CurrentBlock + 1;
		};

		FNameEntry* GetById(int32_t key) const {
			int32_t block = key >> 16;
			uint16_t offset = static_cast<uint16_t>(key);

			if (!IsValidIndex(key, block, offset)) {
				return reinterpret_cast<FNameEntry*>(Blocks[0] + 0); // "None"
			};

			return reinterpret_cast<FNameEntry*>(Blocks[block] + (static_cast<uint64_t>(offset) * Stride));
		};

		bool IsValidIndex(int32_t key) const {
			return IsValidIndex(key, key >> 16, key);
		};

	private:
		bool IsValidIndex(int32_t key, uint32_t block, uint16_t offset) const {
			return (key >= 0 && block < static_cast<uint32_t>(NumBlocks()) && offset * Stride < BlockSizeBytes);
		};
	};


	class FNamePool {
	public:
		enum { MaxENames = 512 };

		FNameEntryAllocator Allocator;
		uint32_t AnsiCount;
		uint32_t WideCount;

		FNameEntry* GetNext(uintptr_t& nextFNameAddress, uint32_t* comparisonId) const {
			static int lastBlock = 0;
			if (!nextFNameAddress) {
				lastBlock = 0;
				nextFNameAddress = reinterpret_cast<uintptr_t>(Allocator.Blocks[0]);
			}

		RePlay:
			int32_t nextFNameComparisonId = MAKELONG(
				(uint16_t)((nextFNameAddress - reinterpret_cast<uintptr_t>(Allocator.Blocks[lastBlock])) / 2),
				(uint16_t)lastBlock
			);
			int32_t block = nextFNameComparisonId >> 16;
			int32_t offset = (uint16_t)nextFNameComparisonId;
			int32_t offsetFromBlock = static_cast<int32_t>(nextFNameAddress - reinterpret_cast<uintptr_t>(Allocator.Blocks[lastBlock]));

			const uintptr_t entryOffset = nextFNameAddress;
			const int toAdd = 0x0 + 0x2; // HeaderOffset + HeaderSize
			const uint16_t nameHeader = *reinterpret_cast<uint16_t*>(entryOffset);
			int nameLength = nameHeader >> 6;
			bool isWide = (nameHeader & 1) != 0;

			if (isWide) {
				nameLength += nameLength;
			}

			if (!isWide && nameLength % 2 != 0) {
				nameLength += 1;
			}

			if (offsetFromBlock + toAdd + (nameLength * 2) >= 0xffff * FNameEntryAllocator::Stride || nameHeader == 0x00 || block == Allocator.CurrentBlock && offset >= Allocator.CurrentByteCursor) {
				nextFNameAddress = reinterpret_cast<uintptr_t>(Allocator.Blocks[++lastBlock]);
				goto RePlay;
			}

			// We hit last Name in last Block
			if (lastBlock > Allocator.CurrentBlock) {
				return nullptr;
			}

			// Get next name address
			nextFNameAddress = entryOffset + toAdd + nameLength;

			// Get name
			FNameEntry* ret = Allocator.GetById(nextFNameComparisonId);

			if (comparisonId) {
				*comparisonId = nextFNameComparisonId;
			}

			return ret;
		};

		int32_t Count() const {
			return AnsiCount;
		};

		bool IsValidIndex(int32_t index) const {
			return Allocator.IsValidIndex(index);
		};

		FNameEntry* GetById(int32_t id) const {
			return Allocator.GetById(id);
		};

		FNameEntry* operator[](int32_t id) const {
			return GetById(id);
		};
	};
}