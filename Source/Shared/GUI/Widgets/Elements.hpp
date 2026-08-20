#pragma once
#include "pch.h"

inline constexpr float kMinSidebarWidth = 210.f;
inline float kSidebarWidth = kMinSidebarWidth;

inline ImVec4 vec4ChromeBg = ImVec4(0.f, 0.f, 0.f, 1.f);

class PageManager
{
public:
	struct PageInfo
	{
		uint8_t iPageId;
		size_t ullLocalizedNameHash;
		std::string sUnlocalizedName;
		const char* sIcon;
		bool bUseUnlocalizedName;
	};

	static inline std::vector<PageInfo> s_RegisteredPages;
	static inline uint8_t s_iNextPageId = 4;

	static uint8_t RegisterPage(size_t ullLocalizedNameHash, const char* sIcon = ICON_FA_CIRCLE)
	{
		uint8_t iNewPageId = s_iNextPageId++;
		s_RegisteredPages.push_back({ iNewPageId, ullLocalizedNameHash, "", sIcon, false });
		return iNewPageId;
	}

	static uint8_t RegisterPage(std::string sUnlocalizedName, const char* sIcon = ICON_FA_CIRCLE)
	{
		uint8_t iNewPageId = s_iNextPageId++;
		s_RegisteredPages.push_back({ iNewPageId, 0, sUnlocalizedName, sIcon, true });
		return iNewPageId;
	}

	static const std::vector<PageInfo>& GetPages()
	{
		return s_RegisteredPages;
	}
};

class ElementBase
{
public:
	enum class EElementType : uint8_t
	{
		None,
		Button,
		AccentButton,
		Checkbox,
		Toggle,
		Child,
		ColorPicker,
		Combo,
		Hotkey,
		InputText,
		Menu,
		RadioButtonIcon,
		HeaderGroup,
		Body,
		Page,
		Group,
		GroupChild,
		SliderFloat,
		SliderInt,
		Separator,
		SeperatorText,
		Spacing,
		Text,
		Table,
		ConfigManager,
	};

	enum class ESameLine : uint8_t
	{
		New,
		Same,
		Dynamic,
	};

	typedef struct Style_t
	{
		bool bVisible : 1 = true;
		bool bChildrenVisible : 1 = true;
		ESameLine eSameLine : 2 = ESameLine::Dynamic;

		float flOffset = 0.f;
		float flSpacing = -1.f;

		ImVec2 vec2Size = { 0.f, 0.f };
		ImDrawFlags iFlags = 0;
	} Style_t;

	Style_t m_stStyle;

	typedef struct Header_t
	{
		uint8_t m_iParentPageID;
		std::vector<size_t> m_ullLocalizedNameHashes;
	}  Header_t;

protected:
	bool m_bUnlocalizedName = false;
	std::string m_sUnlocalizedName = "";
	std::size_t m_ullLocalizedNameHash = 0;

	std::string m_sUnique = "INVALID_UNIQUE";

	ElementBase* m_pParent = nullptr;
	std::vector<ElementBase*> m_Children;

	inline static uint8_t eCurrentPage = 0;
	inline static uint8_t eCurrentSubPage = 0;

	std::function<ImVec2()> m_SameLineSizeCallback = nullptr;

	inline void SameLine()
	{
		switch (m_stStyle.eSameLine)
		{
		case(ESameLine::Same):
			if (m_SameLineSizeCallback)
				ImGui::SameLine(m_SameLineSizeCallback().x, m_SameLineSizeCallback().y);
			else
				ImGui::SameLine(m_stStyle.flOffset, m_stStyle.flSpacing);
			break;

		case(ESameLine::Dynamic):
			if (m_pParent != nullptr)
				m_pParent->RequestSameLine(this);
			break;

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

	void InsertElementAt(void* pElement, size_t index)
	{
		static_cast<ElementBase*>(pElement)->m_pParent = this;
		if (index >= m_Children.size())
			m_Children.push_back(static_cast<ElementBase*>(pElement));
		else
			m_Children.insert(m_Children.begin() + index, static_cast<ElementBase*>(pElement));
	};

	void InsertElementBefore(void* pElement, const std::string& sBeforeUnique)
	{
		auto pElementBase = static_cast<ElementBase*>(pElement);
		pElementBase->m_pParent = this;

		for (auto it = m_Children.begin(); it != m_Children.end(); ++it)
		{
			if ((*it)->m_sUnique == sBeforeUnique)
			{
				m_Children.insert(it, pElementBase);
				return;
			}
		}

		m_Children.push_back(pElementBase);
	};

	void InsertElementAfter(void* pElement, const std::string& sAfterUnique)
	{
		auto pElementBase = static_cast<ElementBase*>(pElement);
		pElementBase->m_pParent = this;

		for (auto it = m_Children.begin(); it != m_Children.end(); ++it)
		{
			if ((*it)->m_sUnique == sAfterUnique)
			{
				m_Children.insert(it + 1, pElementBase);
				return;
			}
		}

		m_Children.push_back(pElementBase);
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

	inline const std::string GetUnique() const
	{
		return m_sUnique;
	};

	inline const std::string GetName() const
	{
		if (m_bUnlocalizedName)
			return m_sUnlocalizedName;

		return Localization::Get(m_ullLocalizedNameHash);
	};

	inline void SetName(std::string s)
	{
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = s;
	};

	inline void SetName(size_t ullHash)
	{
		m_bUnlocalizedName = false;
		m_ullLocalizedNameHash = ullHash;
	};

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

	inline ElementBase* GetChild(const std::string& sUnique) const
	{
		for (auto& pChild : m_Children) {
			if (!pChild)
				continue;

			if (pChild->GetUnique().compare(sUnique) == 0) {
				return pChild;
			}
		}

		return nullptr;
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

	inline const std::vector<ElementBase*>& GetChildren() const
	{
		return m_Children;
	};

	inline void SetWidth(float flWidth)
	{
		m_stStyle.vec2Size.x = flWidth;
	};

	static uint8_t AddPage(size_t ullLocalizedNameHash, const char* sIcon = ICON_FA_CIRCLE)
	{
		return PageManager::RegisterPage(ullLocalizedNameHash, sIcon);
	}

	static uint8_t AddPage(std::string sUnlocalizedName, const char* sIcon = ICON_FA_CIRCLE)
	{
		return PageManager::RegisterPage(sUnlocalizedName, sIcon);
	}

	static uint8_t GetNextPageId()
	{
		return PageManager::s_iNextPageId;
	}

	static void SetDefaultPage(uint8_t iPageId)
	{
		eCurrentPage = iPageId;
		eCurrentSubPage = 0;
	}

	static uint8_t GetCurrentPage()
	{
		return eCurrentPage;
	}

	static uint8_t GetCurrentSubPage()
	{
		return eCurrentSubPage;
	}

	inline void SetStyle(Style_t stStyle)
	{
		m_stStyle = stStyle;
	};

	virtual constexpr EElementType GetType() const
	{
		return EElementType::None;
	};

	virtual ImVec2 GetNaturalSize() const
	{
		return ImVec2(0.f, 0.f);
	};

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

	void SetSameLineSizeCallback(std::function<ImVec2()> callback)
	{
		m_SameLineSizeCallback = callback;
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

	bool m_bForceResize = false;

public:
	void RequestResize(ImVec2 vec2NewSize)
	{
		m_stStyle.vec2Size = vec2NewSize;
		m_bForceResize = true;
	};


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

	void RequestSameLine(ElementBase* pChild) override
	{
		if (!pChild)
			return;

		EElementType eChildType = pChild->GetType();

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

		ImGuiStyle& style = ImGui::GetStyle();

		float FooterHeight = ImGui::GetFrameHeight();
		float HeaderHeight = ImGui::GetFrameHeight() + style.WindowPadding.y * 2;

		ImGui::SetNextWindowSize(m_stStyle.vec2Size, m_bForceResize ? ImGuiCond_Always : ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize / 2 - m_stStyle.vec2Size / 2, m_bForceResize ? ImGuiCond_Always : ImGuiCond_Once);
		m_bForceResize = false;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin(GetName().c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
		ImGui::PopStyleVar(2);

		{
			ImVec2 pos = ImGui::GetWindowPos();
			ImVec2 size = ImGui::GetWindowSize();
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			const ImU32 chromeCol = ImGui::GetColorU32(vec4ChromeBg);
			drawList->AddRectFilled(pos, pos + ImVec2(kSidebarWidth, size.y), chromeCol, style.WindowRounding, ImDrawFlags_RoundCornersLeft);
			drawList->AddRectFilled(pos + ImVec2(kSidebarWidth, 0), pos + ImVec2(size.x, HeaderHeight), chromeCol, style.WindowRounding, ImDrawFlags_RoundCornersTopRight);
			drawList->AddRectFilled(pos + ImVec2(kSidebarWidth, HeaderHeight), pos + ImVec2(size.x, size.y - FooterHeight), ImGui::GetColorU32(ImGuiCol_WindowBg), style.WindowRounding, ImDrawFlags_RoundCornersNone);
			drawList->AddRectFilled(pos + ImVec2(kSidebarWidth, size.y - FooterHeight), pos + size, chromeCol, style.WindowRounding, ImDrawFlags_RoundCornersBottomRight);

			if (style.WindowBorderSize > 0) {
				drawList->AddLine(pos + ImVec2(kSidebarWidth - style.WindowBorderSize, style.WindowBorderSize), pos + ImVec2(kSidebarWidth - style.WindowBorderSize, size.y - style.WindowBorderSize), ImGui::GetColorU32(ImGuiCol_Border), style.WindowBorderSize);
				drawList->AddLine(pos + ImVec2(kSidebarWidth, HeaderHeight - style.WindowBorderSize), pos + ImVec2(size.x - style.WindowBorderSize, HeaderHeight - style.WindowBorderSize), ImGui::GetColorU32(ImGuiCol_Border), style.WindowBorderSize);
				drawList->AddLine(pos + ImVec2(kSidebarWidth, size.y - FooterHeight + style.WindowBorderSize), pos + ImVec2(size.x - style.WindowBorderSize, size.y - FooterHeight + style.WindowBorderSize), ImGui::GetColorU32(ImGuiCol_Border), style.WindowBorderSize);
				drawList->AddRect(pos, pos + size, ImGui::GetColorU32(ImGuiCol_Border), style.WindowRounding);
			}

			drawList->AddText(pos + ImVec2(kSidebarWidth + style.FramePadding.x, size.y - FooterHeight + style.FramePadding.y), ImGui::GetColorU32(ImGuiCol_TextDisabled), "Hello, World!");
			drawList->AddText(pos + ImVec2(size.x - ImGui::CalcTextSize((std::string("v") + STR(FRAMEWORK_VERSION)).c_str()).x - style.FramePadding.x, size.y - FooterHeight + style.FramePadding.y), ImGui::GetColorU32(ImGuiCol_SliderGrab), (std::string("v") + STR(FRAMEWORK_VERSION)).c_str());
		}
		RenderChildren();
		ImGui::End();
	}
};

class Child : public ElementBase
{
protected:
	ImGuiWindowFlags m_WindowFlags;
	std::function<ImVec2()> m_Callback = nullptr;
	std::function<void()> m_PushVarsCallback = nullptr;
	std::function<void()> m_PopVarsCallback = nullptr;

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

		ImGui::BeginChild(GetName().c_str(), m_stStyle.vec2Size, m_stStyle.iFlags | ImGuiChildFlags_AlwaysUseWindowPadding, m_WindowFlags | ImGuiWindowFlags_NoScrollbar);
		if (m_PushVarsCallback)
			m_PushVarsCallback();
		RenderChildren();
		if (m_PopVarsCallback)
			m_PopVarsCallback();
		ImGui::EndChild();
	};

	void SetCallback(std::function<ImVec2()> Callback)
	{
		m_Callback = Callback;
	};

	void SetPushVarsCallback(std::function<void()> Callback)
	{
		m_PushVarsCallback = Callback;
	};

	void SetPopVarsCallback(std::function<void()> Callback)
	{
		m_PopVarsCallback = Callback;
	};
};

class _Text : public ElementBase
{
protected:

public:
	_Text(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
	};

	_Text(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {})
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

	std::function<void()> m_PositionCallback = nullptr;

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

		if (m_PositionCallback)
			m_PositionCallback();

		if (ImAdd::Button(GetName().c_str(), m_stStyle.vec2Size * g_flUIScale))
			if (m_Callback)
				m_Callback();

		RenderChildren();
	};


	void SetCallback(std::function<void()> Callback)
	{
		m_Callback = Callback;
	};

	void SetPositionCallback(std::function<void()> Callback)
	{
		m_PositionCallback = Callback;
	};
};

class AccentButton : public ElementBase
{
protected:
	std::function<void()> m_Callback = nullptr;

public:
	AccentButton(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
	};

	AccentButton(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {})
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

		if (ImAdd::AccentButton(GetName().c_str(), m_stStyle.vec2Size * g_flUIScale))
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
	struct ComboOption
	{
		std::string sLabel;
		std::function<void()> Callback;
	};

	std::string m_sPreviewlabel = "PreviewNotSet";
	std::function<void()> m_Callback;
	std::vector<ComboOption> m_Options;
	int m_iSelectedIndex = -1;

	std::function<float()> m_WidthCallback = nullptr;

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

	void SetWidthCallback(std::function<float()> Callback)
	{
		m_WidthCallback = Callback;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		ImGuiComboFlags iFlags = m_stStyle.iFlags;
		if (m_WidthCallback)
		{
			ImGui::SetNextItemWidth(m_WidthCallback());
			iFlags &= ~ImGuiComboFlags_WidthFitPreview;
		}

		if (ImAdd::BeginCombo(GetName().c_str(), m_sPreviewlabel.c_str(), iFlags))
		{
			if (m_Callback)
			{
				m_Callback();
			}
			else if (!m_Options.empty())
			{
				for (size_t i = 0; i < m_Options.size(); ++i)
				{
					bool bSelected = m_iSelectedIndex == static_cast<int>(i);
					if (ImAdd::Selectable(m_Options[i].sLabel.c_str(), bSelected))
					{
						m_iSelectedIndex = static_cast<int>(i);
						m_sPreviewlabel = m_Options[i].sLabel;
						if (m_Options[i].Callback)
							m_Options[i].Callback();
					}

					if (bSelected)
						ImGui::SetItemDefaultFocus();
				}
			}

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

	void AddOption(std::string sLabel, std::function<void()> Callback = nullptr)
	{
		m_Options.push_back({ sLabel, Callback });

		if (m_iSelectedIndex == -1 && !m_Options.empty())
		{
			m_iSelectedIndex = 0;
			m_sPreviewlabel = m_Options[0].sLabel;
		}
	};

	int GetSelectedIndex() const
	{
		return m_iSelectedIndex;
	};

	void SetSelectedIndex(int iIndex)
	{
		if (iIndex >= 0 && iIndex < static_cast<int>(m_Options.size()))
		{
			m_iSelectedIndex = iIndex;
			m_sPreviewlabel = m_Options[iIndex].sLabel;
		}
	};

	void SetSelectedByLabel(const std::string& sLabel)
	{
		for (size_t i = 0; i < m_Options.size(); ++i)
		{
			if (m_Options[i].sLabel != sLabel)
				continue;

			m_iSelectedIndex = static_cast<int>(i);
			m_sPreviewlabel = m_Options[i].sLabel;
			return;
		}
	};

	void ClearOptions()
	{
		m_Options.clear();
		m_iSelectedIndex = -1;
		m_sPreviewlabel.clear();
	};
};

class PersistentCombo : public Combo
{
public:
	using Combo::Combo;

	void ConfigSave(nlohmann::json& jsonParent) const override
	{
		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()] = nlohmann::json();

		jsonEntry["Value"] = GetSelectedIndex();

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
			SetSelectedIndex(jsonEntry["Value"].get<int>());

		if (jsonEntry.contains("Children"))
			ConfigLoadChildren(jsonEntry["Children"]);
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

		ImAdd::SmallCheckbox(GetName().c_str(), &m_Value);

		RenderChildren();
	};
};

class Toggle : public ElementInput<bool>
{
protected:

public:
	Toggle(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
	};

	Toggle(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::Toggle;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		ImAdd::Togglebutton(GetName().c_str(), &m_Value);

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

		if (ImAdd::BeginCombo(("##CMB" + GetName()).c_str(), "##", ImGuiComboFlags_NoPreview))
		{
			bool bSelected;

			bSelected = m_eMode == EHotkeyMode::AlwaysOn;
			if (ImAdd::Selectable("Always On", bSelected))
				m_eMode = EHotkeyMode::AlwaysOn;

			if (bSelected)
				ImGui::SetItemDefaultFocus();

			bSelected = m_eMode == EHotkeyMode::Hold;
			if (ImAdd::Selectable("Hold", bSelected))
				m_eMode = EHotkeyMode::Hold;

			if (bSelected)
				ImGui::SetItemDefaultFocus();

			bSelected = m_eMode == EHotkeyMode::Toggle;
			if (ImAdd::Selectable("Toggle", bSelected))
				m_eMode = EHotkeyMode::Toggle;

			if (bSelected)
				ImGui::SetItemDefaultFocus();

			bSelected = m_eMode == EHotkeyMode::HoldOff;
			if (ImAdd::Selectable("Hold Off", bSelected))
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
			ImAdd::Button("...", m_stStyle.vec2Size * g_flUIScale);
			ImGui::PopStyleColor();

			ImGui::GetCurrentContext()->ActiveIdAllowOverlap = true;
			if (!ImGui::IsItemHovered() && !ImGui::IsItemFocused() && SetKey())
			{
				ImGui::ClearActiveID();
				m_bSetting = false;
			}
		}
		else if (ImAdd::Button(BtnName.c_str(), m_stStyle.vec2Size * g_flUIScale) || m_bSetting) {
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

		if (m_eMode != EHotkeyMode::AlwaysOn && m_eKey == ImGuiKey_None)
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

		ImAdd::SliderFloat(GetName().c_str(), &m_Value, m_Min, m_Max, m_stStyle.vec2Size.x * g_flUIScale, m_sFormat);

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

		ImAdd::SliderInt(GetName().c_str(), &m_Value, m_Min, m_Max, m_stStyle.vec2Size.x * g_flUIScale, m_sFormat);

		RenderChildren();
	};
};

class InputText : public ElementInput<std::string>
{
protected:
	ImGuiInputTextCallback m_Callback = nullptr;
	void* m_pUserData = nullptr;
	std::string m_sPreview = "";

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

		ImAdd::InputText(GetName().c_str(), m_sPreview.c_str(), m_Value.data(), m_Value.capacity(), m_stStyle.vec2Size.x * g_flUIScale, m_stStyle.iFlags, m_Callback, m_pUserData);

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

	void SetPreviewText(const std::string& s)
	{
		m_sPreview = s;
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

		ImAdd::ColorEdit4(GetName().c_str(), reinterpret_cast<float*>(&m_Value));

		RenderChildren();
	};
};

class Table : public ElementBase
{
protected:
	struct TableColumn_t
	{
		std::string sLabel;
		float flWeight = 1.0f;
		float flWidth = 0.0f;
		bool bWidthInitialized = false;
	};

	int m_iColumns = 0;
	std::vector<TableColumn_t> m_Columns;
	std::vector<std::vector<std::string>> m_Rows;

	std::function<void(int iRow, int iCol)> m_CellClickCallback = nullptr;
	std::function<void(int iRow, int iCol)> m_CellContextMenuCallback = nullptr;
	int m_iSelectedRow = -1;

	bool m_bSaveToConfig = false;

	static constexpr float kMinColumnWidth = 24.0f;
	static constexpr float kResizeHandleWidth = 6.0f;

public:
	Table(std::string sUnique, int iColumns, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_iColumns = iColumns;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::Table;
	};

	void AddColumn(size_t ullLocalizedNameHash, float flWeight = 1.0f)
	{
		m_Columns.push_back({ Localization::Get(ullLocalizedNameHash), flWeight, 0.0f, false });
	};

	void AddColumn(std::string sUnlocalizedLabel, float flWeight = 1.0f)
	{
		m_Columns.push_back({ sUnlocalizedLabel, flWeight, 0.0f, false });
	};

	void AddRow(std::vector<std::string> vecCells)
	{
		m_Rows.push_back(std::move(vecCells));
	};

	void ClearRows()
	{
		m_Rows.clear();
	};

	void SetCell(int iRow, int iCol, std::string sValue)
	{
		if (iRow < 0 || iRow >= static_cast<int>(m_Rows.size()))
			return;

		if (iCol < 0 || iCol >= static_cast<int>(m_Rows[iRow].size()))
			return;

		m_Rows[iRow][iCol] = std::move(sValue);
	};

	std::string GetCell(int iRow, int iCol) const
	{
		if (iRow < 0 || iRow >= static_cast<int>(m_Rows.size()))
			return "";

		if (iCol < 0 || iCol >= static_cast<int>(m_Rows[iRow].size()))
			return "";

		return m_Rows[iRow][iCol];
	};

	void SetCellClickCallback(std::function<void(int iRow, int iCol)> Callback)
	{
		m_CellClickCallback = Callback;
	};

	void SetCellContextMenuCallback(std::function<void(int iRow, int iCol)> Callback)
	{
		m_CellContextMenuCallback = Callback;
	};

	void SetSelectedRow(int iRow)
	{
		m_iSelectedRow = iRow;
	};

	int GetSelectedRow() const
	{
		return m_iSelectedRow;
	};

	void SetSaveToConfig(bool bSave)
	{
		m_bSaveToConfig = bSave;
	};

	void ConfigSave(nlohmann::json& jsonParent) const override
	{
		if (!m_bSaveToConfig)
			return;

		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()] = nlohmann::json();
		jsonEntry["Rows"] = m_Rows;
		jsonEntry["SelectedRow"] = m_iSelectedRow;
	};

	void ConfigLoad(nlohmann::json& jsonParent) override
	{
		if (!m_bSaveToConfig || !jsonParent.contains(m_sUnique.c_str()))
			return;

		const nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()];

		if (jsonEntry.contains("Rows"))
			m_Rows = jsonEntry["Rows"].get<std::vector<std::vector<std::string>>>();

		if (jsonEntry.contains("SelectedRow"))
			m_iSelectedRow = jsonEntry["SelectedRow"].get<int>();
	};

	void Render() override
	{
		if (!m_stStyle.bVisible || m_Columns.empty())
			return;

		SameLine();

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		const float flRowHeight = ImGui::GetFrameHeight();
		const int iRowCount = static_cast<int>(m_Rows.size());
		const float flContentHeight = flRowHeight + iRowCount * flRowHeight;

		const ImVec2 pos = window->DC.CursorPos;
		const float flWidth = (m_stStyle.vec2Size.x > 0.0f) ? m_stStyle.vec2Size.x * g_flUIScale : ImGui::GetContentRegionAvail().x;
		const float flHeight = (m_stStyle.vec2Size.y > 0.0f) ? m_stStyle.vec2Size.y * g_flUIScale : flContentHeight + style.WindowPadding.y * 2.0f;
		const ImVec2 size(flWidth, flHeight);

		const ImRect bb(pos, pos + size);
		const ImGuiID tableId = window->GetID(m_sUnique.c_str());
		ImGui::ItemSize(size);
		if (!ImGui::ItemAdd(bb, tableId))
			return;

		ImDrawList* drawList = window->DrawList;

		bool bNeedsInit = false;
		float flTotalWeight = 0.0f;
		for (const TableColumn_t& column : m_Columns)
		{
			if (!column.bWidthInitialized)
				bNeedsInit = true;
			flTotalWeight += column.flWeight;
		}

		const float flInnerWidth = size.x - style.WindowPadding.x * 2.0f;
		if (bNeedsInit && flTotalWeight > 0.0f)
		{
			for (TableColumn_t& column : m_Columns)
			{
				column.flWidth = flInnerWidth * (column.flWeight / flTotalWeight);
				column.bWidthInitialized = true;
			}
		}

		drawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_ChildBg), style.ChildRounding);
		drawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border), style.ChildRounding);

		ImGui::PushClipRect(bb.Min, bb.Max, true);

		ImVec2 cursor = bb.Min + style.WindowPadding;

		{
			float flColX = cursor.x;
			for (int iCol = 0; iCol < static_cast<int>(m_Columns.size()); ++iCol)
			{
				TableColumn_t& column = m_Columns[iCol];
				const ImRect headerRect(ImVec2(flColX, cursor.y), ImVec2(flColX + column.flWidth, cursor.y + flRowHeight));

				ImGui::RenderTextClipped(ImVec2(headerRect.Min.x + style.FramePadding.x, headerRect.Min.y), headerRect.Max, column.sLabel.c_str(), NULL, NULL, ImVec2(0.0f, 0.5f), &headerRect);

				flColX += column.flWidth;

				if (iCol < static_cast<int>(m_Columns.size()) - 1)
				{
					ImGui::PushID(iCol);
					const ImGuiID handleId = window->GetID("##ColResize");
					const ImRect handleRect(ImVec2(flColX - kResizeHandleWidth * 0.5f, cursor.y), ImVec2(flColX + kResizeHandleWidth * 0.5f, cursor.y + flContentHeight));

					ImGui::ItemAdd(handleRect, handleId);
					bool bHandleHovered, bHandleHeld;
					ImGui::ButtonBehavior(handleRect, handleId, &bHandleHovered, &bHandleHeld);

					if (bHandleHovered || bHandleHeld)
						ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

					if (bHandleHeld && g.IO.MouseDelta.x != 0.0f)
					{
						TableColumn_t& next = m_Columns[iCol + 1];
						const float flNewThis = column.flWidth + g.IO.MouseDelta.x;
						const float flNewNext = next.flWidth - g.IO.MouseDelta.x;

						if (flNewThis >= kMinColumnWidth && flNewNext >= kMinColumnWidth)
						{
							column.flWidth = flNewThis;
							next.flWidth = flNewNext;
						}
					}

					if (bHandleHovered || bHandleHeld)
						drawList->AddRectFilled(handleRect.Min, handleRect.Max, ImGui::GetColorU32(ImGuiCol_SliderGrab));

					ImGui::PopID();
				}
			}

			cursor.y += flRowHeight;
			drawList->AddLine(ImVec2(bb.Min.x, cursor.y), ImVec2(bb.Max.x, cursor.y), ImGui::GetColorU32(ImGuiCol_Border));
		}

		const ImVec4 vec4Accent = style.Colors[ImGuiCol_SliderGrab];
		const ImVec4 vec4TextOnAccent = ImAdd::ShadeColor(vec4Accent, 0.3f);
		const ImVec4 vec4RowAlt = ImAdd::ShadeColor(style.Colors[ImGuiCol_ChildBg], 0.9f);
		const ImVec4 vec4RowHovered = ImAdd::ShadeColor(style.Colors[ImGuiCol_ChildBg], 0.85f);

		for (int iRow = 0; iRow < iRowCount; ++iRow)
		{
			const std::vector<std::string>& row = m_Rows[iRow];
			const bool bRowSelected = (iRow == m_iSelectedRow);
			const ImRect rowRect(ImVec2(bb.Min.x, cursor.y), ImVec2(bb.Max.x, cursor.y + flRowHeight));
			const bool bRowHovered = rowRect.Contains(g.IO.MousePos);

			if (bRowSelected)
				drawList->AddRectFilled(rowRect.Min, rowRect.Max, ImGui::GetColorU32(vec4Accent));
			else if (bRowHovered)
				drawList->AddRectFilled(rowRect.Min, rowRect.Max, ImGui::GetColorU32(vec4RowHovered));
			else if (iRow % 2 == 1)
				drawList->AddRectFilled(rowRect.Min, rowRect.Max, ImGui::GetColorU32(vec4RowAlt));

			if (bRowSelected)
				ImGui::PushStyleColor(ImGuiCol_Text, vec4TextOnAccent);

			float flColX = cursor.x;
			for (int iCol = 0; iCol < static_cast<int>(m_Columns.size()) && iCol < static_cast<int>(row.size()); ++iCol)
			{
				const TableColumn_t& column = m_Columns[iCol];
				const ImRect cellRect(ImVec2(flColX, cursor.y), ImVec2(flColX + column.flWidth, cursor.y + flRowHeight));

				ImGui::PushID(iRow);
				ImGui::PushID(iCol);
				const ImGuiID cellId = window->GetID("##Cell");
				ImGui::ItemAdd(cellRect, cellId);

				bool bCellHovered, bCellHeld;
				if (ImGui::ButtonBehavior(cellRect, cellId, &bCellHovered, &bCellHeld) && m_CellClickCallback)
					m_CellClickCallback(iRow, iCol);

				if (m_CellContextMenuCallback && ImGui::BeginPopupContextItem())
				{
					m_CellContextMenuCallback(iRow, iCol);
					ImGui::EndPopup();
				}

				ImGui::RenderTextClipped(ImVec2(cellRect.Min.x + style.FramePadding.x, cellRect.Min.y), cellRect.Max, row[iCol].c_str(), NULL, NULL, ImVec2(0.0f, 0.5f), &cellRect);

				ImGui::PopID();
				ImGui::PopID();

				flColX += column.flWidth;
			}

			if (bRowSelected)
				ImGui::PopStyleColor();

			cursor.y += flRowHeight;
		}

		ImGui::PopClipRect();

		RenderChildren();
	};
};

class ConfigManager : public ElementBase
{
public:
	struct Callbacks
	{
		std::function<std::string()> GetCurrentName;
		std::function<std::string()> GetDefaultName;
		std::function<std::vector<std::string>()> ListConfigs;

		std::function<void(const std::string&)> Load;
		std::function<void()> Save;
		std::function<void()> Reload;
		std::function<void(const std::string&)> CreateNew;
		std::function<void(const std::string&, const std::string&)> Rename;
		std::function<void(const std::string&)> Delete;
		std::function<void(const std::string&)> SetDefault;
		std::function<void()> EnsureDefaultValid;
		std::function<void()> OpenFolder;
	};

protected:
	Callbacks m_Callbacks;
	char m_szNameBuffer[128] = {};

public:
	ConfigManager(std::string sUnique, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::ConfigManager;
	};

	void SetCallbacks(Callbacks stCallbacks)
	{
		m_Callbacks = stCallbacks;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible || !m_Callbacks.ListConfigs)
			return;

		SameLine();

		const ImGuiStyle& style = ImGui::GetStyle();
		const std::string sCurrent = m_Callbacks.GetCurrentName ? m_Callbacks.GetCurrentName() : "";
		const std::string sDefault = m_Callbacks.GetDefaultName ? m_Callbacks.GetDefaultName() : "";
		const std::vector<std::string> vecConfigs = m_Callbacks.ListConfigs();

		const ImVec4 vec4Accent = style.Colors[ImGuiCol_SliderGrab];
		const ImVec4 vec4TextOnAccent = ImAdd::ShadeColor(vec4Accent, 0.3f);

		const float flRowStartY = ImGui::GetCursorPosY();
		const float flAvailHeight = ImGui::GetContentRegionAvail().y;

		ImGui::BeginChild("##ConfigListBox", ImVec2(180.0f * g_flUIScale, 0.f), ImGuiChildFlags_Border);
		{
			for (const std::string& sName : vecConfigs)
			{
				ImGui::PushID(sName.c_str());

				const bool bIsCurrent = (sName == sCurrent);
				const bool bIsDefault = (sName == sDefault);
				const std::string sLabel = bIsDefault ? (sName + " " + Localization::Get("CONFIG_DEFAULT_SUFFIX"Hashed)) : sName;

				if (bIsCurrent)
				{
					ImGui::PushStyleColor(ImGuiCol_Header, vec4Accent);
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, vec4Accent);
					ImGui::PushStyleColor(ImGuiCol_HeaderActive, vec4Accent);
					ImGui::PushStyleColor(ImGuiCol_Text, vec4TextOnAccent);
				}

				if (ImGui::Selectable(sLabel.c_str(), bIsCurrent) && !bIsCurrent && m_Callbacks.Load)
					m_Callbacks.Load(sName);

				if (bIsCurrent)
					ImGui::PopStyleColor(4);

				if (ImGui::BeginPopupContextItem())
				{
					ImGui::TextUnformatted(Localization::Get("CONFIG_RENAME"Hashed).c_str());

					static char szRenameBuf[128];
					if (ImGui::IsWindowAppearing())
					{
						std::fill(std::begin(szRenameBuf), std::end(szRenameBuf), '\0');
						const size_t iCopyLen = (std::min)(sName.size(), sizeof(szRenameBuf) - 1);
						std::copy(sName.begin(), sName.begin() + iCopyLen, szRenameBuf);
					}

					ImGui::SetNextItemWidth(160.0f * g_flUIScale);
					if (ImGui::InputText("##Rename", szRenameBuf, sizeof(szRenameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
					{
						std::string sNewName(szRenameBuf);
						if (!sNewName.empty() && m_Callbacks.Rename)
							m_Callbacks.Rename(sName, sNewName);

						ImGui::CloseCurrentPopup();
					}

					ImGui::Separator();

					if (m_Callbacks.SetDefault && ImGui::Selectable(Localization::Get("CONFIG_SET_DEFAULT"Hashed).c_str()))
					{
						m_Callbacks.SetDefault(sName);
						ImGui::CloseCurrentPopup();
					}

					if (vecConfigs.size() > 1 && m_Callbacks.Delete && ImGui::Selectable(Localization::Get("CONFIG_DELETE"Hashed).c_str()))
					{
						m_Callbacks.Delete(sName);

						if (m_Callbacks.EnsureDefaultValid)
							m_Callbacks.EnsureDefaultValid();

						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				ImGui::PopID();
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginGroup();
		{
			const float flPlusButtonSize = ImGui::GetFrameHeight();
			const float flNameFieldWidth = ImGui::GetContentRegionAvail().x - flPlusButtonSize - style.ItemSpacing.x;

			ImGui::SetNextItemWidth(flNameFieldWidth);
			ImGui::InputTextWithHint("##ConfigNewName", Localization::Get("CONFIG_NAME_PLACEHOLDER"Hashed).c_str(), m_szNameBuffer, sizeof(m_szNameBuffer));

			ImGui::SameLine();
			if (ImAdd::Button(ICON_FA_PLUS, ImVec2(flPlusButtonSize, flPlusButtonSize)))
			{
				std::string sName(m_szNameBuffer);
				if (!sName.empty() && m_Callbacks.CreateNew)
				{
					m_Callbacks.CreateNew(sName);
					std::fill(std::begin(m_szNameBuffer), std::end(m_szNameBuffer), '\0');
				}
			}

			if (m_Callbacks.Save && ImAdd::Button(Localization::Get("CONFIG_SAVE"Hashed).c_str()))
				m_Callbacks.Save();

			ImGui::SameLine();
			if (m_Callbacks.Reload && ImAdd::Button(Localization::Get("CONFIG_RELOAD"Hashed).c_str()))
				m_Callbacks.Reload();

			const float flOpenFolderY = flRowStartY + flAvailHeight - ImGui::GetFrameHeight();
			if (ImGui::GetCursorPosY() < flOpenFolderY)
				ImGui::SetCursorPosY(flOpenFolderY);

			if (m_Callbacks.OpenFolder && ImAdd::Button(Localization::Get("CONFIG_OPEN_FOLDER"Hashed).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0.f)))
				m_Callbacks.OpenFolder();
		}
		ImGui::EndGroup();

		RenderChildren();
	};
};

class Seperator : public ElementBase
{
protected:

public:
	Seperator(std::string sUnique, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::Separator;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		ImGui::Separator();
	};
};

class SeperatorText : public ElementBase
{
protected:

public:
	SeperatorText(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
	};

	SeperatorText(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::SeperatorText;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		ImAdd::SeparatorText(GetName().c_str());
	};
};

class RadioButtonIcon : public ElementBase
{
protected:
	const char* m_sIcon;
	uint8_t m_iPageId;

public:
	RadioButtonIcon(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle, const char* sIcon, uint8_t iPageId)
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
		m_sIcon = sIcon;
		m_iPageId = iPageId;
	};

	RadioButtonIcon(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle, const char* sIcon, uint8_t iPageId)
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;
		m_sIcon = sIcon;
		m_iPageId = iPageId;
	};

	RadioButtonIcon(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle, const char* sIcon, bool bAutoRegisterPage)
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
		m_sIcon = sIcon;
		if (bAutoRegisterPage)
			m_iPageId = ElementBase::AddPage(ullLocalizedNameHash, sIcon);
		else
			m_iPageId = 0;
	};

	RadioButtonIcon(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle, const char* sIcon, bool bAutoRegisterPage)
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;
		m_sIcon = sIcon;
		if (bAutoRegisterPage)
			m_iPageId = ElementBase::AddPage(sUnlocalizedName, sIcon);
		else
			m_iPageId = 0;
	};

	uint8_t GetPageId() const
	{
		return m_iPageId;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::RadioButtonIcon;
	};

	ImVec2 GetNaturalSize() const override
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 vec2IconSize = ImGui::CalcTextSize(m_sIcon, NULL, true);
		ImVec2 vec2LabelSize = ImGui::CalcTextSize(GetName().c_str(), NULL, true);
		return ImVec2(vec2IconSize.x + vec2LabelSize.x + style.FramePadding.x * 3.0f, ImGui::GetFontSize() + style.FramePadding.y * 2.0f);
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		SameLine();

		if (ImAdd::RadioButtonIcon(GetName().c_str(), m_sIcon, GetName().c_str(), &eCurrentPage, m_iPageId, m_stStyle.vec2Size * g_flUIScale)) {
			eCurrentPage = m_iPageId;
			eCurrentSubPage = 0;
		}
	};
};

class HeaderGroup : public ElementBase
{
protected:
	std::vector<Header_t> m_Headers;

public:
	HeaderGroup(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {}, std::vector<Header_t> Headers = {})
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;
		m_Headers = Headers;
	};

	HeaderGroup(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {}, std::vector<Header_t> Headers = {})
	{
		m_sUnique = sUnique;
		m_bUnlocalizedName = true;
		m_sUnlocalizedName = sUnlocalizedName;
		m_stStyle = stStyle;
		m_Headers = Headers;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::HeaderGroup;
	};

	void AddHeaders(uint8_t iPageId, std::vector<size_t> ullLocalizedNameHashes)
	{
		m_Headers.push_back({ iPageId, ullLocalizedNameHashes });
	}

	void AddHeader(uint8_t iPageId, size_t ullLocalizedNameHash)
	{
		m_Headers.push_back({ iPageId, { ullLocalizedNameHash } });
	}

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		const ImGuiStyle& style = ImGui::GetStyle();

		ImGui::SameLine(kSidebarWidth);
		ImGui::BeginChild(m_sUnique.c_str(), ImVec2(0, ImGui::GetFrameHeight() + style.WindowPadding.y * 2), ImGuiChildFlags_Border, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
		{
			for (auto Header : m_Headers)
			{
				if (Header.m_iParentPageID != eCurrentPage)
					continue;

				for (int i = 0; i < Header.m_ullLocalizedNameHashes.size(); i++) {
					std::string sHeaderName = Localization::Get(Header.m_ullLocalizedNameHashes[i]);
					if (ImAdd::RadioButton(sHeaderName.c_str(), &eCurrentSubPage, i)) {
						eCurrentSubPage = i;
					}
					ImGui::SameLine();
				}
			}
			ImGui::NewLine();
		}
		ImGui::EndChild();
		RenderChildren();
	}
};

class Body : public ElementBase
{
public:
	Body(std::string sUnique, Style_t stStyle = {})
	{
		m_sUnique = sUnique;
		m_stStyle = stStyle;
	};

	constexpr EElementType GetType() const override
	{
		return EElementType::Body;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		const ImGuiStyle& style = ImGui::GetStyle();
		const float flHeaderHeight = ImGui::GetFrameHeight() + style.WindowPadding.y * 2;

		ImGui::SetCursorPosX(kSidebarWidth);
		ImGui::SetCursorPosY(flHeaderHeight);
		ImGui::BeginChild(m_sUnique.c_str(), ImVec2(0, ImGui::GetWindowHeight() - ImGui::GetFrameHeight() - flHeaderHeight), ImGuiChildFlags_Border, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
		{
			RenderChildren();
		}
		ImGui::EndChild();
	}
};

class Page : public ElementBase
{
protected:
	uint8_t m_iPageId;
	uint8_t m_iSubPageId;

public:
	Page(std::string sUnique, Style_t stStyle = {}, uint8_t iPageId = 0, uint8_t iSubPageId = 0)
	{
		m_sUnique = sUnique;
		m_stStyle = stStyle;
		m_iPageId = iPageId;
		m_iSubPageId = iSubPageId;
	};
	
	constexpr EElementType GetType() const override
	{
		return EElementType::Page;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		if (m_iPageId != eCurrentPage || m_iSubPageId != eCurrentSubPage)
			return;

		RenderChildren();
	}

	void SetPageId(uint8_t iPageId)
	{
		m_iPageId = iPageId;
	};

	void SetSubPageId(uint8_t iSubPageId)
	{
		m_iSubPageId = iSubPageId;
	};
};

class Group : public ElementBase {
	public:
		Group(std::string sUnique, Style_t stStyle = {}) {
			m_sUnique = sUnique;
			m_stStyle = stStyle;
		};

		constexpr EElementType GetType() const override
		{
			return EElementType::Group;
		};

		void Render() override {
			if (!m_stStyle.bVisible)
				return;

			SameLine();

			ImGui::BeginGroup();
			RenderChildren();
			ImGui::EndGroup();
		};
};

class GroupChild : public ElementBase
{
	protected:
	ImGuiWindowFlags m_WindowFlags;
	std::function<ImVec2()> m_Callback = nullptr;
	std::function<void()> m_PushVarsCallback = nullptr;
	std::function<void()> m_PopVarsCallback = nullptr;

public:
	GroupChild(std::string sUnique, size_t ullLocalizedNameHash, Style_t stStyle = {}, ImGuiWindowFlags WindowFlags = 0)
	{
		m_sUnique = sUnique;
		m_ullLocalizedNameHash = ullLocalizedNameHash;
		m_stStyle = stStyle;

		m_WindowFlags = WindowFlags;
	};

	GroupChild(std::string sUnique, std::string sUnlocalizedName, Style_t stStyle = {}, ImGuiWindowFlags WindowFlags = 0)
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

		ImGui::BeginChild(GetName().c_str(), m_stStyle.vec2Size, m_stStyle.iFlags | ImGuiChildFlags_AlwaysUseWindowPadding, m_WindowFlags | ImGuiWindowFlags_NoScrollbar);
		if (m_PushVarsCallback)
			m_PushVarsCallback();
		ImGui::TextDisabled(GetName().c_str());
		RenderChildren();
		if (m_PopVarsCallback)
			m_PopVarsCallback();
		ImGui::EndChild();
	};

	void SetCallback(std::function<ImVec2()> Callback)
	{
		m_Callback = Callback;
	};

	void SetPushVarsCallback(std::function<void()> Callback)
	{
		m_PushVarsCallback = Callback;
	};

	void SetPopVarsCallback(std::function<void()> Callback)
	{
		m_PopVarsCallback = Callback;
	};
};