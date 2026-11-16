#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../hooks.h"
#include "../../hacks/entity_cache/entity_cache.h"
#include "../../hacks/avatar_cache/avatar_cache.h"
#include "../../hacks/misc/scaleform/scaleform.h"
#include "../../hacks/visuals/edicts/edicts.h"
void __stdcall n_detoured_functions::level_init_pre_entity( const char* map_name )
{
	static auto original = g_hooks.m_level_init_pre_entity.get_original< decltype( &n_detoured_functions::level_init_pre_entity ) >( );
	if ( !map_name ) {
		return original( map_name );
	}
	g_edicts.reset( );
	g_avatar_cache.reset( );
	if ( g_interfaces.m_global_vars_base && g_interfaces.m_global_vars_base->m_interval_per_tick > 0.0f ) {
		const float rate = 1.f / g_interfaces.m_global_vars_base->m_interval_per_tick;
		static float original_updaterate = 64.0f;
		static float original_cmdrate = 64.0f;
		static bool first_run = true;
		if ( first_run ) {
			auto updaterate_conv = g_convars[ HASH_BT( "cl_updaterate" ) ];
			auto cmdrate_conv = g_convars[ HASH_BT( "cl_cmdrate" ) ];
			if ( updaterate_conv ) original_updaterate = updaterate_conv->get_float();
			if ( cmdrate_conv ) original_cmdrate = cmdrate_conv->get_float();
			first_run = false;
		}
		auto updaterate_conv = g_convars[ HASH_BT( "cl_updaterate" ) ];
		auto cmdrate_conv = g_convars[ HASH_BT( "cl_cmdrate" ) ];
		if ( updaterate_conv ) {
			if ( rate >= 16.0f && rate <= 128.0f ) {
				updaterate_conv->set_value( rate );
			} else {
				updaterate_conv->set_value( original_updaterate );
			}
		}
		if ( cmdrate_conv ) {
			if ( rate >= 16.0f && rate <= 128.0f ) {
				cmdrate_conv->set_value( rate );
			} else {
				cmdrate_conv->set_value( original_cmdrate );
			}
		}
	}
	original( map_name );
	if ( g_interfaces.m_surface ) {
		using lock_cursor_fn = void ( __fastcall* )( void* ecx, void* edx );
		static auto lock_orig = g_hooks.m_lock_cursor.get_original< lock_cursor_fn >( );
		if ( lock_orig )
			lock_orig( g_interfaces.m_surface, nullptr );
	}
}