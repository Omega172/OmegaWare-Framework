# Important Info
After the refactor, I am still finding some instances where things were not renamed or fixed, if you notice any of these issues let me know or make a PR.

#  The Internal Framework
The framework itself can be configured inside of `FrameworkConfig.h`

You can see this framework being used in these places:
-  [Satisfactory Cheat](https://github.com/Omega172/Satisfactory-Cheat)
-  [Crush Crush Cheat](https://github.com/Omega172/Crush-Crush-Cheat)
-  [Deep Rock Galactic](https://github.com/Omega172/Deep-Rock-Galactic-Cheat)
 
## FrameworkConfig.h
`FRAMEWORK_VERSION`:  Is the current version of the framework in a Major.Minor.Rework format Ex. 6.8.6 which is the current version at the time of writing.

`FRAMEWORK_X64`: Is whether or not the framework is currently in a 64 bit configuration, this is set automatically.

  `FRAMEWORK_OTHER |	FRAMEWORK_UNREAL	| FRAMEWORK_UNITY`: These defines are used to set what interfaces and init functions the framework should based on the target engine type.

`FRAMEWORK_RENDER_DYNAMIC`: Is used to let the framework to attempt to decide what rendering backend should be hooked and used
`FRAMEWORK_RENDER_D3D11 | FRAMEWORK_RENDER_D3D12`: These defines are used to manually set what rendering backend to use

## UnrealConfig.h
`SDK_IMPORTED`: This is manually defined and is there to remind you to import an SDK before you attempt to build or add any features.

## UnityConfig.h
`MONO_DLL`: The name of the mono DLL the target application uses by default it is `mono-2.0-bdwgc.dll`
`DEFAULT_ASSEMBLY_NAME`: The path to the default Assemply-CSharp.dll, this should be changed and by default is `".\\GAME_NAME_Data\\Managed\\Assembly-CSharp.dll"`
