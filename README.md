### Cinder-Runtime

**Work in progress.**   

*Previous implementation has moved to [mac_cling branch](https://github.com/simongeilfus/Cinder-Runtime/tree/mac_cling).*


##### Using Cinder-Runtime in a new project

###### Build Cinder as a dll
Clone [msw_dll branch](https://github.com/simongeilfus/Cinder/tree/msw_dll) and build `Debug_DLL` and `Release_DLL` targets.

###### Adding dll targets to existing project:

1. Create new configuration for Debug_DLL and Release_DLL (use "copy setting from" in the configuration manager)
2. Project Properties / General / Platform Toolset: change to "Visual Studio 2015 (v140)"
3. Project Properties / C++ / Code Generation: change Runtime Library to /MDd and /MD
4. Project Properties / C++ / Preprocessor / Preprocessor Definitions: Add CINDER_DLL
5. Project Properties / Linker / Input / Additional Dependencies: Add `glload.lib` and `glloadD.lib`
6. Project Properties / Build Events / Post-Build Event / Command Line: Paste `xcopy /y /d "..\..\..\lib\msw\$(PlatformTarget)\$(Configuration)\$(PlatformToolset)\cinder.dll" "$(OutDir)"` and change the first path to be relative to the project.



###### Wrap the class instances you want to be reloadable into a `rt::shared_ptr` object:
```c++
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Object.h"
#include "runtime/Memory.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class RTCTestApp : public App {
  public:
	RTCTestApp();
	void draw() override;

	rt::shared_ptr<Object> mObject;
};

RTCTestApp::RTCTestApp()
{
	mObject = rt::make_shared<Object>();
}

void RTCTestApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 

	if( mObject ) {
		mObject->draw();
	}
}

CINDER_APP( RTCTestApp, RendererGl )
```

###### Make functions that you intent to use outside the class and want to be reloadable `virtual`:
```c++
#pragma once

class Object {
public:
	Object();

	virtual void draw();
};
```

The system should work for both header only classes and .h/.cpp classes. The later should be much faster as it allow the use of precompiled headers (which is being taking care of automatically by the `runtime::ModuleManager`).

**That's it! If you want a finer control see `runtime::Compiler` and `runtime::Module` classes.**