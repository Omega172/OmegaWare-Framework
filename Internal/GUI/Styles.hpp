#pragma once
#include "pch.h"

inline ImFont* CurrentFont;

inline ImFont* TahomaFont;
inline ImFont* TahomaBigFont;
inline ImFont* TahomaFontFeature;

inline void SetupStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 10;
    style.ChildRounding = 6;
    style.FrameRounding = 3;
    style.GrabRounding = 3;
    style.PopupRounding = 6;
    style.TabRounding = 4;
    style.ScrollbarRounding = 3;

    style.ButtonTextAlign = { 0.5f, 0.5f };
    style.WindowTitleAlign = { 0.5f, 0.5f };
    style.FramePadding = { 8.0f, 8.0f };
    style.WindowPadding = { 10.0f, 10.0f };
    style.ItemSpacing = style.WindowPadding;
    style.ItemInnerSpacing = { style.WindowPadding.x, style.FramePadding.y };

    style.WindowBorderSize = 1;
    style.FrameBorderSize = 1;

    style.ScrollbarSize = 12.f;
    style.GrabMinSize = 8.f;

    style.Colors[ImGuiCol_WindowBg] = ImAdd::HexToColorVec4(0x1A1A1A, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = style.Colors[ImGuiCol_WindowBg];
    style.Colors[ImGuiCol_ChildBg] = ImAdd::HexToColorVec4(0x202020, 1.0f);

    style.Colors[ImGuiCol_Text] = ImAdd::HexToColorVec4(0xccccd4, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = style.Colors[ImGuiCol_Text];
    style.Colors[ImGuiCol_TextDisabled] = ImAdd::HexToColorVec4(0x696969, 1.0f);

    style.Colors[ImGuiCol_SliderGrab] = ImAdd::HexToColorVec4(0xA85CFFF, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImAdd::HexToColorVec4(0xA85CFFF, 1.0f);

    style.Colors[ImGuiCol_ScrollbarGrab] = style.Colors[ImGuiCol_SliderGrab];
    style.Colors[ImGuiCol_ScrollbarGrabActive] = style.Colors[ImGuiCol_SliderGrabActive];
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);

    style.Colors[ImGuiCol_TextSelectedBg] = style.Colors[ImGuiCol_SliderGrab];

    style.Colors[ImGuiCol_Border] = ImAdd::HexToColorVec4(0x323232, 1.0f);
    style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];

    style.Colors[ImGuiCol_Button] = ImAdd::HexToColorVec4(0x1A1A1A, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImAdd::HexToColorVec4(0x1A1A1A, 0.7f);
    style.Colors[ImGuiCol_ButtonActive] = ImAdd::HexToColorVec4(0x1A1A1A, 0.5f);

    style.Colors[ImGuiCol_FrameBg] = style.Colors[ImGuiCol_Button];
    style.Colors[ImGuiCol_FrameBgHovered] = style.Colors[ImGuiCol_ButtonHovered];
    style.Colors[ImGuiCol_FrameBgActive] = style.Colors[ImGuiCol_ButtonActive];

    style.Colors[ImGuiCol_Header] = ImAdd::HexToColorVec4(0x292929, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImAdd::HexToColorVec4(0x292929, 0.7f);
    style.Colors[ImGuiCol_HeaderActive] = ImAdd::HexToColorVec4(0x292929, 0.5f);

    style.Colors[ImGuiCol_TitleBg] = style.Colors[ImGuiCol_ChildBg];
    style.Colors[ImGuiCol_TitleBgActive] = style.Colors[ImGuiCol_TitleBg];
}

inline void ImportFonts()
{
	ImGuiIO& io = ImGui::GetIO();

    ImFontConfig cfg;
    cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_ForceAutoHint;

    // Build merged glyph ranges for all languages
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());        // ASCII + Latin-1
    builder.AddRanges(io.Fonts->GetGlyphRangesVietnamese());     // Vietnamese
    builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());       // Russian, Ukrainian, etc.
    builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());       // Japanese
    builder.AddRanges(io.Fonts->GetGlyphRangesKorean());         // Korean
    builder.AddRanges(io.Fonts->GetGlyphRangesChineseFull());    // Chinese
    builder.AddRanges(io.Fonts->GetGlyphRangesThai());           // Thai
    builder.AddRanges(io.Fonts->GetGlyphRangesGreek());          // Greek
    
    // Latin Extended-A for Polish (ą, ć, ę, ł, ń, ó, ś, ź, ż) and other European languages
    static const ImWchar latinExtendedA[] = { 0x0100, 0x017F, 0 };
    builder.AddRanges(latinExtendedA);
    
    static ImVector<ImWchar> mergedRanges;
    builder.BuildRanges(&mergedRanges);

    TahomaFont = io.Fonts->AddFontFromMemoryCompressedTTF(Poppins_Medium_compressed_data, Poppins_Medium_compressed_size, 14, &cfg, mergedRanges.Data);

    // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    static const ImWchar icons_ranges_brands[] = { ICON_MIN_FAB, ICON_MAX_16_FAB, 0 };

    ImFontConfig fa_config; fa_config.MergeMode = true; fa_config.PixelSnapH = true;
    fa_config.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_ForceAutoHint;

    ImFont* fontAwesome = io.Fonts->AddFontFromMemoryCompressedTTF(fa6_solid_compressed_data, fa6_solid_compressed_size, 14, &fa_config, icons_ranges);
    ImFont* fontAwesomeBrands = io.Fonts->AddFontFromMemoryCompressedTTF(fa_brands_400_compressed_data, fa_brands_400_compressed_size, 14, &fa_config, icons_ranges_brands);

    TahomaBigFont = io.Fonts->AddFontFromMemoryCompressedTTF(Poppins_Medium_compressed_data, Poppins_Medium_compressed_size, 20, &cfg, mergedRanges.Data);

    TahomaFontFeature = TahomaFont;
}