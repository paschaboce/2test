#pragma once
#define WINDOWS_IGNORE_PACKING_MISMATCH
#include <vector>
#include <Windows.h>
#include <string>
#include "../../game/sdk/classes/c_vector.h"
class c_user_cmd;
class c_angle;
enum class route_marker_type_t {
	floor,
	ceiling,
	bounce,
	pixelsurf,
	headbang
};
struct jump_marker_t {
	c_vector position;
	std::string map;
	route_marker_type_t type = route_marker_type_t::floor;
	float scale = 0.f;
	bool active = true;
	bool needs_duck = false;
	bool needs_jump = true;
	float required_speed = 0.f;
	jump_marker_t() = default;
	jump_marker_t( const c_vector& pos, const std::string& map_name, route_marker_type_t marker_type )
		: position( pos ), map( map_name ), type( marker_type ), active( true ) {}
};
struct route_studio_t {
	std::vector< jump_marker_t > markers;
	std::string current_route_name = "default";
	bool scanning = false;
	bool dirty = false;
	void add_marker( const c_vector& pos, const std::string& map, route_marker_type_t type ) {
		markers.push_back( jump_marker_t( pos, map, type ) );
		dirty = true;
	}
	void clear_markers() {
		markers.clear();
		dirty = true;
	}
	void clear_markers_for_map( const std::string& map ) {
		markers.erase( 
			std::remove_if( markers.begin(), markers.end(), 
				[&map]( const jump_marker_t& m ) { return m.map == map; } ),
			markers.end() );
		dirty = true;
	}
	size_t marker_count() const { return markers.size(); }
	size_t marker_count_for_map( const std::string& map ) const {
		return std::count_if( markers.begin(), markers.end(),
			[&map]( const jump_marker_t& m ) { return m.map == map; } );
	}
};
struct points_check_t {
	c_vector pos;
	std::string map;
	float currentScale     = 0.f;
	bool open_settings     = false;
	bool jump              = true;
	bool minijump          = true;
	bool longjump          = true;
	bool jumpbug           = true;
	bool crouch_hop        = true;
	bool mini_crouch_hop   = true;
	bool c_jump            = true;
	bool c_minijump        = true;
	bool c_longjump        = true;
	bool c_jumpbug         = true;
	bool c_crouch_hop      = true;
	bool c_mini_crouch_hop = true;
	bool high_crouch_jump  = true;
	bool c_high_crouch_jump = true;
	bool active            = true;
	float radius           = 300.f;
	float delta_strafe     = 0.0f;
	int selected_preset    = 0;
	bool advanced_enabled  = false;
	int selected_advanced  = 0;
	float first_jump_radius = 250.f;
	bool use_custom_first_jump_radius = false;
	bool enable_restricted_binds = false;
	int selected_restricted_bind = 0;
	float restricted_binds_radius = 200.f;
	bool is_free_point = false;
	int point_type = 0;
	bool show_3d_radius = false; 
	points_check_t( const c_vector& Pos = c_vector(), const std::string& Map = "" ) : pos( Pos ), map( Map ), is_free_point( false ), point_type( 0 ) { }
};

enum edgebug_type_t : int {
	eb_standing = 0,
	eb_ducking,
};
namespace n_movement
{
	// ε — surface extension (Hammer units, 1/32). Review.pdf Table 1; engine trace wall gap / flush brushes — auto_align + texture_bug.
	inline constexpr float surface_extension_epsilon_u = 0.03125f;

	struct impl_t {
		struct edgebug_data_t {
			edgebug_type_t m_edgebug_method{ };
			bool m_will_edgebug{ };
			bool m_will_fail{ };
			bool m_strafing{ };
			float m_yaw_delta{ };
			float m_starting_yaw{ };
			float m_side_move{ };
			float m_forward_move{ };
			float m_saved_mousedx{ };
			int m_ticks_to_stop{ };
			int m_last_tick{ };
			void reset( );
		} m_edgebug_data;
		struct jumpbug_data_t {
			int m_height_diff                    = 0.f;
			float m_vertical_velocity_at_landing = 0.f;
			float m_abs_height_diff              = 0.f;
			int m_ticks_till_land                = 0;
			bool m_can_jb                        = false;
		} m_jumpbug_data;
		struct trajectory_segment_t {
			std::vector<c_vector> points;
			std::string map_name;
		};
		
		struct pixelsurf_data_t {
			bool m_predicted_succesful = false, m_in_pixel_surf = false, m_should_duck = false;
			int m_prediction_ticks      = 0;
			c_user_cmd* m_simulated_cmd = { };
			bool should_pixel_surf      = false;
			
			// Траектория для постоянного рендера
			std::vector<c_vector> m_trajectory_points;
			std::vector<trajectory_segment_t> m_trajectory_segments;
			bool m_was_in_surf = false;
			c_vector m_last_position;
			bool m_delete_mode = false;
			
			void reset( );
		} m_pixelsurf_data;
		struct autoduck_data_t {
			bool m_did_land_ducking  = false;
			bool m_did_land_standing = false;
			float m_ducking_vert  = 0.f;
			float m_standing_vert = 0.f;
			void reset( );
		} m_autoduck_data;
		struct fireman_data_t {
			// compatibility flags (consumed by indicators.cpp / auto_align)
			bool is_ladder = false;
			bool awall     = false;

			// ported state from fr.txt
			bool detected         = false;
			float ladder_yaw      = 0.f;
			int glide_remaining   = 0;   // ticks left of "falling through ladder"
			int strip_remaining   = 0;   // ticks left where we must keep IN_JUMP stripped

			// legacy fields kept to minimize diff to other call sites (no longer used here)
			bool fr_hit_1 = false;
			bool fr_hit   = false;

			bool in_glide( ) const
			{
				return detected && glide_remaining > 0;
			}
			bool should_strip_jump( ) const
			{
				return detected && strip_remaining > 0;
			}
			void set_detected( const float yaw, const int glide_ticks, const int total_ticks )
			{
				detected         = true;
				is_ladder        = true;
				ladder_yaw       = yaw;
				glide_remaining  = glide_ticks;
				strip_remaining  = total_ticks - glide_ticks;
			}
			void set_undetected( )
			{
				detected        = false;
				is_ladder       = false;
				awall           = false;
				glide_remaining = 0;
				strip_remaining = 0;
			}
			void consume_tick( )
			{
				if ( glide_remaining > 0 )
					glide_remaining--;
				else if ( strip_remaining > 0 )
					strip_remaining--;
			}
		} m_fireman_data;
		struct pixelsurf_assist_t {
			bool in_crosshair = false;
			bool set_point    = false;
			bool pending_announce = false;
			std::string pending_info;
			std::string pending_stance;
		} m_pixelsurf_assist_t;
		bool m_lirili_larila_pending_render = false;
		c_vector m_lirili_larila_pending_origin{ };
		struct headbang_data_t {
			bool m_is_active = false;
			bool m_scanning = false;
			c_vector m_wall_normal{ };
			c_vector m_edge_position{ };
			c_vector m_bounce_position{ };
			float m_ceiling_z = 0.f;
			float m_ground_z = 0.f;
		} m_headbang_data;
		route_studio_t m_route_studio;
		struct ladder_bug_t {
			bool founded = false;
			bool jump_impulse_applied = false;
		} m_ladder_bug_data;
		struct antoha_debug_ray_t {
			c_vector start{ };
			c_vector end{ };
			bool hit = false;
			bool is_primary = false;
			float z_frac = 0.f;
		};
		struct air_stuck_t {
			bool m_air_stuck         = false;
			bool m_wall_stuck        = false;
			bool m_antoha_bug_detect = false;
			bool m_wall_climb_detect = false;
			// bool m_tb_no_cam_detect    = false;
			bool m_antoha_tb_holding = false;    // true while in TB velocity-lock state
			c_vector m_antoha_tb_locked_vel{ };  // XY velocity to restore after prediction
			c_vector m_antoha_tb_wall_normal{ }; // wall normal captured at TB lock moment
			std::vector< antoha_debug_ray_t > m_antoha_debug_rays{ };
			float m_antoha_debug_fwd          = 0.f;
			float m_antoha_debug_side         = 0.f;
			float m_antoha_debug_dist         = 0.f;
			bool m_wall_found                 = false;
			bool m_aligned                    = false;
			bool m_predicted                  = false;
			bool m_used_fallback              = false;
			bool m_duck                       = false;
			bool m_has_pixel_jump_target      = false;
			bool m_point_descend_active       = false;
			float m_align                     = 0.f;
			float m_adjust                    = 0.f;
			float m_best_yaw                  = 0.f;
			float m_best_forward              = 0.f;
			float m_pixel_jump_gap            = 0.f;
			int m_auto_align_block_until_tick = -1;
			c_vector m_wall_normal{ };
			c_vector m_predicted_velocity{ };
			c_vector m_pixel_jump_target{ };

			void reset( )
			{
				m_air_stuck         = false;
				m_wall_stuck        = false;
				m_antoha_bug_detect = false;
				m_wall_climb_detect = false;
				// m_tb_no_cam_detect  = false;
				m_antoha_tb_holding           = false;
				m_antoha_tb_locked_vel        = c_vector{ };
				m_antoha_tb_wall_normal       = c_vector{ };
				m_antoha_debug_fwd            = 0.f;
				m_antoha_debug_side           = 0.f;
				m_antoha_debug_dist           = 0.f;
				m_wall_found                  = false;
				m_aligned                     = false;
				m_predicted                   = false;
				m_used_fallback               = false;
				m_duck                        = false;
				m_has_pixel_jump_target       = false;
				m_point_descend_active        = false;
				m_align                       = 0.f;
				m_adjust                      = 0.f;
				m_best_yaw                    = 0.f;
				m_best_forward                = 0.f;
				m_pixel_jump_gap              = 0.f;
				m_auto_align_block_until_tick = -1;
				m_wall_normal                 = c_vector( );
				m_predicted_velocity          = c_vector( );
				m_pixel_jump_target           = c_vector( );
			}
		} m_air_stuck_data;

		struct edge_jump_data_t {
			bool m_detected = false;
		} m_edge_jump_data;
		struct long_jump_data_t {
			bool m_detected = false;
		} m_long_jump_data;
		struct mini_jump_data_t {
			bool m_detected = false;
		} m_mini_jump_data;
		struct head_ceiling_data_t {
			bool m_detected = false;
			bool m_was_detected_last_tick = false;
		} m_head_ceiling_data;
		struct auto_one_hop_data_t {
			static constexpr int phase_idle = 0;
			static constexpr int phase_climbing = 1;
			static constexpr int phase_jumped = 2;
			static constexpr int phase_strafing = 3;
			int m_phase = phase_idle;
			c_vector m_ladder_normal{};
			c_angle m_saved_view{};
			int m_air_ticks = 0;
			void reset( ) {
				m_phase = phase_idle;
				m_ladder_normal = c_vector( 0, 0, 0 );
				m_saved_view = c_angle( 0, 0, 0 );
				m_air_ticks = 0;
			}
		} m_auto_one_hop_data;
		struct ladder_freelook_climb_data_t {
			static constexpr int phase_idle = 0;
			static constexpr int phase_climbing = 1;
			static constexpr int phase_jumping = 2;
			static constexpr int phase_strafing = 3;
			int m_phase = phase_idle;
			c_vector m_ladder_normal{ };
			float m_server_yaw = 0.0f;
			float m_exit_yaw = 0.0f;
			int m_air_ticks = 0;
			bool m_keep_strafing = false;
			void reset() {
				m_phase = phase_idle;
				m_ladder_normal = c_vector( 0, 0, 0 );
				m_server_yaw = 0.0f;
				m_exit_yaw = 0.0f;
				m_air_ticks = 0;
				m_keep_strafing = false;
			}
		} m_ladder_freelook_climb_data;
		
		struct auto_strafe_onladder_data_t {
			bool m_was_on_ladder = false;
			bool m_active = false;
			float m_start_time = 0.0f;
			const float m_duration = 5.0f;
			void reset() {
				m_was_on_ladder = false;
				m_active = false;
				m_start_time = 0.0f;
			}
		} m_autostrafe_onladder_data;
		
		struct auto_one_hop_onladder_data_t {
			bool m_was_on_ladder = false;
			bool m_active = false;
			float m_start_time = 0.0f;
			const float m_duration = 5.0f;
			float m_current_rotation = 0.0f;
			float m_initial_yaw = 0.0f;
			bool m_is_rotating = false;
			bool m_has_jumped_off = false;
			bool m_strafing = false;
			void reset() {
				m_was_on_ladder = false;
				m_active = false;
				m_start_time = 0.0f;
				m_current_rotation = 0.0f;
				m_initial_yaw = 0.0f;
				m_is_rotating = false;
				m_has_jumped_off = false;
				m_strafing = false;
			}
		} m_auto_one_hop_onladder_data;
		
		struct auto_crouch_data_t {
			float m_ducking_velo = 0.f;
			float m_standing_velo = 0.f;
			float m_ducking_origin = 0.f;
			float m_standing_origin = 0.f;
			bool m_founded = false;
		} m_auto_crouch_data;

		c_vector m_wall_normal  = { };
		float m_wall_dist       = 0.f;
		float m_brush_bottom_z  = -999999.f;
		bool m_wall_found       = false;
		float m_strafe_dir      = 1.f;
		float m_wall_scan_start = 0.f;
		

		struct texture_bug_data_t {
			bool m_hit = false;
			bool m_setup = false;
			bool m_debug_box_valid = false;
			float m_founded_forward = 0.f;
			float m_founded_side = 0.f;
			c_angle m_founded_view{ 0, 0, 0 };
			c_vector m_wall_normal{ };
			c_vector m_debug_box_origin{ };
			c_vector m_debug_box_mins{ };
			c_vector m_debug_box_maxs{ };
			c_angle m_debug_box_angles{ 0.f, 0.f, 0.f };
			float m_brush_height = 0.f;
			float m_v_perp = 0.f;
			float m_ratio = 0.f;
			float m_required_ratio = 0.f;
			void reset() {
				m_hit = false; m_setup = false; m_debug_box_valid = false;
				m_founded_forward = 0.f; m_founded_side = 0.f;
				m_founded_view = c_angle( 0, 0, 0 ); m_wall_normal = c_vector( 0.f, 0.f, 0.f );
				m_debug_box_origin = c_vector( 0.f, 0.f, 0.f ); m_debug_box_mins = c_vector( 0.f, 0.f, 0.f );
				m_debug_box_maxs = c_vector( 0.f, 0.f, 0.f ); m_debug_box_angles = c_angle( 0.f, 0.f, 0.f );
				m_brush_height = 0.f; m_v_perp = 0.f;
				m_ratio = 0.f; m_required_ratio = 0.f;
			}
		} m_texture_bug_data;
		
		struct pixel_calc_t {
			c_vector point_vec{};
			c_vector normal_pos_vec{};
			std::vector< std::string > combo{};
			float display_time = 0.f;
		} m_pixel_calc_data;

		void on_create_move_pre( );
		void on_create_move_post( );
		void on_paint_traverse( );
		void on_frame_stage_notify( int stage );
		void on_end_scene( );
		void rotate_movement( c_user_cmd* cmd, c_angle& angle );
		bool is_head_ceiling_detected( ) const { return m_head_ceiling_data.m_detected; }
		void pixel_finder( c_user_cmd* cmd );
		void pixelsurf_assist_ground_help( c_user_cmd* cmd );
		void auto_crouch( c_user_cmd* cmd );
		void pixelsurf_assist_render( );
		bool can_reach_point( const c_vector& target_pos, float radius, const std::string& jump_type, const c_vector& current_vel );
		float point_to_line_distance( const c_vector_2d& point, const c_vector_2d& line_start, const c_vector_2d& line_end );
		void pixel_calc( c_user_cmd* cmd );
		void pixel_calc_render( );
		bool get_closest_wall_normal( c_vector& out_normal );
		
	private:
		void apply_nulls( );
		void bunny_hop( );
		void edge_jump( );
		void air_stuck( c_user_cmd* cmd );
		void wall_stuck( c_user_cmd* cmd );
		void extended_texture_bug( c_user_cmd* cmd );
		void antoha_bug( c_user_cmd* cmd );
		void antoha_bug_land( c_user_cmd* cmd );
		void wall_climb( c_user_cmd* cmd );
		void air_stuck_serverside( c_user_cmd* cmd );
		void edge_bug( );
		void long_jump( );
		void mini_jump( );
		void jump_bug( );
		void auto_duck( );
		void pixel_surf_locking( float target_ps_velocity );
		void pixel_surf_fix( );
		void pixel_surf( float target_ps_velocity );
		void detect_edgebug( c_user_cmd* cmd );
		void auto_align( c_user_cmd* cmd );
		void AutoStrafe( c_user_cmd* cmd );
		void strafe_to_yaw( c_user_cmd* cmd, c_angle& angle, const float yaw );
		void movement_fix( const c_angle& old_view_point );
		void autobounce_assist( c_user_cmd* cmd );
		void pixelsurf_assist( c_user_cmd* cmd );
		void fire_man( c_user_cmd* cmd );
		void ladder_bug( c_user_cmd* cmd );
		void fast_ladder( c_user_cmd* cmd );
		void ladder_glide( c_user_cmd* cmd );
		void anti_ladder( c_user_cmd* cmd );
		void test( c_user_cmd* cmd );
		void detect_head_ceiling_hit( );
		void auto_one_hop( c_user_cmd* cmd );
		void ladder_freelook_climb( c_user_cmd* cmd );
		void lirili_larila( c_user_cmd* cmd );
		void god_mode( c_user_cmd* cmd );
		void avoid_collision( c_user_cmd* cmd );
		bool is_bind_restricted_in_radius( const points_check_t& point, const c_vector& player_pos, const std::string& bind_type );
	};
}
inline n_movement::impl_t g_movement{ };
extern std::vector< points_check_t > m_bounce_points_check;
extern std::vector< points_check_t > m_points_check;
void movement_add_window( int type, const std::string& info );
void lester_kill_sound_play( );
void lester_kill_sound_stop( );
void lester_kill_begin_notify( float duration );
