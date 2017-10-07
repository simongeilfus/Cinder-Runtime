#pragma once

#include "cinder/gl/wrapper.h"
#include "runtime/Virtual.h"
#include "runtime/ClassWatcher.h"

class Clear {
public:
	rt_virtual void clear()
	{
		ci::gl::clear( ci::ColorA( 0, 0, 0, 1 ) );
	}

#if ! defined( RT_COMPILED )
	void* operator new( size_t size )
	{
		void * ptr = ::operator new( size );
		auto headerPath = ci::fs::absolute( ci::fs::path( __FILE__ ) );
		rt::ClassWatcher<Clear>::instance().watch( static_cast<Clear*>( ptr ), "Clear", { headerPath }, CI_RT_INTERMEDIATE_DIR / "runtime/Clear/build/Clear.dll", rt::Compiler::BuildSettings().default() );
		return ptr;
	}
	void operator delete( void* ptr )
	{
		rt::ClassWatcher<Clear>::instance().unwatch( static_cast<Clear*>( ptr ) );
		::operator delete( ptr );
	}
#endif
};