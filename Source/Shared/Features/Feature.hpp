#pragma once
#include "pch.h"

#include <vector>

// Owns the list of registered features explicitly, rather than relying on a bare global
// living in whichever target happens to include this header first.
class FeatureRegistry
{
public:
	static std::vector<class BaseFeature*>& GetFeatures()
	{
		static std::vector<BaseFeature*> vecFeatures;
		return vecFeatures;
	}
};

class BaseFeature
{
public:
	BaseFeature() {
		FeatureRegistry::GetFeatures().emplace_back(this);
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
};
