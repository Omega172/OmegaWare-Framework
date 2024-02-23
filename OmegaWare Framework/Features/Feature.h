#pragma once
#include "pch.h"

class Feature
{
private:
	bool Initialized = false;
	std::mutex Mutex; // Prevent problems with multithreading by using locks!

public:
	Feature() {};

	// Handle setup, like hook creation and variable initalization
	virtual bool Setup() = 0;

	// Handle clean up, like restoring hooked functions 
	virtual void Destroy() = 0;

	// Handle checking for any key/hotkey presses or holds needed for features
	virtual void HandleKeys() = 0;

	virtual void PopulateMenu() = 0;

	virtual void Render() = 0;

	// This should be run in the feature loop, used to run any acutal feature code like setting a value for godmode
	virtual void Run() = 0;

	// Save the feature's config to the config file
	virtual void SaveConfig() = 0;

	// Load the feature's config from the config file
	virtual void LoadConfig() = 0;
};