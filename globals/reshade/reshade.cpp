#include "reshade.h"
#include "../includes/includes.h"
#include <psapi.h>
bool n_reshade::impl_t::is_reshade_loaded( ) const
{
	if ( m_reshade_detected )
		return true;
	HMODULE reshade_modules[] = {
		GetModuleHandleA( "d3d9.dll" ),
		GetModuleHandleA( "dxgi.dll" ),
		GetModuleHandleA( "d3d11.dll" ),
		GetModuleHandleA( "d3d12.dll" ),
		GetModuleHandleA( "opengl32.dll" ),
		GetModuleHandleA( "vulkan-1.dll" ),
		GetModuleHandleA( "d3dcompiler_47.dll" ),
		GetModuleHandleA( "ReShade64.dll" ),
		GetModuleHandleA( "ReShade32.dll" )
	};
	for ( auto module : reshade_modules ) {
		if ( module ) {
			m_reshade_detected = true;
			return true;
		}
	}
	char module_name[ MAX_PATH ] = { };
	GetModuleFileNameA( GetModuleHandleA( nullptr ), module_name, MAX_PATH );
	if ( strstr( module_name, "reshade" ) || 
		 strstr( module_name, "ReShade" ) ||
		 strstr( module_name, "RESHADE" ) ) {
		m_reshade_detected = true;
		return true;
	}
	HMODULE modules[ 1024 ];
	DWORD needed;
	if ( EnumProcessModules( GetCurrentProcess( ), modules, sizeof( modules ), &needed ) ) {
		for ( DWORD i = 0; i < ( needed / sizeof( HMODULE ) ); i++ ) {
			char module_name[ MAX_PATH ] = { };
			if ( GetModuleFileNameA( modules[ i ], module_name, sizeof( module_name ) ) ) {
				if ( strstr( module_name, "reshade" ) || 
					 strstr( module_name, "ReShade" ) ||
					 strstr( module_name, "RESHADE" ) ) {
					m_reshade_detected = true;
					return true;
				}
			}
		}
	}
	return false;
}
bool n_reshade::impl_t::is_reshade_menu_open( ) const
{
	return m_reshade_menu_open;
}
void n_reshade::impl_t::set_reshade_menu_state( bool state )
{
	m_reshade_menu_open = state;
}
bool n_reshade::impl_t::should_skip_rendering( ) const
{
	return is_reshade_menu_open( );
}
bool n_reshade::impl_t::is_reshade_call( ) const
{
	if ( !m_reshade_address ) {
		MEMORY_BASIC_INFORMATION memory_basic_information = { };
		VirtualQuery( _ReturnAddress( ), &memory_basic_information, sizeof( MEMORY_BASIC_INFORMATION ) );
		char module_name[ MAX_PATH ] = { };
		GetModuleFileNameA( static_cast< HMODULE >( memory_basic_information.AllocationBase ), module_name, MAX_PATH );
		if ( strstr( module_name, "d3d9.dll" ) || 
			 strstr( module_name, "dxgi.dll" ) ||
			 strstr( module_name, "d3d11.dll" ) ||
			 strstr( module_name, "d3d12.dll" ) ||
			 strstr( module_name, "opengl32.dll" ) ||
			 strstr( module_name, "vulkan-1.dll" ) ||
			 strstr( module_name, "reshade" ) ) {
			m_reshade_address = _ReturnAddress( );
			return true;
		}
	}
	return _ReturnAddress( ) == m_reshade_address;
}
std::string n_reshade::impl_t::get_module_name( void* address ) const
{
	MEMORY_BASIC_INFORMATION memory_basic_information = { };
	VirtualQuery( address, &memory_basic_information, sizeof( MEMORY_BASIC_INFORMATION ) );
	char module_name[ MAX_PATH ] = { };
	GetModuleFileNameA( static_cast< HMODULE >( memory_basic_information.AllocationBase ), module_name, MAX_PATH );
	return std::string( module_name );
}
void n_reshade::impl_t::force_disable_rendering( bool disable )
{
	m_force_disable_rendering = disable;
}
bool n_reshade::impl_t::is_rendering_disabled( ) const
{
	return m_force_disable_rendering || m_reshade_menu_open;
}