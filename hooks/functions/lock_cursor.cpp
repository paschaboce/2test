#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../../hacks/movement_recorder/movement_recorder.h"
#include "../hooks.h"
extern bool point_menu_is_opened( );
bool g_game_menu_open = false;
void __fastcall n_detoured_functions::lock_cursor( void* ecx, void* edx )
{
	static auto original = g_hooks.m_lock_cursor.get_original< decltype( &n_detoured_functions::lock_cursor ) >( );
	if ( g_menu.m_opened || g_movement_recorder.m_editor_open ) {
		g_interfaces.m_surface->unlock_cursor( );
		return;
	}
	original( ecx, edx );
}