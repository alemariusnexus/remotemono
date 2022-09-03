RemoteMono
==========

RemoteMono is a modern C++ (requires C++17) header-only library that allows you to call the functions of the [Mono Embedded API](http://docs.go-mono.com/?link=root:/embed) in a remote process from the local process. It was mainly intended for hacking Unity games, but it can work with any program that exports the Mono Embedded API.

It was inspired by [Cheat Engine's MonoDataCollector](https://github.com/cheat-engine/cheat-engine/tree/master/Cheat%20Engine/MonoDataCollector) and provides similar functionality in a library form, although it works quite differently internally.

Note that RemoteMono is a bit different from what many people seem to do in memory hacking: You use RemoteMono in your own **local process** in order to call Mono API functions in a **remote process** via RPC. If you use DLL injection, your own code is already running in the remote process, in which case you can just get pointers to the raw Mono API functions and call them directly like you would any other function. **RemoteMono is for when your process and the one you're hacking are different**. With RemoteMono, you can do things like create C# objects, get and set field and property values, call methods, and other things without having to do any injection or modifying the remote process yourself. You can probably even write a .NET assembly loader with it, though I haven't tried that.

Before trying RemoteMono, please at least read the chapters **Requirements** and **Caveats** below to make sure it can do what you want it do do.

Requirements
------------

Before anything else, there are a few requirements for using RemoteMono in your project:

* You need the memory hacking library [**BlackBone**](https://github.com/DarthTon/Blackbone).
* Only **Windows** is supported. Sorry, no Linux, Mac OS, Android, or anything else. Mostly due to BlackBone.
* Currently, BlackBone only builds in **Visual Studio 2017+**. RemoteMono uses C++17 features heavily, so older versions of Visual Studio are not supported. I would love to add support for MinGW or Clang, but as long as RemoteMono depends on BlackBone and BlackBone doesn't support it, it's not gonna happen.

If you want to build the unit tests, sample project or documentation, there are additional requirements:

* For the unit tests and/or the sample project, you need [CMake](https://cmake.org/). You *don't* need it for using RemoteMono itself.
* For the unit tests, you need [Google Test](https://github.com/google/googletest) and a working installation of [Mono](https://www.mono-project.com/).
* For building the documentation, you need [Doxygen](https://www.doxygen.nl/index.html).

Aside from these requirements on your own project, there are a few cases in which RemoteMono will not work with certain remote processes. See the section **Caveats** below.

Notes about Building
--------------------

You don't need to build RemoteMono itself, but there are a few things to keep in mind when building your own project:

* Make sure to enable C++17 (`/std:c++17` in Visual Studio)
* With Visual Studio, you need to compile with the `/bigobj` option, because apparently RemoteMono uses too many template functions/methods to fit into a normal object file...
* **Do not try to include any headers in the `impl` directories directly**, it will blow up in your face. You should only have to include the headers in `remotemono`. You *can* however include the ones in `remotemono/impl/mono`.

For BlackBone (and other dependencies), I **strongly** recommend you compile them from source yourself. ~~You can currently find prebuilt dependencies in the GitHub releases page, but they might not work for you, and I might be too lazy to update them.~~ Currently, you **should not use the prebuilt dependencies** in the GitHub releases page. They contain a BlackBone version that was compiled with a "fix" that actually causes very sporadic crashes of the remote program. Maybe I'll update them one day, but don't hold your breath.

How do you use it?
------------------

RemoteMono is a header-only library, so there is no need to compile it. Just make sure that the directory containing the `remotemono` directory (which in turn contains `RMonoAPI.h`) is in your include path. That being said, RemoteMono depends on BlackBone, and you *do* need to compile that (or use some prebuilt library). You do not need the BlackBone driver, only the BlackBone library itself.

The main class you will be using is `RMonoAPI`, so take a look at `remotemono/impl/RMonoAPI_Def.h`. Most of the methods it provides have a 1:1 correspondence to one of the Mono Embedded API functions in the remote process, so take a look at the Mono Embedded API docs and just call the corresponding method in `RMonoAPI`.

You should also look at the classes `RMonoVariant` and `RMonoVariantArray`. It's very important that you know how to use them, especially when calling methods or working with properties. If you use them incorrectly, you can produce hard-to-trace crashes.

To write more compact and easier-to-read code, you can also look at the helper classes in `remotemono/helper`. They provide a more high-level wrapper around some of the functionality of `RMonoAPI`.

I don't currently have a tutorial or anything like that (other than what you're reading right now), so the best place to start is probably the...

Sample Project
---------------

In place of a tutorial, you can take a look at the sample project in `samples/redrunner`. It is a simple console application that uses RemoteMono to manipulate an instance of the open-source Unity platformer [Red Runner](https://github.com/BayatGames/RedRunner). Install and run Red Runner, enter the game world (where you can move around with your character), then start the sample executable from the console. It will attach RemoteMono to the running game and then do a bit of silly stuff to demonstrate a few things that RemoteMono can do: Increase your movement speed, enable double-jumping, play a sound, and add a bit of text to the bottom-left corner of the game that is updated with your character's current position. It's nothing fancy, and certainly doesn't show off all of the things you can do with the Mono API that's behind RemoteMono, but it's a good starting point.

The sample project now provides two implementations of the same code: One that just uses direct API in `RMonoAPI`, and one that uses the helper classes. You can take a look at both of them to see how they differ and to decide if you want to use the helper classes.

API Documentation
-----------------

You can find the API documentation [here](https://dlerch.de/remotemono/doc/), although that may not always be the latest version. Note that it is of limited use for many of RemoteMono's internals, since Doxygen isn't capable of parsing some of the template stuff correctly. It's best to look at the sources directly instead.

Features
--------

* It's a **header-only** library, so you don't have to compile it before using it and you can give it a try rather quickly. This would be a better selling point if it didn't depend on BlackBone. Meh, it counts.
* **Most of the important Mono API functions are exposed**, and ones that aren't exposed yet can usually be added very quickly. Just open an issue if you need something, or add it yourself.
* A **relatively simple interface that is quite close to the original Mono API**. If you want to know what a certain method does, just look for the corresponding method in the Mono Embedded API documentation.
* **Transparent handling of different ABIs** in the remote process. You can use the same local application for both 32-bit and 64-bit remotes without having to build two separate versions of your program.
* **Safe handling of GC-managed objects**. You don't need to worry about the Mono GC deleting or moving objects while you're still using them (as long as the remote process stays alive).
* For the most part, **memory management is automatic** through shared pointers for GC handles and caller-owned structures in remote memory.

Caveats
-------

There are a few things you have to be aware of before deciding to give RemoteMono a try:

* RemoteMono **only works with remote processes that export the Mono Embedded API**. Most regular Mono applications *do not* export it. The most common example of remotes that *do* export it are most Unity games.
* Presently, it does **not support IL2CPP**. IL2CPP is a Unity-specific reimplementation of parts of Mono, and Unity games compiled in this mode will not work with RemoteMono. IL2CPP's functions are slightly different from the original Mono API, and support for them has not been implemented yet in RemoteMono. It is *probably* not that difficult (if a little tedious) to support it, and it's something I may consider doing if and when I feel motivated.
* RemoteMono **does not expose every single Mono API function**. To support a Mono API function, its signature has to be defined in remotemono/impl/RMonoAPIBackend_Def.h, and a small corresponding method has to be added to remotemono/impl/RMonoAPI_*.h. Most of the time that's all that is needed. If it's currently not available, I have probably just not needed it yet. Feel free to open an issue about any functions you would like to see.
* RemoteMono can **only support API functions that are exported** in the remote process. Sometimes a remote process may use an older version of Mono that doesn't support certain functions, or functions may be missing for other reasons. In many such cases, RemoteMono can usually still be used, but you can't use those specific functions.
* RemoteMono is **not very resilient to programming errors**. If you pass a wrong pointer, a null value where one shouldn't be, or some other wrong value, either your local process or the remote process (or both at once) will crash, and debugging can be hard sometimes. This **will** happen to you. This is memory hacking, so I hope you're used to it.
* Because it's a heavily template-based header-only library, your own source files may take quite a while to compile if they include a RemoteMono header. For this reason, **it's very important to use precompiled headers** if you don't want to wait an eternity.
* ~~When using BlackBone without changes, RemoteMono's RPC calls are **very slow**, meaning <100 per second. This is [an issue with BlackBone](https://github.com/DarthTon/Blackbone/issues/430), and it's fixed by a pending pull request (referenced in that issue), which boosts this by a factor of around 100. Until that is merged, you can use [my fork of BlackBone](https://github.com/alemariusnexus/Blackbone) instead if you require higher RPC throughput.~~ As of 2021-06-20, this is fixed in BlackBone's upstream master branch.

How does it work?
-----------------

RemoteMono uses the memory hacking library [BlackBone](https://github.com/DarthTon/Blackbone), most importantly for its RPC functionality, but also for other features like reading and writing remote memory and listing remote process modules. BlackBone allows us to call the raw Mono API functions in the remote from the local process. RemoteMono is a layer on top of this RPC functionality that abstracts away a lot of the difficulties of using the Mono API properly.

While you *could* just call the raw Mono API functions directly using BlackBone or any other RPC mechanism, there are a few problems with that approach:

1. Having to copy around memory between the local and remote processes can be tedious
2. There are a few Mono API functions that take function pointers as parameters (the `mono_*_foreach()` functions), so how would you call those?
3. You'd need to find the remote function addresses and then specify the function signatures manually. When specifying the function signatures, you have to know whether the remote process is 32-bit or 64-bit.
4. It is actually unsafe to use many of the pointers returned by the Mono API from outside the remote process. This is because Mono uses a garbage collector, specifically (in newer versions) [a moving garbage collector](https://www.mono-project.com/docs/advanced/garbage-collector/sgen/). That means that unless certain conditions are fulfilled, pointers to managed objects can become invalid at any time because the underlying memory was moved by the GC. The only time when this does not happen is as long as a raw pointer to the memory can be found somewhere in a CPU register or on the stack *in the remote process*.

RemoteMono solves all of these problems. On the frontend, it provides a bunch of methods that closely match the ones of the original Mono API (with some extensions and changes to make them more C++-friendly). In the backend, it generates wrapper functions for many of the Mono API functions using the [AsmJit](https://asmjit.com/) that's bundled with BlackBone. These wrapper functions are generated automatically from a table of Mono API function signatures, and the generated code is then uploaded into the remote process. These wrapper functions in turn call the raw Mono API functions with some added pre- and post-processing. For example, the wrapper functions will take [GC handles](http://docs.go-mono.com/?link=xhtml%3adeploy%2fmono-api-gchandle.html) instead of the raw `MonoObject*` parameters, and convert them to raw pointers just before the call (so that the raw pointer will then be on the remote stack). Likewise, return values and output parameters of type MonoObject* will be converted to GC handles by the wrapper functions.

License
-------

RemoteMono is released under the terms of the [GNU Lesser General Public License](https://www.gnu.org/licenses/lgpl-3.0.en.html) version 3, or (at your option) any later version.

In short, this means that you *can* use RemoteMono in closed-source projects, as long as you publish any changes you make to RemoteMono itself under the terms of the LGPL. If that means anything to you, I would however urge you to think about making your project open-source in the spirit of knowledge sharing unless you have a really good reason not to.
