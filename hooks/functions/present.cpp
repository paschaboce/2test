#include "../../globals/includes/includes.h"
#include "../../hacks/misc/misc.h"
#include "../hooks.h"
#include "../../hacks/movement/movement.h"
#include "../../hacks/movement_recorder/movement_recorder.h"
#include "../../hacks/indicators/indicators.h"
#include "../../globals/reshade/reshade.h"
long __stdcall n_detoured_functions::present( IDirect3DDevice9* device, const RECT* source_rect, const RECT* dest_rect, HWND dest_window_override, const RGNDATA* dirty_region )
{
	static auto original = g_hooks.m_present.get_original< decltype( &n_detoured_functions::present ) >( );
	static bool reshade_detected = false;
	static bool first_call = true;

	if ( g_ctx.m_unloading )
		return original( device, source_rect, dest_rect, dest_window_override, dirty_region );

	if ( first_call ) {
		first_call = false;
		reshade_detected = g_reshade.is_reshade_loaded( );
	}
	if ( reshade_detected ) {
		if ( !g_reshade.is_reshade_menu_open( ) ) {
			g_render.on_end_scene(
				[ & ]( ) {
					g_menu.on_end_scene( );
					g_misc.on_end_scene( );
					g_movement.on_end_scene( );
					g_movement_recorder.on_end_scene( );
					g_indicators.on_end_scene( );
				},
				device );
		}
	}
	return original( device, source_rect, dest_rect, dest_window_override, dirty_region );
}