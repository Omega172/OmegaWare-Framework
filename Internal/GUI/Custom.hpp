#pragma once
#include "pch.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../Libs/ImGUI/imgui_internal.h"


#include <algorithm>
#include <array>
#include <string_view>

// I have no clue where I got this from but I fixed it up a bit to work with newer changes to the ImGui API, and cleaned it up a bit
class KeyBind
{
public:
    ImGuiKey key;
    bool bSetting = false;

    explicit KeyBind(ImGuiKey _key = ImGuiKey_None) noexcept : key{ _key } {};

    const char* ToString() const noexcept { return ImGui::GetKeyName(key); };

    bool IsPressed(bool repeat = false) const noexcept
    {
        if (!IsSet())
            return false;

        return ImGui::IsKeyPressed(key, repeat);
    }

    bool IsDown() const noexcept
    {
        if (!IsSet())
            return false;

        return ImGui::IsKeyDown(key);
    }

    bool IsSet() const noexcept { return key != ImGuiKey_None; }

    void Set(ImGuiKey _key) { key = _key; }

    bool Set() noexcept
    {
        for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; ++i) {
            ImGuiKey _key = static_cast<ImGuiKey>(i);
            if (!ImGui::IsKeyPressed(_key))
                continue;

            key = _key;
            return true;
        }

        return false;
    }
};

class KeyBindToggle : public KeyBind {
public:
    using KeyBind::KeyBind;

    inline void HandleToggle() noexcept;
    bool IsToggled() const noexcept { return toggledOn; }
private:
    bool toggledOn = false;
};

inline void KeyBindToggle::HandleToggle() noexcept
{
    if (IsPressed())
        toggledOn = !toggledOn;
}

namespace ImGui
{
    inline void Hotkey(const char* label, KeyBind* key, const ImVec2& size = { 100.0f, 0.0f }) noexcept
    {
        const auto id = GetID(label);
        PushID(label);

        std::string BtnName = (key->bSetting) ? "..." : key->ToString();

        if (GetActiveID() == id) {
            PushStyleColor(ImGuiCol_Button, GetColorU32(ImGuiCol_ButtonActive));
            Button("...", size);
            PopStyleColor();

            GetCurrentContext()->ActiveIdAllowOverlap = true;
            if (!IsItemHovered() && !IsItemFocused() && key->Set())
            {
                ClearActiveID();
                key->bSetting = false;
            }
        }
        else if (Button(BtnName.c_str(), size) || key->bSetting) {
            SetActiveID(id, GetCurrentWindow());
            key->bSetting = true;
        }

        PopID();
    }

    inline void OutlinedText(ImVec2 Pos, ImU32 Color, const char* Text)
    {
        ImU32 Black = ColorConvertFloat4ToU32({ 0.f, 0.f, 0.f, 1.f });
        auto* pDL = GetBackgroundDrawList();

        pDL->AddText(CurrentFontESP, CurrentFontESP->FontSize, Pos + ImVec2(-1, -1), Black, Text);
        pDL->AddText(CurrentFontESP, CurrentFontESP->FontSize, Pos + ImVec2(1, -1), Black, Text);
        pDL->AddText(CurrentFontESP, CurrentFontESP->FontSize, Pos + ImVec2(-1, 1), Black, Text);
        pDL->AddText(CurrentFontESP, CurrentFontESP->FontSize, Pos + ImVec2(1, 1), Black, Text);

        pDL->AddText(CurrentFontESP, CurrentFontESP->FontSize, Pos, Color, Text);
    }

    static ImVector<ImRect> s_GroupPanelLabelStack;

    inline void BeginGroupPanel(const char* name, ImVec2 size = ImVec2(-1.f, -1.f))
    {
        auto CursorPos = ImGui::GetCursorPos();
        size.x -= 16.f;
        size.y -= 4.f;

        ImGui::BeginGroup();

        auto itemSpacing = ImGui::GetStyle().ItemSpacing;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

        auto frameHeight = ImGui::GetFrameHeight();
        
        ImGui::BeginGroup();

        ImGui::Dummy(ImVec2(size.x, 0.0f));

        ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::BeginGroup();

        CursorPos = ImGui::GetCursorPos();
        ImGui::Dummy(size);
        ImGui::SetCursorPos(CursorPos);

        ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::TextUnformatted(name);
        auto labelMin = ImGui::GetItemRectMin();
        auto labelMax = ImGui::GetItemRectMax();
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
        ImGui::BeginGroup();

        //ImGui::GetWindowDrawList()->AddRect(labelMin, labelMax, IM_COL32(255, 0, 255, 255));

        ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
        ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->WorkRect.Max.x -= frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->InnerRect.Max.x -= frameHeight * 0.5f;
#else
        ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x -= frameHeight * 0.5f;
#endif
        ImGui::GetCurrentWindow()->Size.x -= frameHeight;

        auto itemWidth = ImGui::CalcItemWidth();
        ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight));

        s_GroupPanelLabelStack.push_back(ImRect(labelMin, labelMax));
    }

    inline void EndGroupPanel(ImVec2 size)
    {
        ImGui::PopItemWidth();

        auto itemSpacing = ImGui::GetStyle().ItemSpacing;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

        auto frameHeight = ImGui::GetFrameHeight();

        ImGui::EndGroup();

        //ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(0, 255, 0, 64), 4.0f);

        ImGui::EndGroup();

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
        ImGui::Dummy(ImVec2(0.0, frameHeight - frameHeight * 0.5f - itemSpacing.y));

        ImGui::EndGroup();

        auto itemMin = ImGui::GetItemRectMin();
        auto itemMax = ImGui::GetItemRectMax();
        //ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(255, 0, 0, 64), 4.0f);

        auto labelRect = s_GroupPanelLabelStack.back();
        s_GroupPanelLabelStack.pop_back();

        ImVec2 halfFrame = ImVec2(frameHeight * 0.25f, frameHeight) * 0.5f;
        ImRect frameRect = ImRect(itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, 0.0f));
        labelRect.Min.x -= itemSpacing.x;
        labelRect.Max.x += itemSpacing.x;
        for (int i = 0; i < 4; ++i)
        {
            switch (i)
            {
                // left half-plane
            case 0: ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true); break;
                // right half-plane
            case 1: ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true); break;
                // top
            case 2: ImGui::PushClipRect(ImVec2(labelRect.Min.x, -FLT_MAX), ImVec2(labelRect.Max.x, labelRect.Min.y), true); break;
                // bottom
            case 3: ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true); break;
            }

            ImGui::GetWindowDrawList()->AddRect(
                frameRect.Min, frameRect.Max,
                ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)),
                halfFrame.x);

            ImGui::PopClipRect();
        }

        ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
        ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->WorkRect.Max.x += frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->InnerRect.Max.x += frameHeight * 0.5f;
#else
        ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x += frameHeight * 0.5f;
#endif
        ImGui::GetCurrentWindow()->Size.x += frameHeight;

        ImGui::Dummy(ImVec2(0.0f, 0.0f));

        ImGui::EndGroup();
    }
}