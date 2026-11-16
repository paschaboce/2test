#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../hooks.h"
#include "../../hacks/animations/animations.h"
#include "../../hacks/debug/debug.h"
#include "../../hacks/entity_cache/entity_cache.h"
#include "../../hacks/inventory/inventory_changer.h"
#include "../../hacks/knife_changer/knife_changer.h"
#include "../../hacks/movement/movement.h"
#include "../../hacks/visuals/edicts/edicts.h"
#include "../../hacks/visuals/screen/screen.h"
#include "../../hacks/movement/edgebug.h"
#include "../../hacks/movement_recorder/movement_recorder.h"
#include "../../globals/crash_logger/crash_logger.h"
void __fastcall n_detoured_functions::frame_stage_notify( void* ecx, void* edx, e_client_frame_stage stage )
{
	static auto original = g_hooks.m_frame_stage_notify.get_original< decltype( &n_detoured_functions::frame_stage_notify ) >( );
	if ( g_ctx.m_unloading )
		return original( ecx, edx, stage );
	if ( !g_interfaces.m_engine_client || !g_interfaces.m_client_state ) {
		return original( ecx, edx, stage );
	}
	if ( !g_interfaces.m_engine_client->is_connected_safe( ) ) {
		g_edicts.reset( );
		return original( ecx, edx, stage );
	}
	static auto backtrackInit = ( Backtrack::init( ), false );
	if ( !g_ctx.m_local )
		return original( ecx, edx, stage );
	LOG_CAT("FSN_TRACE", "1_edicts");
	g_edicts.on_frame_stage_notify( stage );
	LOG_CAT("FSN_TRACE", "2_movement");
	g_movement.on_frame_stage_notify( stage );
	LOG_CAT("FSN_TRACE", "3_edgebug");
	g_edgebug.frame_stage( stage );
	LOG_CAT("FSN_TRACE", "4_animations");
	g_animations.on_frame_stage_notify( stage );
	g_movement_recorder.on_frame_stage( static_cast< int >( stage ) );
	g_invetory_changer.OnFrameStageNotify( stage );
	g_knife_changer.on_frame_stage_notify( stage );
	g_screen.on_frame_stage_notify( stage );
#ifdef _DEBUG
	g_debugger.on_frame_stage_notify( stage );
#endif
	if ( stage == net_update_end ) {
		for ( auto events = g_interfaces.m_client_state->m_events; events; events = events->m_next ) {
			if ( events->m_class_id )
				break;
			events->m_fire_delay = 0.f;
		}
		g_interfaces.m_engine_client->fire_events( );
		[ & ]( ) {
#ifdef _DEBUG
			if ( !GET_VARIABLE( g_variables.m_disable_interp, bool ) )
				return;
#else
			return;
#endif
			if ( g_ctx.m_local->get_observer_mode( ) != e_obs_mode::obs_mode_none )
				return;
			g_entity_cache.enumerate( e_enumeration_type::type_players, [ & ]( c_base_entity* entity ) {
				if ( !entity->is_valid_enemy( ) )
					return;
				const auto var_map = entity->get_var_map( );
				if ( !var_map )
					return;
				if ( var_map->interpolated_entries > 20108 )
					return;
				for ( int i = 0; i < var_map->interpolated_entries; i++ )
					var_map->entries[ i ].needs_to_interpolate = false;
			} );
		}( );
	}
	if ( g_interfaces.m_engine_client->is_in_game( ) )
		Backtrack::update( stage );
	original( ecx, edx, stage );
}
