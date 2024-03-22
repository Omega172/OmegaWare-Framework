#pragma once
#include "pch.h"
#include <functional>
#include <nlohmann/json.hpp>
#include <fstream>

class ElementBase
{
public:
	typedef struct Style_t
	{
		bool bVisible = true;

		bool bSameLine = false;
		float flSpacing = -1.f;

		ImVec2 vec2Size = { 100.f, 0.f };
		ImDrawFlags iFlags = 0;

	} Style_t;

	Style_t m_stStyle;



protected:
	std::string m_sUnique = "INVALID_UNIQUE";
	
	ElementBase* m_pParent = nullptr;
	std::vector<ElementBase*> m_Children;

public:

	ElementBase() = default;
	
	void AddElement(ElementBase* pElement, std::string sUnique, Style_t stStyle)
	{
		pElement->m_sUnique = sUnique;
		pElement->m_stStyle = stStyle;
		pElement->m_pParent = this;
		m_Children.push_back(pElement);
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
		return GetUnique();//"NOT_IMPLEMENTED_YET_KEKW";
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

	virtual void ConfigSave(nlohmann::json& jsonParent) const
	{
		if (!HasChildren())
			return;

		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()] = nlohmann::json();

		jsonEntry["Children"] = nlohmann::json();

		ConfigSaveChildren(jsonEntry["Children"]);
	};

	virtual void ConfigSaveChildren(nlohmann::json& jsonParent) const
	{
		for (ElementBase* const pElement : m_Children)
			pElement->ConfigSave(jsonParent);
	};

	virtual void ConfigLoad(nlohmann::json& jsonParent)
	{
		if (!HasChildren() || !jsonParent.contains(m_sUnique.c_str()))
			return;

		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()];

		if (jsonEntry.contains("Children"))
			ConfigLoadChildren(jsonEntry["Children"]);
	};

	virtual void ConfigLoadChildren(nlohmann::json& jsonParent)
	{
		for (ElementBase* const pElement : m_Children)
			pElement->ConfigLoad(jsonParent);
	};

	virtual void Render()
	{
		if (!m_stStyle.bVisible)
			return;

		RenderChildren();
	};

	virtual void RenderChildren()
	{
		for (ElementBase* const pElement : m_Children)
			pElement->Render();
	};
};

class Spacing : public ElementBase
{
public:
	Spacing() {};

	void Render() override
	{ 
		ImGui::Spacing(); 
	}
};

class Menu : public ElementBase
{
protected:

public:
	Menu(std::string sUnique, Style_t stStyle)
	{
		m_sUnique = sUnique;
		m_stStyle = stStyle;
	};

	void Render() override
	{
		ImGui::SetNextWindowSize(m_stStyle.vec2Size);
		ImGui::Begin(GetName().c_str(), &m_stStyle.bVisible, m_stStyle.iFlags);
		RenderChildren();
		ImGui::End();
	}

	void StartRender()
	{
		ImGui::SetNextWindowSize(m_stStyle.vec2Size);
		ImGui::Begin(GetName().c_str(), &m_stStyle.bVisible, m_stStyle.iFlags);
		RenderChildren();
	}

	void EndRender()
	{
		ImGui::End();
	}
};

class Child : public ElementBase
{
protected:
	ImGuiWindowFlags m_WindowFlags;
	std::function<ImVec2()> m_funcCallback = nullptr;

public:
	Child(ImGuiWindowFlags WindowFlags = 0)
	{
		m_WindowFlags = WindowFlags;
	};

	Child(std::function<ImVec2()> funcCallback = nullptr, ImGuiWindowFlags WindowFlags = 0)
	{
		m_funcCallback = funcCallback;
		m_WindowFlags = WindowFlags;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		if (m_stStyle.bSameLine)
			ImGui::SameLine(0.f, m_stStyle.flSpacing);

		if (m_funcCallback)
			m_stStyle.vec2Size = m_funcCallback();

		ImGui::BeginChild(GetName().c_str(), m_stStyle.vec2Size, m_stStyle.iFlags, m_WindowFlags);
		RenderChildren();
		ImGui::EndChild();
	};
};

class Text : public ElementBase
{
protected:

public:
	Text()
	{};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		if (m_stStyle.bSameLine)
			ImGui::SameLine(0.f, m_stStyle.flSpacing);

		ImGui::Text(GetName().c_str());
	};

};

class Button : public ElementBase
{
protected:
	std::function<void()> m_Callback = nullptr;

public:
	Button()
	{};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		if (m_stStyle.bSameLine)
			ImGui::SameLine(0.f, m_stStyle.flSpacing);

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
	std::string m_sPreviewlabel;
	std::function<void()> m_Callback;

public:
	Combo(std::string sPreviewlabel) :
		m_sPreviewlabel(sPreviewlabel)
	{};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		if (m_stStyle.bSameLine)
			ImGui::SameLine(0.f, m_stStyle.flSpacing);

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
};

class Checkbox : public ElementBase
{
protected:
	bool m_Value;

public:
	Checkbox(bool Value = false) : m_Value(Value)
	{};

	void ConfigSave(nlohmann::json& jsonParent) const override
	{
		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()] = nlohmann::json();

		jsonEntry["Value"] = std::to_string(m_Value);

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
			m_Value = std::stoi(jsonEntry["Value"].get<std::string>());

		if (jsonEntry.contains("Children"))
			ConfigLoadChildren(jsonEntry["Children"]);
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		if (m_stStyle.bSameLine)
			ImGui::SameLine(0.f, m_stStyle.flSpacing);

		ImGui::Checkbox(GetName().c_str(), &m_Value);

		RenderChildren();
	};

	inline bool GetValue() const
	{
		return m_Value;
	};

	inline void SetValue(const bool& n)
	{
		m_Value = n;
	};
};

class Hotkey : public ElementBase
{
protected:
	KeyBind* m_Key;

public:
	Hotkey(KeyBind* Key) : m_Key(Key)
	{};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		if (m_stStyle.bSameLine)
			ImGui::SameLine(0.f, m_stStyle.flSpacing);

		ImGui::Hotkey(GetName().c_str(), m_Key, m_stStyle.vec2Size);

		RenderChildren();
	};
};

class SliderFloat : public ElementBase
{
protected:
	float m_Value;
	float m_Min;
	float m_Max;
	const char* m_sFormat;

public:
	SliderFloat(float Value, float Min, float Max, const char* sFormat = "%.3f") :
		m_Value(Value), m_Min(Min), m_Max(Max), m_sFormat(sFormat)
	{};

	void ConfigSave(nlohmann::json& jsonParent) const override
	{
		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()] = nlohmann::json();

		jsonEntry["Value"] = std::to_string(m_Value);

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
			m_Value = std::stof(jsonEntry["Value"].get<std::string>());

		if (jsonEntry.contains("Children"))
			ConfigLoadChildren(jsonEntry["Children"]);
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		if (m_stStyle.bSameLine)
			ImGui::SameLine(0.f, m_stStyle.flSpacing);

		ImGui::SliderFloat(GetName().c_str(), &m_Value, m_Min, m_Max, m_sFormat, m_stStyle.iFlags);

		RenderChildren();
	};

	inline float GetValue() const
	{
		return m_Value;
	};

	inline void SetValue(const float& n)
	{
		m_Value = n;
	};
};

class SliderInt : public ElementBase
{
protected:
	int m_Value;
	int m_Min;
	int m_Max;
	const char* m_sFormat;

public:
	SliderInt(int Value, int Min, int Max, const char* sFormat = "%d") :
		m_Value(Value), m_Min(Min), m_Max(Max), m_sFormat(sFormat)
	{};

	void ConfigSave(nlohmann::json& jsonParent) const override
	{
		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()] = nlohmann::json();

		jsonEntry["Value"] = std::to_string(m_Value);

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
			m_Value = std::stoi(jsonEntry["Value"].get<std::string>());

		if (jsonEntry.contains("Children"))
			ConfigLoadChildren(jsonEntry["Children"]);
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		if (m_stStyle.bSameLine)
			ImGui::SameLine(0.f, m_stStyle.flSpacing);

		ImGui::SliderInt(GetName().c_str(), &m_Value, m_Min, m_Max, m_sFormat, m_stStyle.iFlags);

		RenderChildren();
	};

	inline int GetValue() const
	{
		return m_Value;
	};

	inline void SetValue(const int& n)
	{
		m_Value = n;
	};
};

class InputText : public ElementBase
{
protected:
	char* m_pBuffer;
	size_t m_ullBuffSize;
	ImGuiInputTextCallback m_InputTextCallback;
	void* m_pUserData;

public:
	InputText(char* pBuffer, size_t ullBuffSize, ImGuiInputTextCallback InputTextCallback = (ImGuiInputTextCallback)0, void* pUserData = nullptr) :
		m_pBuffer(pBuffer), m_ullBuffSize(ullBuffSize), m_InputTextCallback(InputTextCallback), m_pUserData(pUserData)
	{};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		if (m_stStyle.bSameLine)
			ImGui::SameLine(0.f, m_stStyle.flSpacing);

		ImGui::InputText(GetName().c_str(), m_pBuffer, m_ullBuffSize, m_stStyle.iFlags, m_InputTextCallback, m_pUserData);

		RenderChildren();
	};
};

class ColorPicker : public ElementBase
{
protected:
	ImVec4 m_Value;

public:
	ColorPicker()
	{};

	void ConfigSave(nlohmann::json& jsonParent) const override
	{
		nlohmann::json& jsonEntry = jsonParent[m_sUnique.c_str()] = nlohmann::json();

		jsonEntry["Value"] = std::to_string(ImGui::ColorConvertFloat4ToU32(m_Value));

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
			m_Value = ImGui::ColorConvertU32ToFloat4(std::stoul(jsonEntry["Value"].get<std::string>()));

		if (jsonEntry.contains("Children"))
			ConfigLoadChildren(jsonEntry["Children"]);
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		if (m_stStyle.bSameLine)
			ImGui::SameLine(0.f, m_stStyle.flSpacing);

		ImGui::ColorEdit4(GetName().c_str(), reinterpret_cast<float*>(&m_Value), m_stStyle.iFlags);

		RenderChildren();
	};

	inline ImVec4 GetValue() const
	{
		return m_Value;
	};

	inline void SetValue(const ImVec4& n)
	{
		m_Value = n;
	};
};