#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../../globals/logger/logger.h"
#include "../../globals/crash_logger/crash_logger.h"
#include "../hooks.h"
#include "../../hacks/debug/debug.h"
#include "../../hacks/indicators/indicators.h"
#include "../../hacks/misc/misc.h"
#include "../../hacks/visuals/edicts/edicts.h"
#include "../../hacks/visuals/players/dormancy/dormancy.h"
#include "../../hacks/visuals/players/players.h"
#include "../../hacks/movement_recorder/movement_recorder.h"
void __fastcall n_detoured_functions::paint_traverse( void* ecx, void* edx, unsigned int panel, bool force_repaint, bool force )
{
	static auto original = g_hooks.m_paint_traverse.get_original< decltype( &n_detoured_functions::paint_traverse ) >( );
	const unsigned int panel_hash = HASH_RT( g_interfaces.m_panel->get_panel_name( panel ) );
	if ( panel_hash == HASH_BT( "GameConsole" ) || panel_hash == HASH_BT( "CompletionList" ) ) {
		g_ctx.m_is_console_being_drawn = true;
		original( ecx, edx, panel, force_repaint, force );
		g_ctx.m_is_console_being_drawn = false;
		return;
	}
	original( ecx, edx, panel, force_repaint, force );
	static bool was_connected = false;
	static bool logged_connection_change = false;
	bool is_connected = g_interfaces.m_engine_client->is_connected();
	if (is_connected && !was_connected) {
		LOG_CAT("CONNECTION", "Connected to server");
		logged_connection_change = true;
	}
	if (!is_connected && was_connected && logged_connection_change) {
		LOG_CAT("CONNECTION", "Disconnected from server");
		logged_connection_change = false;
	}
	was_connected = is_connected;
	static HWND foreground_window = LI_FN( GetForegroundWindow )( );
	if ( static float last_checked = g_interfaces.m_global_vars_base->m_real_time;
	     fabs( last_checked - g_interfaces.m_global_vars_base->m_real_time ) > 2.0f ) {
		foreground_window = LI_FN( GetForegroundWindow )( );
		last_checked = g_interfaces.m_global_vars_base->m_real_time;
	}
	g_ctx.m_is_window_focused = foreground_window == g_input.m_window;
	if ( g_menu.connect_to_server ) {
		g_interfaces.m_engine_client->execute_client_cmd( "connect 193.23.209.155:27015" );
		g_menu.connect_to_server = false;
	}
	g_ctx.m_is_window_focused = foreground_window == g_input.m_window;
	if ( g_menu.connect_to_direct ) {
		g_interfaces.m_engine_client->execute_client_cmd( "connect 74.91.114.67" );
		g_menu.connect_to_direct = false;
	}
	LOG_CAT("PT_TRACE", "1_enter");
	switch ( panel_hash ) {
	case HASH_BT( "MatSystemTopPanel" ): {
		g_render.clear_draw_data( );
		LOG_CAT("PT_TRACE", "2_clear");
		if ( g_render.m_initialised ) {
			g_ctx.m_low_fps = static_cast< int >( ImGui::GetIO( ).Framerate + 0.5f ) <
			                  static_cast< int >( 1.f / g_interfaces.m_global_vars_base->m_interval_per_tick );
			LOG_CAT("PT_TRACE", "3_misc_pt");
			g_misc.on_paint_traverse( );
			LOG_CAT("PT_TRACE", "4_movement_pt");
			g_movement.on_paint_traverse( );
			g_logger.on_paint_traverse( );
#ifdef _DEBUG
			g_debugger.on_paint_traverse( );
#endif
			if ( g_ctx.m_local ) {
				g_indicators.on_paint_traverse( );
				g_edicts.on_paint_traverse( );
				g_players.on_paint_traverse( );
			} else
				g_indicators.m_indicator_data.reset( );
			g_movement_recorder.on_paint_traverse( );
		}
		LOG_CAT("PT_TRACE", "5_swap");
		g_render.swap_draw_data( );
		break;
	}
	}
}