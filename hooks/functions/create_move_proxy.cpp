#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../../globals/crash_logger/crash_logger.h"
#include "../hooks.h"
#include "../../hacks/aimbot/aimbot.h"
#include "../../hacks/lagcomp/lagcomp.h"
#include "../../hacks/misc/misc.h"
#include "../../hacks/misc/scaleform/scaleform.h"
#include "../../hacks/movement/movement.h"
#include "../../hacks/prediction/prediction.h"
#include "../../hacks/movement/edgebug.h"
#include "../../hacks/movement_recorder/movement_recorder.h"
void __stdcall create_move( int sequence_number, float input_sample_frametime, bool is_active, bool& send_packet )
{
	LOG_CREATE_MOVE();
	static int log_counter = 0;
	log_counter++;
	if (log_counter % 100 == 0) {
		LOG_CAT("CREATE_MOVE", "Called " + std::to_string(g_crash_logger.get_create_move_calls()) + " times");
	}
	try {
		static auto original = g_hooks.m_create_move_proxy.get_original< decltype( &n_detoured_functions::create_move_proxy ) >( );
		if (!original) {
			LOG_ERROR("CREATE_MOVE: original function is null!");
			return;
		}
		original( g_interfaces.m_base_client, 0, sequence_number, input_sample_frametime, is_active );
		c_user_cmd* cmd                   = g_interfaces.m_input->get_user_cmd( sequence_number );
		c_verified_user_cmd* verified_cmd = g_interfaces.m_input->get_verified_cmd( sequence_number );
		if ( !cmd || !verified_cmd || !is_active ) {
			if (!cmd) LOG_CAT("CREATE_MOVE", "cmd is null");
			if (!verified_cmd) LOG_CAT("CREATE_MOVE", "verified_cmd is null");
			return;
		}
		g_ctx.m_cmd = cmd;
		if ( g_menu.m_opened || g_movement_recorder.m_editor_open ) {
			cmd->m_buttons &= ~( in_attack | in_second_attack | in_reload | in_use );
		}
		const auto local       = g_interfaces.m_client_entity_list->get< c_base_entity >( g_interfaces.m_engine_client->get_local_player( ) );
		const auto net_channel = g_interfaces.m_client_state->m_net_channel;
		if (!local) {
			LOG_CAT("CREATE_MOVE", "local player is null");
		}
		g_ctx.m_local = local;
		g_ctx.old_view_point = cmd->m_view_point;
		g_ctx.serverTime( cmd );
		LOG_CAT("CM_TRACE", "1_serverTime");
			g_prediction.update( );
			LOG_CAT("CM_TRACE", "2_prediction_update");
			g_scaleform.on_createmove( );
			LOG_CAT("CM_TRACE", "3_scaleform");
			g_menu.on_create_move( );
			[ & ]( ) {
			if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) || !g_ctx.m_cmd || g_ctx.m_local->get_observer_mode( ) != e_obs_mode::obs_mode_none ) {
				g_movement.m_edgebug_data.reset( );
				g_movement.m_pixelsurf_data.reset( );
				g_movement.m_autoduck_data.reset( );
				return;
			}
			LOG_CAT("CM_TRACE", "4_alive_enter");
			g_edgebug.PrePredictionEdgeBug( cmd );
			LOG_CAT("CM_TRACE", "5_edgebug_pre");
			g_movement.on_create_move_pre( );
			LOG_CAT("CM_TRACE", "6_movement_pre");
			g_misc.on_create_move_pre( );
			LOG_CAT("CM_TRACE", "7_misc_pre");
			if ( GET_VARIABLE( g_variables.fast_quak, bool ) ) {
				cmd->m_buttons |= in_bullrush;
			}
			g_prediction.begin( g_ctx.m_local, cmd );
			LOG_CAT("CM_TRACE", "8_prediction_begin");
			g_aimbot.on_create_move_post( );
			LOG_CAT("CM_TRACE", "9_aimbot");
			if ( GET_VARIABLE( g_variables.m_backtrack_enable, bool ) ) {
				Backtrack::run( cmd );
			}
			LOG_CAT("CM_TRACE", "10_backtrack");
			g_misc.stepan_dolbaeb_ne_smog_pts_realize( );
			LOG_CAT("CM_TRACE", "11_pts");
			g_prediction.end( g_ctx.m_local );
			LOG_CAT("CM_TRACE", "12_prediction_end");
			g_movement.on_create_move_post( );
			LOG_CAT("CM_TRACE", "13_movement_post");
			// during playback the recorder owns cmd entirely — running edgebug
			// post-predict over a desynced state overwrites the recorded duck
			// timing and breaks crouch-edgebug replay
			if ( !g_movement_recorder.m_playing && !g_movement_recorder.m_wish_to_start && !g_movement_recorder.m_going_to_start && !g_movement_recorder.m_smoothing_to_start ) {
				g_edgebug.EdgeBugPostPredict( cmd );
			}
			LOG_CAT("CM_TRACE", "14_edgebug_post");
			g_movement_recorder.on_create_move( cmd );
			LOG_CAT("CM_TRACE", "13b_movement_recorder");
			LOG_CAT("CM_TRACE", "15_done");
		}( );
		try {
			if ( GET_VARIABLE( g_variables.m_backtrack_enable, bool ) && GET_VARIABLE( g_variables.m_backtrack_extend, bool ) ) {
				Backtrack::UpdateIncomingSequences( );
			}
		} catch (...) {
			LOG_ERROR("CREATE_MOVE: Exception in Backtrack::UpdateIncomingSequences");
		}
		if ( !cmd ) {
			LOG_ERROR("CREATE_MOVE: cmd became null before finalization");
			return;
		}
		cmd->m_view_point.normalize( );
		cmd->m_view_point.clamp( );
		g_ctx.m_last_tick_yaw = cmd->m_view_point.m_y;
		g_ctx.last_view_point    = cmd->m_view_point;
		verified_cmd->m_user_cmd = *cmd;
		verified_cmd->m_hash_crc = cmd->get_check_sum( );
	} catch (const std::exception& e) {
		LOG_CAT("CREATE_MOVE", std::string("Exception caught: ") + e.what());
	} catch (...) {
		LOG_ERROR("CREATE_MOVE: Unknown exception caught");
	}
}
__declspec( naked ) void __fastcall n_detoured_functions::create_move_proxy( [[maybe_unused]] void* ecx, [[maybe_unused]] void* edx,
                                                                             [[maybe_unused]] int sequence_number,
                                                                             [[maybe_unused]] float input_sample_frametime,
                                                                             [[maybe_unused]] bool is_active )
{
	__asm
	{
		push	ebp
		mov		ebp, esp; 
		push	ebx; 
		push	esp; 
		push	dword ptr[is_active]; 
		push	dword ptr[input_sample_frametime]; 
		push	dword ptr[sequence_number];
		call	create_move
		pop		ebx
		pop		ebp
		retn	0Ch
	}
}
