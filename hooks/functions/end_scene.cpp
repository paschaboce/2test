#include "../../globals/includes/includes.h"
#include "../../hacks/misc/misc.h"
#include "../hooks.h"
#include "../../hacks/movement/movement.h"
#include "../../hacks/movement_recorder/movement_recorder.h"
#include "../../hacks/indicators/indicators.h"
#include "../../globals/reshade/reshade.h"
long __stdcall n_detoured_functions::end_scene( IDirect3DDevice9* device )
{
	static auto original      = g_hooks.m_end_scene.get_original< decltype( &n_detoured_functions::end_scene ) >( );
	static void* used_address = nullptr;
	static bool reshade_detected = false;
	static bool first_call = true;

	if ( g_ctx.m_unloading ) {
		if ( g_render.m_initialised ) {
			g_render.on_release( );
			g_render.m_initialised = false;
		}
		return original( device );
	}

	if ( first_call ) {
		first_call = false;
		reshade_detected = g_reshade.is_reshade_loaded( );
		if ( reshade_detected ) {
			g_console.print( "ReShade detected - using compatibility mode" );
		}
	}
	if ( !used_address ) {
		MEMORY_BASIC_INFORMATION memory_basic_information = { };
		VirtualQuery( _ReturnAddress( ), &memory_basic_information, sizeof( MEMORY_BASIC_INFORMATION ) );
		char module_name[ MAX_PATH ] = { };
		GetModuleFileName( static_cast< HMODULE >( memory_basic_information.AllocationBase ), module_name, MAX_PATH );
		if ( strstr( module_name, ( "gameoverlayrenderer.dll" ) ) )
			used_address = _ReturnAddress( );
	}
	if ( _ReturnAddress( ) != used_address )
		return original( device );
	if ( reshade_detected ) {
		return original( device );
	}
	if ( g_reshade.is_reshade_menu_open( ) ) {
		return original( device );
	}
	g_render.on_end_scene(
		[ & ]( ) {
			g_menu.on_end_scene( );
			g_misc.on_end_scene( );
			g_movement.on_end_scene( );
			g_movement_recorder.on_end_scene( );
			g_indicators.on_end_scene( );
		},
		device );
	return original( device );
}