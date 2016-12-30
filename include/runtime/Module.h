#pragma once

#include "Compiler.h"
#include "cinder/Signals.h"

namespace runtime {

using ModuleRef = std::shared_ptr<class Module>;

class Module : public std::enable_shared_from_this<Module> {
public:
	//! Returns a new ref-counted Module object
	static ModuleRef create( const ci::fs::path &path );
	//! Constructs a new Module object
	Module( const ci::fs::path &path );
	//! Destroys the Module object, release its handles and delete the temporary files
	~Module();

	//! Updates the module with a new handle
	void updateHandle();
	//! Changes the disk name of the current module to enable writing a new one 
	void unlockHandle();
	
#if defined( CINDER_MSW )
	// Alias to Windows HINSTANCE
	using Handle = void*;
#else
#endif
	
	//! Returns the current Handle to the module
	Handle getHandle() const;
	//! Returns the current path to the module
	ci::fs::path getPath() const;
	//! Returns the temporary path to the module
	ci::fs::path getTempPath() const;
	//! Returns whether the current Handle is valid
	bool isValid() const;
	
	//! Sets the map of exported symbols and their mangled names
	void setSymbols( const std::map<std::string,std::string>& symbols );
	//! Returns a map of exported symbols and their mangled names
	const std::map<std::string,std::string>& getSymbols() const;
	
	//! Returns the signal used to notify when the Module/Handle is about to be unloaded
	ci::signals::Signal<void(const ModuleRef&)>& getCleanupSignal();
	//! Returns the signal used to notify Module/Handle changes
	ci::signals::Signal<void(const ModuleRef&)>& getChangedSignal();

protected:
	Handle			mHandle;
	ci::fs::path	mPath, mTempPath;
	
	ci::signals::Signal<void(const ModuleRef&)> mChangedSignal;
	ci::signals::Signal<void(const ModuleRef&)> mCleanupSignal;
	std::map<std::string,std::string> mSymbols;
};

using ModuleManagerRef = std::shared_ptr<class ModuleManager>;

class ModuleManager {
public:
	static ModuleManagerRef create();
	static ModuleManagerRef get();
	ModuleManager();
	~ModuleManager();
	
	ModuleRef add( const ci::fs::path &path );

protected:
	
	std::vector<ModuleRef>	mModules;
	CompilerRef				mCompiler;
};

} // namespace runtime

namespace rt = runtime;