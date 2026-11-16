#pragma once
#define WINDOWS_IGNORE_PACKING_MISMATCH
#include <Windows.h>
#include <string>
#include <vector>
#include <deque>
#include "../../game/sdk/classes/c_vector.h"
#include "../../game/sdk/classes/c_angle.h"
class c_user_cmd;
namespace n_edgebug
{
	struct predicted_cmd_t {
		c_angle viewangle;
		float forwardmove;
		float sidemove;
		int buttons;
		c_vector origin;
	};
	struct edge_context_t {
		float backup_vel_z = 0.f;
		float current_vel_z = 0.f;
		float gravity_tick = 0.f;
		bool was_falling = false;
		bool on_ground = false;
	};
	struct impl_t {
	public:
		void PrePredictionEdgeBug( c_user_cmd* cmd );
		void EdgeBugPostPredict( c_user_cmd* cmd );
		void edgebug_lock( float& x, float& y );
		void frame_stage( int stage );
		bool m_found = false;
		int m_found_tick = 0;
		int m_prediction_ticks = 0;
		int m_prediction_timestamp = 0;
		int m_mouse_offset = 0;
		bool m_ducked = false;
	private:
		bool EdgeBugCheck( const edge_context_t& ctx );
		bool IsNearEdge( );
		void ApplyAutoStrafe( c_user_cmd* cmd, bool duck );
		void ApplyAutoStrafeToEdge( c_user_cmd* cmd, bool duck );
		void ReStorePrediction( );
		c_vector m_backup_velocity{ };
		c_vector m_backup_origin{ };
		int m_backup_move_type = 0;
		int m_backup_flags = 0;
		c_angle m_backup_viewangle{ };
		predicted_cmd_t m_predicted_cmds[256];
		int m_found_variant = 0;
		int m_replay_hits = 0;
		c_angle m_aim_target{ };
	};
}
inline n_edgebug::impl_t g_edgebug{ };