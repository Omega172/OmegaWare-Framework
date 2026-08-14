#pragma once

// Loads the real version.dll from System32 and resolves every export ProxyMain.cpp forwards.
// Must succeed before anything else runs, since the forwarded exports need to work immediately;
// called from Internal/dllmain.cpp's DllMain under #ifdef PROXY.
bool ProxyInitRealVersionDll();
