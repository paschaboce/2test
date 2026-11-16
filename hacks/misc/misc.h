#pragma once
#include "../../globals/includes/includes.h"
#include <string>
struct IDirect3DTexture9;
inline std::string oldtitle;
inline std::string oldartist;
inline IDirect3DTexture9* albumArtTexture = nullptr;
inline std::string strartist;
inline std::string strtitle;
namespace n_misc
{
	struct spectator_data_t {
		std::string m_text          = { };
		IDirect3DTexture9* m_avatar = { };
		c_color m_color             = { };
	};
	struct impl_t {
		struct practice_t {
			c_angle saved_angles    = { };
			c_vector saved_position = { };
		} practice;
		struct points_system_t {
			int m_current_points = 0;
			int m_previous_points = 0;
			float m_last_detect_time = 0.0f;
			bool m_jb_detected = false;
			bool m_eb_detected = false;
			bool m_ast_detected = false;
			bool m_pixelsurf_detected = false;
			bool m_fm_detected = false;
			bool m_head_ceiling_detected = false;
			float m_pixelsurf_start_time = 0.0f;
			bool m_pixelsurf_active = false;
			float m_last_pixelsurf_bonus_time = 0.0f;
			int m_jb_points_given = 0;
			int m_eb_points_given = 0;
			int m_ast_points_given = 0;
			int m_pixelsurf_points_given = 0;
			int m_fm_points_given = 0;
			int m_head_ceiling_points_given = 0;
			bool m_tb_detected = false;
			int m_tb_points_given = 0;
		} m_points_system;
		struct pts_images_system_t {
			IDirect3DTexture9* m_image1_texture = nullptr;
			IDirect3DTexture9* m_image2_texture = nullptr;
			IDirect3DTexture9* m_image3_texture = nullptr;
			IDirect3DTexture9* m_image4_texture = nullptr;
		} m_pts_images;
		struct spectator_gif_system_t {
			IDirect3DTexture9* m_gif_texture = nullptr;
			std::vector< IDirect3DTexture9* > m_frames;
			std::vector< int > m_frame_delays_ms;
			int m_current_frame = 0;
			float m_accumulated_time = 0.f;
			std::string m_last_loaded_url;
			bool m_loaded = false;
			void update( float dt );
			void cleanup();
		} m_spectator_gif;
	struct outline_sync_t {
		bool m_pts_active = false;
		float m_outline_alpha = 0.0f;
		float m_pts_alpha_with_animation = 0.0f;
	} m_outline_sync;
		struct rgb_state_t {
			int type[4] = { 0, 0, 0, 0 };
			float r[4] = { 255.0f, 0.0f, 0.0f, 0.0f };
			float g[4] = { 0.0f, 255.0f, 0.0f, 0.0f };
			float b[4] = { 0.0f, 0.0f, 255.0f, 0.0f };
		} m_rgb_state;
		struct drag_state_t {
			bool dragging = false;
			bool old_dragging = false;
			float offset_x = 0.0f;
			float offset_y = 0.0f;
		} m_drag_state;
		void on_create_move_pre( );
		void on_end_scene( );
		void on_fire_event( );
		void killfeed_changer( game_event_t* event );
		void vote_revealer( game_event_t* event );
		void on_paint_traverse( );
		void update_clantag( );
		void add_points_on_detect();
		void check_and_add_points_for_detects();
		void reset_detect_states();
		void update_pixelsurf_bonus_points();
		void mouse_fix();
		void camera_fix();
		void stepan_dolbaeb_ne_smog_pts_realize();
		void stepan_dolnaeb_render();
		void render_watermark();
		IDirect3DTexture9* create_texture_from_url(const std::string& url);
		IDirect3DTexture9* create_texture_from_file(const std::string& filepath);
		IDirect3DTexture9* create_texture_from_bytes(const unsigned char* data, size_t data_size);
	private:
		void practice_window_think( );
		void disable_post_processing( );
		void remove_panorama_blur( );
		void draw_spectator_list( );
		void draw_spectating_local( );
	};
}
inline n_misc::impl_t g_misc{ };
