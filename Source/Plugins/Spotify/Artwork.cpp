#include "Artwork.hpp"

#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cmath>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace Spotify::Artwork
{
	namespace
	{
		using Microsoft::WRL::ComPtr;

		struct stHsl
		{
			float flHue = 0.f;
			float flSaturation = 0.f;
			float flLightness = 0.f;
		};

		[[nodiscard]] stHsl ToHsl(float flRed, float flGreen, float flBlue)
		{
			const float flMax = std::max({ flRed, flGreen, flBlue });
			const float flMin = std::min({ flRed, flGreen, flBlue });
			stHsl stResult;
			stResult.flLightness = (flMax + flMin) * .5f;
			const float flDelta = flMax - flMin;
			if (flDelta <= 0.f)
				return stResult;
			stResult.flSaturation = flDelta / (1.f - std::abs(2.f * stResult.flLightness - 1.f));
			if (flMax == flRed)
				stResult.flHue = std::fmod((flGreen - flBlue) / flDelta, 6.f);
			else if (flMax == flGreen)
				stResult.flHue = (flBlue - flRed) / flDelta + 2.f;
			else
				stResult.flHue = (flRed - flGreen) / flDelta + 4.f;
			stResult.flHue *= 60.f;
			if (stResult.flHue < 0.f)
				stResult.flHue += 360.f;
			return stResult;
		}

		[[nodiscard]] std::uint32_t FromHsl(const stHsl& rHsl)
		{
			const float flChroma = (1.f - std::abs(2.f * rHsl.flLightness - 1.f)) * rHsl.flSaturation;
			const float flSector = rHsl.flHue / 60.f;
			const float flX = flChroma * (1.f - std::abs(std::fmod(flSector, 2.f) - 1.f));
			std::array<float, 3> aRgb{};
			if (flSector < 1.f)
				aRgb = { flChroma, flX, 0.f };
			else if (flSector < 2.f)
				aRgb = { flX, flChroma, 0.f };
			else if (flSector < 3.f)
				aRgb = { 0.f, flChroma, flX };
			else if (flSector < 4.f)
				aRgb = { 0.f, flX, flChroma };
			else if (flSector < 5.f)
				aRgb = { flX, 0.f, flChroma };
			else
				aRgb = { flChroma, 0.f, flX };
			const float flOffset = rHsl.flLightness - flChroma * .5f;
			std::uint32_t uColor = 0xFF000000u;
			for (std::size_t i = 0; i < aRgb.size(); ++i)
				uColor |= static_cast<std::uint32_t>(std::clamp((aRgb[i] + flOffset) * 255.f + .5f, 0.f, 255.f)) << (i * 8);
			return uColor;
		}

		[[nodiscard]] std::uint32_t Pack(float flRed, float flGreen, float flBlue)
		{
			const auto fnByte = [](float flValue) { return static_cast<std::uint32_t>(std::clamp(flValue * 255.f + .5f, 0.f, 255.f)); };
			return fnByte(flRed) | (fnByte(flGreen) << 8) | (fnByte(flBlue) << 16) | 0xFF000000u;
		}
	}

	std::uint32_t PrimaryColor(std::span<const std::uint8_t> aPixelsRGBA)
	{
		struct stBucket
		{
			std::uint32_t uCount = 0;
			std::uint64_t uRed = 0;
			std::uint64_t uGreen = 0;
			std::uint64_t uBlue = 0;
		};
		std::vector<stBucket> aBuckets(4096);
		std::uint64_t uTotal = 0;
		std::uint64_t uRed = 0;
		std::uint64_t uGreen = 0;
		std::uint64_t uBlue = 0;
		for (std::size_t i = 0; i + 3 < aPixelsRGBA.size(); i += 4)
		{
			if (aPixelsRGBA[i + 3] < 128)
				continue;
			const std::uint8_t byRed = aPixelsRGBA[i];
			const std::uint8_t byGreen = aPixelsRGBA[i + 1];
			const std::uint8_t byBlue = aPixelsRGBA[i + 2];
			stBucket& rBucket = aBuckets[(static_cast<std::size_t>(byRed >> 4) << 8) | (static_cast<std::size_t>(byGreen >> 4) << 4) | static_cast<std::size_t>(byBlue >> 4)];
			++rBucket.uCount;
			rBucket.uRed += byRed;
			rBucket.uGreen += byGreen;
			rBucket.uBlue += byBlue;
			++uTotal;
			uRed += byRed;
			uGreen += byGreen;
			uBlue += byBlue;
		}
		if (!uTotal)
			return 0xFFFFFFFFu;

		std::uint32_t uMaxCount = 0;
		for (const stBucket& rBucket : aBuckets)
			uMaxCount = std::max(uMaxCount, rBucket.uCount);
		const std::uint64_t uMinCount = std::max<std::uint64_t>(1, uTotal / 1000);

		float flBestScore = -1.f;
		std::uint32_t uBest = 0;
		for (const stBucket& rBucket : aBuckets)
		{
			if (rBucket.uCount < uMinCount)
				continue;
			const float flRed = static_cast<float>(rBucket.uRed) / static_cast<float>(rBucket.uCount) / 255.f;
			const float flGreen = static_cast<float>(rBucket.uGreen) / static_cast<float>(rBucket.uCount) / 255.f;
			const float flBlue = static_cast<float>(rBucket.uBlue) / static_cast<float>(rBucket.uCount) / 255.f;
			const stHsl stColor = ToHsl(flRed, flGreen, flBlue);
			if (stColor.flSaturation < .35f || stColor.flLightness < .3f || stColor.flLightness > .7f)
				continue;
			const float flScore = ((1.f - std::abs(stColor.flSaturation - 1.f)) * 3.f + (1.f - std::abs(stColor.flLightness - .5f)) * 6.5f + static_cast<float>(rBucket.uCount) / static_cast<float>(uMaxCount) * .5f) / 10.f;
			if (flScore > flBestScore)
			{
				flBestScore = flScore;
				uBest = Pack(flRed, flGreen, flBlue);
			}
		}
		if (flBestScore >= 0.f)
			return uBest;

		const float flTotal = static_cast<float>(uTotal) * 255.f;
		stHsl stAverage = ToHsl(static_cast<float>(uRed) / flTotal, static_cast<float>(uGreen) / flTotal, static_cast<float>(uBlue) / flTotal);
		stAverage.flLightness = std::clamp(stAverage.flLightness, .45f, .8f);
		return FromHsl(stAverage);
	}

	std::optional<stImage> Decode(std::span<const std::uint8_t> aData, int iMaxSize)
	{
		if (aData.empty() || aData.size() > 0xFFFFFFFFu)
			return std::nullopt;
		ComPtr<IWICImagingFactory> pFactory;
		if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory))))
			return std::nullopt;
		ComPtr<IWICStream> pStream;
		if (FAILED(pFactory->CreateStream(&pStream)) || FAILED(pStream->InitializeFromMemory(const_cast<BYTE*>(aData.data()), static_cast<DWORD>(aData.size()))))
			return std::nullopt;
		ComPtr<IWICBitmapDecoder> pDecoder;
		if (FAILED(pFactory->CreateDecoderFromStream(pStream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &pDecoder)))
			return std::nullopt;
		ComPtr<IWICBitmapFrameDecode> pFrame;
		if (FAILED(pDecoder->GetFrame(0, &pFrame)))
			return std::nullopt;
		UINT uWidth = 0;
		UINT uHeight = 0;
		if (FAILED(pFrame->GetSize(&uWidth, &uHeight)) || !uWidth || !uHeight)
			return std::nullopt;

		ComPtr<IWICBitmapSource> pSource = pFrame;
		const UINT uLargest = std::max(uWidth, uHeight);
		if (iMaxSize > 0 && uLargest > static_cast<UINT>(iMaxSize))
		{
			const UINT uScaledWidth = std::max<UINT>(1, uWidth * static_cast<UINT>(iMaxSize) / uLargest);
			const UINT uScaledHeight = std::max<UINT>(1, uHeight * static_cast<UINT>(iMaxSize) / uLargest);
			ComPtr<IWICBitmapScaler> pScaler;
			if (FAILED(pFactory->CreateBitmapScaler(&pScaler)) || FAILED(pScaler->Initialize(pSource.Get(), uScaledWidth, uScaledHeight, WICBitmapInterpolationModeFant)))
				return std::nullopt;
			pSource = pScaler;
			uWidth = uScaledWidth;
			uHeight = uScaledHeight;
		}

		ComPtr<IWICFormatConverter> pConverter;
		if (FAILED(pFactory->CreateFormatConverter(&pConverter)) || FAILED(pConverter->Initialize(pSource.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
			return std::nullopt;

		stImage stResult;
		stResult.iWidth = static_cast<int>(uWidth);
		stResult.iHeight = static_cast<int>(uHeight);
		stResult.aPixels.resize(static_cast<std::size_t>(uWidth) * uHeight * 4);
		if (FAILED(pConverter->CopyPixels(nullptr, uWidth * 4, static_cast<UINT>(stResult.aPixels.size()), stResult.aPixels.data())))
			return std::nullopt;
		stResult.uPrimary = PrimaryColor(stResult.aPixels);
		return stResult;
	}
}
