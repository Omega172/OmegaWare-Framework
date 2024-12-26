#pragma once

#include "pch.h"
#include "../Headers/lua.hpp"

#include <variant>

namespace Lua {
	class Manager;

	/**
	 * For both loaded and unloaded scripts.
	 */
	class Script {
	private:
		Script() {};
		
		// Either a path to a file or code in a string.
		std::variant<std::filesystem::path, std::string> m_code{};
		size_t m_iHash{};
		std::string m_sUnique{};
		Manager* m_pManager;

	public:
		/**
		 * Returns if this script is currently loaded or not.
		 */
		__forceinline bool IsLoaded() const { return m_sUnique.size() > 0; };

		/**
		 * Simply creates an instance of script.
		 */
		static std::unique_ptr<Script> New(Manager* pManager, std::variant<std::filesystem::path, std::string> code);

		/**
		 * Loads this script into memory and starts executing it.
		 */
		bool Load();
		
		/**
		 * Unloads the scripts and its contents from the script manager it was given.
		 */
		void Unload();

		~Script() { Unload(); }
	};

	class Manager
	{
	friend Script;

	private:
		std::vector <std::unique_ptr<Script>> m_vecScripts;
		lua_State* m_pState;

	public:
		bool Initialize();

		void Shutdown();

		void UpdateScriptsList();



		Manager() {};

		~Manager() {};
		
	};
};