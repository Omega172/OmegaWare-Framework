#pragma once
#include "pch.h"
#include <functional>

#include <nlohmann/json.hpp>
#include <fstream>

class ElementBase
{
public:
	enum class EElementType : uint8_t
	{
		None,
		Button,
		Checkbox,
		Child,
		ColorPicker,
		Combo,
		Hotkey,
		InputText,
		Menu,
		SliderFloat,
		SliderInt,
		Spacing,
		Text,
	};

	enum class ESameLine : uint8_t
	{
		New,      // Force element to be on a new line
		Same,     // Force element to be on the same line
		Dynamic,  // Potentially buggy, only use if you know what you are doing
	};

	typedef struct Style_t
	{
		bool bVisible : 1 = true;
		bool bChildrenVisible : 1 = true;
		ESameLine eSameLine : 2 = ESameLine::Dynamic;

		float flSpacing = -1.f;

		ImVec2 vec2Size = { 100.f, 0.f };
		ImDrawFlags iFlags = 0;
	} Style_t;

	Style_t m_stStyle;

protected:
	// used to display a custom or unlocalized name for elements like buttons
	bool m_bUnlocalizedName = false;
	std::string m_sUnlocalizedName = "";
	std::size_t m_ullLocalizedNameHash = 0;

	// used for internal reference for removing elements or config handling
	std::string m_sUnique = "INVALID_UNIQUE";

	ElementBase* m_pParent = nullptr;
	std::vector<ElementBase*> m_Children;

	inline void SameLine()
	{
		switch (m_stStyle.eSameLine)
		{
			// Force draw on the same line
		case(ESameLine::Same):
			ImGui::SameLine(0.f, m_stStyle.flSpacing);
			break;

			// Ask our parent if we should draw on the same line or not
		case(ESameLine::Dynamic):
			if (m_pParent != nullptr)
				m_pParent->RequestSameLine(this);
			break;

			// Draw on a new line
		case(ESameLine::New):
		default:
			break;
		}
	};

	void ConfigSaveChildren(nlohmann::json& jsonParent) const
	{
		for (ElementBase* const pElement : m_Children)
			pElement->ConfigSave(jsonParent);
	};

	void ConfigLoadChildren(nlohmann::json& jsonParent)
	{
		for (ElementBase* const pElement : m_Children)
			pElement->ConfigLoad(jsonParent);
	};

	void RenderChildren()
	{
		if (!m_stStyle.bChildrenVisible)
			return;

		for (ElementBase* const pElement : m_Children)
			pElement->Render();
	};

public:

	ElementBase() = default;

	void AddElement(void* pElement)
	{
		static_cast<ElementBase*>(pElement)->m_pParent = this;
		m_Children.push_back(static_cast<ElementBase*>(pElement));
	};

	void RemoveElement(std::string sUnique)
	{
		for (auto it = m_Children.begin(); it != m_Children.end(); ++it)
		{
			if ((*it)->m_sUnique != sUnique)
				continue;

			m_Children.erase(it);
			break;
		}
	};

	void LeaveParent()
	{
		if (!m_pParent)
			return;

		for (auto it = m_pParent->m_Children.begin(); it != m_pParent->m_Children.end(); ++it)
		{
			if (*it != this)
				continue;

			m_pParent->m_Children.erase(it);
			break;
		}

		m_pParent = nullptr;
	};

	// Gets internal element name
	inline const std::string GetUnique() const
	{
		return m_sUnique;
	};

	// Gets localized element name
	inline const std::string GetName() const
	{
		if (m_bUnlocalizedName)
			return m_sUnlocalizedName;

		return Localization::Get(m_ullLocalizedNameHash);
	};

	// Sets that we want an unlocalized name and replaces the internal unlocalized name string
	inline void SetName(std::string s)
	{
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = s;
	};

	// Sets that we want to use a localized name and overrides the used localized name
	inline void SetName(size_t ullHash)
	{
		m_bUnlocalizedName = false;
		m_ullLocalizedNameHash = ullHash;
	};

	// Sets if we want to use the localized name or not
	inline void SetName(bool bUseUnlocalized = false)
	{
		m_bUnlocalizedName = bUseUnlocalized;
	};

	inline ElementBase* GetParent() const
	{
		return m_pParent;
	};

	inline const bool HasParent() const
	{
		return m_pParent != nullptr;
	};

	inline const bool HasChildren() const
	{
		return m_Children.size() > 0;
	};

	inline void SetVisible(bool vis)
	{
		m_stStyle.bVisible = vis;
	};

	inline bool IsVisible() const
	{
		return m_stStyle.bVisible;
	};

	inline void SetChildrenVisible(bool vis)
	{
		m_stStyle.bChildrenVisible = vis;
	};

	inline bool IsChildrenVisible() const
	{
		return m_stStyle.bChildrenVisible;
	};

	inline Style_t GetStyle() const
	{
		return m_stStyle;
	};

	inline void SetStyle(Style_t stStyle)
	{
		m_stStyle = stStyle;
	};

	virtual constexpr EElementType GetType() const
	{
		return EElementType::None;
	};

	// Will determine if elements should be placed on the same line or not
	virtual void RequestSameLine(ElementBase* pChild)
	{};

	virtual void ConfigSave(nlohmann::json& jsonParent) const
	{
		if (!HasChildren())
			return;

		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()] = nlohmann::json();

		ConfigSaveChildren(jsonEntry);
	};

	virtual void ConfigLoad(nlohmann::json& jsonParent)
	{
		if (!HasChildren() || !jsonParent.contains(m_sUnique.c_str()))
			return;

		ConfigLoadChildren(jsonParent[m_sUnique.c_str()]);
	};

	virtual void Render()
	{
		if (!m_stStyle.bVisible)
			return;

		RenderChildren();
	};
};

template<typename T>
class ElementInput : public ElementBase
{
protected:
	bool m_bOverride = false;
	T m_ValueOverride{};

	T m_Value{};

	inline std::string ConvertToString() const
	{
		return std::to_string(m_Value);
	};

	inline T ConvertFromString(const std::string& str) const
	{
		return std::stoi(str);
	};

public:
	void ConfigSave(nlohmann::json& jsonParent) const override
	{
		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()] = nlohmann::json();

		jsonEntry["Value"] = ConvertToString();

		if (!HasChildren())
			return;

		jsonEntry["Children"] = nlohmann::json();

		ConfigSaveChildren(jsonEntry["Children"]);
	};

	void ConfigLoad(nlohmann::json& jsonParent) override
	{
		if (!jsonParent.contains(m_sUnique.c_str()))
			return;

		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()];

		if (jsonEntry.contains("Value"))
			m_Value = ConvertFromString(jsonEntry["Value"].get<std::string>());

		if (jsonEntry.contains("Children"))
			ConfigLoadChildren(jsonEntry["Children"]);
	};

	inline T GetValue() const
	{
		return m_bOverride ? m_ValueOverride : m_Value;
	};

	inline void SetValue(const T& Value)
	{
		m_Value = Value;
	};

	inline T GetOverride() const
	{
		return m_ValueOverride;
	};

	inline const bool GetOverrideActive() const
	{
		return m_bOverride;
	};

	inline void SetOverride(const T& Value) const
	{
		m_bOverride = true;
		m_ValueOverride = Value;
	};

	inline void SetOverride() const
	{
		m_bOverride = false;
	};
};

template<>
inline std::string ElementInput<ImVec4>::ConvertToString() const
{
	return std::to_string(ImGui::ColorConvertFloat4ToU32(m_Value));
};

template<>
inline std::string ElementInput<std::string>::ConvertToString() const
{
	return m_Value;
};

template<>
inline ImVec4 ElementInput<ImVec4>::ConvertFromString(const std::string& str) const
{
	return ImGui::ColorConvertU32ToFloat4(std::stoul(str));
};

template<>
inline std::string ElementInput<std::string>::ConvertFromString(const std::string& str) const
{
	return str;
};

template<>
inline float ElementInput<float>::ConvertFromString(const std::string& str) const
{
	return std::stof(str);
};

class Spacing : public ElementBase
{
public:
	Spacing(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
	};

	Spacing(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::Spacing;
	};

	void Render() override
	{
		ImGui::Spacing();
	}
};

class Menu : public ElementBase
{
protected:
	uint8_t m_ucSameLinedElements = 1;
	EElementType m_eLastSameLinedElement = EElementType::None;

	ImVec2 m_vec2MinSize = { 0.f, 0.f };
	ImVec2 m_vec2MaxSize = { 9999.f, 9999.f };

public:

	Menu(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;

		m_vec2MinSize = m_stStyle.vec2Size * 0.75f;
		m_vec2MaxSize = m_stStyle.vec2Size * 2.f;
	};

	Menu(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;

		m_vec2MinSize = m_stStyle.vec2Size * 0.75f;
		m_vec2MaxSize = m_stStyle.vec2Size * 2.f;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::Menu;
	};

	// Will determine if elements should be placed on the same line or not
	void RequestSameLine(ElementBase* pChild) override
	{
		if (!pChild)
			return;

		EElementType eChildType = pChild->GetType();

		// Using the same line for multiple different element types should be done manually
		if (eChildType != m_eLastSameLinedElement)
		{
			m_ucSameLinedElements = 1;
			m_eLastSameLinedElement = eChildType;
			return;
		}

		switch (eChildType)
		{
		case(EElementType::Button):
		case(EElementType::Child):
		{
			if (m_ucSameLinedElements >= 3)
			{
				m_ucSameLinedElements = 1;
				break;
			}

			++m_ucSameLinedElements;
			ImGui::SameLine(0.f, pChild->m_stStyle.flSpacing);

			break;
		}
		default:
			break;
		}
	};

	void Render() override
	{
		m_ucSameLinedElements = 1;
		m_eLastSameLinedElement = EElementType::None;


		ImGui::SetNextWindowSize(m_stStyle.vec2Size);
		ImGui::SetNextWindowSizeConstraints(m_vec2MinSize, m_vec2MaxSize);
		ImGui::Begin(GetName().c_str(), NULL, m_stStyle.iFlags);
		m_stStyle.vec2Size = ImGui::GetWindowSize();

		RenderChildren();
		ImGui::End();
	}
};

class Child : public ElementBase
{
protected:
	ImGuiWindowFlags m_WindowFlags;
	std::function<ImVec2()> m_Callback = nullptr;

public:
	Child(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {}, ImGuiWindowFlags WindowFlags = 0)
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;

		m_WindowFlags = WindowFlags;
	};

	Child(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {}, ImGuiWindowFlags WindowFlags = 0)
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;

		m_WindowFlags = WindowFlags;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::Child;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		if (m_Callback)
			m_stStyle.vec2Size = m_Callback();

		ImGui::BeginChild(GetName().c_str(), m_stStyle.vec2Size, m_stStyle.iFlags, m_WindowFlags);
		RenderChildren();
		ImGui::EndChild();
	};

	void SetCallback(std::function<ImVec2()> Callback)
	{
		m_Callback = Callback;
	};
};

class Text : public ElementBase
{
protected:

public:
	Text(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
	};

	Text(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::Text;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		ImGui::Text(GetName().c_str());
	};

};

class Button : public ElementBase
{
protected:
	std::function<void()> m_Callback = nullptr;

public:
	Button(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
	};

	Button(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::Button;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		if (ImGui::Button(GetName().c_str()))
			if (m_Callback)
				m_Callback();

		RenderChildren();
	};


	void SetCallback(std::function<void()> Callback)
	{
		m_Callback = Callback;
	};
};

class Combo : public ElementBase
{
protected:
	std::string m_sPreviewlabel = "PreviewNotSet";
	std::function<void()> m_Callback;

public:
	Combo(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
	};

	Combo(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::Combo;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		if (ImGui::BeginCombo(GetName().c_str(), m_sPreviewlabel.c_str(), m_stStyle.iFlags))
		{
			if (m_Callback)
				m_Callback();

			ImGui::EndCombo();
		}

		RenderChildren();
	};

	void SetCallback(std::function<void()> Callback)
	{
		m_Callback = Callback;
	};

	void SetPreviewLabel(std::string s)
	{
		m_sPreviewlabel = s;
	};
};

class Checkbox : public ElementInput<bool>
{
protected:

public:
	Checkbox(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
	};

	Checkbox(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::Checkbox;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		ImGui::Checkbox(GetName().c_str(), &m_Value);

		RenderChildren();
	};
};

class Hotkey : public ElementBase
{
public:
	enum class EHotkeyMode : uint8_t
	{
		AlwaysOn,
		Hold,
		Toggle,
		HoldOff,
	};

protected:
	ImGuiKey m_eKey = ImGuiKey_None;
	EHotkeyMode m_eMode = EHotkeyMode::Hold;

	bool m_bSetting = false;
	bool m_bActive = false;

	bool SetKey() noexcept
	{
		for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; ++i) {
			ImGuiKey _key = static_cast<ImGuiKey>(i);
			if (!ImGui::IsKeyPressed(_key))
				continue;

			m_eKey = _key;
			return true;
		}

		return false;
	}

public:
	Hotkey(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
	};

	Hotkey(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::Hotkey;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		if (ImGui::BeginCombo(("##CMB" + GetName()).c_str(), "##", ImGuiComboFlags_NoPreview))
		{
			bool bSelected;

			bSelected = m_eMode == EHotkeyMode::AlwaysOn;
			if (ImGui::Selectable("Always On", bSelected))
				m_eMode = EHotkeyMode::AlwaysOn;

			if (bSelected)
				ImGui::SetItemDefaultFocus();

			bSelected = m_eMode == EHotkeyMode::Hold;
			if (ImGui::Selectable("Hold", bSelected))
				m_eMode = EHotkeyMode::Hold;

			if (bSelected)
				ImGui::SetItemDefaultFocus();

			bSelected = m_eMode == EHotkeyMode::Toggle;
			if (ImGui::Selectable("Toggle", bSelected))
				m_eMode = EHotkeyMode::Toggle;

			if (bSelected)
				ImGui::SetItemDefaultFocus();

			bSelected = m_eMode == EHotkeyMode::HoldOff;
			if (ImGui::Selectable("Hold Off", bSelected))
				m_eMode = EHotkeyMode::HoldOff;

			if (bSelected)
				ImGui::SetItemDefaultFocus();

			ImGui::EndCombo();
		}

		ImGui::SameLine(0.f, 2.f);

		const char* szLabel = GetName().c_str();
		const auto id = ImGui::GetID(szLabel);
		ImGui::PushID(szLabel);

		std::string BtnName = (m_bSetting) ? "..." : ImGui::GetKeyName(m_eKey);

		if (ImGui::GetActiveID() == id) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
			ImGui::Button("...", m_stStyle.vec2Size);
			ImGui::PopStyleColor();

			ImGui::GetCurrentContext()->ActiveIdAllowOverlap = true;
			if (!ImGui::IsItemHovered() && !ImGui::IsItemFocused() && SetKey())
			{
				ImGui::ClearActiveID();
				m_bSetting = false;
			}
		}
		else if (ImGui::Button(BtnName.c_str(), m_stStyle.vec2Size) || m_bSetting) {
			ImGui::SetActiveID(id, ImGui::GetCurrentWindow());
			m_bSetting = true;
		}

		ImGui::SameLine(0.f, 4.f);

		ImGui::Text("%s", GetName().c_str());

		ImGui::PopID();

		RenderChildren();
	};

	void ConfigSave(nlohmann::json& jsonParent) const override
	{
		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()] = nlohmann::json();

		unsigned int v = 0;
		v |= static_cast<unsigned int>(m_eKey) << 2;
		v |= static_cast<unsigned int>(m_eMode);
		jsonEntry["Value"] = std::to_string(v);

		if (!HasChildren())
			return;

		jsonEntry["Children"] = nlohmann::json();

		ConfigSaveChildren(jsonEntry["Children"]);
	};

	void ConfigLoad(nlohmann::json& jsonParent) override
	{
		if (!jsonParent.contains(m_sUnique.c_str()))
			return;

		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()];

		if (jsonEntry.contains("Value"))
		{
			unsigned int v = std::stoi(jsonEntry["Value"].get<std::string>());
			m_eMode = static_cast<EHotkeyMode>(v & 0b11);
			m_eKey = static_cast<ImGuiKey>(v >> 2);
		}

		if (jsonEntry.contains("Children"))
			ConfigLoadChildren(jsonEntry["Children"]);
	};

	void Update()
	{
		if (m_bSetting && m_eMode != EHotkeyMode::AlwaysOn)
		{
			m_bActive = false;
			return;
		}

		switch (m_eMode)
		{
		case (EHotkeyMode::AlwaysOn):
		{
			m_bActive = true;
			return;
		}
		case (EHotkeyMode::Hold):
		{
			m_bActive = ImGui::IsKeyDown(m_eKey);
			return;
		}
		case (EHotkeyMode::HoldOff):
		{
			m_bActive = !ImGui::IsKeyDown(m_eKey);
			return;
		}
		default:
		{
			if (ImGui::IsKeyPressed(m_eKey, false))
				m_bActive = !m_bActive;
			return;
		}
		};
	}

	inline bool GetValue() const
	{
		return m_bActive;
	};
};

class SliderFloat : public ElementInput<float>
{
protected:
	float m_Min;
	float m_Max;
	const char* m_sFormat;

public:
	SliderFloat(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {}, float Value = 0.f, float Min = -1000.f, float Max = 1000.f, const char* sFormat = "%.3f") :
		m_Min(Min), m_Max(Max), m_sFormat(sFormat)
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;

		SetValue(Value);
	};

	SliderFloat(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {}, float Value = 0.f, float Min = -1000.f, float Max = 1000.f, const char* sFormat = "%.3f") :
		m_Min(Min), m_Max(Max), m_sFormat(sFormat)
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;

		SetValue(Value);
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::SliderFloat;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		ImGui::SliderFloat(GetName().c_str(), &m_Value, m_Min, m_Max, m_sFormat, m_stStyle.iFlags);

		RenderChildren();
	};
};

class SliderInt : public ElementInput<int>
{
protected:
	int m_Min;
	int m_Max;
	const char* m_sFormat;

public:
	SliderInt(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {}, int Value = 0, int Min = -1000, int Max = 1000, const char* sFormat = "%d") :
		m_Min(Min), m_Max(Max), m_sFormat(sFormat)
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;

		SetValue(Value);
	};

	SliderInt(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {}, int Value = 0, int Min = -1000, int Max = 1000, const char* sFormat = "%d") :
		m_Min(Min), m_Max(Max), m_sFormat(sFormat)
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;

		SetValue(Value);
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::SliderInt;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		ImGui::SliderInt(GetName().c_str(), &m_Value, m_Min, m_Max, m_sFormat, m_stStyle.iFlags);

		RenderChildren();
	};
};

class InputText : public ElementInput<std::string>
{
protected:
	ImGuiInputTextCallback m_Callback = nullptr;
	void* m_pUserData = nullptr;

public:
	InputText(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {}, size_t ullBufferSize = 1024)
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;

		m_Value.reserve(ullBufferSize);
	};

	InputText(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {}, size_t ullBufferSize = 1024)
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;

		m_Value.resize(ullBufferSize);
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::InputText;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		ImGui::InputText(GetName().c_str(), m_Value.data(), m_Value.capacity(), m_stStyle.iFlags, m_Callback, m_pUserData);

		RenderChildren();
	};

	inline void Resize(const size_t& n)
	{
		m_Value.resize(n);
	};

	void SetCallback(ImGuiInputTextCallback Callback)
	{
		m_Callback = Callback;
	};
};

class ColorPicker : public ElementInput<ImVec4>
{
protected:

public:
	ColorPicker(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
	};

	ColorPicker(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::ColorPicker;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		ImGui::ColorEdit4(GetName().c_str(), reinterpret_cast<float*>(&m_Value), m_stStyle.iFlags);

		RenderChildren();
	};
};