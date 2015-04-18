# Sigrlinn - An abstract graphics API library

### Intro
Sigrlinn is an abstract cross platform graphical API, that tries to make you forget about different GPU APIs and their evolution in time.

### What does the name mean?
Sigrlinn is the name of a pretty Scandinavian princess, daugther of the king Sváfnir of Sváfaland, from the [Helgakviða Hjörvarðssonar](http://en.wikipedia.org/wiki/Helgakvi%C3%B0a_Hj%C3%B6rvar%C3%B0ssonar) poem.

### Motivation
There are 2 major cross-platform problems: the APIs and the shaders.
In the nearest future we will have to support 5 (!) graphics APIs: DX11, DX12, OpenGL (ES?), Metal and Vulkan, and 4 (!) shader languages: HLSL, GLSL, Metal and Vulkan shaders (GLSL? I hope so).

SGFX tries to address only the first issue by providing an extremely thin layer that connects your application code to all 5 graphics APIs and does not stand in your way. It is designed with simplicity and flexibility in mind, it is open source (MIT license), and it is extremely small and easy to integrate.

It features DX12-like interface, all API functions are inside a single header file, while implementations are in several source files (typically one source file per gfx backend). Everything is somewhere around 5 KLOC, so it is also easy to contribute new features, modify and debug the code and even replace the parts you don’t like. The code is extremely clean and every comma is there for a reason.

### Target audience
This library is intented for those developers, who either have their own graphics engine, or use an open source one which may or will not be updated for some reasons. SGFX aims to simplify different platforms and different graphics API support, which in turn allows developers to focus more on the game visuals rather then fiddling with platform support and API-specific bugs.

The abstraction layer is as thin as possible, therefore it allows transparent API debugging and it makes that SGFX code will also run at _roughly_ the same performance across different rendering backends.

The performance statement should be explained. There are 2 different groups of supported APIs: next-gen group and legacy group. The code performance is the same between different APIs from the same group, e.g. SGFX with DX12 backend will run at the same speed as SGFX with Vulkan backend.

Here is the table that shows API groups:

Next-Gen | Legacy
---------|-----------
D3D12    | OpenGL
Mantle   | D3D11
Metal    |
Vulkan   |
GNM      |

SGFX is designed with common legacy engines in mind, therefore certain decisions are made to simplify integration process.
For instance, SGFX does not sort the draw calls for you, it does not ship with a custom cross-API shader language, etc.

Library complilation is also as simple, as possible: just drag and drop the source files to your solution or simply use the bundled [CMake](http://www.cmake.org/) script. No extra dependencies or any additional include directories are required (except for the DX SDK, but you probably already has this).

### Features
At the current stage of development the library supports:

API                  | Status
---------------------|--------------------------------------------------
D3D11                | Full support (starting from feature level 10.0) 
OpenGL 4.0           | Partial support (WIP)
Apple METAL          | WIP
D3D12                | WIP
AMD Mantle           | NDA closed source (will be shipped as Vulkan)
Khronos Vulkan       | See above (Mantle)
PlayStation 4 libGNM | Possible, but not planned for the nearest future
OpenGL ES 3.0        | Possible, but not planned for the nearest future

### What it is NOT
* It is not a game/graphics engine
* It is not a library that loads textures and 3D models
* It is not a [new universal standard that covers everyone's use cases](http://xkcd.com/927/)
* It is not a native graphics API - there will always be some limitations and overheads
* It is not a thing that you can easily drag-n-drop to your engine to enable fancy graphics

### Similar projects

* [BGFX](https://github.com/bkaradzic/bgfx) - A similar library that does the same things differently.
  - It supports DX9 and above, while SGFX aims at DX10 and above. Supporting higher feature level simplifies library design and removes a lot of legacy. *Use BGFX if you need to support DX9 or WindowsXP!* SGFX will likely never support those.
  - It strictly supports mobile platforms, while SGFX has no plans for it. This may change in the future, however.
  - It adresses cross-platform shaders issue, however this is somewhat limiting. SGFX intentionally does not address this.
  - It does *a lot* of additional work under the hood (e.g. draw call sorting). SGFX has a different puprose and different target audience, therefore it avoids doing extra work unless this is vital. SGFX assumes that users graphics engine already has draw call sorting and other important things as well.
  - It ships with its own tools for shader compiling, texture compressing, etc. which are meant to be used with the library. SGFX assumes that users graphics engine already has this.

* More similar libraries, anyone?

### Who is using it?

* BitBox, Ltd. is going to use the library in their current big project, [Life is Feudal](http://lifeisfeudal.com/), as well, as in the future projects (that’s why it is sponsoring SGFX development!)
* Bug-Zen is using the library in one of their unannounced projects (thanks for the valuable feedback!)
* [Peter Ellsum](https://github.com/cfehunter) from [Westwood 3D](http://w3dhub.com/) has provided lots of valuable feedback.
* [Luis Anton Rebollo](https://github.com/luisantonrebollo) from [BeamNG](http://www.beamng.com/) has participated in the library design and implementation.

### Credits

Name                          | Role                  | Position
------------------------------|-----------------------|-------------
Kirill Bazhenov aka bazhenovc | Library developer     | Lead graphics programmer at [BitBox Ltd](http://lifeisfeudal.com/)
Peter Ellsum aka CfeHunter    | Feedback              | Lead graphics programmer at [Westwood 3D](http://w3dhub.com/)
Luis Anton Rebollo            | Ideas and feedback    | Lead graphics programmer at [BeamNG](http://www.beamng.com/)
Linda MacGill aka Methelina   | Marketing and support | Technical art director at [Westwood 3D](http://w3dhub.com/)

### Contact
Current library maintainer is Kirill Bazhenov, bazhenovc AT gmail DOT com.
