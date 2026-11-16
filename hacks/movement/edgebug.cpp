#include "edgebug.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../prediction/prediction.h"
#include "../misc/misc.h"
#define M_PI 3.14159265358979323846

namespace {
	inline float get_edgebug_half_gravity_per_tick( )
	{
		static auto sv_gravity = g_interfaces.m_convar->find_var( "sv_gravity" );
		const float gravity = sv_gravity ? sv_gravity->get_float( ) : 800.f;
		const float interval = g_interfaces.m_global_vars_base ? g_interfaces.m_global_vars_base->m_interval_per_tick : 0.015625f;
		return gravity * 0.5f * interval;
	}
}

void CorrectMovement( c_user_cmd* cmd, c_angle wish_angle, c_angle old_angles )
{
	if ( old_angles.m_x == wish_angle.m_x && old_angles.m_y == wish_angle.m_y )
		return;
	c_vector wish_fwd, wish_right, wish_up, cmd_fwd, cmd_right, cmd_up;
	auto viewangles = old_angles;
	auto movedata = c_vector( cmd->m_forward_move, cmd->m_side_move, cmd->m_up_move );
	viewangles.normalize( );
	if ( !( g_ctx.m_local->get_flags( ) & 1 ) && viewangles.m_z != 0.f )
		movedata.m_y = 0.f;
	g_math.angle_vectors( wish_angle, &wish_fwd, &wish_right, &wish_up );
	g_math.angle_vectors( viewangles, &cmd_fwd, &cmd_right, &cmd_up );
	auto norm = []( float x, float y ) { float l = sqrt( x * x + y * y ); return l > 0.f ? 1.f / l : 0.f; };
	float wf = norm( wish_fwd.m_x, wish_fwd.m_y ), wr = norm( wish_right.m_x, wish_right.m_y );
	float cf = norm( cmd_fwd.m_x, cmd_fwd.m_y ), cr = norm( cmd_right.m_x, cmd_right.m_y );
	c_vector wfn( wf * wish_fwd.m_x, wf * wish_fwd.m_y, 0.f );
	c_vector wrn( wr * wish_right.m_x, wr * wish_right.m_y, 0.f );
	c_vector cfn( cf * cmd_fwd.m_x, cf * cmd_fwd.m_y, 0.f );
	c_vector crn( cr * cmd_right.m_x, cr * cmd_right.m_y, 0.f );
	float vx = wfn.m_x * movedata.m_x + wrn.m_x * movedata.m_y;
	float vy = wfn.m_y * movedata.m_x + wrn.m_y * movedata.m_y;
	float det = cfn.m_x * crn.m_y - cfn.m_y * crn.m_x;
	if ( fabsf( det ) > 0.001f ) {
		cmd->m_forward_move = std::clamp( ( vx * crn.m_y - vy * crn.m_x ) / det, -450.f, 450.f );
		cmd->m_side_move = std::clamp( ( cfn.m_x * vy - cfn.m_y * vx ) / det, -450.f, 450.f );
	}
	cmd->m_buttons &= ~( in_moveright | in_moveleft | in_back | in_forward );
	if ( cmd->m_side_move > 0.f ) cmd->m_buttons |= in_moveright;
	else if ( cmd->m_side_move < 0.f ) cmd->m_buttons |= in_moveleft;
	if ( cmd->m_forward_move > 0.f ) cmd->m_buttons |= in_forward;
	else if ( cmd->m_forward_move < 0.f ) cmd->m_buttons |= in_back;
}
static c_angle s_original_angle;
static float s_original_fwd, s_original_side;
void start_movement_fix( c_user_cmd* cmd )
{
	s_original_angle = cmd->m_view_point;
	s_original_fwd = cmd->m_forward_move;
	s_original_side = cmd->m_side_move;
}
void end_movement_fix( c_user_cmd* cmd )
{
	float f1 = s_original_angle.m_y < 0.f ? 360.f + s_original_angle.m_y : s_original_angle.m_y;
	float f2 = cmd->m_view_point.m_y < 0.f ? 360.f + cmd->m_view_point.m_y : cmd->m_view_point.m_y;
	float delta = f2 < f1 ? fabsf( f2 - f1 ) : 360.f - fabsf( f1 - f2 );
	delta = 360.f - delta;
	cmd->m_forward_move = cosf( deg2rad( delta ) ) * s_original_fwd + cosf( deg2rad( delta + 90.f ) ) * s_original_side;
	cmd->m_side_move = sinf( deg2rad( delta ) ) * s_original_fwd + sinf( deg2rad( delta + 90.f ) ) * s_original_side;
}
void n_edgebug::impl_t::PrePredictionEdgeBug( c_user_cmd* cmd )
{
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return;
	m_backup_velocity = g_ctx.m_local->get_velocity( );
	m_backup_origin = g_ctx.m_local->get_abs_origin( );
	m_backup_move_type = g_ctx.m_local->get_move_type( );
	m_backup_flags = g_ctx.m_local->get_flags( );
	m_backup_viewangle = cmd->m_view_point;
}
bool n_edgebug::impl_t::IsNearEdge( )
{
	if ( !g_ctx.m_local )
		return false;
	c_vector origin = g_ctx.m_local->get_abs_origin( );
	c_vector velocity = g_ctx.m_local->get_velocity( );
	float speed_2d = velocity.length_2d( );
	if ( speed_2d < 1.f )
		return false;
	c_vector move_dir( velocity.m_x / speed_2d, velocity.m_y / speed_2d, 0.f );
	float tick_interval = g_interfaces.m_global_vars_base->m_interval_per_tick;
	const int max_steps = speed_2d < 120.f ? 2 : 3;
	for ( int step = 1; step <= max_steps; step++ ) {
		c_vector check_pos = origin + move_dir * ( speed_2d * tick_interval * step );
		c_vector trace_start( check_pos.m_x, check_pos.m_y, origin.m_z + 2.f );
		c_vector trace_end( check_pos.m_x, check_pos.m_y, origin.m_z - 64.f );
		ray_t ray( trace_start, trace_end );
		trace_t trace;
		c_trace_filter filter( g_ctx.m_local );
		g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &filter, &trace );
		if ( trace.m_fraction > 0.8f )
			return true;
	}
	return false;
}
bool n_edgebug::impl_t::EdgeBugCheck( const edge_context_t& ctx )
{
	if ( !ctx.was_falling || ctx.on_ground )
		return false;
	const float half_gravity = get_edgebug_half_gravity_per_tick( );
	const int detection_mode = GET_VARIABLE( g_variables.m_edgebug_detection_mode, int );
	if ( detection_mode == 0 ) {
		if ( ctx.backup_vel_z < -ctx.gravity_tick * 0.65f &&
		     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 2.8f )
			return true;
		if ( ctx.backup_vel_z < -ctx.gravity_tick - 0.25f &&
		     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 4.8f )
			return true;
		if ( ctx.backup_vel_z < -ctx.gravity_tick * 1.4f &&
		     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 5.8f )
			return true;
		if ( ctx.backup_vel_z < -( half_gravity * 1.44f ) &&
		     fabsf( ctx.current_vel_z + half_gravity ) < 1.4f )
			return true;
		if ( ctx.backup_vel_z < -ctx.gravity_tick * 0.7f &&
		     roundf( ctx.current_vel_z ) == -roundf( ctx.gravity_tick ) )
			return true;
		if ( ctx.backup_vel_z < -75.f &&
		     fabsf( ctx.current_vel_z ) < 14.5f )
			return true;
		if ( ctx.backup_vel_z < -18.f &&
		     fabsf( ctx.current_vel_z ) < 7.5f )
			return true;
		if ( ctx.backup_vel_z < -ctx.gravity_tick * 0.5f &&
		     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 2.5f )
			return true;
		if ( ctx.backup_vel_z < -5.f &&
		     fabsf( ctx.current_vel_z ) < 2.2f )
			return true;
		if ( ctx.backup_vel_z < -ctx.gravity_tick * 2.0f &&
		     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 6.5f )
			return true;
		if ( ctx.backup_vel_z < -95.f &&
		     fabsf( ctx.current_vel_z ) < 16.f )
			return true;
		if ( ctx.backup_vel_z < -ctx.gravity_tick * 0.4f &&
		     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 2.0f )
			return true;
		if ( ctx.backup_vel_z < -3.f &&
		     fabsf( ctx.current_vel_z ) < 1.8f )
			return true;
		if ( ctx.backup_vel_z < -ctx.gravity_tick * 2.5f &&
		     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 7.0f )
			return true;
		if ( ctx.backup_vel_z < -105.f &&
		     fabsf( ctx.current_vel_z ) < 17.5f )
			return true;
		if ( ctx.backup_vel_z < -ctx.gravity_tick * 0.3f &&
		     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 1.7f )
			return true;
		if ( ctx.backup_vel_z < -2.f &&
		     fabsf( ctx.current_vel_z ) < 1.5f )
			return true;
		if ( ctx.backup_vel_z < -ctx.gravity_tick * 3.0f &&
		     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 7.5f )
			return true;
		if ( ctx.backup_vel_z < -115.f &&
		     fabsf( ctx.current_vel_z ) < 19.f )
			return true;
		if ( ctx.backup_vel_z < -ctx.gravity_tick * 0.25f &&
		     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 1.5f )
			return true;
		if ( ctx.backup_vel_z < -1.5f &&
		     fabsf( ctx.current_vel_z ) < 1.2f )
			return true;
		if ( ctx.backup_vel_z < -ctx.gravity_tick * 3.5f &&
		     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 8.0f )
			return true;
		if ( ctx.backup_vel_z < -125.f &&
		     fabsf( ctx.current_vel_z ) < 20.5f )
			return true;
		return false;
	}
	const bool near_edge = IsNearEdge( );
	const float vel_change = fabsf( ctx.current_vel_z - ctx.backup_vel_z );
	const float vel_ratio = ctx.backup_vel_z != 0.f ? fabsf( ctx.current_vel_z / ctx.backup_vel_z ) : 0.f;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 0.6f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 3.0f )
		return true;
	if ( ctx.backup_vel_z < -( half_gravity * 1.12f ) &&
	     fabsf( ctx.current_vel_z + half_gravity ) < 1.8f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -12.f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 4.5f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -25.f &&
	     fabsf( ctx.current_vel_z ) < 9.f )
		return true;
	if ( ctx.backup_vel_z < -20.f &&
	     fabsf( ctx.current_vel_z ) < 10.f )
		return true;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 1.1f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 3.8f )
		return true;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 0.4f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 2.2f )
		return true;
	if ( ctx.backup_vel_z < -4.f &&
	     fabsf( ctx.current_vel_z ) < 2.5f )
		return true;
	if ( ctx.backup_vel_z < -35.f &&
	     fabsf( ctx.current_vel_z ) < 13.f )
		return true;
	if ( ctx.backup_vel_z < -55.f &&
	     fabsf( ctx.current_vel_z ) < 18.f )
		return true;
	if ( ctx.backup_vel_z < -75.f &&
	     fabsf( ctx.current_vel_z ) < 23.f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -18.f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 6.5f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -8.f &&
	     fabsf( ctx.current_vel_z ) < 4.5f )
		return true;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 0.9f &&
	     roundf( ctx.current_vel_z ) == -roundf( ctx.gravity_tick ) )
		return true;
	if ( ctx.backup_vel_z < -12.f &&
	     fabsf( ctx.current_vel_z - ctx.backup_vel_z ) < 1.8f )
		return true;
	if ( ctx.backup_vel_z < -30.f &&
	     ctx.current_vel_z > ctx.backup_vel_z + 18.f )
		return true;
	if ( ctx.backup_vel_z < -45.f &&
	     ctx.current_vel_z > ctx.backup_vel_z + 28.f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -65.f &&
	     fabsf( ctx.current_vel_z ) < 28.f )
		return true;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 0.3f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 2.0f )
		return true;
	if ( ctx.backup_vel_z < -3.f &&
	     fabsf( ctx.current_vel_z ) < 2.0f )
		return true;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 1.5f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 4.5f )
		return true;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 2.0f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 5.5f )
		return true;
	if ( ctx.backup_vel_z < -90.f &&
	     fabsf( ctx.current_vel_z ) < 28.f )
		return true;
	if ( ctx.backup_vel_z < -100.f &&
	     fabsf( ctx.current_vel_z ) < 32.f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -5.f &&
	     fabsf( ctx.current_vel_z ) < 3.5f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -40.f &&
	     fabsf( ctx.current_vel_z ) < 14.f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -80.f &&
	     fabsf( ctx.current_vel_z ) < 32.f )
		return true;
	if ( vel_change > 15.f && ctx.backup_vel_z < -20.f &&
	     fabsf( ctx.current_vel_z ) < 11.f )
		return true;
	if ( vel_change > 25.f && ctx.backup_vel_z < -35.f &&
	     fabsf( ctx.current_vel_z ) < 15.f )
		return true;
	if ( vel_change > 40.f && ctx.backup_vel_z < -50.f &&
	     fabsf( ctx.current_vel_z ) < 20.f )
		return true;
	if ( vel_ratio < 0.3f && ctx.backup_vel_z < -15.f )
		return true;
	if ( vel_ratio < 0.2f && ctx.backup_vel_z < -10.f )
		return true;
	if ( vel_ratio < 0.15f && ctx.backup_vel_z < -8.f )
		return true;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 0.25f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 1.8f )
		return true;
	if ( ctx.backup_vel_z < -2.5f &&
	     fabsf( ctx.current_vel_z ) < 1.5f )
		return true;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 2.5f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 6.0f )
		return true;
	if ( ctx.backup_vel_z < -110.f &&
	     fabsf( ctx.current_vel_z ) < 35.f )
		return true;
	if ( ctx.backup_vel_z < -120.f &&
	     fabsf( ctx.current_vel_z ) < 38.f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -3.f &&
	     fabsf( ctx.current_vel_z ) < 2.5f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -50.f &&
	     fabsf( ctx.current_vel_z ) < 17.f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -90.f &&
	     fabsf( ctx.current_vel_z ) < 35.f )
		return true;
	if ( vel_change > 10.f && ctx.backup_vel_z < -15.f &&
	     fabsf( ctx.current_vel_z ) < 8.f )
		return true;
	if ( vel_change > 35.f && ctx.backup_vel_z < -45.f &&
	     fabsf( ctx.current_vel_z ) < 18.f )
		return true;
	if ( vel_change > 50.f && ctx.backup_vel_z < -60.f &&
	     fabsf( ctx.current_vel_z ) < 23.f )
		return true;
	if ( vel_ratio < 0.25f && ctx.backup_vel_z < -12.f )
		return true;
	if ( vel_ratio < 0.18f && ctx.backup_vel_z < -9.f )
		return true;
	if ( vel_ratio < 0.12f && ctx.backup_vel_z < -7.f )
		return true;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 0.2f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 1.5f )
		return true;
	if ( ctx.backup_vel_z < -2.0f &&
	     fabsf( ctx.current_vel_z ) < 1.2f )
		return true;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 3.0f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 7.0f )
		return true;
	if ( ctx.backup_vel_z < -1.0f &&
	     fabsf( ctx.current_vel_z ) < 0.8f )
		return true;
	if ( ctx.backup_vel_z < -15.f &&
	     ctx.current_vel_z > -2.f && ctx.current_vel_z < 2.f )
		return true;
	if ( ctx.backup_vel_z < -130.f &&
	     fabsf( ctx.current_vel_z ) < 22.f )
		return true;
	if ( ctx.backup_vel_z < -140.f &&
	     fabsf( ctx.current_vel_z ) < 24.f )
		return true;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 4.0f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 9.0f )
		return true;
	if ( ctx.backup_vel_z < -ctx.gravity_tick * 5.0f &&
	     fabsf( ctx.current_vel_z + ctx.gravity_tick ) < 10.5f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -100.f &&
	     fabsf( ctx.current_vel_z ) < 38.f )
		return true;
	if ( near_edge && ctx.backup_vel_z < -2.f &&
	     fabsf( ctx.current_vel_z ) < 1.8f )
		return true;
	if ( vel_change > 60.f && ctx.backup_vel_z < -70.f &&
	     fabsf( ctx.current_vel_z ) < 25.f )
		return true;
	if ( vel_change > 80.f && ctx.backup_vel_z < -85.f &&
	     fabsf( ctx.current_vel_z ) < 30.f )
		return true;
	if ( vel_ratio < 0.08f && ctx.backup_vel_z < -5.f )
		return true;
	if ( vel_ratio < 0.05f && ctx.backup_vel_z < -3.f )
		return true;
	return false;
}
void n_edgebug::impl_t::ApplyAutoStrafe( c_user_cmd* cmd, bool duck )
{
	if ( duck )
		cmd->m_buttons |= in_duck;
	else
		cmd->m_buttons &= ~in_duck;
	static float side_flip = 1.f;
	side_flip = -side_flip;
	auto velocity = g_ctx.m_local->get_velocity( );
	c_angle wish_angle = cmd->m_view_point;
	float speed = velocity.length_2d( );
	float ideal_strafe = std::clamp( rad2deg( atanf( 15.f / speed ) ), 0.f, 90.f );
	cmd->m_forward_move = 0.f;
	static auto cl_sidespeed = g_interfaces.m_convar->find_var( "cl_sidespeed" );
	float cl_sidespeed_val = cl_sidespeed ? cl_sidespeed->get_float( ) : 450.f;
	static float old_yaw = 0.f;
	float yaw_delta = std::remainderf( wish_angle.m_y - old_yaw, 360.f );
	float abs_yaw_delta = fabsf( yaw_delta );
	old_yaw = wish_angle.m_y;
	if ( abs_yaw_delta <= ideal_strafe || abs_yaw_delta >= 30.f ) {
		c_angle vel_dir;
		g_math.vector_angles( velocity, vel_dir );
		float vel_delta = std::remainderf( wish_angle.m_y - vel_dir.m_y, 360.f );
		float retrack = std::clamp( rad2deg( atanf( 30.f / speed ) ), 0.f, 90.f ) * 2.f;
		if ( vel_delta <= retrack || speed <= 15.f ) {
			if ( -retrack <= vel_delta || speed <= 15.f ) {
				wish_angle.m_y += side_flip * ideal_strafe;
				cmd->m_side_move = cl_sidespeed_val * side_flip;
			} else {
				wish_angle.m_y = vel_dir.m_y - retrack;
				cmd->m_side_move = cl_sidespeed_val;
			}
		} else {
			wish_angle.m_y = vel_dir.m_y + retrack;
			cmd->m_side_move = -cl_sidespeed_val;
		}
		CorrectMovement( cmd, wish_angle, cmd->m_view_point );
	} else if ( yaw_delta > 0.f ) {
		cmd->m_side_move = -cl_sidespeed_val;
	} else {
		cmd->m_side_move = cl_sidespeed_val;
	}
}
void n_edgebug::impl_t::ApplyAutoStrafeToEdge( c_user_cmd* cmd, bool duck )
{
	if ( duck )
		cmd->m_buttons |= in_duck;
	else
		cmd->m_buttons &= ~in_duck;
	auto velocity = g_ctx.m_local->get_velocity( );
	float speed = velocity.length_2d( );
	if ( speed < 1.f ) {
		cmd->m_forward_move = 0.f;
		cmd->m_side_move = 0.f;
		return;
	}
	c_vector origin = g_ctx.m_local->get_abs_origin( );
	c_vector move_dir( velocity.m_x / speed, velocity.m_y / speed, 0.f );
	float tick_interval = g_interfaces.m_global_vars_base->m_interval_per_tick;
	c_vector best_dir = move_dir;
	bool found_edge_dir = false;
	for ( int angle_step = 0; angle_step <= 8; angle_step++ ) {
		float offset_angle = ( angle_step == 0 ) ? 0.f : ( ( angle_step % 2 == 1 ? 1.f : -1.f ) * ( ( angle_step + 1 ) / 2 ) * 22.5f );
		float rad = deg2rad( rad2deg( atan2f( move_dir.m_y, move_dir.m_x ) ) + offset_angle );
		c_vector test_dir( cosf( rad ), sinf( rad ), 0.f );
		for ( int step = 1; step <= 3; step++ ) {
			c_vector check_pos = origin + test_dir * ( speed * tick_interval * step );
			c_vector trace_start( check_pos.m_x, check_pos.m_y, origin.m_z + 2.f );
			c_vector trace_end( check_pos.m_x, check_pos.m_y, origin.m_z - 64.f );
			ray_t ray( trace_start, trace_end );
			trace_t trace;
			c_trace_filter filter( g_ctx.m_local );
			g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &filter, &trace );
			if ( trace.m_fraction > 0.8f ) {
				best_dir = test_dir;
				found_edge_dir = true;
				break;
			}
		}
		if ( found_edge_dir )
			break;
	}
	c_angle target_angle;
	target_angle.m_y = rad2deg( atan2f( best_dir.m_y, best_dir.m_x ) );
	target_angle.m_x = 0.f;
	float rotation = deg2rad( target_angle.m_y - cmd->m_view_point.m_y );
	cmd->m_forward_move = cosf( rotation ) * 450.f;
	cmd->m_side_move = -sinf( rotation ) * 450.f;
	cmd->m_forward_move = std::clamp( cmd->m_forward_move, -450.f, 450.f );
	cmd->m_side_move = std::clamp( cmd->m_side_move, -450.f, 450.f );
	CorrectMovement( cmd, target_angle, cmd->m_view_point );
}
void n_edgebug::impl_t::ReStorePrediction( )
{
	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
}
void n_edgebug::impl_t::EdgeBugPostPredict( c_user_cmd* cmd )
{
	if ( !GET_VARIABLE( g_variables.edge_bug, bool ) ||
	     !g_input.check_input( &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) ) ) {
		m_found = false;
		m_replay_hits = 0;
		m_ducked = false;
		return;
	}
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return;
	const auto mt = g_ctx.m_local->get_move_type( );
	if ( mt == move_type_ladder || mt == move_type_noclip || mt == move_type_observer ) {
		m_found = false;
		m_replay_hits = 0;
		m_ducked = false;
		return;
	}
	if ( m_ducked ) {
		if ( !( m_backup_flags & 1 ) )
			cmd->m_buttons &= ~in_jump;
		else
			cmd->m_buttons |= in_jump;
		cmd->m_buttons |= in_duck;
	}
	if ( m_backup_flags & 1 ) {
		m_ducked = false;
		return;
	}
	if ( !m_found && m_ducked && m_backup_velocity.m_z < -40.f )
		m_ducked = false;
	const int detection_mode = GET_VARIABLE( g_variables.m_edgebug_detection_mode, int );
	bool mega = GET_VARIABLE( g_variables.MegaEdgeBug, bool );
	if ( mega )
		GET_VARIABLE( g_variables.deltaScaler, float ) = 179.f;
	static c_angle prev_angle;
	float delta_scale = 1.f;
	int dt = GET_VARIABLE( g_variables.DeltaType, int );
	if ( dt == 0 ) delta_scale = 1.f;
	else if ( dt == 1 ) delta_scale = 0.8f;
	else if ( dt == 2 ) delta_scale = 0.5f;
	else if ( dt == 3 ) delta_scale = 0.25f;
	if ( mega ) delta_scale = 1.f;
	c_angle delta_angle = ( cmd->m_view_point - prev_angle ) * delta_scale;
	prev_angle = cmd->m_view_point;
	auto is_large_delta = []( float y1, float y2 ) {
		float d = fabsf( y1 - y2 );
		if ( d > 180.f ) d = 360.f - d;
		return d >= GET_VARIABLE( g_variables.deltaScaler, float );
	};
	int search_count = 2;
	if ( GET_VARIABLE( g_variables.EdgeBugAdvanceSearch, bool ) ) search_count = 4;
	if ( mega ) search_count = 6;
	if ( detection_mode == 1 ) {
		search_count = std::clamp( GET_VARIABLE( g_variables.m_edgebug_enhanced_search_amount, int ), 1, 8 );
	}
	bool silent = GET_VARIABLE( g_variables.SiletEdgeBug, bool );
	bool autostrafe = GET_VARIABLE( g_variables.AutoStrafeEdgeBug, bool );
	if ( detection_mode == 1 && !GET_VARIABLE( g_variables.m_edgebug_enhanced_strafing, bool ) )
		autostrafe = false;
	int max_ticks = GET_VARIABLE( g_variables.EdgeBugTicks, int );
	if ( detection_mode == 1 ) {
		const float interval = g_interfaces.m_global_vars_base->m_interval_per_tick;
		const float time_window = std::max( 0.01f, GET_VARIABLE( g_variables.m_edgebug_enhanced_time, float ) );
		max_ticks = ( interval > 0.0f ) ? static_cast<int>( std::round( time_window / interval ) ) : max_ticks;
	}
	if ( max_ticks < 1 ) max_ticks = 1;
	if ( detection_mode == 1 ) {
		if ( max_ticks > 256 ) max_ticks = 256;
	} else {
		if ( max_ticks > 16 ) max_ticks = 16;
	}
	int orig_buttons = cmd->m_buttons;
	float orig_fwd = cmd->m_forward_move;
	float orig_side = cmd->m_side_move;
	c_angle orig_viewangle = cmd->m_view_point;
	static int last_variant = 0;
	if ( !m_found ) {
		if ( m_backup_velocity.m_z >= ( detection_mode == 1 ? -4.f : -10.f ) )
			return;
		int total_predictions = 0;
		const int MAX_PREDICTIONS = ( detection_mode == 1 ) ? 2048 : 64;
		for ( int v = 0; v < search_count; v++ ) {
			if ( last_variant ) {
				v = last_variant;
				last_variant = 0;
			}
			ReStorePrediction( );
			c_vector search_vel_backup = g_ctx.m_local->get_velocity( );
			cmd->m_view_point = orig_viewangle;
			cmd->m_forward_move = orig_fwd;
			cmd->m_side_move = orig_side;
			cmd->m_buttons = orig_buttons;
			c_angle current_angle = cmd->m_view_point;
			for ( int i = 0; i < max_ticks; i++ ) {
				if ( m_found || ( g_ctx.m_local->get_flags( ) & 1 ) || g_ctx.m_local->get_velocity( ).m_z > 0.f )
					break;
				switch ( v ) {
				case 0:
					cmd->m_buttons &= ~in_duck;
					cmd->m_forward_move = 0.f;
					cmd->m_side_move = 0.f;
					break;
				case 1:
					cmd->m_buttons |= in_duck;
					cmd->m_forward_move = 0.f;
					cmd->m_side_move = 0.f;
					break;
				case 2:
					cmd->m_buttons &= ~in_duck;
					if ( autostrafe || mega ) {
						ApplyAutoStrafe( cmd, false );
					} else {
						cmd->m_forward_move = orig_fwd;
						cmd->m_side_move = orig_side;
						if ( !is_large_delta( current_angle.m_y, orig_viewangle.m_y ) )
							current_angle = ( current_angle + delta_angle ).normalize( ).clamp( );
						cmd->m_view_point = current_angle;
						if ( silent ) {
							start_movement_fix( cmd );
							cmd->m_view_point = orig_viewangle;
							end_movement_fix( cmd );
						}
					}
					break;
				case 3:
					cmd->m_buttons |= in_duck;
					if ( autostrafe || mega ) {
						ApplyAutoStrafe( cmd, true );
					} else {
						cmd->m_forward_move = orig_fwd;
						cmd->m_side_move = orig_side;
						if ( !is_large_delta( current_angle.m_y, orig_viewangle.m_y ) )
							current_angle = ( current_angle + delta_angle ).normalize( ).clamp( );
						cmd->m_view_point = current_angle;
						if ( silent ) {
							start_movement_fix( cmd );
							cmd->m_view_point = orig_viewangle;
							end_movement_fix( cmd );
						}
					}
					break;
				case 4:
					ApplyAutoStrafe( cmd, false );
					break;
				case 5:
					ApplyAutoStrafe( cmd, true );
					break;
				case 6:
					if ( GET_VARIABLE( g_variables.m_edgebug_enhanced_autostrafe_to_edge, bool ) )
						ApplyAutoStrafeToEdge( cmd, false );
					else
						ApplyAutoStrafe( cmd, false );
					break;
				case 7:
					if ( GET_VARIABLE( g_variables.m_edgebug_enhanced_autostrafe_to_edge, bool ) )
						ApplyAutoStrafeToEdge( cmd, true );
					else
						ApplyAutoStrafe( cmd, true );
					break;
				}
				m_predicted_cmds[i].forwardmove = cmd->m_forward_move;
				m_predicted_cmds[i].sidemove = cmd->m_side_move;
				m_predicted_cmds[i].buttons = cmd->m_buttons;
				m_predicted_cmds[i].viewangle = cmd->m_view_point;
				m_predicted_cmds[i].origin = g_ctx.m_local->get_origin( );
				if ( m_found || ( g_ctx.m_local->get_flags( ) & 1 ) || g_ctx.m_local->get_velocity( ).m_z > 0.f )
					break;
				if ( total_predictions >= MAX_PREDICTIONS ) break;
				g_prediction.begin( g_ctx.m_local, cmd );
				g_prediction.end( g_ctx.m_local );
				total_predictions++;
				edge_context_t ctx;
				ctx.backup_vel_z = search_vel_backup.m_z;
				ctx.current_vel_z = g_ctx.m_local->get_velocity( ).m_z;
				ctx.gravity_tick = get_edgebug_half_gravity_per_tick( );
				ctx.was_falling = search_vel_backup.m_z < -6.f;
				ctx.on_ground = ( g_ctx.m_local->get_flags( ) & 1 ) != 0;
				m_found = EdgeBugCheck( ctx );
				search_vel_backup = g_ctx.m_local->get_velocity( );
				if ( m_found ) {
					m_prediction_ticks = i;
					m_found_variant = v;
					last_variant = v;
					m_found_tick = g_interfaces.m_global_vars_base->m_tick_count + ( i + 1 );
					m_prediction_timestamp = g_interfaces.m_global_vars_base->m_tick_count;
					m_mouse_offset = abs( cmd->m_mouse_delta_x );
					m_replay_hits = 0;
				}
			}
			cmd->m_view_point = orig_viewangle;
			cmd->m_forward_move = orig_fwd;
			cmd->m_side_move = orig_side;
			cmd->m_buttons = orig_buttons;
			if ( m_found || total_predictions >= MAX_PREDICTIONS )
				break;
		}
	}
	if ( m_found ) {
		ReStorePrediction( );
		c_vector org = g_ctx.m_local->get_origin( );
		float dist = org.dist_to( m_predicted_cmds[m_replay_hits].origin );
		if ( m_replay_hits > m_prediction_ticks || dist > 1.f ) {
			m_replay_hits = 0;
			m_found = false;
			m_found_variant = 0;
			return;
		}
		if ( m_predicted_cmds[m_replay_hits].buttons & in_duck ) {
			cmd->m_buttons |= in_duck;
			m_ducked = true;
		} else {
			cmd->m_buttons &= ~in_duck;
		}
		if ( m_backup_flags & 1 )
			cmd->m_buttons |= in_jump;
		else
			cmd->m_buttons &= ~in_jump;
		cmd->m_forward_move = m_predicted_cmds[m_replay_hits].forwardmove;
		cmd->m_side_move = m_predicted_cmds[m_replay_hits].sidemove;
		cmd->m_view_point = m_predicted_cmds[m_replay_hits].viewangle;
		if ( silent ) {
			start_movement_fix( cmd );
			cmd->m_view_point = orig_viewangle;
			end_movement_fix( cmd );
		}
		m_aim_target = cmd->m_view_point;
		m_replay_hits++;
	}
}

void n_edgebug::impl_t::edgebug_lock( float& x, float& y )
{
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) ||
	     !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( g_ctx.m_local->get_flags( ) == fl_inwater || g_ctx.m_local->get_flags( ) == fl_waterjump )
		return;
	const auto mt = g_ctx.m_local->get_move_type( );
	if ( mt == move_type_ladder || mt == move_type_noclip )
		return;
	if ( m_found ) {
		if ( x != 0.f ) {
			float remaining = (float)( m_prediction_timestamp + m_prediction_ticks - g_interfaces.m_global_vars_base->m_tick_count );
			float ratio = remaining / x;
			float lock_val = GET_VARIABLE( g_variables.EdgeBugMouseLock, float );
			float scale = 100.f / ( atan2f( (float)m_prediction_ticks, ratio ) + 100.f + lock_val * m_mouse_offset );
			if ( !isnan( scale ) )
				x *= scale;
		}
	}
}
void n_edgebug::impl_t::frame_stage( int stage )
{
	if ( stage != e_client_frame_stage::start )
		return;
	static c_angle aim_smooth = m_backup_viewangle;
	if ( m_found ) {
		bool silent = GET_VARIABLE( g_variables.SiletEdgeBug, bool );
		bool autostrafe = GET_VARIABLE( g_variables.AutoStrafeEdgeBug, bool );
		if ( !silent && m_found_variant > 1 && !autostrafe ) {
			const float smooth_speed = 0.15f;
			const float max_delta = 15.f;
			auto norm_delta = []( float d ) -> float {
				while ( d > 180.f ) d -= 360.f;
				while ( d < -180.f ) d += 360.f;
				return d;
			};
			float dp = norm_delta( m_aim_target.m_x - aim_smooth.m_x );
			if ( fabsf( dp ) > max_delta ) dp = dp > 0 ? max_delta : -max_delta;
			aim_smooth.m_x += dp * smooth_speed;
			float dy = norm_delta( m_aim_target.m_y - aim_smooth.m_y );
			if ( fabsf( dy ) > max_delta ) dy = dy > 0 ? max_delta : -max_delta;
			aim_smooth.m_y += dy * smooth_speed;
			aim_smooth.normalize( );
			g_interfaces.m_engine_client->set_view_angles( aim_smooth );
		}
	} else {
		aim_smooth = m_backup_viewangle;
	}
}
