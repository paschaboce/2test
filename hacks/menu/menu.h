#pragma once
#include "../../dependencies/imgui/imgui.h"
#include <array>
#include <string>
#include <vector>

struct c_base_entity;

constexpr const char* base_material_names[ 2 ]    = { "flat", "textured" };
constexpr const char* overlay_material_names[ 1 ] = { "snow" };
namespace n_menu
{
	struct server_player_settings_t {
		int m_entity_index = 0;
		bool m_auto_respawn = false;
		int m_team_action = 0;
		int m_last_applied_team_action = 0;
		bool m_target_switch = false;
		bool m_whitelist = false;
		bool m_whitelist_visuals = false;
		bool m_whitelist_aimbot = false;
		std::string m_custom_name{ };
	};

	struct impl_t {
		void on_end_scene( );
		void render_point_settings_popup( );
		void on_create_move( );
		server_player_settings_t* find_server_player_settings( int entity_index, bool create = false );
		std::string get_server_player_name( int entity_index, const char* fallback = "" ) const;
		void set_server_player_alias( const char* steam_id, const std::string& alias );
		bool is_server_player_whitelisted( int entity_index ) const;
		bool is_server_player_visuals_whitelisted( int entity_index ) const;
		bool is_server_player_aimbot_whitelisted( int entity_index ) const;
		bool is_server_player_marked( int entity_index ) const;
		void mark_server_player( int entity_index );
		bool is_same_server_player( c_base_entity* entity, int entity_index ) const;
		bool m_opened              = false;
		bool m_is_opening          = false;
		float m_animation_speed    = 15.f;
		bool connect_to_server     = false;
		bool connect_to_direct     = false;
		int m_selected_config    = 0;
		char m_config_file[ 32 ] = { };
		float m_animation_progress = 0.f;
		int m_selected_server_player = -1;
		std::array< char, 128 > m_server_custom_name_buffer{ };
		std::array< char, 128 > m_server_alias_buffer{ };
		std::vector< server_player_settings_t > m_server_players{ };
		std::vector< int > m_server_marked_players{ };
		std::vector< int > m_server_reported_airstuck_players{ };
		std::vector< int > m_server_active_noclip_players{ };
	};
}
inline n_menu::impl_t g_menu{ };
