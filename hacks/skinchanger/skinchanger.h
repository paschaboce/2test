#pragma once

#include "../../game/sdk/classes/c_color.h"
#include <map>
#include <string>
#include <vector>

struct skinchanger_item_setting_t {
	char name[ 32 ] = "Default";
	int definition_vector_index = 0;
	int definition_index = 0;
	int paint_kit_vector_index = -1;
	int paint_kit_index = -1;
	int definition_override_vector_index = 0;
	int definition_override_index = 0;
	int seed = 0;
	bool stat_trak = false;
	float wear = FLT_MIN;
	char custom_name[ 32 ] = "";
};

struct skinchanger_stattrack_setting_t {
	int definition_index = 1;
	struct {
		int counter = 0;
	} stattrack_new;
};

struct skinchanger_weapon_info_t {
	constexpr skinchanger_weapon_info_t( const char* model, const char* icon = nullptr ) : model( model ), icon( icon ) { }
	const char* model;
	const char* icon;
};

struct skinchanger_weapon_name_t {
	constexpr skinchanger_weapon_name_t( const int definition_index, const char* name ) : definition_index( definition_index ), name( name ) { }
	int definition_index = 0;
	const char* name = nullptr;
};

struct skinchanger_paint_kit_t {
	int id = -1;
	int rarity = 0;
	std::string name{ };
	std::string name_short{ };
	std::vector< std::string > weapon_names{ };
	bool operator<( const skinchanger_paint_kit_t& other ) const { return name < other.name; }
};

namespace n_skinchanger
{
	struct impl_t {
		void on_attach( );
		void on_frame_stage_notify( int stage );
		void on_shutdown( );
		bool is_enabled( ) const;
		void request_full_update( );
		void render_menu( );
		std::map< int, skinchanger_item_setting_t > m_items{ };
		std::map< int, skinchanger_stattrack_setting_t > m_stattrack_items{ };
		std::map< std::string, std::string > m_icon_overrides{ };
		bool m_enabled = false;
		bool m_show_current_only = true;
		int m_player_model_t = 0;
		int m_player_model_ct = 0;
		bool m_kits_initialized = false;
		int m_selected_definition_index = 0;
		std::vector< skinchanger_paint_kit_t > m_skins{ };
		std::vector< skinchanger_paint_kit_t > m_gloves{ };
	};
}

inline n_skinchanger::impl_t g_skinchanger{ };

extern const std::map< size_t, skinchanger_weapon_info_t > g_skinchanger_weapon_info;
extern const std::vector< skinchanger_weapon_name_t > g_skinchanger_knife_names;
extern const std::vector< skinchanger_weapon_name_t > g_skinchanger_glove_names;
extern const std::vector< skinchanger_weapon_name_t > g_skinchanger_weapon_names;
extern const std::vector< skinchanger_weapon_name_t > g_skinchanger_knife_names_preview;
extern const std::vector< skinchanger_weapon_name_t > g_skinchanger_glove_names_preview;
extern const std::vector< skinchanger_weapon_name_t > g_skinchanger_weapon_names_preview;
