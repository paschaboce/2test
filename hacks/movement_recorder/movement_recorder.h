#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "../../game/sdk/classes/c_vector.h"
#include "../../game/sdk/classes/c_angle.h"

class c_user_cmd;
struct ImDrawList;

namespace n_movement_recorder
{
	struct frame_t {
		float view_pitch = 0.f;
		float view_yaw   = 0.f;
		float forward    = 0.f;
		float side       = 0.f;
		float up         = 0.f;
		int   buttons    = 0;
		short mouse_dx   = 0;
		short mouse_dy   = 0;
		bool  jetpack       = false;
		bool  air_stuck     = false;
		bool  wall_stuck    = false;
		bool  has_velocity  = false;
		c_vector position{ };
		c_vector eye_position{ };
		c_vector velocity{ };

		frame_t( ) = default;
		frame_t( c_user_cmd* cmd, const c_vector& pos );

		void replay( c_user_cmd* cmd ) const;
	};

	struct route_t {
		std::string name;
		std::vector< frame_t > frames;
	};

	// One clip = one on-disk .mr file. Each file holds a single route,
	// the map it was recorded on, and its display name.
	struct clip_t {
		std::string filename;       // physical filename on disk (e.g. "de_nuke 27_04_16_04.mr")
		std::string display_name;   // shown in menu (filename minus extension)
		std::string map;            // map this clip belongs to
		std::vector< frame_t > frames;
	};

	struct impl_t {
		// playback state ----------------------------------------------------
		bool m_playing     = false;
		size_t m_play_idx  = 0;
		int    m_play_start_tick = 0;
		float  m_route_render_start_time = 0.f;

		std::vector< frame_t > m_recording_frames;   // active session buffer / playback source
		std::vector< frame_t > m_playback_capture;   // tick-by-tick capture of cmd during playback
		std::vector< clip_t >  m_clips;              // clips matching current map
		int m_selected_clip = -1;
		std::string m_active_route_name;             // for the indicator

		// approach-to-start state (used by playback)
		bool m_going_to_start     = false;
		bool m_smoothing_to_start = false;
		bool m_wish_to_start      = false;

		// indicator animation
		float m_indicator_alpha = 0.f;
		float m_last_auto_refresh = 0.f;

		// continuous-record session state
		bool  m_session_active   = false;
		float m_session_start_t  = 0.f;
		float m_session_elapsed  = 0.f;
		float m_last_moving_time = 0.f;

		std::string m_current_map;
		bool m_recording_jetpack_active = false;
		bool m_replay_jetpack_active = false;
		bool m_editor_open = false;
		bool m_editor_ignore_enter_release = false;
		bool m_auto_start_armed = true;
		bool m_recording_from_playback = false;
		bool m_round_frozen = false;
		size_t m_prefix_size = 0;
		bool m_waiting_for_playback_idle = false;
		int m_editor_clip_index = -1;
		int m_editor_trim_start = 0;
		int m_editor_trim_end = 0;
		char m_editor_name[ 128 ]{ };
		float m_editor_anim_progress = 0.f;

		// per-route render anim helpers
		std::vector< float > m_point_progress;
		std::vector< float > m_time_anim;

		// API ---------------------------------------------------------------
		void on_create_move( c_user_cmd* cmd );
		void on_frame_stage( int stage );
		void on_paint_traverse( );
		void on_end_scene( );
		void camera_lock( float& x, float& y );
		void force_stop( bool keep_replay_prefix = false, bool clear_recording = false );
		void set_round_frozen( bool frozen );
		void notify_jetpack_state( bool active );

		// disk / clip management (UI lives in hacks/menu/menu.cpp)
		std::filesystem::path get_root_path( ) const;
		void ensure_root( );
		void refresh_clips( );                 // rescan disk, filter by m_current_map
		void save_current_clip( );             // dumps m_recording_frames as a new .mr clip
		void delete_clip( size_t index );
		bool play_clip( size_t index );

	private:
		void check_map_change( );
		bool is_player_moving( c_user_cmd* cmd ) const;
		void apply_replay_jetpack( bool active );
		void open_clip_editor( size_t index );
		void render_clip_editor( );
		void apply_clip_editor( );
		void rename_clip( size_t index, const std::string& new_name );
		void render_route_preview( ImDrawList* draw );
		void render_edited_clip( );
		void render_active_path( ImDrawList* draw );
		void render_indicator( ImDrawList* draw );
		void render_clipper_box( );
	};
}

inline n_movement_recorder::impl_t g_movement_recorder{ };
