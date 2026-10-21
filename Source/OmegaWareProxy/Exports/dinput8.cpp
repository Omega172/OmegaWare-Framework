#ifdef _WIN64
#pragma comment(linker, "/EXPORT:DirectInput8Create=\\\\.\\GLOBALROOT\\SystemRoot\\System32\\dinput8.DirectInput8Create,@1")
#pragma comment(linker, "/EXPORT:DllCanUnloadNow=\\\\.\\GLOBALROOT\\SystemRoot\\System32\\dinput8.DllCanUnloadNow,@2")
#pragma comment(linker, "/EXPORT:DllGetClassObject=\\\\.\\GLOBALROOT\\SystemRoot\\System32\\dinput8.DllGetClassObject,@3")
#pragma comment(linker, "/EXPORT:DllRegisterServer=\\\\.\\GLOBALROOT\\SystemRoot\\System32\\dinput8.DllRegisterServer,@4")
#pragma comment(linker, "/EXPORT:DllUnregisterServer=\\\\.\\GLOBALROOT\\SystemRoot\\System32\\dinput8.DllUnregisterServer,@5")
#pragma comment(linker, "/EXPORT:GetdfDIJoystick=\\\\.\\GLOBALROOT\\SystemRoot\\System32\\dinput8.GetdfDIJoystick,@6")
#else
#pragma comment(linker, "/EXPORT:DirectInput8Create=\\\\.\\GLOBALROOT\\SystemRoot\\SysWOW64\\dinput8.DirectInput8Create,@1")
#pragma comment(linker, "/EXPORT:DllCanUnloadNow=\\\\.\\GLOBALROOT\\SystemRoot\\SysWOW64\\dinput8.DllCanUnloadNow,@2")
#pragma comment(linker, "/EXPORT:DllGetClassObject=\\\\.\\GLOBALROOT\\SystemRoot\\SysWOW64\\dinput8.DllGetClassObject,@3")
#pragma comment(linker, "/EXPORT:DllRegisterServer=\\\\.\\GLOBALROOT\\SystemRoot\\SysWOW64\\dinput8.DllRegisterServer,@4")
#pragma comment(linker, "/EXPORT:DllUnregisterServer=\\\\.\\GLOBALROOT\\SystemRoot\\SysWOW64\\dinput8.DllUnregisterServer,@5")
#pragma comment(linker, "/EXPORT:GetdfDIJoystick=\\\\.\\GLOBALROOT\\SystemRoot\\SysWOW64\\dinput8.GetdfDIJoystick,@6")
#endif
