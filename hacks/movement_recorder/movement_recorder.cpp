#define IMGUI_DEFINE_MATH_OPERATORS
#include "movement_recorder.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <fstream>

#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../../globals/render/render.h"
#include "../prediction/prediction.h"
#include "../movement/movement.h"
#include "../../instalisation fonts/fonts2.h"

#include "../../dependencies/json/json.hpp"
#include "../../elements/smbutton.hh"

using json = nlohmann::json;

extern void start_movement_fix( c_user_cmd* cmd );
extern void end_movement_fix( c_user_cmd* cmd );

namespace
{
	constexpr const char* k_root_dir   = "C:\\bebra\\csgorecords";
	constexpr float k_start_radius     = 28.f;   // distance to start point that triggers playback start
	constexpr float k_pos_close_eps    = 0.1f;
	constexpr float k_angle_close_eps  = 0.1f;
	constexpr float k_indicator_lerp_k = 6.f;

	std::string sanitize_clip_name( std::string name )
	{
		for ( char& ch : name ) {
			if ( ch == '<' || ch == '>' || ch == ':' || ch == '"' || ch == '/' || ch == '\\' || ch == '|' || ch == '?' || ch == '*' )
				ch = '_';
		}

		while ( !name.empty( ) && std::isspace( static_cast< unsigned char >( name.front( ) ) ) )
			name.erase( name.begin( ) );
		while ( !name.empty( ) && std::isspace( static_cast< unsigned char >( name.back( ) ) ) )
			name.pop_back( );

		return name;
	}

	std::filesystem::path unique_clip_path( const std::filesystem::path& root, std::string& display_name )
	{
		display_name = sanitize_clip_name( display_name );
		if ( display_name.empty( ) )
			display_name = "Clip";

		auto path = root / ( display_name + ".mr" );
		if ( !std::filesystem::exists( path ) )
			return path;

		const std::string base = display_name;
		for ( int i = 1; i < 1000; ++i ) {
			display_name = base + " " + std::to_string( i );
			path = root / ( display_name + ".mr" );
			if ( !std::filesystem::exists( path ) )
				return path;
		}

		return path;
	}
}

// ---------------------------------------------------------------------------
// frame_t
// ---------------------------------------------------------------------------
n_movement_recorder::frame_t::frame_t( c_user_cmd* cmd, const c_vector& pos )
{
	view_pitch = cmd->m_view_point.m_x;
	view_yaw   = cmd->m_view_point.m_y;
	forward    = cmd->m_forward_move;
	side       = cmd->m_side_move;
	up         = cmd->m_up_move;
	buttons    = cmd->m_buttons;
	mouse_dx   = cmd->m_mouse_delta_x;
	mouse_dy   = cmd->m_mouse_delta_y;
	air_stuck  = g_movement.m_air_stuck_data.m_air_stuck;
	wall_stuck = g_movement.m_air_stuck_data.m_wall_stuck;
	position     = pos;
	eye_position = g_ctx.m_local ? g_ctx.m_local->get_eye_position( false ) : pos + c_vector( 0.f, 0.f, 64.f );
	velocity     = g_prediction.backup_data.m_velocity;
	has_velocity = true;
}

void n_movement_recorder::frame_t::replay( c_user_cmd* cmd ) const
{
	cmd->m_view_point.m_x = view_pitch;
	cmd->m_view_point.m_y = view_yaw;
	cmd->m_forward_move   = forward;
	cmd->m_side_move      = side;
	cmd->m_up_move        = up;
	cmd->m_buttons        = buttons;
	cmd->m_mouse_delta_x  = mouse_dx;
	cmd->m_mouse_delta_y  = mouse_dy;
}

// ---------------------------------------------------------------------------
// disk / route management
// ---------------------------------------------------------------------------
std::filesystem::path n_movement_recorder::impl_t::get_root_path( ) const
{
	return std::filesystem::path( k_root_dir );
}

void n_movement_recorder::impl_t::ensure_root( )
{
	std::error_code ec;
	const auto root = get_root_path( );
	if ( !std::filesystem::exists( root ) )
		std::filesystem::create_directories( root, ec );
}

// load a single .mr file from disk into the supplied clip.
// returns true if the file was a valid clip and clip_out was populated.
static bool read_clip_from_file( const std::filesystem::path& file_path, n_movement_recorder::clip_t& clip_out )
{
	std::ifstream in( file_path );
	if ( !in.good( ) )
		return false;
	if ( in.peek( ) == std::ifstream::traits_type::eof( ) )
		return false;

	json j;
	try {
		in >> j;
	} catch ( ... ) {
		return false;
	}

	if ( !j.contains( "frames" ) || !j[ "frames" ].is_array( ) )
		return false;

	clip_out.frames.clear( );
	clip_out.map      = j.value( "map", std::string{ } );
	clip_out.filename = reinterpret_cast< const char* >( file_path.filename( ).u8string( ).c_str( ) );
	clip_out.display_name = reinterpret_cast< const char* >( file_path.stem( ).u8string( ).c_str( ) );

	for ( const auto& jf : j[ "frames" ] ) {
		n_movement_recorder::frame_t f{ };
		f.view_pitch   = jf.value( "vpx", 0.f );
		f.view_yaw     = jf.value( "vpy", 0.f );
		f.forward      = jf.value( "fwd", 0.f );
		f.side         = jf.value( "side", 0.f );
		f.up           = jf.value( "up", 0.f );
		f.buttons      = jf.value( "btn", 0 );
		f.mouse_dx     = static_cast< short >( jf.value( "mdx", 0 ) );
		f.mouse_dy     = static_cast< short >( jf.value( "mdy", 0 ) );
		f.jetpack      = jf.value( "jp", false );
		f.air_stuck    = jf.value( "as", false );
		f.wall_stuck   = jf.value( "ws", false );
		f.position.m_x = jf.value( "px", 0.f );
		f.position.m_y = jf.value( "py", 0.f );
		f.position.m_z = jf.value( "pz", 0.f );
		f.eye_position.m_x = jf.value( "ex", f.position.m_x );
		f.eye_position.m_y = jf.value( "ey", f.position.m_y );
		f.eye_position.m_z = jf.value( "ez", f.position.m_z + 64.f );
		f.has_velocity = jf.contains( "vx" );
		f.velocity.m_x = jf.value( "vx", 0.f );
		f.velocity.m_y = jf.value( "vy", 0.f );
		f.velocity.m_z = jf.value( "vz", 0.f );
		clip_out.frames.emplace_back( std::move( f ) );
	}

	// fallback: derive map from filename ("<map> <date>.mr")
	if ( clip_out.map.empty( ) ) {
		const auto sp = clip_out.display_name.find( ' ' );
		if ( sp != std::string::npos )
			clip_out.map = clip_out.display_name.substr( 0, sp );
	}

	return true;
}

static void write_clip_to_file( const std::filesystem::path& file_path, const n_movement_recorder::clip_t& clip )
{
	json j;
	j[ "map" ]    = clip.map;
	j[ "frames" ] = json::array( );
	for ( const auto& f : clip.frames ) {
		json jf;
		jf[ "vpx" ]  = f.view_pitch;
		jf[ "vpy" ]  = f.view_yaw;
		jf[ "fwd" ]  = f.forward;
		jf[ "side" ] = f.side;
		jf[ "up" ]   = f.up;
		jf[ "btn" ]  = f.buttons;
		jf[ "mdx" ]  = static_cast< int >( f.mouse_dx );
		jf[ "mdy" ]  = static_cast< int >( f.mouse_dy );
		jf[ "jp" ]   = f.jetpack;
		jf[ "as" ]   = f.air_stuck;
		jf[ "ws" ]   = f.wall_stuck;
		jf[ "px" ]   = f.position.m_x;
		jf[ "py" ]   = f.position.m_y;
		jf[ "pz" ]   = f.position.m_z;
		jf[ "ex" ]   = f.eye_position.m_x;
		jf[ "ey" ]   = f.eye_position.m_y;
		jf[ "ez" ]   = f.eye_position.m_z;
		jf[ "vx" ]   = f.velocity.m_x;
		jf[ "vy" ]   = f.velocity.m_y;
		jf[ "vz" ]   = f.velocity.m_z;
		j[ "frames" ].push_back( jf );
	}

	std::ofstream out( file_path );
	if ( out.good( ) )
		out << j.dump( 2 );
}

void n_movement_recorder::impl_t::refresh_clips( )
{
	ensure_root( );
	m_clips.clear( );

	std::error_code ec;
	for ( const auto& entry : std::filesystem::directory_iterator{ get_root_path( ), ec } ) {
		if ( !entry.is_regular_file( ) )
			continue;
		if ( entry.path( ).extension( ).u8string( ) != reinterpret_cast< const char8_t* >( ".mr" ) )
			continue;

		clip_t c{ };
		if ( !read_clip_from_file( entry.path( ), c ) )
			continue;
		if ( !m_current_map.empty( ) && !c.map.empty( ) && c.map != m_current_map )
			continue;
		m_clips.emplace_back( std::move( c ) );
	}

	std::sort( m_clips.begin( ), m_clips.end( ), []( const clip_t& a, const clip_t& b ) {
		std::error_code lwt_ec;
		auto time_a = std::filesystem::last_write_time( std::filesystem::path( k_root_dir ) / a.filename, lwt_ec );
		auto time_b = std::filesystem::last_write_time( std::filesystem::path( k_root_dir ) / b.filename, lwt_ec );
		return time_a < time_b;
	} );

	if ( m_selected_clip >= static_cast< int >( m_clips.size( ) ) )
		m_selected_clip = -1;
}

void n_movement_recorder::impl_t::save_current_clip( )
{
	ensure_root( );
	if ( m_current_map.empty( ) )
		return;

	const std::vector< frame_t >& src = ( m_playing && !m_playback_capture.empty( ) ) ? m_playback_capture : m_recording_frames;
	if ( src.empty( ) )
		return;

	std::time_t now = std::time( nullptr );
	std::tm tm{ };
	::localtime_s( &tm, &now );
	char date_buf[ 32 ]{ };
	std::strftime( date_buf, sizeof( date_buf ), "%Y-%m-%d_%H-%M-%S", &tm );

	clip_t c{ };
	c.map          = m_current_map;
	c.display_name = m_current_map + " Clip " + date_buf;
	c.filename     = c.display_name + ".mr";
	c.frames       = src;

	auto file_path = unique_clip_path( get_root_path( ), c.display_name );
	c.display_name = file_path.stem( ).string( );
	c.filename     = file_path.filename( ).string( );

	write_clip_to_file( file_path, c );
	const std::string saved_filename = c.filename;
	m_clips.emplace_back( std::move( c ) );
	m_selected_clip = static_cast< int >( m_clips.size( ) ) - 1;

	movement_add_window( 0, "clipped \"" + saved_filename + "\"" );
	m_session_active = false;
	m_session_elapsed = 0.f;
	m_playback_capture.clear( );
	m_recording_from_playback = false;
	m_prefix_size = 0;
}

void n_movement_recorder::impl_t::delete_clip( size_t index )
{
	if ( index >= m_clips.size( ) )
		return;
	std::error_code ec;
	std::filesystem::remove( get_root_path( ) / m_clips[ index ].filename, ec );
	m_clips.erase( m_clips.begin( ) + index );
	if ( m_selected_clip >= static_cast< int >( m_clips.size( ) ) )
		m_selected_clip = -1;
	if ( m_editor_clip_index == static_cast< int >( index ) )
		m_editor_open = false;
}

void n_movement_recorder::impl_t::rename_clip( size_t index, const std::string& new_name )
{
	if ( index >= m_clips.size( ) )
		return;

	auto& clip = m_clips[ index ];
	std::string display_name = sanitize_clip_name( new_name );
	if ( display_name.empty( ) )
		display_name = clip.display_name.empty( ) ? "Clip" : clip.display_name;

	const auto old_path = get_root_path( ) / clip.filename;
	auto new_path = get_root_path( ) / ( display_name + ".mr" );
	if ( new_path != old_path && std::filesystem::exists( new_path ) )
		new_path = unique_clip_path( get_root_path( ), display_name );

	if ( new_path != old_path ) {
		std::error_code ec;
		std::filesystem::rename( old_path, new_path, ec );
		if ( ec )
			std::filesystem::remove( old_path, ec );
	}

	clip.display_name = display_name;
	clip.filename = display_name + ".mr";
}

void n_movement_recorder::impl_t::open_clip_editor( size_t index )
{
	if ( index >= m_clips.size( ) || m_clips[ index ].frames.empty( ) )
		return;

	m_editor_open = true;
	m_editor_ignore_enter_release = true;
	m_editor_clip_index = static_cast< int >( index );
	m_editor_trim_start = 0;
	m_editor_trim_end = static_cast< int >( m_clips[ index ].frames.size( ) ) - 1;
	std::memset( m_editor_name, 0, sizeof( m_editor_name ) );
	strncpy_s( m_editor_name, m_clips[ index ].display_name.c_str( ), sizeof( m_editor_name ) - 1 );
	m_editor_anim_progress = 0.f;
}

void n_movement_recorder::impl_t::apply_clip_editor( )
{
	if ( m_editor_clip_index < 0 || m_editor_clip_index >= static_cast< int >( m_clips.size( ) ) )
		return;

	auto& clip = m_clips[ m_editor_clip_index ];
	if ( clip.frames.empty( ) )
		return;
	const std::string old_filename = clip.filename;

	const int max_frame = static_cast< int >( clip.frames.size( ) ) - 1;
	m_editor_trim_start = std::clamp( m_editor_trim_start, 0, max_frame );
	m_editor_trim_end = std::clamp( m_editor_trim_end, 0, max_frame );
	if ( m_editor_trim_start > m_editor_trim_end )
		std::swap( m_editor_trim_start, m_editor_trim_end );

	if ( m_editor_trim_start > 0 || m_editor_trim_end < max_frame ) {
		std::vector< frame_t > trimmed;
		trimmed.reserve( static_cast< size_t >( m_editor_trim_end - m_editor_trim_start + 1 ) );
		for ( int i = m_editor_trim_start; i <= m_editor_trim_end; ++i )
			trimmed.emplace_back( clip.frames[ i ] );
		clip.frames = std::move( trimmed );
		m_editor_trim_start = 0;
		m_editor_trim_end = static_cast< int >( clip.frames.size( ) ) - 1;
	}

	rename_clip( static_cast< size_t >( m_editor_clip_index ), m_editor_name );
	write_clip_to_file( get_root_path( ) / m_clips[ m_editor_clip_index ].filename, m_clips[ m_editor_clip_index ] );
	m_editor_open = false;
	movement_add_window( 0, "edited \"" + old_filename + "\" to \"" + m_clips[ m_editor_clip_index ].filename + "\"" );
}

bool n_movement_recorder::impl_t::play_clip( size_t index )
{
	if ( index >= m_clips.size( ) )
		return false;
	const auto& c = m_clips[ index ];
	if ( c.frames.empty( ) )
		return false;
	if ( !g_ctx.m_local )
		return false;
	if ( g_prediction.backup_data.m_origin.dist_to( c.frames[ 0 ].position ) >= k_start_radius )
		return false;

	m_recording_frames  = c.frames;
	m_playback_capture.clear( );
	m_playback_capture.reserve( c.frames.size( ) );
	m_active_route_name = c.display_name;
	m_session_active    = false;
	m_session_elapsed   = 0.f;
	m_recording_from_playback = false;
	m_prefix_size       = 0;
	apply_replay_jetpack( false );

	// approach-to-start flow (matches original recorder): drives the player to
	// the recorded start position + viewangles before kicking off the replay,
	// so frame-0 state matches and tick-precise tricks (edgebug, etc.) line up
	m_going_to_start    = true;
	m_wish_to_start     = true;
	if ( GET_VARIABLE( g_variables.m_movement_rec_lockva, bool ) )
		m_smoothing_to_start = true;
	return true;
}

void n_movement_recorder::impl_t::check_map_change( )
{
	static bool s_was_in_game = false;

	const bool in_game = g_interfaces.m_engine_client && g_interfaces.m_engine_client->is_in_game( );
	if ( !in_game ) {
		s_was_in_game = false;
		m_current_map.clear( );
		m_clips.clear( );
		m_selected_clip   = -1;
		m_recording_frames.clear( );
		m_session_active  = false;
		m_session_elapsed = 0.f;
		m_recording_from_playback = false;
		m_prefix_size = 0;
		m_editor_open = false;
		apply_replay_jetpack( false );
		return;
	}

	const char* level = g_interfaces.m_engine_client->get_level_name_short( );
	if ( !level || !*level )
		return;

	// fire on every fresh in-game transition (covers rejoin to the same map)
	if ( s_was_in_game && m_current_map == level )
		return;

	s_was_in_game     = true;
	m_current_map     = level;
	m_recording_frames.clear( );
	m_session_active  = false;
	m_session_elapsed = 0.f;
	m_recording_from_playback = false;
	m_prefix_size = 0;
	m_selected_clip   = -1;
	m_editor_open     = false;
	apply_replay_jetpack( false );
	refresh_clips( );

	movement_add_window( 0, std::string{ "joined to: " } + m_current_map );
}

bool n_movement_recorder::impl_t::is_player_moving( c_user_cmd* cmd ) const
{
	if ( !g_ctx.m_local )
		return false;

	const c_vector& v    = g_prediction.backup_data.m_velocity;
	const float speed_2d = std::sqrtf( v.m_x * v.m_x + v.m_y * v.m_y );
	const bool input     = cmd && ( std::fabs( cmd->m_forward_move ) > 0.1f || std::fabs( cmd->m_side_move ) > 0.1f );

	// edgebug / jump / duck attempts: player may have zero 2D velocity and no
	// movement input but still need to be recorded (falling, mid-air, ducking on edge)
	const bool airborne  = !( g_ctx.m_local->get_flags( ) & e_flags::fl_onground );
	const bool z_motion  = std::fabs( v.m_z ) > 5.f;
	const bool action    = cmd && ( cmd->m_buttons & ( e_command_buttons::in_jump | e_command_buttons::in_duck ) );
	const bool stuck     = g_movement.m_air_stuck_data.m_air_stuck || g_movement.m_air_stuck_data.m_wall_stuck;

	return speed_2d > 5.f || input || ( airborne && z_motion ) || action || stuck;
}

// ---------------------------------------------------------------------------
// public entry points
// ---------------------------------------------------------------------------
void n_movement_recorder::impl_t::force_stop( bool keep_replay_prefix, bool clear_recording )
{
	const bool was_playing = m_playing;
	const size_t stopped_play_idx = m_play_idx;
	apply_replay_jetpack( false );
	m_playing = false;
	m_play_idx = 0;
	m_going_to_start = false;
	m_smoothing_to_start = false;
	m_wish_to_start = false;
	m_waiting_for_playback_idle = false;
	if ( keep_replay_prefix && was_playing && !m_recording_frames.empty( ) ) {
		const size_t played_frames = std::min( stopped_play_idx, m_recording_frames.size( ) );
		std::vector< frame_t > replay_prefix;
		if ( !m_playback_capture.empty( ) ) {
			const size_t captured_frames = std::min( played_frames, m_playback_capture.size( ) );
			replay_prefix.assign( m_playback_capture.begin( ), m_playback_capture.begin( ) + captured_frames );
		} else {
			replay_prefix.assign( m_recording_frames.begin( ), m_recording_frames.begin( ) + played_frames );
		}
		m_prefix_size      = replay_prefix.size( );
		m_recording_frames = std::move( replay_prefix );
		m_playback_capture.clear( );
		m_session_active = !m_recording_frames.empty( );
		m_recording_from_playback = !m_recording_frames.empty( );
		m_auto_start_armed = false;
		const float interval = g_interfaces.m_global_vars_base ? g_interfaces.m_global_vars_base->m_interval_per_tick : 0.f;
		m_session_elapsed = static_cast< float >( m_recording_frames.size( ) ) * interval;
		m_session_start_t = g_interfaces.m_global_vars_base ? g_interfaces.m_global_vars_base->m_current_time - m_session_elapsed : 0.f;
	}
	else {
		m_session_active = false;
		m_recording_from_playback = false;
		m_session_elapsed = 0.f;
		m_prefix_size = 0;
		m_playback_capture.clear( );
		if ( clear_recording )
			m_recording_frames.clear( );
	}
}

void n_movement_recorder::impl_t::set_round_frozen( bool frozen )
{
	m_round_frozen = frozen;
	if ( frozen )
		force_stop( false, true );
}

void n_movement_recorder::impl_t::notify_jetpack_state( bool active )
{
	m_recording_jetpack_active = active;
}

void n_movement_recorder::impl_t::apply_replay_jetpack( bool active )
{
	if ( m_replay_jetpack_active == active )
		return;
	m_replay_jetpack_active = active;
	if ( g_interfaces.m_engine_client )
		g_interfaces.m_engine_client->execute_client_cmd( active ? "+jetpack" : "-jetpack" );
}

// ---------------------------------------------------------------------------
// create_move handler — record & playback
// ---------------------------------------------------------------------------
void n_movement_recorder::impl_t::on_create_move( c_user_cmd* cmd )
{
	if ( !cmd )
		return;
	if ( !GET_VARIABLE( g_variables.m_movement_rec, bool ) ) {
		apply_replay_jetpack( false );
		return;
	}

	// 64-tick gate — same restriction as the source
	if ( g_interfaces.m_engine_client && g_interfaces.m_engine_client->is_connected( ) ) {
		const int tickrate = static_cast< int >( 1.f / g_interfaces.m_global_vars_base->m_interval_per_tick );
		if ( tickrate != 64 ) {
			apply_replay_jetpack( false );
			return;
		}
	}

	static bool s_was_alive = false;
	const bool alive_now = g_ctx.m_local && g_ctx.m_local->is_alive( );
	if ( !alive_now ) {
		apply_replay_jetpack( false );
		if ( s_was_alive )
			force_stop( false, true );
		s_was_alive = false;
		return;
	}
	s_was_alive = true;

	if ( m_round_frozen ) {
		force_stop( false, true );
		return;
	}

	static bool s_save_was_down = false;
	static bool s_play_was_down = false;
	static bool s_stop_was_down = false;
	static bool s_clear_was_down = false;

	const bool key_save_now         = g_input.check_input( &GET_VARIABLE( g_variables.m_movement_rec_keyclip, key_bind_t ) );
	const bool key_start_play_now = g_input.check_input( &GET_VARIABLE( g_variables.m_movement_rec_keystartplay, key_bind_t ) );
	const bool key_stop_play_now  = g_input.check_input( &GET_VARIABLE( g_variables.m_movement_rec_keystopplay,  key_bind_t ) );
	const bool key_clear_now      = g_input.check_input( &GET_VARIABLE( g_variables.m_movement_rec_keyclearrecord, key_bind_t ) );

	const bool key_save         = key_save_now && !s_save_was_down;
	const bool key_start_play = key_start_play_now && !s_play_was_down;
	const bool key_stop_play  = key_stop_play_now  && !s_stop_was_down;
	const bool key_clear      = key_clear_now && !s_clear_was_down;

	s_save_was_down         = key_save_now;
	s_play_was_down = key_start_play_now;
	s_stop_was_down = key_stop_play_now;
	s_clear_was_down = key_clear_now;

	const bool menu_open = g_menu.m_opened;

	const float now_t = g_interfaces.m_global_vars_base->m_current_time;
	const bool movement_input = std::fabs( cmd->m_forward_move ) > 0.1f || std::fabs( cmd->m_side_move ) > 0.1f ||
	                            std::fabs( cmd->m_up_move ) > 0.1f ||
	                            ( cmd->m_buttons & ( e_command_buttons::in_forward | e_command_buttons::in_back |
	                                                 e_command_buttons::in_moveleft | e_command_buttons::in_moveright |
	                                                 e_command_buttons::in_jump | e_command_buttons::in_duck ) );
	const bool moving = is_player_moving( cmd );

	if ( m_waiting_for_playback_idle ) {
		if ( moving ) {
			m_auto_start_armed = false;
			m_session_active = false;
			m_recording_from_playback = false;
			m_session_elapsed = 0.f;
			m_playback_capture.clear( );
			return;
		}
		m_waiting_for_playback_idle = false;
	}

	if ( !m_playing && !m_going_to_start && !m_smoothing_to_start && !m_wish_to_start ) {
		if ( moving ) {
			m_auto_start_armed = true;
			if ( !m_session_active ) {
				if ( !m_recording_from_playback ) {
					m_recording_frames.clear( );
					m_playback_capture.clear( );
				}
				m_session_active = true;
				m_session_start_t = now_t - m_session_elapsed;
				if ( !m_recording_from_playback ) {
					m_session_elapsed = 0.f;
					m_active_route_name.clear( );
				}
			}

			m_session_elapsed = now_t - m_session_start_t;
			m_last_moving_time = now_t;
			g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
			m_recording_frames.emplace_back( cmd, g_ctx.m_local->get_origin( ) );
			m_recording_frames.back( ).jetpack = m_recording_jetpack_active;
		} else if ( m_session_active ) {
			m_session_active = false;
			if ( !m_recording_from_playback ) {
				m_session_elapsed = 0.f;
				m_recording_frames.clear( );
				m_playback_capture.clear( );
			} else {
				m_recording_from_playback = false;
				m_prefix_size = 0;
				m_session_elapsed = 0.f;
				m_playback_capture.clear( );
			}
		}
	}

	if ( !m_playing && !m_going_to_start && !m_smoothing_to_start && !m_wish_to_start && m_auto_start_armed &&
	     !movement_input && !menu_open && ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) ) {
		const c_vector& cur_origin = g_prediction.backup_data.m_origin;
		float best_dist = 1e9f;
		size_t best_index = static_cast< size_t >( -1 );
		for ( size_t i = 0; i < m_clips.size( ); ++i ) {
			if ( m_clips[ i ].frames.empty( ) )
				continue;
			const float dist = cur_origin.dist_to( m_clips[ i ].frames[ 0 ].position );
			if ( dist < k_start_radius && dist < best_dist ) {
				best_dist = dist;
				best_index = i;
			}
		}
		if ( best_index != static_cast< size_t >( -1 ) ) {
			m_selected_clip = static_cast< int >( best_index );
			m_auto_start_armed = false;
			m_recording_frames = m_clips[ best_index ].frames;
			m_active_route_name = m_clips[ best_index ].display_name;
			m_going_to_start = best_dist > k_pos_close_eps;
		}
	}

	if ( key_save && !menu_open ) {
		save_current_clip( );
	}

	if ( key_start_play && !m_session_active && !m_playing && !menu_open ) {
		size_t target_idx = static_cast< size_t >( -1 );
		const c_vector& cur_origin = g_prediction.backup_data.m_origin;

		if ( m_selected_clip >= 0 && m_selected_clip < static_cast< int >( m_clips.size( ) ) &&
		     !m_clips[ m_selected_clip ].frames.empty( ) &&
		     cur_origin.dist_to( m_clips[ m_selected_clip ].frames[ 0 ].position ) < k_start_radius ) {
			target_idx = static_cast< size_t >( m_selected_clip );
		} else if ( !m_clips.empty( ) ) {
			float best_dist = 1e9f;
			for ( size_t i = 0; i < m_clips.size( ); ++i ) {
				if ( m_clips[ i ].frames.empty( ) )
					continue;
				const float d = cur_origin.dist_to( m_clips[ i ].frames[ 0 ].position );
				if ( d < k_start_radius && d < best_dist ) {
					target_idx = i;
					best_dist  = d;
				}
			}
		}

		if ( target_idx != static_cast< size_t >( -1 ) )
			play_clip( target_idx );
	}

	if ( key_stop_play && ( m_playing || m_wish_to_start || m_going_to_start || m_smoothing_to_start ) ) {
		const bool was_playing = m_playing;
		force_stop( was_playing );
		if ( was_playing && m_session_active && m_recording_from_playback && g_ctx.m_local ) {
			if ( moving ) {
				g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
				m_recording_frames.emplace_back( cmd, g_ctx.m_local->get_origin( ) );
				m_recording_frames.back( ).jetpack = m_recording_jetpack_active;
			} else {
				m_session_active = false;
				m_recording_from_playback = false;
				m_prefix_size = 0;
				m_session_elapsed = 0.f;
				m_playback_capture.clear( );
			}
		}
		return;
	}

	if ( m_going_to_start ) {
		if ( m_recording_frames.empty( ) ) {
			m_going_to_start = false;
		} else {
			const c_vector& start_pos = m_recording_frames[ 0 ].position;
			const c_vector& cur_pos   = g_prediction.backup_data.m_origin;

			if ( cur_pos.dist_to( start_pos ) <= k_pos_close_eps ) {
				m_going_to_start = false;
			} else {
				const c_vector diff = cur_pos - start_pos;
				const float yaw_rad = cmd->m_view_point.m_y * 3.14159265358979323846f / 180.f;
				const float c       = std::cos( yaw_rad );
				const float s       = std::sin( yaw_rad );

				// rotate world delta into local frame, then push the opposite way
				const float local_x = diff.m_x * c + diff.m_y * s;
				const float local_y = diff.m_y * c - diff.m_x * s;

				cmd->m_forward_move = -local_x * 20.f;
				cmd->m_side_move    =  local_y * 20.f;
			}
		}
	}

	if ( m_smoothing_to_start ) {
		if ( m_recording_frames.empty( ) ) {
			m_smoothing_to_start = false;
		} else {
			cmd->m_mouse_delta_x = 0;
			cmd->m_mouse_delta_y = 0;

			const float target_x = m_recording_frames[ 0 ].view_pitch;
			const float target_y = m_recording_frames[ 0 ].view_yaw;

			float delta_x = target_x - cmd->m_view_point.m_x;
			float delta_y = target_y - cmd->m_view_point.m_y;
			while ( delta_y >  180.f ) delta_y -= 360.f;
			while ( delta_y < -180.f ) delta_y += 360.f;

			const float total = std::sqrtf( delta_x * delta_x + delta_y * delta_y );
			if ( total < k_angle_close_eps || !GET_VARIABLE( g_variables.m_movement_rec_lockva, bool ) ) {
				m_smoothing_to_start = false;
			} else {
				const float smoothing = std::max( 1.f, GET_VARIABLE( g_variables.m_movement_rec_smoothing, float ) );
				if ( total > 1e-4f ) {
					const float inv = 1.f / total;
					delta_x *= inv;
					delta_y *= inv;
				}
				delta_x /= smoothing;
				delta_y /= smoothing;

				cmd->m_view_point.m_x += delta_x;
				cmd->m_view_point.m_y += delta_y;

				c_angle aim_to = cmd->m_view_point;
				g_interfaces.m_engine_client->set_view_angles( aim_to );
			}
		}
	}

	if ( m_wish_to_start ) {
		if ( !m_smoothing_to_start && !m_going_to_start && !m_recording_frames.empty( ) ) {
			const auto& first = m_recording_frames[ 0 ];
			const c_vector& cur_pos = g_prediction.backup_data.m_origin;
			const float dist        = cur_pos.dist_to( first.position );

			float dyaw   = first.view_yaw - cmd->m_view_point.m_y;
			float dpitch = first.view_pitch - cmd->m_view_point.m_x;
			while ( dyaw >  180.f ) dyaw -= 360.f;
			while ( dyaw < -180.f ) dyaw += 360.f;
			const float angle_dist = std::sqrtf( dyaw * dyaw + dpitch * dpitch );
			const bool angles_ok   = !GET_VARIABLE( g_variables.m_movement_rec_lockva, bool ) || angle_dist < k_angle_close_eps;

			if ( dist < k_pos_close_eps && angles_ok ) {
				m_playing = true;
				m_play_idx = 0;
				m_play_start_tick = g_interfaces.m_global_vars_base->m_tick_count;
				m_route_render_start_time = g_interfaces.m_global_vars_base->m_current_time;
				m_wish_to_start = false;
			}
		}
	}

	if ( m_playing ) {
		if ( m_play_idx >= m_recording_frames.size( ) ) {
			force_stop( false, true );
			m_waiting_for_playback_idle = true;
		} else {
			const auto& f = m_recording_frames[ m_play_idx ];
			const c_angle backup_view = cmd->m_view_point;
			apply_replay_jetpack( f.jetpack );
			f.replay( cmd );
			const c_angle target_view = cmd->m_view_point;
			const bool stuck_frame = f.air_stuck || f.wall_stuck;

			if ( !GET_VARIABLE( g_variables.m_movement_rec_lockva, bool ) && !stuck_frame ) {
				// keep current viewangles, but rotate forward/side into them so
				// the recorded movement still points the right way (a la fix_movement)
				cmd->m_view_point = target_view;
				start_movement_fix( cmd );
				cmd->m_view_point = backup_view;
				end_movement_fix( cmd );
			} else if ( stuck_frame ) {
				c_angle replay_view = target_view;
				g_interfaces.m_engine_client->set_view_angles( replay_view );
			}

			m_playback_capture.emplace_back( cmd, g_ctx.m_local->get_origin( ) );
			m_playback_capture.back( ).jetpack = f.jetpack;

			if ( m_prefix_size > 0 && m_play_idx + 1 == m_prefix_size &&
			     m_prefix_size < m_recording_frames.size( ) ) {
				const auto& first_new = m_recording_frames[ m_prefix_size ];
				if ( first_new.has_velocity && g_ctx.m_local )
					g_ctx.m_local->get_velocity( ) = first_new.velocity;
			}

			++m_play_idx;
		}
	}

	if ( key_clear && !menu_open && !m_session_active && !m_playing ) {
		m_recording_frames.clear( );
		m_playback_capture.clear( );
	}

	cmd->m_view_point.normalize( );
}

// ---------------------------------------------------------------------------
// frame_stage_notify — interpolated viewangles during playback (when locking)
// ---------------------------------------------------------------------------
void n_movement_recorder::impl_t::on_frame_stage( int stage )
{
	if ( stage != static_cast< int >( e_client_frame_stage::start ) )
		return;
	if ( !m_playing )
		return;
	if ( !GET_VARIABLE( g_variables.m_movement_rec_lockva, bool ) )
		return;

	if ( m_play_idx >= m_recording_frames.size( ) )
		return;
	if ( m_play_idx + 1 >= m_recording_frames.size( ) )
		return;

	const auto& fr_cur  = m_recording_frames[ m_play_idx ];
	const auto& fr_next = m_recording_frames[ m_play_idx + 1 ];

	const float tick_dt    = g_interfaces.m_global_vars_base->m_interval_per_tick;
	const float cur_tick_t = ( m_play_start_tick + static_cast< int >( m_play_idx ) ) * tick_dt;
	const float total      = tick_dt;
	const float elapsed    = g_interfaces.m_global_vars_base->m_current_time - cur_tick_t;
	float progress         = total > 1e-6f ? elapsed / total : 1.f;
	progress               = std::clamp( progress, 0.f, 1.f );

	const float pitch_interp = fr_cur.view_pitch + ( fr_next.view_pitch - fr_cur.view_pitch ) * progress;

	float yaw_delta = fr_next.view_yaw - fr_cur.view_yaw;
	while ( yaw_delta >  180.f ) yaw_delta -= 360.f;
	while ( yaw_delta < -180.f ) yaw_delta += 360.f;
	const float yaw_interp = fr_cur.view_yaw + yaw_delta * progress;

	c_angle interpolated{ pitch_interp, yaw_interp, 0.f };
	g_interfaces.m_engine_client->set_view_angles( interpolated );
}

// ---------------------------------------------------------------------------
// camera_lock — disable mouse delta while locking is active
// ---------------------------------------------------------------------------
void n_movement_recorder::impl_t::camera_lock( float& x, float& y )
{
	if ( !GET_VARIABLE( g_variables.m_movement_rec, bool ) )
		return;

	const bool stuck_frame = m_playing && m_play_idx < m_recording_frames.size( ) &&
	                         ( m_recording_frames[ m_play_idx ].air_stuck || m_recording_frames[ m_play_idx ].wall_stuck );

	if ( m_playing && ( GET_VARIABLE( g_variables.m_movement_rec_lockva, bool ) || stuck_frame ) ) {
		x = 0.f;
		y = 0.f;
	} else if ( GET_VARIABLE( g_variables.m_movement_rec_lockgoingtostart, bool ) &&
	            m_smoothing_to_start ) {
		x = 0.f;
		y = 0.f;
	}
}

// ---------------------------------------------------------------------------
// rendering
// ---------------------------------------------------------------------------
static unsigned int rgba( int r, int g, int b, int a )
{
	return ImGui::GetColorU32( ImVec4( r / 255.f, g / 255.f, b / 255.f, a / 255.f ) );
}

static void enqueue_line( const c_vector_2d& a, const c_vector_2d& b, unsigned int col, float th )
{
	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( a, b, col, th ) );
}

static void enqueue_circle( const c_vector_2d& c, float r, unsigned int col, int seg, float th )
{
	circle_draw_object_t obj{ };
	obj.m_center    = c;
	obj.m_radius    = r;
	obj.m_color     = col;
	obj.m_segments  = seg;
	obj.m_thickness = th;
	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_circle, std::make_any< circle_draw_object_t >( obj ) );
}

static void enqueue_filled_circle( const c_vector_2d& c, float r, unsigned int col, int seg )
{
	filled_circle_draw_object_t obj{ };
	obj.m_center   = c;
	obj.m_radius   = r;
	obj.m_color    = col;
	obj.m_segments = seg;
	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_filled_circle, std::make_any< filled_circle_draw_object_t >( obj ) );
}

static void enqueue_world_floor_circle( const c_vector& center, float radius, unsigned int col, int seg, float th )
{
	c_vector_2d prev{ };
	bool has_prev = false;
	c_vector_2d first{ };
	bool has_first = false;

	for ( int i = 0; i <= seg; ++i ) {
		const float a = ( static_cast< float >( i ) / static_cast< float >( seg ) ) * 6.28318530718f;
		c_vector p{ center.m_x + std::cos( a ) * radius, center.m_y + std::sin( a ) * radius, center.m_z + 1.f };
		c_vector_2d screen{ };
		if ( !g_render.world_to_screen( p, screen ) ) {
			has_prev = false;
			continue;
		}
		if ( !has_first ) {
			first = screen;
			has_first = true;
		}
		if ( has_prev )
			enqueue_line( prev, screen, col, th );
		prev = screen;
		has_prev = true;
	}
	if ( has_prev && has_first )
		enqueue_line( prev, first, col, th );
}

static void enqueue_world_floor_disc( const c_vector& center, float radius, unsigned int fill_col, unsigned int outline_col, int seg, float outline_th = 1.5f )
{
	convex_poly_draw_object_t obj{ };
	obj.m_points.reserve( seg );
	obj.m_fill_color       = fill_col;
	obj.m_outline_color    = outline_col;
	obj.m_outline_thickness = outline_th;

	for ( int i = 0; i < seg; ++i ) {
		const float a = ( static_cast< float >( i ) / static_cast< float >( seg ) ) * 6.28318530718f;
		c_vector p{ center.m_x + std::cos( a ) * radius, center.m_y + std::sin( a ) * radius, center.m_z + 1.f };
		c_vector_2d screen{ };
		if ( !g_render.world_to_screen( p, screen ) )
			return;
		obj.m_points.push_back( screen );
	}

	if ( obj.m_points.size( ) < 3 )
		return;

	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_convex_poly, std::make_any< convex_poly_draw_object_t >( std::move( obj ) ) );
}

static void enqueue_text( ImFont* font, const c_vector_2d& pos, const std::string& text, unsigned int col, float scale = 1.f )
{
	text_draw_object_t obj{ };
	obj.m_font          = font;
	obj.m_position      = pos;
	obj.m_text          = text;
	obj.m_color         = col;
	obj.m_outline_color = ImGui::GetColorU32( ImVec4( 0.f, 0.f, 0.f, 1.f ) );
	obj.m_draw_flags    = static_cast< e_text_flags >( text_flag_outline | text_flag_dropshadow );
	obj.m_font_scale    = scale;
	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_text, std::make_any< text_draw_object_t >( obj ) );
}

static unsigned int accent_u32( float alpha = 1.f )
{
	const ImVec4 c = ImGui::GetStyleColorVec4( ImGuiCol_::ImGuiCol_Accent );
	return ImGui::GetColorU32( ImVec4( c.x, c.y, c.z, alpha ) );
}

static void enqueue_eye_start_trace( const n_movement_recorder::frame_t& frame, float alpha_mul )
{
	if ( !GET_VARIABLE( g_variables.m_movement_rec_show_eye_start, bool ) )
		return;
	if ( g_movement_recorder.m_playing )
		return;
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) || !g_interfaces.m_engine_client || !g_interfaces.m_engine_trace )
		return;
	if ( g_ctx.m_local->get_origin( ).dist_to( frame.position ) >= k_start_radius )
		return;

	const c_vector eye = g_ctx.m_local->get_eye_position( false );
	c_angle current_angles{ };
	g_interfaces.m_engine_client->get_view_angles( current_angles );

	auto trace_view = [ & ]( const c_vector& direction ) {
		const c_vector trace_end = eye + direction * 900.f;
		trace_t trace{ };
		ray_t ray( eye, trace_end );
		c_trace_filter filter( g_ctx.m_local );
		g_interfaces.m_engine_trace->trace_ray( ray, mask_shot | contents_grate, &filter, &trace );
		return trace.m_fraction < 1.0f ? trace.m_end : trace_end;
	};

	const c_vector current_forward = c_vector::fromAngle( c_vector( current_angles.m_x, current_angles.m_y, 0.f ) );
	const c_vector target_forward = c_vector::fromAngle( c_vector( frame.view_pitch, frame.view_yaw, 0.f ) );
	const c_vector current_end = trace_view( current_forward );
	const c_vector target_end = trace_view( target_forward );

	c_vector_2d current_screen{ }, target_screen{ };
	if ( !g_render.world_to_screen( current_end, current_screen ) || !g_render.world_to_screen( target_end, target_screen ) )
		return;

	const int alpha = static_cast< int >( 100.f * alpha_mul );
	enqueue_line( current_screen, target_screen, rgba( 0, 0, 0, alpha ), 2.5f );
	enqueue_line( current_screen, target_screen, rgba( 235, 235, 235, alpha ), 1.0f );
	enqueue_filled_circle( current_screen, 2.5f, rgba( 0, 0, 0, alpha ), 20 );
	enqueue_filled_circle( target_screen, 8.5f, rgba( 0, 0, 0, alpha ), 32 );
}

void n_movement_recorder::impl_t::render_edited_clip( )
{
	if ( !m_editor_open || m_editor_clip_index < 0 || m_editor_clip_index >= static_cast< int >( m_clips.size( ) ) )
		return;

	const auto& clip = m_clips[ m_editor_clip_index ];
	if ( clip.frames.size( ) < 2 )
		return;

	const float dt = ImGui::GetIO( ).DeltaTime;
	m_editor_anim_progress += dt * 0.75f; // Animate over ~1.3 seconds
	m_editor_anim_progress = std::clamp( m_editor_anim_progress, 0.f, 1.f );

	// Calculate ease-out quad for smoother animation
	const float ease_out = 1.f - ( 1.f - m_editor_anim_progress ) * ( 1.f - m_editor_anim_progress );

	const ImVec4 acc = ImGui::GetStyleColorVec4( ImGuiCol_::ImGuiCol_Accent );
	const unsigned int col = rgba( static_cast< int >( acc.x * 255.f ), static_cast< int >( acc.y * 255.f ), static_cast< int >( acc.z * 255.f ), 200 );
	
	// We only show up to the trimmed end
	const int max_frame = std::clamp( m_editor_trim_end, 0, static_cast< int >( clip.frames.size( ) ) - 1 );
	
	// Total frames to draw based on animation
	const int draw_frames = static_cast< int >( std::max( 1.0f, static_cast< float >( max_frame ) * ease_out ) );

	for ( int i = 1; i <= draw_frames; ++i ) {
		c_vector_2d from{ }, to{ };
		if ( !g_render.world_to_screen( clip.frames[ i ].position, from ) )
			continue;
		if ( !g_render.world_to_screen( clip.frames[ i - 1 ].position, to ) )
			continue;

		enqueue_line( from, to, col, 1.5f );

		if ( i == 1 ) {
			const float pad_radius = k_start_radius * 0.45f;
			enqueue_world_floor_disc( clip.frames[ 0 ].position, pad_radius, rgba( 0, 0, 0, 100 ), accent_u32( 0.85f ), 64, 1.5f );
			enqueue_eye_start_trace( clip.frames[ 0 ], 1.0f );
		}
	}
}

void n_movement_recorder::impl_t::render_active_path( ImDrawList* /*draw*/ )
{
	if ( !GET_VARIABLE( g_variables.m_movement_rec_show_line, bool ) )
		return;
	if ( m_recording_frames.size( ) < 2 )
		return;

	const float elapsed = ( g_interfaces.m_global_vars_base->m_current_time - m_route_render_start_time ) * 4.f;
	const float progress = std::clamp( elapsed, 0.f, 1.f );

	const ImVec4 acc = ImGui::GetStyleColorVec4( ImGuiCol_::ImGuiCol_Accent );
	const int alpha   = static_cast< int >( 255.f * progress );
	const float r     = 255.f + ( acc.x * 255.f - 255.f ) * progress;
	const float g     = 255.f + ( acc.y * 255.f - 255.f ) * progress;
	const float b     = 255.f + ( acc.z * 255.f - 255.f ) * progress;
	const unsigned int col = rgba( static_cast< int >( r ), static_cast< int >( g ), static_cast< int >( b ), alpha );

	for ( size_t i = 1; i < m_recording_frames.size( ); ++i ) {
		c_vector_2d from{ }, to{ };
		if ( !g_render.world_to_screen( m_recording_frames[ i ].position, from ) )
			continue;
		if ( !g_render.world_to_screen( m_recording_frames[ i - 1 ].position, to ) )
			continue;

		enqueue_line( from, to, col, 0.5f );

		if ( i == 1 ) {
			const float pad_radius = k_start_radius * 0.45f;
			enqueue_world_floor_disc( m_recording_frames[ 0 ].position, pad_radius, rgba( 0, 0, 0, 100 ), accent_u32( 0.85f ), 64, 1.5f );
			enqueue_eye_start_trace( m_recording_frames[ 0 ], progress );
		}
	}
}

void n_movement_recorder::impl_t::render_route_preview( ImDrawList* /*draw*/ )
{
	if ( m_clips.empty( ) )
		return;
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return;

	const float dt = ImGui::GetIO( ).DeltaTime;
	if ( m_point_progress.size( ) != m_clips.size( ) )
		m_point_progress.assign( m_clips.size( ), 1.f );
	if ( m_time_anim.size( ) != m_clips.size( ) )
		m_time_anim.assign( m_clips.size( ), 0.f );

	const bool enter_pressed = !m_editor_open && !g_menu.m_opened && g_input.is_key_released( VK_RETURN );

	for ( size_t i = 0; i < m_clips.size( ); ++i ) {
		const auto& route = m_clips[ i ];
		if ( route.frames.empty( ) )
			continue;

		const c_vector& start_pos = route.frames[ 0 ].position;
		c_vector_2d screen{ };
		if ( !g_render.world_to_screen( start_pos, screen ) )
			continue;

		const float distance = g_ctx.m_local->get_origin( ).dist_to( start_pos );
		if ( enter_pressed && distance < k_start_radius ) {
			m_selected_clip = static_cast< int >( i );
			open_clip_editor( i );
			return;
		}

		const float base_radius = k_start_radius;
		const float target_scale = 1.f;
		m_point_progress[ i ] += ( target_scale - m_point_progress[ i ] ) * std::clamp( dt * 5.f, 0.f, 1.f );

		float alpha_mul = 1.f;
		if ( distance > 900.f )
			alpha_mul = std::clamp( ( 1000.f - distance ) / 100.f, 0.f, 1.f );

		const float effective_r = base_radius * 0.45f * m_point_progress[ i ];

		enqueue_world_floor_disc( start_pos, effective_r, rgba( 0, 0, 0, static_cast< int >( 100.f * alpha_mul ) ), accent_u32( 0.85f * alpha_mul ), 64, 1.5f );
		enqueue_eye_start_trace( route.frames[ 0 ], alpha_mul );
		m_time_anim[ i ] = 0.f;

		// label
		auto* font = fonts_for_gui::regular_11 ? fonts_for_gui::regular_11 : g_render.m_fonts[ e_font_names::font_name_verdana_11 ];
		if ( font && !route.display_name.empty( ) ) {
			constexpr float title_scale = 0.82f;
			const float title_size = font->FontSize * title_scale;
			const ImVec2 text_size = font->CalcTextSizeA( title_size, FLT_MAX, 0.f, route.display_name.c_str( ) );
			enqueue_text( font, c_vector_2d{ screen.m_x - text_size.x * 0.5f, screen.m_y - 34.f }, route.display_name, rgba( 225, 225, 225, static_cast< int >( 255.f * alpha_mul ) ), title_scale );
		}
	}
}

void n_movement_recorder::impl_t::render_indicator( ImDrawList* /*draw*/ )
{
	if ( !GET_VARIABLE( g_variables.m_movement_rec_render, bool ) )
		return;

	const float dt = ImGui::GetIO( ).DeltaTime;
	const bool any_active = m_session_active || m_playing;
	const float target = any_active ? 1.f : 0.f;
	const float k = 1.f - std::exp( -k_indicator_lerp_k * dt );
	m_indicator_alpha += ( target - m_indicator_alpha ) * k;
	m_indicator_alpha = std::clamp( m_indicator_alpha, 0.f, 1.f );
	if ( m_indicator_alpha <= 0.01f )
		return;

	const std::string text = std::to_string( m_play_idx ) + " / " + std::to_string( m_recording_frames.size( ) );
	auto* font = g_render.m_fonts[ e_font_names::font_name_verdana_11 ];
	if ( !font )
		return;

	const ImVec2 ts = font->CalcTextSizeA( font->FontSize, FLT_MAX, 0.f, text.c_str( ) );
	const ImVec2 padding{ 7.f, 7.f };
	const ImVec2 margin{ 4.f, 3.f };
	const ImVec2 size{ ts.x + margin.x * 2.f, ts.y + margin.y * 2.f };
	const ImVec2 display = ImGui::GetIO( ).DisplaySize;

	c_vector_2d pos{ };
	switch ( GET_VARIABLE( g_variables.m_movement_rec_position, int ) ) {
	default:
	case 0: pos = { padding.x, padding.y }; break;
	case 1: pos = { padding.x, display.y - size.y - padding.y }; break;
	case 2: pos = { display.x - size.x - padding.x, display.y - size.y - padding.y }; break;
	}

	const c_vector_2d text_pos{ pos.m_x + margin.x, pos.m_y + margin.y };

	const ImVec4 acc = ImGui::GetStyleColorVec4( ImGuiCol_::ImGuiCol_Accent );
	const unsigned int outline_col = ImGui::GetColorU32( ImVec4( acc.x, acc.y, acc.z, m_indicator_alpha ) );
	const unsigned int bg_col      = ImGui::GetColorU32( ImVec4( 0.08f, 0.08f, 0.08f, m_indicator_alpha ) );

	rect_draw_object_t bg{ };
	bg.m_min            = pos;
	bg.m_max            = c_vector_2d{ pos.m_x + size.x, pos.m_y + size.y };
	bg.m_color          = bg_col;
	bg.m_outline_color  = bg_col;
	bg.m_filled         = true;
	bg.m_rounding       = 0.f;
	bg.m_corner_rounding_flags = 0;
	bg.m_thickness      = 1.f;
	bg.m_outline_flags  = e_rect_flags::rect_flag_none;
	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_rect, std::make_any< rect_draw_object_t >( bg ) );

	rect_draw_object_t accent_bar{ };
	accent_bar.m_min            = c_vector_2d{ pos.m_x, pos.m_y - 1.f };
	accent_bar.m_max            = c_vector_2d{ pos.m_x + size.x, pos.m_y };
	accent_bar.m_color          = outline_col;
	accent_bar.m_outline_color  = outline_col;
	accent_bar.m_filled         = true;
	accent_bar.m_rounding       = 0.f;
	accent_bar.m_corner_rounding_flags = 0;
	accent_bar.m_thickness      = 1.f;
	accent_bar.m_outline_flags  = e_rect_flags::rect_flag_none;
	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_rect, std::make_any< rect_draw_object_t >( accent_bar ) );

	enqueue_text( font, text_pos, text, ImGui::GetColorU32( ImVec4( 1.f, 1.f, 1.f, m_indicator_alpha ) ) );
}

void n_movement_recorder::impl_t::render_clipper_box( )
{
	if ( !GET_VARIABLE( g_variables.m_movement_rec_clipper_box, bool ) )
		return;

	float elapsed = 0.f;
	float total = 0.f;
	if ( m_session_active ) {
		elapsed = m_session_elapsed;
	} else if ( m_playing ) {
		const float dt = g_interfaces.m_global_vars_base->m_interval_per_tick;
		elapsed = static_cast< float >( m_play_idx ) * dt;
		total = static_cast< float >( m_recording_frames.size( ) ) * dt;
	} else {
		elapsed = m_session_elapsed;
	}

	const bool any_active = m_session_active || m_playing;
	if ( !any_active && elapsed < 0.01f )
		return;

	auto* font = fonts_for_gui::rubik_regular ? fonts_for_gui::rubik_regular : g_render.m_fonts[ e_font_names::font_name_verdana_11 ];
	if ( !font )
		return;

	char buf[ 32 ]{ };
	if ( m_playing )
		std::snprintf( buf, sizeof( buf ), "%.1f / %.1f", elapsed, total );
	else
		std::snprintf( buf, sizeof( buf ), "%.1f", elapsed );

	ImGui::SetNextWindowPos( ImVec2( 24.f, 96.f ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( m_playing ? 76.f : 42.f, 22.f ), ImGuiCond_Always );
	ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.04f, 0.04f, 0.04f, 1.f ) );
	ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 0.f, 0.f, 0.f, 1.f ) );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 3.f );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 5.f, 4.f ) );
	ImGui::PushFont( font );
	if ( ImGui::Begin( "##mr_clipper_box", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar ) ) {
		ImGui::SetWindowFontScale( 0.72f );
		const ImVec2 text_size = ImGui::CalcTextSize( buf );
		ImGui::SetCursorPosX( ( ImGui::GetWindowSize( ).x - text_size.x ) * 0.5f );
		ImGui::TextUnformatted( buf );
	}
	ImGui::End( );
	ImGui::PopFont( );
	ImGui::PopStyleVar( 2 );
	ImGui::PopStyleColor( 2 );
}

void n_movement_recorder::impl_t::render_clip_editor( )
{
	if ( !m_editor_open )
		return;
	if ( m_editor_ignore_enter_release ) {
		m_editor_ignore_enter_release = false;
	} else if ( g_input.is_key_released( VK_ESCAPE ) || ( g_input.is_key_released( VK_RETURN ) && !ImGui::IsAnyItemActive( ) ) ) {
		m_editor_open = false;
		return;
	}
	if ( m_editor_clip_index < 0 || m_editor_clip_index >= static_cast< int >( m_clips.size( ) ) ) {
		m_editor_open = false;
		return;
	}

	auto& clip = m_clips[ m_editor_clip_index ];
	if ( clip.frames.empty( ) ) {
		m_editor_open = false;
		return;
	}

	auto* font = fonts_for_gui::regular_11 ? fonts_for_gui::regular_11 : g_render.m_fonts[ e_font_names::font_name_verdana_11 ];
	const ImVec2 display = ImGui::GetIO( ).DisplaySize;
	ImGui::SetNextWindowPos( ImVec2( display.x - 280.f, 20.f ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 260.f, 120.f ), ImGuiCond_Always );
	ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.06f, 0.06f, 0.06f, 0.95f ) );
	ImGui::PushStyleColor( ImGuiCol_TitleBg, ImVec4( 0.06f, 0.06f, 0.06f, 0.95f ) );
	ImGui::PushStyleColor( ImGuiCol_TitleBgActive, ImVec4( 0.06f, 0.06f, 0.06f, 0.95f ) );
	ImGui::PushStyleColor( ImGuiCol_TitleBgCollapsed, ImVec4( 0.06f, 0.06f, 0.06f, 0.95f ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.12f, 0.12f, 0.12f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImVec4( 0.15f, 0.15f, 0.15f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImVec4( 0.18f, 0.18f, 0.18f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.f, 0.f, 0.f, 0.f ) );
	ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 0.f, 0.f, 0.f, 0.f ) );
	ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.f, 1.f, 1.f, 1.f ) );
	ImGui::PushStyleColor( ImGuiCol_SliderGrab, ImVec4( 0.9f, 0.9f, 0.9f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_SliderGrabActive, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 8.f );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.f );
	if ( font )
		ImGui::PushFont( font );

	const int flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse;
	if ( ImGui::Begin( "Record editor", &m_editor_open, flags ) ) {
		const float old_anim_progress = g_menu.m_animation_progress;
		g_menu.m_animation_progress = 1.0f;

		ImGui::Dummy( ImVec2( 0.f, 5.f ) );

		ImGui::Dummy( ImVec2( 0.f, 5.f ) );

		ImGui::SetNextItemWidth( 245.f );
		ImGui::InputText( "##mr_editor_name", m_editor_name, sizeof( m_editor_name ) );
		
		ImVec2 min_p = ImGui::GetItemRectMin( );
		ImVec2 max_p = ImGui::GetItemRectMax( );
		ImGui::GetWindowDrawList( )->AddLine( ImVec2( min_p.x, max_p.y ), ImVec2( max_p.x, max_p.y ), IM_COL32( 255, 255, 255, 255 ) );

		ImGui::Dummy( ImVec2( 0.f, 5.f ) );

		if ( smbutton_hh( "Save" ) )
			apply_clip_editor( );

		ImGui::SameLine( );

		if ( smbutton_hh( "Delete" ) )
			delete_clip( m_editor_clip_index );

		ImGui::Dummy( ImVec2( 0.f, 5.f ) );

		const int max_frame = static_cast< int >( clip.frames.size( ) ) - 1;
		m_editor_trim_start = 0;
		m_editor_trim_end = std::clamp( m_editor_trim_end, 0, max_frame );

		char format_buf[ 64 ];
		std::snprintf( format_buf, sizeof( format_buf ), "%%d/%d", max_frame );

		ImGui::SetNextItemWidth( 150.f );
		ImGui::SliderInt( "##frames_slider", &m_editor_trim_end, 0, max_frame, format_buf );

		ImGui::SameLine( );
		ImGui::TextColored( ImVec4( 1.f, 1.f, 1.f, 1.f ), "%d", max_frame + 1 );

		g_menu.m_animation_progress = old_anim_progress;
	}
	ImGui::End( );
	if ( font )
		ImGui::PopFont( );
	ImGui::PopStyleVar( 2 );
	ImGui::PopStyleColor( 12 );
}

void n_movement_recorder::impl_t::on_end_scene( )
{
	if ( !GET_VARIABLE( g_variables.m_movement_rec, bool ) )
		return;
	if ( !g_interfaces.m_engine_client || !g_interfaces.m_engine_client->is_in_game( ) )
		return;

	render_clip_editor( );
	render_clipper_box( );
}

void n_movement_recorder::impl_t::on_paint_traverse( )
{
	check_map_change( );

	if ( !GET_VARIABLE( g_variables.m_movement_rec, bool ) )
		return;
	if ( !g_render.m_initialised )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;

	if ( !m_session_active && !m_playing ) {
		const float now = g_interfaces.m_global_vars_base ? g_interfaces.m_global_vars_base->m_current_time : 0.f;
		if ( now - m_last_auto_refresh > 5.f ) {
			m_last_auto_refresh = now;
			refresh_clips( );
		}
	}

	if ( g_ctx.m_local && g_ctx.m_local->is_alive( ) ) {
		// idle: preview start markers; only show line during playback
		if ( m_editor_open && m_editor_clip_index >= 0 && m_editor_clip_index < static_cast< int >( m_clips.size( ) ) )
			render_edited_clip( );
		else if ( m_playing )
			render_active_path( nullptr );
		else
			render_route_preview( nullptr );
	}

}

// menu UI lives in hacks/menu/menu.cpp under the "Recorder" tab — see display_tab_number == 9 block.
