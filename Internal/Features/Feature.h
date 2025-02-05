#pragma once
#include "pch.h"

class BaseFeature
{
public:
	BaseFeature() {
		RegisterFeature(this);
	};

	/**
	 * Called upon menu creation. Create any and all menu elements for this feature here.
	 */
	virtual bool SetupMenu() { return true; };

	/**
	 * Called once upon startup for hooking or variable initialization.
	 */
	virtual bool Setup() { return true; };

	/**
	 * Called once upon shutdown to handle cleanup such as restoring hooks.
	 */
	virtual void Destroy() {};

	/**
	 * Called at the start of every frame for input handling.
	 */
	virtual void HandleInput() {};

	/**
	 * Called once every frame.
	 */
	virtual void Render() {};

	/**
	 * Called 10 times per second in the main thread.
	 */
	virtual void Run() {};

	/**
	 * Called every frame while the menu is open to update any menu elements.
	 */
	virtual void HandleMenu() {};

	/**
	 * Returns the name of this feature for logging purposes.
	 */
	virtual std::string GetName() = 0;

private:
	static void RegisterFeature(BaseFeature* pFeature)
	{
		Framework::g_vecFeatures.emplace_back(pFeature);
	}
};