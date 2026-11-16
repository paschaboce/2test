#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.h"
#include "../../globals/fonts/fonts.h"
#include "../../instalisation fonts/fonts2.h"
#include "../../globals/includes/includes.h"
#include "../../globals/logger/logger.h"
#include "../misc/scaleform/scaleform.h"
#include "../movement/movement.h"
#include "../../globals/config/variables.h"
#include "../../dependencies/imgui/imgui.h"
#include "../elements/tabs.hh"
bool is_dev_hwid_allowed( );
const std::string& get_cached_hwid( );
#include "../elements/childbox.hh"
#include "../elements/checkbox.hh"
#include "../popup_elements/popup_elements.h"
#include "../elements/button.hh"
#include "../../elements/smbutton.hh"
#include "../elements/sliders.hh"
#include "../elements/spinner.hh"
#include "../elements/combobox.hh"
#include "../elements/i_text.hh"
#include "../elements/color_picker.hh"
#include "../elements/text_with_color_picker.hh"
#include "../aimbot/aimbot.h"
#include "../inventory/inventory_changer.h"
#include "../knife_changer/knife_changer.h"
#include "../skinchanger/skinchanger.h"
#include "../movement_recorder/movement_recorder.h"
#include <cmath>
#include <algorithm>

constexpr int color_picker_alpha_flags = ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf |
                                         ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_NoInputs |
                                         ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar |
                                         ImGuiColorEditFlags_NoBorder;
constexpr int color_picker_no_alpha_flags = ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_InputRGB |
                                            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop |
                                            ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoBorder;
constexpr static const auto chams_materials = "flat\0textured\0metallic\0glow";
static int& menu_dpi_percent( )
{
	auto& dpi = GET_VARIABLE( g_variables.m_menu_dpi_scale, int );
	dpi = std::clamp( dpi, 75, 200 );
	return dpi;
}
static float menu_dpi_scale_value( )
{
	return static_cast< float >( menu_dpi_percent( ) ) / 100.0f;
}
bool mini_button( const char* name )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return false;
	const auto name_text_size = g_render.m_custom_fonts[e_custom_font_names::custom_font_name_googlesans_addwindow]->CalcTextSizeA( g_render.m_custom_fonts[e_custom_font_names::custom_font_name_googlesans_addwindow]->FontSize, FLT_MAX, NULL, name );
	const float width            = 140.0f;
	const float height           = 20.0f;
	const float rounding         = 4.0f;
	const ImVec2 position = ImGui::GetCurrentWindow( )->DC.CursorPos;
	ImRect total_bb( position, position + ImVec2( width, height ) );
	ImGui::ItemSize( total_bb, height );
	if ( !ImGui::ItemAdd( total_bb, ImGui::GetID( name ) ) )
		return false;
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( total_bb, ImGui::GetID( name ), &hovered, &held );
	float dt = ImGui::GetIO( ).DeltaTime;
	static std::unordered_map< ImGuiID, float > pressAnim;
	static std::unordered_map< ImGuiID, float > hoverAnim;
	ImGuiID id = ImGui::GetID( name );
	if ( pressAnim.find( id ) == pressAnim.end( ) )
		pressAnim[ id ] = 0.0f;
	if ( hoverAnim.find( id ) == hoverAnim.end( ) )
		hoverAnim[ id ] = 0.0f;
	float pressTarget = held ? 1.0f : 0.0f;
	float hoverTarget = hovered ? 1.0f : 0.0f;
	pressAnim[ id ] = smoothLerp( pressAnim[ id ], pressTarget, dt * 12.0f );
	hoverAnim[ id ] = smoothLerp( hoverAnim[ id ], hoverTarget, dt * 8.0f );
	ImColor baseColor( 16, 16, 16, 255 );
	ImColor accentColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 255 ) );
	ImColor hoverColor( 25, 25, 25, 255 );
	ImColor finalColor = smoothLerpColor( 
		smoothLerpColor( baseColor, hoverColor, hoverAnim[ id ] ),
		accentColor, 
		pressAnim[ id ] 
	);
	ImGui::GetWindowDrawList( )->AddRectFilled( total_bb.Min, total_bb.Max, finalColor, rounding, ImDrawFlags_RoundCornersAll );
	ImColor baseOutlineColor( 33, 33, 33, 255 );
	ImColor accentOutlineColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 255 ) );
	ImColor hoverOutlineColor( 50, 50, 50, 255 );
	ImColor finalOutlineColor = smoothLerpColor( 
		smoothLerpColor( baseOutlineColor, hoverOutlineColor, hoverAnim[ id ] ),
		accentOutlineColor, 
		pressAnim[ id ] 
	);
	float outlineThickness = 1.0f + hoverAnim[ id ] * 0.5f;
	ImGui::GetWindowDrawList( )->AddRect( total_bb.Min, total_bb.Max, finalOutlineColor, rounding, ImDrawFlags_RoundCornersAll, outlineThickness );
	const float text_x = total_bb.Min.x + ( width - name_text_size.x ) * 0.5f;
	const float text_y = total_bb.Min.y + ( height - name_text_size.y ) * 0.5f - 1.0f;
	ImColor baseTextColor( 150, 150, 150, 255 );
	ImColor accentTextColor( 255, 255, 255, 255 );
	ImColor hoverTextColor( 255, 255, 255, 255 );
	ImColor finalTextColor = smoothLerpColor( 
		smoothLerpColor( baseTextColor, hoverTextColor, hoverAnim[ id ] ),
		accentTextColor, 
		pressAnim[ id ] 
	);
	ImGui::GetWindowDrawList( )->AddText( g_render.m_custom_fonts[e_custom_font_names::custom_font_name_googlesans_addwindow], g_render.m_custom_fonts[e_custom_font_names::custom_font_name_googlesans_addwindow]->FontSize, ImVec2( text_x, text_y ),
	                                      finalTextColor, name );
	return pressed;
}
bool convar_remove_button( const char* id )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return false;

	auto* font = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ];
	if ( !font )
		return ImGui::Button( id, ImVec2( 16.0f, 16.0f ) );

	const float width = 16.0f;
	const float height = 16.0f;
	const float rounding = 3.0f;
	const ImVec2 position = ImGui::GetCurrentWindow( )->DC.CursorPos;
	ImRect total_bb( position, position + ImVec2( width, height ) );

	ImGui::ItemSize( total_bb, height );
	if ( !ImGui::ItemAdd( total_bb, ImGui::GetID( id ) ) )
		return false;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( total_bb, ImGui::GetID( id ), &hovered, &held );

	ImColor base_color( 110, 22, 22, 255 );
	ImColor hover_color( 155, 30, 30, 255 );
	ImColor held_color( 185, 40, 40, 255 );
	ImColor final_color = held ? held_color : ( hovered ? hover_color : base_color );

	ImGui::GetWindowDrawList( )->AddRectFilled( total_bb.Min, total_bb.Max, final_color, rounding, ImDrawFlags_RoundCornersAll );
	ImGui::GetWindowDrawList( )->AddRect( total_bb.Min, total_bb.Max, ImColor( 255, 255, 255, 45 ), rounding, ImDrawFlags_RoundCornersAll, 1.0f );

	const char* text = "x";
	const ImVec2 text_size = font->CalcTextSizeA( font->FontSize, FLT_MAX, 0.0f, text );
	const ImVec2 text_pos( total_bb.Min.x + ( width - text_size.x ) * 0.5f, total_bb.Min.y + ( height - text_size.y ) * 0.5f - 0.5f );
	ImGui::GetWindowDrawList( )->AddText( font, font->FontSize, text_pos, ImColor( 255, 255, 255, 255 ), text );

	return pressed;
}

namespace {
	const char* get_server_team_command_suffix( int team_action )
	{
		switch ( team_action ) {
		case 1:
			return "ct";
		case 2:
			return "t";
		case 3:
			return "spec";
		default:
			return nullptr;
		}
	}

	std::string make_server_player_command( const char* command_name, const std::string& player_name, const char* extra = nullptr )
	{
		if ( player_name.empty( ) )
			return { };

		std::string quoted_name = "\"" + player_name + "\"";
		std::string command = std::string( command_name ) + " " + quoted_name;
		if ( extra && *extra ) {
			command += " ";
			command += extra;
		}
		return command;
	}

	std::string normalize_server_steam_id( const char* steam_id )
	{
		if ( !steam_id )
			return { };
		std::string normalized = steam_id;
		if ( normalized.rfind( "STEAM_0:", 0 ) == 0 || normalized.rfind( "STEAM_1:", 0 ) == 0 )
			normalized[ 6 ] = 'X';
		return normalized;
	}

	bool has_wall_trace_near_entity( c_base_entity* entity )
	{
		if ( !entity )
			return false;
		auto colidable = entity->get_collideable( );
		if ( !colidable )
			return false;

		trace_t trace;
		constexpr float pi = 3.14159265358979323846f;
		const float step = pi * 2.0f / 16.0f;
		for ( float a = 0.0f; a < pi * 2.0f; a += step ) {
			c_vector wishdir( cosf( a ), sinf( a ), 0.0f );
			const auto start_pos = entity->get_abs_origin( );
			const auto end_pos = start_pos + wishdir;
			c_trace_filter_world flt;
			ray_t ray( start_pos, end_pos, colidable->get_obb_mins( ), colidable->get_obb_maxs( ) );
			g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
			if ( trace.m_fraction < 1.0f )
				return true;
		}
		return false;
	}
}

n_menu::server_player_settings_t* n_menu::impl_t::find_server_player_settings( int entity_index, bool create )
{
	for ( auto& player : m_server_players ) {
		if ( player.m_entity_index == entity_index )
			return &player;
	}

	if ( !create )
		return nullptr;

	m_server_players.push_back( { } );
	auto& player = m_server_players.back( );
	player.m_entity_index = entity_index;
	player.m_last_applied_team_action = 0;
	return &player;
}

std::string n_menu::impl_t::get_server_player_name( int entity_index, const char* fallback ) const
{
	for ( const auto& player : m_server_players ) {
		if ( player.m_entity_index == entity_index && !player.m_custom_name.empty( ) )
			return player.m_custom_name;
	}

	return fallback ? std::string( fallback ) : std::string( );
}

void n_menu::impl_t::set_server_player_alias( const char* steam_id, const std::string& alias )
{
	if ( !steam_id || !*steam_id )
		return;

	auto& alias_ids = GET_VARIABLE( g_variables.m_server_player_alias_steam_ids, std::vector< std::string > );
	auto& alias_names = GET_VARIABLE( g_variables.m_server_player_alias_names, std::vector< std::string > );
	for ( size_t i = 0; i < alias_ids.size( ) && i < alias_names.size( ); ++i ) {
		if ( alias_ids[ i ] == steam_id ) {
			if ( alias.empty( ) ) {
				alias_ids.erase( alias_ids.begin( ) + i );
				alias_names.erase( alias_names.begin( ) + i );
			} else {
				alias_names[ i ] = alias;
			}
			return;
		}
	}

	if ( !alias.empty( ) ) {
		alias_ids.push_back( steam_id );
		alias_names.push_back( alias );
	}
}

bool n_menu::impl_t::is_server_player_whitelisted( int entity_index ) const
{
	for ( const auto& player : m_server_players ) {
		if ( player.m_entity_index == entity_index )
			return player.m_whitelist;
	}

	return false;
}

bool n_menu::impl_t::is_server_player_visuals_whitelisted( int entity_index ) const
{
	for ( const auto& player : m_server_players ) {
		if ( player.m_entity_index == entity_index )
			return player.m_whitelist && !player.m_whitelist_visuals;
	}

	return false;
}

bool n_menu::impl_t::is_server_player_aimbot_whitelisted( int entity_index ) const
{
	for ( const auto& player : m_server_players ) {
		if ( player.m_entity_index == entity_index )
			return player.m_whitelist && !player.m_whitelist_aimbot;
	}

	return false;
}

bool n_menu::impl_t::is_server_player_marked( int entity_index ) const
{
	return std::find( m_server_marked_players.begin( ), m_server_marked_players.end( ), entity_index ) != m_server_marked_players.end( );
}

void n_menu::impl_t::mark_server_player( int entity_index )
{
	if ( entity_index <= 0 )
		return;
	if ( !is_server_player_marked( entity_index ) )
		m_server_marked_players.push_back( entity_index );
}

bool n_menu::impl_t::is_same_server_player( c_base_entity* entity, int entity_index ) const
{
	return entity && entity->get_index( ) == entity_index;
}

void n_menu::impl_t::on_create_move( )
{
	if ( !g_interfaces.m_engine_client || !g_interfaces.m_engine_client->is_connected_safe( ) )
		return;

	if ( m_server_players.empty( ) ) {
		const auto& saved_names = GET_VARIABLE( g_variables.m_server_player_custom_names, std::vector< std::string > );
		const auto& saved_name_indexes = GET_VARIABLE( g_variables.m_server_player_custom_name_indexes, std::vector< int > );
		const auto& whitelist_indexes = GET_VARIABLE( g_variables.m_server_player_whitelist_indexes, std::vector< int > );
		const auto& visuals_whitelist_indexes = GET_VARIABLE( g_variables.m_server_player_visuals_whitelist_indexes, std::vector< int > );
		const auto& aimbot_whitelist_indexes = GET_VARIABLE( g_variables.m_server_player_aimbot_whitelist_indexes, std::vector< int > );

		for ( size_t i = 0; i < saved_name_indexes.size( ) && i < saved_names.size( ); ++i ) {
			auto* player = find_server_player_settings( saved_name_indexes[ i ], true );
			if ( player )
				player->m_custom_name = saved_names[ i ];
		}

		for ( int index : whitelist_indexes ) {
			auto* player = find_server_player_settings( index, true );
			if ( player )
				player->m_whitelist = true;
		}

		for ( int index : visuals_whitelist_indexes ) {
			auto* player = find_server_player_settings( index, true );
			if ( player ) {
				player->m_whitelist = true;
				player->m_whitelist_visuals = true;
			}
		}

		for ( int index : aimbot_whitelist_indexes ) {
			auto* player = find_server_player_settings( index, true );
			if ( player ) {
				player->m_whitelist = true;
				player->m_whitelist_aimbot = true;
			}
		}
	}

	for ( auto& player : m_server_players ) {
		if ( player.m_entity_index <= 0 )
			continue;

		auto entity = g_interfaces.m_client_entity_list->get< c_base_entity >( player.m_entity_index );
		if ( !entity )
			continue;

		player_info_t info{ };
		if ( !g_interfaces.m_engine_client->get_player_info( player.m_entity_index, &info ) )
			continue;

		const std::string actual_name = info.m_name;
		if ( player.m_target_switch ) {
			if ( entity->get_team( ) != e_team_id::team_spectator ) {
				std::string command = make_server_player_command( "sm_team", actual_name, "spec" );
				if ( !command.empty( ) )
					g_interfaces.m_engine_client->execute_client_cmd( command.c_str( ) );
			}
		}

		const char* team_suffix = get_server_team_command_suffix( player.m_team_action );
		if ( team_suffix && player.m_team_action != player.m_last_applied_team_action ) {
			std::string command = make_server_player_command( "sm_team", actual_name, team_suffix );
			if ( !command.empty( ) )
				g_interfaces.m_engine_client->execute_client_cmd( command.c_str( ) );
			player.m_last_applied_team_action = player.m_team_action;
		} else if ( !team_suffix ) {
			player.m_last_applied_team_action = 0;
		}
	}

	if ( GET_VARIABLE( g_variables.m_server_detect_illegal_airstuck, bool ) ) {
		const int max_clients = g_interfaces.m_engine_client->get_max_clients( );
		for ( int i = 1; i <= max_clients; ++i ) {
			auto entity = g_interfaces.m_client_entity_list->get< c_base_entity >( i );
			if ( !entity )
				continue;

			player_info_t info{ };
			if ( !g_interfaces.m_engine_client->get_player_info( i, &info ) )
				continue;

			const bool in_noclip = entity->get_move_type( ) == e_move_types::move_type_noclip;
			const bool illegal = in_noclip && has_wall_trace_near_entity( entity );
			auto it = std::find( m_server_reported_airstuck_players.begin( ), m_server_reported_airstuck_players.end( ), i );
			auto noclip_it = std::find( m_server_active_noclip_players.begin( ), m_server_active_noclip_players.end( ), i );

			if ( in_noclip ) {
				if ( noclip_it == m_server_active_noclip_players.end( ) ) {
					m_server_active_noclip_players.push_back( i );
					if ( it != m_server_reported_airstuck_players.end( ) ) {
						m_server_reported_airstuck_players.erase( it );
						it = m_server_reported_airstuck_players.end( );
					}
				}
			} else {
				if ( noclip_it != m_server_active_noclip_players.end( ) )
					m_server_active_noclip_players.erase( noclip_it );
				if ( it != m_server_reported_airstuck_players.end( ) )
					m_server_reported_airstuck_players.erase( it );
				continue;
			}

			if ( illegal ) {
				if ( it == m_server_reported_airstuck_players.end( ) ) {
					std::string alias = "";
					const std::string shown_name = alias.empty( ) ? std::string( info.m_name ) : alias;
					movement_add_window( 5, std::string( "[ac] Detected illegal airstuck: " ) + shown_name + " (" + info.m_steam_id + ")" );
					m_server_reported_airstuck_players.push_back( i );
				}
			}
		}
	} else {
		m_server_reported_airstuck_players.clear( );
		m_server_active_noclip_players.clear( );
	}

	auto& saved_names = GET_VARIABLE( g_variables.m_server_player_custom_names, std::vector< std::string > );
	auto& saved_name_indexes = GET_VARIABLE( g_variables.m_server_player_custom_name_indexes, std::vector< int > );
	auto& whitelist_indexes = GET_VARIABLE( g_variables.m_server_player_whitelist_indexes, std::vector< int > );
	auto& visuals_whitelist_indexes = GET_VARIABLE( g_variables.m_server_player_visuals_whitelist_indexes, std::vector< int > );
	auto& aimbot_whitelist_indexes = GET_VARIABLE( g_variables.m_server_player_aimbot_whitelist_indexes, std::vector< int > );
	saved_names.clear( );
	saved_name_indexes.clear( );
	whitelist_indexes.clear( );
	visuals_whitelist_indexes.clear( );
	aimbot_whitelist_indexes.clear( );

	for ( const auto& player : m_server_players ) {
		if ( player.m_entity_index <= 0 )
			continue;
		if ( !player.m_custom_name.empty( ) ) {
			saved_name_indexes.push_back( player.m_entity_index );
			saved_names.push_back( player.m_custom_name );
		}
		if ( player.m_whitelist )
			whitelist_indexes.push_back( player.m_entity_index );
		if ( player.m_whitelist_visuals )
			visuals_whitelist_indexes.push_back( player.m_entity_index );
		if ( player.m_whitelist_aimbot )
			aimbot_whitelist_indexes.push_back( player.m_entity_index );
	}

}
void content_subtabs( const std::vector< std::pair< const char*, int > >& tabs, int& selected_tab, int& local_subtab_state )
{
	if ( tabs.empty( ) )
		return;

	auto* font = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ];
	if ( !font )
		return;

	ImDrawList* draw = ImGui::GetWindowDrawList( );
	const ImVec2 base_pos = ImGui::GetWindowPos( ) + ImVec2( 220.0f, 10.0f );
	const float height = 22.0f;
	const float rounding = 4.0f;
	float cursor_x = base_pos.x;

	for ( size_t i = 0; i < tabs.size( ); ++i ) {
		const auto& tab = tabs[ i ];
		const ImVec2 text_size = font->CalcTextSizeA( font->FontSize, FLT_MAX, 0.0f, tab.first );
		const float width = text_size.x + 22.0f;
		const ImVec2 tab_min( cursor_x, base_pos.y );
		const ImVec2 tab_max( cursor_x + width, base_pos.y + height );
		const bool hovered = ImGui::IsMouseHoveringRect( tab_min, tab_max );
		const bool selected = selected_tab == tab.second;

		ImColor fill = selected ? ImColor( 23, 23, 23, 255 ) : ( hovered ? ImColor( 18, 18, 18, 255 ) : ImColor( 14, 14, 14, 220 ) );
		ImColor outline = selected ? ImColor( 255, 255, 255, 28 ) : ImColor( 33, 33, 33, 255 );
		ImColor text = selected ? ImColor( 245, 245, 245, 255 ) : ( hovered ? ImColor( 205, 205, 205, 255 ) : ImColor( 150, 150, 150, 255 ) );

		draw->AddRectFilled( tab_min, tab_max, fill, rounding, ImDrawFlags_RoundCornersAll );
		draw->AddRect( tab_min, tab_max, outline, rounding, ImDrawFlags_RoundCornersAll, 1.0f );
		draw->AddText( font, font->FontSize, ImVec2( tab_min.x + ( width - text_size.x ) * 0.5f, tab_min.y + ( height - text_size.y ) * 0.5f - 1.0f ), text, tab.first );

		if ( hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
			selected_tab = tab.second;
			local_subtab_state = static_cast< int >( i );
		}

		cursor_x += width + 8.0f;
	}
}
void config_popup( const char* str_id, const std::function< void( ) >& fn )
{
	ImGui::OpenPopup( str_id );
	ImGui::SetNextWindowSize( ImVec2( 150.f, 55.f ) );
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
	                         ImGuiWindowFlags_NoScrollWithMouse;
	if ( ImGui::BeginPopup( str_id, flags ) ) {
		const auto draw_list = ImGui::GetWindowDrawList( );
		const ImVec2 position = ImGui::GetWindowPos( ), size = ImGui::GetWindowSize( );
		ImGui::PushClipRect( position, ImVec2( position.x + size.x, position.y + size.y ), false );
		draw_list->AddRectFilled( position, ImVec2( position.x + size.x, position.y + size.y ), ImColor( 13, 13, 13, 255 ), 6.0f );
		draw_list->AddRect( position, ImVec2( position.x + size.x, position.y + size.y ), ImColor( 33, 33, 33, 255 ), 6.0f, 0, 1.0f );
		ImGui::PopClipRect( );
		ImGui::SetCursorPos( ImVec2( 5.f, 5.f ) );
		ImGui::SetCursorPosX( 5.f );
		fn( );
		ImGui::EndPopup( );
	}
}
void save_popup( const char* str_id, const ImVec2& window_size, const std::function< void( ) >& fn )
{
	ImGuiContext& g         = *GImGui;
	const ImGuiStyle& style = g.Style;
	const auto window = g.CurrentWindow;
	const auto hashed_str_id = ImHashStr( str_id );
	const auto text_size     = g_render.m_custom_fonts[e_custom_font_names::custom_font_name_googlesans_addwindow]->CalcTextSizeA(
        g_render.m_custom_fonts[e_custom_font_names::custom_font_name_googlesans_addwindow]->FontSize, FLT_MAX, 0.f, str_id );
	const ImColor accent_color = ImGui::GetColorU32( ImGuiCol_::ImGuiCol_Accent );
	ImGui::OpenPopup( str_id );
	ImGui::SetNextWindowSize( window_size );
	if ( ImGui::BeginPopup( str_id ) ) {
		const auto draw_list = ImGui::GetWindowDrawList( );
		const ImVec2 position = ImGui::GetWindowPos( ), size = ImGui::GetWindowSize( );
		auto text_animation = ImAnimationHelper( hashed_str_id, ImGui::GetIO( ).DeltaTime );
		draw_list->AddRectFilled( ImVec2( position ), ImVec2( position.x + size.x, position.y + 20.f ), ImColor( 25 / 255.f, 25 / 255.f, 25 / 255.f ),
		                          g.Style.WindowRounding - 2.f, ImDrawFlags_RoundCornersTop );
		ImGui::PushClipRect( position, ImVec2( position.x + size.x, position.y + size.y ), false );
		draw_list->AddRect( position, ImVec2( position.x + size.x, position.y + size.y ), ImColor( 50, 50, 50, 100 ), g.Style.WindowRounding - 2.f );
		text_animation.Update( ImGui::IsMouseHoveringRect( position, ImVec2( position.x + size.x, position.y + size.y ) ) ? 3.f : -2.f, 1.f, 0.5f,
		                       1.f );
		ImGui::PopClipRect( );
		const ImColor text_color = ImGui::GetColorU32( ImGuiCol_Text );
		draw_list->AddText(
			g_render.m_custom_fonts[e_custom_font_names::custom_font_name_googlesans_addwindow], g_render.m_custom_fonts[e_custom_font_names::custom_font_name_googlesans_addwindow]->FontSize,
			ImVec2( position.x + ( size.x - text_size.x ) / 2.f, position.y + ( 20.f - text_size.y ) / 2.f ),
			ImColor( text_color.Value.x, text_color.Value.y, text_color.Value.z, text_color.Value.w * text_animation.AnimationData->second ),
			str_id );
		RenderFadedGradientLine( draw_list, ImVec2( position.x, position.y + 20.f ), ImVec2( size.x, 1.f ),
		                         ImColor( accent_color.Value.x, accent_color.Value.y, accent_color.Value.z ) );
		ImGui::SetCursorPosY( ImGui::GetCursorPosY( ) + 20.f );
		fn( );
		ImGui::EndPopup( );
	}
}
void custom_point_popup( const char* str_id, const ImVec2& window_size, const std::function< void( ) >& fn )
{
	ImGui::OpenPopup( str_id );
	ImGui::SetNextWindowSize( window_size );
	if ( ImGui::BeginPopup( str_id, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) ) {
		const auto draw_list = ImGui::GetWindowDrawList( );
		const ImVec2 position = ImGui::GetWindowPos( );
		const ImVec2 size = ImGui::GetWindowSize( );
		draw_list->AddRectFilled( position, position + size, ImColor( 13, 13, 13, 255 ), 8.0f, ImDrawFlags_RoundCornersAll );
		draw_list->AddRect( position, position + size, ImColor( 33, 33, 33, 255 ), 8.0f, ImDrawFlags_RoundCornersAll, 1.0f );
		ImGui::BeginGroup( );
		fn( );
		ImGui::EndGroup( );
		ImGui::EndPopup( );
	}
}
extern void loadConfig( const std::string& filename );
extern void saveConfig( const std::string& filename );
extern bool isEnterPressed( );
constexpr std::array skyboxList{ "Default",
	                             "cs_baggage_skybox_",
	                             "embassy",
	                             "italy",
	                             "nukeblank",
	                             "office",
	                             "sky_cs15_daylight01_hdr",
	                             "sky_cs15_daylight02_hdr",
	                             "sky_cs15_daylight03_hdr",
	                             "sky_cs15_daylight04_hdr",
	                             "sky_csgo_cloudy01",
	                             "sky_csgo_night_flat",
	                             "sky_csgo_night02",
	                             "sky_day02_05_hdr",
	                             "sky_day02_05",
	                             "sky_dust",
	                             "sky_l4d_rural02_ldr",
	                             "sky_venice",
	                             "vertigo",
	                             "vertigoblue_hdr",
	                             "vietnam",
	                             "sky_lunacy",
	                             "sky_hr_aztec",
	                             "Custom" };
extern bool point_menu_is_opened( );
ImColor RarityGetter( int rarity )
{
	if ( rarity == 1 )
		return ImColor( 0.69f, 0.76f, 0.85f, 1.0f );
	else if ( rarity == 2 )
		return ImColor( 0.29f, 0.41f, 1.f, 1.0f );
	else if ( rarity == 3 )
		return ImColor( 0.36f, 0.60f, 0.85f, 1.0f );
	else if ( rarity == 4 )
		return ImColor( 0.53f, 0.27f, 1.f, 1.0f );
	else if ( rarity == 5 )
		return ImColor( 0.82f, 0.17f, 0.90f, 1.0f );
	else if ( rarity == 6 )
		return ImColor( 0.92f, 0.29f, 0.29f, 1.0f );
	else if ( rarity == 7 )
		return ImColor( 0.89f, 0.68f, 0.22f, 1.0f );
}
std::string ToUtf888( std::wstring_view wstr )
{
	if ( wstr.empty( ) )
		return std::string( );
	auto size_needed = WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], static_cast< int32_t >( wstr.size( ) ), NULL, 0, NULL, NULL );
	std::string out( size_needed, 0 );
	WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], static_cast< int >( wstr.size( ) ), &out[ 0 ], size_needed, NULL, NULL );
	return out;
}
std::string LocalizeTex22( const char* in )
{
	const auto wide_name = g_interfaces.m_localize->find_safe( in );
	if ( !wcslen( wide_name ) )
		return "";
	return ToUtf888( wide_name );
}
std::string ToUtf88( std::wstring_view wstr )
{
	if ( wstr.empty( ) )
		return std::string( );
	auto size_needed = WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], static_cast< int32_t >( wstr.size( ) ), NULL, 0, NULL, NULL );
	std::string out( size_needed, 0 );
	WideCharToMultiByte( CP_UTF8, 0, &wstr[ 0 ], static_cast< int >( wstr.size( ) ), &out[ 0 ], size_needed, NULL, NULL );
	return out;
}
std::string LocalizeTex2( const char* in )
{
	const auto wide_name = g_interfaces.m_localize->find_safe( in );
	if ( !wcslen( wide_name ) )
		return "";
	return ToUtf88( wide_name );
}
void n_menu::impl_t::on_end_scene( )
{
	ImGui::GetIO( ).FontGlobalScale = 1.0f;
	render_point_settings_popup( );
	const float delta_time       = ImGui::GetIO( ).DeltaTime;
	const float animation_target = m_opened ? 1.0f : 0.0f;
	m_animation_progress         = std::clamp(m_animation_progress + ( animation_target - m_animation_progress ) * std::min( delta_time * m_animation_speed, 1.0f ), 0.0f, 1.0f );
	m_is_opening = m_animation_progress > 0.0f && m_animation_progress < 1.0f;
	if ( point_menu_is_opened( ) ) { // ???
		return;
	}
	if ( !m_opened && m_animation_progress < 0.05f ) {
		return;
	}
	{
		auto& style                                      = ImGui::GetStyle( );
		static ImGuiStyle base_style                     = style;
		style                                            = base_style;
		const float menu_dpi_scale                       = menu_dpi_scale_value( );
		style.Colors[ ImGuiCol_::ImGuiCol_WindowBg ]     = ImVec4( 16 / 255.f, 16 / 255.f, 16 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_::ImGuiCol_ChildBg ]      = ImVec4( 16 / 255.f, 16 / 255.f, 16 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_::ImGuiCol_FrameBg ]      = ImVec4( 13 / 255.f, 13 / 255.f, 13 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_::ImGuiCol_PopupBg ]      = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_::ImGuiCol_CheckMark ]    = ImVec4( 0 / 255.f, 0 / 255.f, 0 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_::ImGuiCol_Button ]       = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_::ImGuiCol_Border ]       = ImVec4( 13 / 255.f, 13 / 255.f, 13 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_::ImGuiCol_BorderShadow ] = ImVec4( 0 / 255.f, 0 / 255.f, 0 / 255.f, 0.f );
		style.FrameBorderSize                            = 1.0f;
		style.Colors[ ImGuiCol_Text ]                    = ImVec4( 230 / 255.f, 230 / 255.f, 230 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TextDisabled ]            = ImVec4( 128 / 255.f, 128 / 255.f, 128 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_FrameBgHovered ]          = ImVec4( 30 / 255.f, 30 / 255.f, 30 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_FrameBgActive ]           = ImVec4( 35 / 255.f, 35 / 255.f, 35 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TitleBg ]                 = ImVec4( 15 / 255.f, 15 / 255.f, 15 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TitleBgActive ]           = ImVec4( 15 / 255.f, 15 / 255.f, 15 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TitleBgCollapsed ]        = ImVec4( 15 / 255.f, 15 / 255.f, 15 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_MenuBarBg ]               = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_ScrollbarBg ]             = ImVec4( 15 / 255.f, 15 / 255.f, 15 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_ScrollbarGrab ]           = ImVec4( 60 / 255.f, 60 / 255.f, 60 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_ScrollbarGrabHovered ]    = ImVec4( 80 / 255.f, 80 / 255.f, 80 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_ScrollbarGrabActive ]     = ImVec4( 100 / 255.f, 100 / 255.f, 100 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_SliderGrab ]              = ImVec4( 80 / 255.f, 80 / 255.f, 80 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_SliderGrabActive ]        = ImVec4( 100 / 255.f, 100 / 255.f, 100 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_ButtonHovered ]           = ImVec4( 25 / 255.f, 25 / 255.f, 25 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_ButtonActive ]            = ImVec4( 30 / 255.f, 30 / 255.f, 30 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_Header ]                  = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_HeaderHovered ]           = ImVec4( 50 / 255.f, 50 / 255.f, 50 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_HeaderActive ]            = ImVec4( 45 / 255.f, 45 / 255.f, 45 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_Separator ]               = ImVec4( 0 / 255.f, 0 / 255.f, 0 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_SeparatorHovered ]        = ImVec4( 60 / 255.f, 60 / 255.f, 60 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_SeparatorActive ]         = ImVec4( 80 / 255.f, 80 / 255.f, 80 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_ResizeGrip ]              = ImVec4( 80 / 255.f, 80 / 255.f, 80 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_ResizeGripHovered ]       = ImVec4( 100 / 255.f, 100 / 255.f, 100 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_ResizeGripActive ]        = ImVec4( 120 / 255.f, 120 / 255.f, 120 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_Tab ]                     = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TabHovered ]              = ImVec4( 40 / 255.f, 40 / 255.f, 40 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TabActive ]               = ImVec4( 25 / 255.f, 25 / 255.f, 25 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TabUnfocused ]            = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TabUnfocusedActive ]      = ImVec4( 25 / 255.f, 25 / 255.f, 25 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_PlotLines ]               = ImVec4( 100 / 255.f, 100 / 255.f, 100 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_PlotLinesHovered ]        = ImVec4( 120 / 255.f, 120 / 255.f, 120 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_PlotHistogram ]           = ImVec4( 150 / 255.f, 150 / 255.f, 150 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_PlotHistogramHovered ]    = ImVec4( 170 / 255.f, 170 / 255.f, 170 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TableHeaderBg ]           = ImVec4( 25 / 255.f, 25 / 255.f, 25 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TableBorderStrong ]       = ImVec4( 30 / 255.f, 30 / 255.f, 30 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TableBorderLight ]        = ImVec4( 25 / 255.f, 25 / 255.f, 25 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TableRowBg ]              = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TableRowBgAlt ]           = ImVec4( 15 / 255.f, 15 / 255.f, 15 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_TextSelectedBg ]          = ImVec4( 40 / 255.f, 40 / 255.f, 40 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_DragDropTarget ]          = ImVec4( 255 / 255.f, 255 / 255.f, 0 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_NavHighlight ]            = ImVec4( 80 / 255.f, 80 / 255.f, 80 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_NavWindowingHighlight ]   = ImVec4( 255 / 255.f, 255 / 255.f, 255 / 255.f, 1.0f );
		style.Colors[ ImGuiCol_NavWindowingDimBg ]       = ImVec4( 0 / 255.f, 0 / 255.f, 0 / 255.f, 0.5f );
		style.Colors[ ImGuiCol_ModalWindowDimBg ]        = ImVec4( 0 / 255.f, 0 / 255.f, 0 / 255.f, 0.5f );
		style.Colors[ ImGuiCol_::ImGuiCol_Accent ]       = GET_VARIABLE( g_variables.m_menu_accent_color, c_color ).get_vec4( );
		GET_VARIABLE( g_variables.m_accent, c_color )    = GET_VARIABLE( g_variables.m_menu_accent_color, c_color );
		style.ScaleAllSizes( menu_dpi_scale );
		ImGui::GetIO( ).FontGlobalScale = 1.0f;
	}
	const float menu_dpi_scale = menu_dpi_scale_value( );
	//float eased_progress       = m_animation_progress * m_animation_progress * ( 3.0f - 2.0f * m_animation_progress ); // unused // alpha animation
	ImVec2 animated_size  = ImVec2( 850.f * menu_dpi_scale * m_animation_progress, 582.f * menu_dpi_scale * m_animation_progress );
	ImVec2 display_center = ImGui::GetIO( ).DisplaySize * 0.5f;
	ImGui::SetNextWindowPos( display_center, ImGuiCond_::ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
	ImGui::SetNextWindowSize( animated_size, ImGuiCond_::ImGuiCond_Always );
	//ImGui::PushStyleVar( ImGuiStyleVar_Alpha, eased_progress ); // unused // alpha animation
	ImGui::Begin( "LilPeepXANAX", nullptr,
	              ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground |
	                  ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse |
	                  ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground );
	{
		const float layout_scale = menu_dpi_scale * m_animation_progress;
		ImVec2 tab_area_pos      = ImGui::GetWindowPos( ) + ImVec2( 0, 0 );
		ImVec2 tab_area_size     = ImVec2( 200, 582 ) * layout_scale;
		ImGui::GetBackgroundDrawList( )->AddRectFilled( tab_area_pos, tab_area_pos + tab_area_size, ImColor( 16, 16, 16, 255 ), 6.0f,
		                                                ImDrawFlags_RoundCornersLeft );
		ImGui::GetBackgroundDrawList( )->AddRect( tab_area_pos, tab_area_pos + tab_area_size, ImColor( 11, 11, 11, 255 ), 6.0f,
		                                          ImDrawFlags_RoundCornersLeft, 1.0f );
		ImGui::GetBackgroundDrawList( )->AddRect( tab_area_pos + ImVec2( 1, 1 ), tab_area_pos + tab_area_size - ImVec2( 1, 1 ),
		                                          ImColor( 25, 25, 25, 50 ), 7.0f, ImDrawFlags_RoundCornersLeft, 1.0f );
		ImVec2 combat_text_pos = tab_area_pos + ImVec2( 25, 20 ) * layout_scale;
		ImGui::GetBackgroundDrawList( )->AddText( g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ],
		                                          g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ]->FontSize *
		                                              menu_dpi_scale,
		                                          combat_text_pos, ImColor( 63, 63, 63, 255 ), "Combat" );
		ImVec2 visual_text_pos = tab_area_pos + ImVec2( 25, 70 ) * layout_scale;
		ImGui::GetBackgroundDrawList( )->AddText( g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ],
		                                          g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ]->FontSize *
		                                              menu_dpi_scale,
		                                          visual_text_pos, ImColor( 63, 63, 63, 255 ), "Players" );
		ImVec2 misc_text_pos = tab_area_pos + ImVec2( 25, 150 ) * layout_scale;
		ImGui::GetBackgroundDrawList( )->AddText( g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ],
		                                          g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ]->FontSize *
		                                              menu_dpi_scale,
		                                          misc_text_pos, ImColor( 63, 63, 63, 255 ), "Other" );
		ImVec2 main_area_pos  = ImGui::GetWindowPos( ) + ImVec2( 200, 0 ) * layout_scale;
		ImVec2 main_area_size = ImVec2( 650, 582 ) * layout_scale;
		ImGui::GetBackgroundDrawList( )->AddRectFilled( main_area_pos, main_area_pos + main_area_size, ImColor( 13, 13, 13, 255 ), 6.0f,
		                                                ImDrawFlags_RoundCornersRight );
		ImGui::GetBackgroundDrawList( )->AddRect( main_area_pos, main_area_pos + main_area_size, ImColor( 11, 11, 11, 255 ), 6.0f,
		                                          ImDrawFlags_RoundCornersRight, 1.0f );
		ImGui::GetBackgroundDrawList( )->AddRect( main_area_pos + ImVec2( 1, 1 ), main_area_pos + main_area_size - ImVec2( 1, 1 ),
		                                          ImColor( 20, 20, 20, 30 ), 7.0f, ImDrawFlags_RoundCornersRight, 1.0f );
		ImGui::GetBackgroundDrawList( )->AddRect( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ),
		                                          ImColor( 11, 11, 11, 255 ), 6.0f, ImDrawFlags_RoundCornersAll, 1.0f );
		ImGui::GetBackgroundDrawList( )->AddRect( ImGui::GetWindowPos( ) + ImVec2( 1, 1 ),
		                                          ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ) - ImVec2( 1, 1 ), ImColor( 15, 15, 15, 100 ), 7.0f,
		                                          ImDrawFlags_RoundCornersAll, 1.0f );
		ImVec2 separation_start = ImGui::GetWindowPos( ) + ImVec2( 200, 0 ) * layout_scale;
		ImVec2 separation_end   = ImGui::GetWindowPos( ) + ImVec2( 200, 582 ) * layout_scale;
		ImGui::GetBackgroundDrawList( )->AddLine( separation_start, separation_end, ImColor( 11, 11, 11, 255 ), 1.0f );
		std::vector< tab_t > combat_tabs = { { "Aimbot", "f", {} } };
		std::vector< tab_t > visual_tabs = { { "Players", "h", {} }, { "World", "g", {} } };
		std::vector< tab_t > misc_tabs   = { { "Other", "e", {} },  { "Movement", "m", {} }, { "Assist", "f", {} }, { "Style", "j", {} },
			                                 { "Config", "a", {} }, { "Server", "s", {} },   { "Convar", "c", {} }, { "Recorder", "v", {} } };

		static int combat_tab        = 0;
		static int visual_tab        = 0;
		static int misc_tab          = 0;
		static int current_group     = 0;
		auto custom_tabs_with_offset = [ layout_scale, menu_dpi_scale ]( std::vector< tab_t > tabs, float y_offset, int group_id, int& group_tab,
		                                                                 int global_tab_offset ) {
			float y_offset_adjustment = ( -110.0f + y_offset ) * layout_scale;
			const static float spacing[ 2 ]{ 13.0f, 143.0f };
			const static float size[ 2 ]{ 175.0f, 30.0f };
			const static float text_spacing{ 37.0f };
			const static float rounding{ 2.0f };
			for ( size_t i = 0; i < tabs.size( ); ++i ) {
				int mapped_tab_index = global_tab_offset + static_cast< int >( i );
				auto windowPos       = ImGui::GetWindowPos( );
				float x_offset       = windowPos.x + ( spacing[ 0 ] + text_spacing ) * layout_scale;
				float y_offset_pos =
					windowPos.y + spacing[ 1 ] * layout_scale + ( i * size[ 1 ] ) * layout_scale + 7.0f * layout_scale + y_offset_adjustment;
				auto text_position = ImVec2( x_offset, y_offset_pos );
				auto tab_min       = ImVec2( windowPos.x + spacing[ 0 ] * layout_scale,
				                             windowPos.y + spacing[ 1 ] * layout_scale + i * size[ 1 ] * layout_scale + y_offset_adjustment );
				auto tab_max       = ImVec2( windowPos.x + size[ 0 ] * layout_scale,
				                             windowPos.y + spacing[ 1 ] * layout_scale + ( i + 1 ) * size[ 1 ] * layout_scale + y_offset_adjustment );
				bool hovered       = ImGui::IsMouseHoveringRect( tab_min, tab_max );
				bool selected      = ( current_group == group_id && mapped_tab_index == tab_number );
				if ( !selected && hovered ) {
					ImGui::GetWindowDrawList( )->AddRectFilled( tab_min, tab_max, ImColor( 18, 18, 18, 255 ), rounding * layout_scale,
					                                            ImDrawFlags_RoundCornersAll );
				}
				if ( selected ) {
					ImGui::GetWindowDrawList( )->AddRectFilled( tab_min, tab_max, ImColor( 23, 23, 23, 255 ), rounding * layout_scale,
					                                            ImDrawFlags_RoundCornersAll );
				}
				ImColor text_color =
					selected ? ImColor( 245, 245, 245, 255 ) : ( hovered ? ImColor( 200, 200, 200, 255 ) : ImColor( 160, 160, 160, 255 ) );
				ImGui::GetWindowDrawList( )->AddText(
					g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ],
					g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ]->FontSize * menu_dpi_scale, text_position,
					text_color, tabs[ i ].name.c_str( ) );
				float icon_x_offset = -20.0f;
				float icon_y_offset = 0.0f;
				if ( tabs[ i ].icon == "m" ) {
					icon_x_offset = -18.0f;
				} else if ( tabs[ i ].icon == "c" ) {
					icon_x_offset = -22.0f;
				} else if ( tabs[ i ].icon == "s" ) {
					icon_x_offset = -21.0f;
				} else if ( tabs[ i ].icon == "v" ) {
					icon_x_offset = -20.0f;
				} else if ( tabs[ i ].icon == "r" ) {
					icon_x_offset = -20.0f;
				} else if ( tabs[ i ].name == "Assist" ) {
					icon_x_offset = -21.0f;
				}
				ImColor icon_color =
					selected ? ImColor( 220, 215, 220, 255 ) : ( hovered ? ImColor( 180, 180, 180, 255 ) : ImColor( 160, 160, 160, 255 ) );
				ImFont* icon_font           = fonts_for_gui::fontello_font;
				const char* icon_char       = tabs[ i ].icon.c_str( );
				bool used_sidebar_icon_font = false;
				if ( fonts_for_gui::fontello_sidebar_icon ) {
					if ( tabs[ i ].name == "World" ) {
						icon_font              = fonts_for_gui::fontello_sidebar_icon;
						icon_char              = "\xEE\x94\xAF";
						used_sidebar_icon_font = true;
					} else if ( tabs[ i ].name == "Players" ) {
						icon_font              = fonts_for_gui::fontello_sidebar_icon;
						icon_char              = "\xEF\x83\x80";
						used_sidebar_icon_font = true;
					} else if ( tabs[ i ].name == "Config" ) {
						icon_font              = fonts_for_gui::fontello_sidebar_icon;
						icon_char              = "\xEF\x82\x93";
						used_sidebar_icon_font = true;
					} else if ( tabs[ i ].name == "Other" ) {
						icon_font              = fonts_for_gui::fontello_sidebar_icon;
						icon_char              = "\xEF\x87\x9E";
						used_sidebar_icon_font = true;
					} else if ( tabs[ i ].name == "Assist" ) {
						icon_font              = fonts_for_gui::fontello_font;
						icon_char              = "f";
						used_sidebar_icon_font = true;
					}
				}
				if ( !used_sidebar_icon_font ) {
					if ( group_id == 3 ) {
						static const char icon_convar[] = "n";
						static const char icon_fun[]    = "m";
						static const char icon_route[]  = "c";
						if ( i == 0 ) {
							icon_font = fonts_for_gui::fontello_convar_icon;
							icon_char = icon_convar;
						} else if ( i == 1 ) {
							icon_font = fonts_for_gui::fontello_fun_icon;
							icon_char = icon_fun;
						} else {
							icon_font = fonts_for_gui::fontello_route_icon;
							icon_char = icon_route;
						}
					} else if ( tabs[ i ].icon == "s" ) {
						icon_font = fonts_for_gui::fontello_route_icon;
						icon_char = "c";
					}
				}
				if ( !icon_font )
					icon_font = fonts_for_gui::fontello_font;
				constexpr float k_tab_icon_scale = 0.92f;
				const float icon_draw_size       = icon_font->FontSize * k_tab_icon_scale * menu_dpi_scale;
				ImFont* label_font_tab           = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ];
				const float label_fs_tab         = ( label_font_tab ? label_font_tab->FontSize : icon_font->FontSize ) * menu_dpi_scale;
				const float icon_align_y         = icon_y_offset * layout_scale + ( label_fs_tab - icon_draw_size ) * 0.42f;
				const float icon_align_x         = icon_x_offset * layout_scale - ( icon_font->FontSize * menu_dpi_scale - icon_draw_size ) * 0.2f;
				ImGui::GetWindowDrawList( )->AddText(
					icon_font, icon_draw_size, ImVec2( text_position.x + icon_align_x, text_position.y + icon_align_y ), icon_color, icon_char );
				if ( hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
					combat_tab    = 0;
					visual_tab    = 0;
					misc_tab      = 0;
					group_tab     = i;
					current_group = group_id;
					tab_number    = mapped_tab_index;
					subtab_number = 0;
				}
			}
		};
		custom_tabs_with_offset( combat_tabs, 0.0f, 0, combat_tab, 0 );
		custom_tabs_with_offset( visual_tabs, 50.0f, 1, visual_tab, 1 );
		custom_tabs_with_offset( misc_tabs, 130.0f, 2, misc_tab, 3 );

		{
			auto* draw_list             = ImGui::GetBackgroundDrawList( );
			auto* font                  = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ];
			ImVec2 win_pos              = ImGui::GetWindowPos( );
			const ImU32 icon_col        = IM_COL32( 160, 160, 160, 255 );
			const bool dev_hwid_allowed = is_dev_hwid_allowed( );

			{
				constexpr float pi = 3.14159265f;
				const ImVec2 c     = win_pos + ImVec2( 35, 526 );
				const float R      = 7.f;
				const float thick  = 1.8f;

				constexpr int seg          = 40;
				constexpr float gap_center = -0.35f;
				constexpr float gap_half   = 0.5f;
				for ( int i = 0; i < seg; i++ ) {
					float a0 = ( float( i ) / seg ) * pi * 2.f;
					float a1 = ( float( i + 1 ) / seg ) * pi * 2.f;
					float m  = ( a0 + a1 ) * 0.5f;
					float d  = m - gap_center;
					while ( d > pi )
						d -= pi * 2.f;
					while ( d < -pi )
						d += pi * 2.f;
					if ( std::fabs( d ) < gap_half )
						continue;
					draw_list->AddLine( ImVec2( c.x + cosf( a0 ) * R, c.y + sinf( a0 ) * R ), ImVec2( c.x + cosf( a1 ) * R, c.y + sinf( a1 ) * R ),
					                    icon_col, thick );
				}

				struct dot_t {
					float x, y, r;
				};
				const dot_t dots[] = {
					{ -0.42f, -0.50f, 1.7f }, { 0.20f, -0.60f, 1.5f }, { 0.55f, -0.10f, 1.3f }, { -0.65f, 0.10f, 1.5f }, { -0.10f, 0.50f, 1.8f },
				};
				for ( auto& d : dots )
					draw_list->AddCircleFilled( ImVec2( c.x + d.x * R, c.y + d.y * R ), d.r, icon_col, 8 );

				float na = gap_center;
				float nr = R * 0.45f;
				ImVec2 notch_c( c.x + cosf( na ) * ( R - nr * 0.6f ), c.y + sinf( na ) * ( R - nr * 0.6f ) );
				constexpr int nseg = 20;
				for ( int i = 0; i < nseg; i++ ) {
					float a0 = ( float( i ) / nseg ) * pi * 2.f;
					float a1 = ( float( i + 1 ) / nseg ) * pi * 2.f;
					draw_list->AddLine( ImVec2( notch_c.x + cosf( a0 ) * nr, notch_c.y + sinf( a0 ) * nr ),
					                    ImVec2( notch_c.x + cosf( a1 ) * nr, notch_c.y + sinf( a1 ) * nr ), icon_col, thick * 0.8f );
				}
			}

			const float sidebar_font_size = font ? font->FontSize * menu_dpi_scale : 13.0f * menu_dpi_scale;
			ImVec2 accent_text_pos        = win_pos + ImVec2( 46, 520 ) * layout_scale;
			draw_list->AddText( font, sidebar_font_size, accent_text_pos, ImColor( 160, 160, 160, 255 ), "Accent" );

			c_color& accent_col = GET_VARIABLE( g_variables.m_menu_accent_color, c_color );
			const float sq_w    = 22.f * layout_scale;
			const float sq_h    = 16.f * layout_scale;
			ImVec2 sq_min       = win_pos + ImVec2( 175, 519 ) * layout_scale - ImVec2( sq_w + 1.0f * layout_scale, 0.0f );
			ImVec2 sq_max       = sq_min + ImVec2( sq_w, sq_h );

			draw_list->AddRectFilled( sq_min, sq_max, accent_col.get_u32( 1.f ), 3.f * layout_scale );
			draw_list->AddRect( sq_min, sq_max, IM_COL32( 50, 50, 50, 200 ), 3.f * layout_scale, 0, 1.f * layout_scale );

			ImRect sq_rect( sq_min, sq_max );
			if ( sq_rect.Contains( ImGui::GetIO( ).MousePos ) && ImGui::IsMouseClicked( 0 ) ) {
				ImGui::OpenPopup( "##sidebar_accent_picker" );
			}
			if ( ImGui::BeginPopup( "##sidebar_accent_picker" ) ) {
				float col4[ 4 ] = { accent_col.get< 0 >( ) / 255.f, accent_col.get< 1 >( ) / 255.f, accent_col.get< 2 >( ) / 255.f,
					                accent_col.get< 3 >( ) / 255.f };
				if ( ImGui::ColorPicker4( "##accent_picker", col4,
				                          ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_NoInputs |
				                              ImGuiColorEditFlags_PickerHueBar ) ) {
					accent_col = c_color( static_cast< int >( col4[ 0 ] * 255.f ), static_cast< int >( col4[ 1 ] * 255.f ),
					                      static_cast< int >( col4[ 2 ] * 255.f ), static_cast< int >( col4[ 3 ] * 255.f ) );
				}
				ImGui::EndPopup( );
			}

			{
				const ImVec2 dpi_icon_center   = win_pos + ImVec2( 35, 558 ) * layout_scale;
				const float dpi_icon_radius    = 7.0f * layout_scale;
				const float dpi_icon_thickness = 1.6f * layout_scale;
				constexpr float pi             = 3.14159265f;
				for ( int pass = 0; pass < 2; ++pass ) {
					const float start = pass == 0 ? -0.2f : pi - 0.2f;
					const float end   = start + pi * 1.25f;
					ImVec2 prev( dpi_icon_center.x + cosf( start ) * dpi_icon_radius, dpi_icon_center.y + sinf( start ) * dpi_icon_radius );
					for ( int seg = 1; seg <= 18; ++seg ) {
						const float a = start + ( end - start ) * ( float( seg ) / 18.0f );
						ImVec2 cur( dpi_icon_center.x + cosf( a ) * dpi_icon_radius, dpi_icon_center.y + sinf( a ) * dpi_icon_radius );
						draw_list->AddLine( prev, cur, icon_col, dpi_icon_thickness );
						prev = cur;
					}
					const ImVec2 tip( dpi_icon_center.x + cosf( end ) * dpi_icon_radius, dpi_icon_center.y + sinf( end ) * dpi_icon_radius );
					const ImVec2 a1( tip.x + cosf( end - 2.45f ) * 3.5f * layout_scale, tip.y + sinf( end - 2.45f ) * 3.5f * layout_scale );
					const ImVec2 a2( tip.x + cosf( end + 2.45f ) * 3.5f * layout_scale, tip.y + sinf( end + 2.45f ) * 3.5f * layout_scale );
					draw_list->AddTriangleFilled( tip, a1, a2, icon_col );
				}

				draw_list->AddText( font, sidebar_font_size, win_pos + ImVec2( 46, 552 ) * layout_scale, ImColor( 160, 160, 160, 255 ), "Scale" );

				int& dpi_scale          = menu_dpi_percent( );
				const ImVec2 slider_min = win_pos + ImVec2( 112, 561 ) * layout_scale;
				const ImVec2 slider_max = win_pos + ImVec2( 155, 562 ) * layout_scale;
				const ImVec2 value_min  = win_pos + ImVec2( 162, 552 ) * layout_scale;
				const ImVec2 value_max  = value_min + ImVec2( 30, 18 ) * layout_scale;
				ImGui::SetCursorScreenPos( slider_min - ImVec2( 5, 7 ) * layout_scale );
				ImGui::InvisibleButton( "##sidebar_dpi_scale", ImVec2( 85, 18 ) * layout_scale );
				static bool scale_dragging   = false;
				static int scale_before_drag = 100;
				if ( ImGui::IsItemActivated( ) ) {
					scale_dragging    = true;
					scale_before_drag = dpi_scale;
				}
				if ( ImGui::IsItemActive( ) ) {
					const float progress = std::clamp( ( ImGui::GetIO( ).MousePos.x - slider_min.x ) / ( slider_max.x - slider_min.x ), 0.0f, 1.0f );
					dpi_scale            = 75 + static_cast< int >( std::round( progress * 125.0f ) );
				}
				if ( scale_dragging && ImGui::IsItemDeactivated( ) ) {
					scale_dragging = false;
					if ( dpi_scale != scale_before_drag )
						g_render.request_imgui_refresh( );
				}
				const float dpi_progress = static_cast< float >( dpi_scale - 75 ) / 125.0f;
				const ImVec2 grab_center( slider_min.x + ( slider_max.x - slider_min.x ) * dpi_progress, ( slider_min.y + slider_max.y ) * 0.5f );
				draw_list->AddRectFilled( slider_min, slider_max, ImColor( 88, 88, 88, 255 ), 1.0f * layout_scale );
				draw_list->AddRectFilled( slider_min, ImVec2( grab_center.x, slider_max.y ),
				                          ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 220 ) ), 1.0f * layout_scale );
				draw_list->AddCircleFilled( grab_center, 5.0f * layout_scale, ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 255 ) ),
				                            16 );
				draw_list->AddRectFilled( value_min, value_max, ImColor( 16, 16, 16, 255 ), 2.0f * layout_scale );
				draw_list->AddRect( value_min, value_max, ImColor( 32, 32, 32, 255 ), 2.0f * layout_scale, 0, 1.0f * layout_scale );
				char dpi_value[ 8 ];
				snprintf( dpi_value, sizeof( dpi_value ), "%d%%", dpi_scale );
				draw_list->AddText( font, sidebar_font_size, value_min + ImVec2( 4, 2 ) * layout_scale, ImColor( 160, 160, 160, 255 ), dpi_value );
			}

			if ( dev_hwid_allowed ) {
				const char* dev_label     = "Dev";
				const ImVec2 dev_text_pos = win_pos + ImVec2( 120.f, 535.f ) * layout_scale;
				ImVec2 dev_text_size = font ? font->CalcTextSizeA( sidebar_font_size, FLT_MAX, 0.0f, dev_label ) : ImGui::CalcTextSize( dev_label );
				if ( !font ) {
					dev_text_size.x *= menu_dpi_scale;
					dev_text_size.y *= menu_dpi_scale;
				}
				const ImRect dev_rect( dev_text_pos - ImVec2( 6.f, 3.f ) * layout_scale,
				                       dev_text_pos + dev_text_size + ImVec2( 6.f, 3.f ) * layout_scale );
				const bool dev_hovered  = dev_rect.Contains( ImGui::GetIO( ).MousePos );
				const bool dev_selected = tab_number == 11;

				draw_list->AddText( font, sidebar_font_size, dev_text_pos,
				                    dev_selected  ? ImColor( 235, 235, 235, 255 )
				                    : dev_hovered ? ImColor( 195, 195, 195, 255 )
				                                  : ImColor( 120, 120, 120, 255 ),
				                    dev_label );

				if ( dev_hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
					tab_number    = 11;
					subtab_number = 0;
				}
			}
		}

		int display_tab_number          = tab_number;
		static int movement_content_tab = 0;
		static int misc_content_tab     = 0;
		static int movement_section_tab = 0;
		static int server_section_tab   = 0;
		if ( display_tab_number == 11 && !is_dev_hwid_allowed( ) ) {
			tab_number         = 4;
			display_tab_number = 4;
		}
		ImGui::PushFont( g_render.m_custom_fonts[e_custom_font_names::custom_font_name_googlesans_addwindow] );
		if ( display_tab_number == 0 ) {
						ImGui::SetCursorPos( ImVec2( 215.0f, -5.0f ) );
		child( "Main", ImVec2( 300.0f, 515.0f ),
			       []( ) { 
					int& index = g_aimbot.m_menu_weapon_selection;
					
					const char* weapons[] = { "Pistol", "Heavy Pistol", "Shotgun", "Heavy", "SMG", "Rifle", "Sniper", "Auto Sniper" };
					combo( "Weapon", index, weapons, 8 );
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 9.0f);
					checkbox( "Enable aimbot", &g_aimbot.m_aimbot_settings[ index ].m_enable, false, true, ImVec2( 300, -1 ), []( ) {
						text_with_keybind( "Aimbot panic key" );
						keybind( "##aimbot panic key", &GET_VARIABLE( g_variables.m_aimbot_panic_key, key_bind_t ) );
						popup_checkbox( "Nospread", &GET_VARIABLE( g_variables.m_aimbot_nospread, bool ) );
						popup_checkbox( "Friendly fire", &GET_VARIABLE( g_variables.m_aimbot_friendly_fire, bool ) );
					} );
					text_with_keybind( "Aimbot key" );
					keybind( "##aimbot key", &GET_VARIABLE( g_variables.m_aimbot_key, key_bind_t ) );
					checkbox( "Enable silent", &g_aimbot.m_aimbot_settings[ index ].m_silent );
					checkbox( "Enable recoil control", &g_aimbot.m_aimbot_settings[ index ].m_recoil_control );
					checkbox( "Enable visibility check", &g_aimbot.m_aimbot_settings[ index ].m_visibility_check );
					checkbox( "Enable backtrack", &GET_VARIABLE( g_variables.m_backtrack_enable, bool ) );
					checkbox( "Enable extended backtrack", &GET_VARIABLE( g_variables.m_backtrack_extend, bool ) );
					if ( GET_VARIABLE( g_variables.m_backtrack_enable,bool) )
						checkbox( "Enable aim to backtrack", &g_aimbot.m_aimbot_settings[ index ].m_aim_to_backtrack );
					const char* hitboxes[] = { "Head", "Neck", "Body", "Legs" };
					multi_combo( "Hitboxes", g_aimbot.m_aimbot_settings[ index ].m_hitboxes, hitboxes, 4 );
				} );
							ImGui::SetCursorPos( ImVec2( 215.0f+315.f, -5.0f ) );
			child( "Configuration", ImVec2( 300.0f, 515.0f ), []( ) {
					int& index = g_aimbot.m_menu_weapon_selection;
					slider_float( "FOV", &g_aimbot.m_aimbot_settings[ index ].m_fov, 0.f, 180.f, "%.1f", 75.f, 46.f );
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);
					slider_float( "Smooth", &g_aimbot.m_aimbot_settings[ index ].m_smooth, 1.f, 100.f, "%.1f", 75.f, 46.f );
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);
					if ( g_aimbot.m_aimbot_settings[ index ].m_recoil_control ) {
						slider_int( "RCS amount", &g_aimbot.m_aimbot_settings[ index ].m_rcs_amount, 0, 100, "%d%%", 75.f, 46.f );
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);
					}
					if ( !g_aimbot.m_aimbot_settings[ index ].m_visibility_check ) {
						checkbox( "Autowall lethal only", &g_aimbot.m_aimbot_settings[ index ].m_autowall_lethal_only );
						if ( !g_aimbot.m_aimbot_settings[ index ].m_autowall_lethal_only ) {
							slider_int( "Autowall min damage", &g_aimbot.m_aimbot_settings[ index ].m_autowall_min_damage, 1, 100, "%d", 75.f, 46.f );
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);
						}
					}
					slider_int( "Backtrack time limit", &GET_VARIABLE( g_variables.m_backtrack_time_limit,int ), 0, 200, "%d", 75.f, 36.f );
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);
					slider_int( "Ping adder", &GET_VARIABLE( g_variables.fakeLatencyAmount, int ), 0, 200, "%d", 75.f, 36.f );
				} );
		}
		if ( display_tab_number == 1 ) {
							ImGui::SetCursorPos( ImVec2( 215.0f, -5.0f ) );
			child( "Players ESP", ImVec2( 300.0f, 515.0f ), []( ) {
					checkbox( "Players", &GET_VARIABLE( g_variables.m_players, bool ) );
					checkbox( "Visibility range", &GET_VARIABLE( g_variables.m_players_visibility_range, bool ), false, true, { 300, -1 }, []( ) {
						popup_slider_float( "Distance", &GET_VARIABLE( g_variables.m_players_visibility_range_distance, float ), 500.0f, 10000.0f, "%.0f" );
					} );
					if ( GET_VARIABLE( g_variables.m_players, bool ) ) {
						checkbox( "Bounding box", &GET_VARIABLE( g_variables.m_players_box, bool ), false, true, { 300, -1 }, []( ) {
							popup_checkbox( "Corner bounding box", &GET_VARIABLE( g_variables.m_players_box_corner, bool ) );
							if ( GET_VARIABLE( g_variables.m_players_box_corner, bool ) ) {
								popup_slider_float( "Corner length", &GET_VARIABLE( g_variables.m_players_box_corner_length, float ), 4.0f, 20.0f, "%.1f" );
							}
							popup_checkbox( "Bounding box outline", &GET_VARIABLE( g_variables.m_players_box_outline, bool ) );
							ImGui::Spacing( );
							text_with_color_picker( "Player bounding box color", &GET_VARIABLE( g_variables.m_players_box_color, c_color ) );
							if ( GET_VARIABLE( g_variables.m_players_box_outline, bool ) ) {
								ImGui::Spacing( );
								text_with_color_picker( "Player box outline color", &GET_VARIABLE( g_variables.m_players_box_outline_color, c_color ) );
							}
						} );
						checkbox_with_color_picker( "Name", &GET_VARIABLE( g_variables.m_players_name, bool ), false, true, { 300, -1 }, []( ) {
							popup_checkbox( "Outline", &GET_VARIABLE( g_variables.m_players_name_outline, bool ) );
							ImGui::Spacing( );
							ImGui::Text( "Player name color" );
							popup_color_picker_button( "##player name color", &GET_VARIABLE( g_variables.m_players_name_color, c_color ) );
						}, &GET_VARIABLE( g_variables.m_players_name_color, c_color ) );
						checkbox( "Health bar", &GET_VARIABLE( g_variables.health_bar_enable, bool ), false, true, { 300, -1 }, []( ) {
							popup_checkbox( "Gradient color", &GET_VARIABLE( g_variables.health_bar_gradient, bool ) );
							popup_checkbox( "Color based on health", &GET_VARIABLE( g_variables.health_bar_baseonhealth, bool ) );
						popup_checkbox( "Fill outline background", &GET_VARIABLE( g_variables.health_bar_background, bool ) );
						popup_checkbox( "Text on bar", &GET_VARIABLE( g_variables.health_bar_text, bool ) );
						popup_checkbox( "Hp indicator", &GET_VARIABLE( g_variables.health_bar_hp_indicator, bool ) );
						static const char* health_bar_positions[] = { "Up", "Down", "Right", "Left" };
						popup_combo( "Position", GET_VARIABLE( g_variables.health_bar_position, int ), health_bar_positions, 4 );
						popup_slider_int( "Bar width", &GET_VARIABLE( g_variables.health_bar_size, int ), 1, 10, "%.d" );
							ImGui::Spacing( );
							ImGui::Text( "Healthbar outline color" );
							ImGui::ColorEdit4( "##healthbar outline", &GET_VARIABLE( g_variables.colors_health_bar_outline, c_color ) );
							if ( GET_VARIABLE( g_variables.colors_custom, bool ) ) {
								ImGui::Spacing( );
								ImGui::Text( "Healthbar color" );
								ImGui::ColorEdit4( "##healthbar color", &GET_VARIABLE( g_variables.colors_health_bar, c_color ) );
							}
							if ( GET_VARIABLE( g_variables.health_bar_gradient, bool ) ) {
								ImGui::Spacing( );
								ImGui::Text( "Healthbar upper color" );
								ImGui::Spacing( );
								ImGui::ColorEdit4( "##healthbar upper color", &GET_VARIABLE( g_variables.colors_health_bar_upper, c_color ) );
								ImGui::Text( "Healthbar middle color" );
								ImGui::ColorEdit4( "##healthbar middle color", &GET_VARIABLE( g_variables.colors_health_bar_middle, c_color ) );
								ImGui::Text( "Healthbar lower color" );
								ImGui::ColorEdit4( "##healthbar lower color", &GET_VARIABLE( g_variables.colors_health_bar_lower, c_color ) );
							}
						} );
						checkbox( "Health", &GET_VARIABLE( g_variables.m_players_health_text, bool ), false, true, { 300, -1 }, []( ) {
							static const char* health_positions[] = { "Top", "Left", "Right", "Bottom" };
							popup_combo( "Position", GET_VARIABLE( g_variables.m_players_health_text_position, int ), health_positions, 4 );
						} );
						checkbox( "Ammo", &GET_VARIABLE( g_variables.m_player_ammo_text, bool ), false, true, { 300, -1 }, []( ) {
							static const char* ammo_text_positions[] = { "Top", "Left", "Right", "Bottom" };
							popup_combo( "Position", GET_VARIABLE( g_variables.m_player_ammo_text_position, int ), ammo_text_positions, 4 );
						} );
						checkbox_with_color_picker( "Weapon name", &GET_VARIABLE( g_variables.m_weapon_name, bool ), false, true, { 300, -1 }, []( ) {
							popup_checkbox( "Outline", &GET_VARIABLE( g_variables.m_weapon_name_outline, bool ) );
							static const char* weapon_positions[] = { "Top", "Left", "Right", "Bottom" };
							popup_combo( "Position", GET_VARIABLE( g_variables.m_weapon_name_position, int ), weapon_positions, 4 );
							ImGui::Spacing( );
							ImGui::Text( "Weapon name color" );
							popup_color_picker_button( "##weapon name color", &GET_VARIABLE( g_variables.m_weapon_name_color, c_color ) );
						}, &GET_VARIABLE( g_variables.m_weapon_name_color, c_color ) );
						checkbox_with_color_picker( "Weapon icon", &GET_VARIABLE( g_variables.m_weapon_icon, bool ), false, true, { 300, -1 }, []( ) {
							popup_checkbox( "Outline", &GET_VARIABLE( g_variables.m_weapon_icon_outline, bool ) );
							ImGui::Spacing( );
							ImGui::Text( "Weapon icon color" );
							popup_color_picker_button( "##weapon icon color", &GET_VARIABLE( g_variables.m_weapon_icon_color, c_color ) );
						}, &GET_VARIABLE( g_variables.m_weapon_icon_color, c_color ) );
						checkbox_with_color_picker( "Weapon ammo bar", &GET_VARIABLE( g_variables.m_player_ammo_bar, bool ), false, true, { 300, -1 }, []( ) {
							popup_checkbox( "Outline", &GET_VARIABLE( g_variables.m_player_ammo_bar_outline, bool ) );
							const char* ammo_positions[] = { "Top", "Left", "Right", "Bottom" };
							popup_combo( "Position", GET_VARIABLE( g_variables.m_player_ammo_bar_position, int ), ammo_positions, 4 );
							ImGui::Spacing( );
							ImGui::Text( "Weapon ammo bar color" );
							popup_color_picker_button( "##weapon ammo bar color", &GET_VARIABLE( g_variables.m_player_ammo_bar_color, c_color ) );
						}, &GET_VARIABLE( g_variables.m_player_ammo_bar_color, c_color ) );
						checkbox( "Player skeleton", &GET_VARIABLE( g_variables.m_players_skeleton, bool ), false, true, { 300, -1 }, []( ) {
							popup_checkbox( "Outline", &GET_VARIABLE( g_variables.m_players_skeleton_outline, bool ) );
							const char* skeleton_types[] = { "Normal", "Lag compensated" };
							popup_combo( "Skeleton type", GET_VARIABLE( g_variables.m_players_skeleton_type, int ), skeleton_types, 2 );
							ImGui::Spacing( );
							ImGui::Text( "Player skeleton color" );
							ImGui::ColorEdit4( "##player skeleton color", &GET_VARIABLE( g_variables.m_players_skeleton_color, c_color ),
							                   color_picker_alpha_flags );
							if ( GET_VARIABLE( g_variables.m_players_skeleton_outline, bool ) ) {
								ImGui::Spacing( );
								ImGui::Text( "Skeleton outline color" );
								ImGui::ColorEdit4( "##skeleton outline color", &GET_VARIABLE( g_variables.m_players_skeleton_outline_color, c_color ),
								                   color_picker_alpha_flags );
							}
						} );
						checkbox(
								"Out of FOV arrows", &GET_VARIABLE( g_variables.m_out_of_fov_arrows, bool ), false, true, ImVec2( 300.f, -1.f ), []( ) {
								ImGui::SetCursorPosX( 25.f );
								ImGui::Text( "FOV arrows color" );
								ImGui::ColorEdit4( "##out of fov arrows color", &GET_VARIABLE( g_variables.m_out_of_fov_arrows_color, c_color ),
							                       color_picker_alpha_flags );
								ImGui::Spacing( );
								popup_slider_float( "Arrows size", &GET_VARIABLE( g_variables.m_out_of_fov_arrows_size, float ), 0.1f, 50.f, "%.1f px" );
								popup_slider_int( "Arrows distance", &GET_VARIABLE( g_variables.m_out_of_fov_arrows_distance, int ), 10, 500, "%d px" );
							} );
						slider_int( "Visible alpha", &GET_VARIABLE( g_variables.m_visible_alpha, int ), 0, 255, "%d", 75.f, 36.f );
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);
						slider_int( "In visible alpha", &GET_VARIABLE( g_variables.m_invisuals_alpha, int ), 0, 255, "%d", 75.f, 36.f );
					}
				} );
							ImGui::SetCursorPos( ImVec2( 215.0f + 315.f, -5.0f ) );
			child( "Chams", ImVec2( 300.0f, 515.0f ), []( ) {
					static const char* chams_materialss[] = { "Default", "Flat", "Glow", "Wireframe", "Crystal", "Metallic", "Energy", "Animated", "Platinum", "Glass", "Chrome", "Silver", "Gold", "Plastic", "Pearlescent", "Smoke", "Smoke 2", "Leaves", "Lines", "Snowflakes" };
					static int selected_chams_category = 0;
					checkbox( "Enable chams", &GET_VARIABLE( g_variables.m_chams_enable, bool ), false, true, { 300, 350 }, [ & ]( ) {
						const char* chams_categories[] = { "Local", "Weapon", "Ragdoll", "Visible", "Invisible", "Backtrack", "Ally", "Hands", "Sleeves" };
						popup_combo( "Category", selected_chams_category, chams_categories, IM_ARRAYSIZE( chams_categories ) );

						auto draw_chams_settings = [ & ]( bool& enabled, int& material, c_color& color, bool& ignore_z, bool& wireframe, bool& cover, const char* id, bool force_ignore_z = false ) {
							popup_checkbox( "Enabled", &enabled );
							popup_combo( "Material", material, chams_materialss, 20 );
							if ( force_ignore_z ) {
								bool always_on = true;
								popup_checkbox( "Ignore Z", &always_on, true );
								ignore_z = true;
							} else {
								popup_checkbox( "Ignore Z", &ignore_z );
							}
							popup_checkbox( "Wireframe", &wireframe );
							popup_checkbox( "Z cover", &cover );
							ImGui::Spacing( );
							ImGui::Text( "Color" );
							ImGui::ColorEdit4( id, &color, color_picker_alpha_flags );
						};

						switch ( selected_chams_category ) {
						case 0:
							draw_chams_settings( GET_VARIABLE( g_variables.m_player_local_chams, bool ), GET_VARIABLE( g_variables.m_player_local_chams_material, int ),
							                     GET_VARIABLE( g_variables.m_player_local_chams_color, c_color ), GET_VARIABLE( g_variables.m_player_local_chams_ignorez, bool ),
							                     GET_VARIABLE( g_variables.m_player_local_chams_wireframe, bool ), GET_VARIABLE( g_variables.m_player_local_chams_cover, bool ),
							                     "##local_chams_color" );
							break;
						case 1:
							draw_chams_settings( GET_VARIABLE( g_variables.m_weapon_chams, bool ), GET_VARIABLE( g_variables.m_weapon_chams_material, int ),
							                     GET_VARIABLE( g_variables.m_weapon_chams_color, c_color ), GET_VARIABLE( g_variables.m_weapon_chams_ignorez, bool ),
							                     GET_VARIABLE( g_variables.m_weapon_chams_wireframe, bool ), GET_VARIABLE( g_variables.m_weapon_chams_cover, bool ),
							                     "##weapon_chams_color" );
							break;
						case 2:
							draw_chams_settings( GET_VARIABLE( g_variables.m_ragdoll_chams, bool ), GET_VARIABLE( g_variables.m_ragdoll_chams_material, int ),
							                     GET_VARIABLE( g_variables.m_ragdoll_chams_color, c_color ), GET_VARIABLE( g_variables.m_ragdoll_chams_ignorez, bool ),
							                     GET_VARIABLE( g_variables.m_ragdoll_chams_wireframe, bool ), GET_VARIABLE( g_variables.m_ragdoll_chams_cover, bool ),
							                     "##ragdoll_chams_color" );
							break;
						case 3:
							draw_chams_settings( GET_VARIABLE( g_variables.m_player_visible_chams, bool ), GET_VARIABLE( g_variables.m_player_visible_chams_material, int ),
							                     GET_VARIABLE( g_variables.m_player_visible_chams_color, c_color ), GET_VARIABLE( g_variables.m_player_visible_chams_ignorez, bool ),
							                     GET_VARIABLE( g_variables.m_player_visible_chams_wireframe, bool ), GET_VARIABLE( g_variables.m_player_visible_chams_cover, bool ),
							                     "##visible_chams_color" );
							break;
						case 4:
							draw_chams_settings( GET_VARIABLE( g_variables.m_player_invisible_chams, bool ), GET_VARIABLE( g_variables.m_player_invisible_chams_material, int ),
							                     GET_VARIABLE( g_variables.m_player_invisible_chams_color, c_color ), GET_VARIABLE( g_variables.m_player_invisible_chams_ignorez, bool ),
							                     GET_VARIABLE( g_variables.m_player_invisible_chams_wireframe, bool ), GET_VARIABLE( g_variables.m_player_invisible_chams_cover, bool ),
							                     "##invisible_chams_color", true );
							break;
						case 5:
							draw_chams_settings( GET_VARIABLE( g_variables.m_player_lag_chams, bool ), GET_VARIABLE( g_variables.m_player_lag_chams_material, int ),
							                     GET_VARIABLE( g_variables.m_player_lag_chams_color, c_color ), GET_VARIABLE( g_variables.m_player_lag_chams_ignorez, bool ),
							                     GET_VARIABLE( g_variables.m_player_lag_chams_wireframe, bool ), GET_VARIABLE( g_variables.m_player_lag_chams_cover, bool ),
							                     "##backtrack_chams_color" );
							GET_VARIABLE( g_variables.m_player_lag_chams_xqz, bool ) = GET_VARIABLE( g_variables.m_player_lag_chams_ignorez, bool );
							{
								const char* lag_chams_types[] = { "Oldest record", "All records" };
								popup_combo( "Record type", GET_VARIABLE( g_variables.m_player_lag_chams_type, int ), lag_chams_types, 2 );
							}
							break;
						case 6:
							draw_chams_settings( GET_VARIABLE( g_variables.m_player_ally_chams, bool ), GET_VARIABLE( g_variables.m_player_ally_chams_material, int ),
							                     GET_VARIABLE( g_variables.m_player_ally_chams_color, c_color ), GET_VARIABLE( g_variables.m_player_ally_chams_ignorez, bool ),
							                     GET_VARIABLE( g_variables.m_player_ally_chams_wireframe, bool ), GET_VARIABLE( g_variables.m_player_ally_chams_cover, bool ),
							                     "##ally_chams_color" );
							break;
						case 7:
							draw_chams_settings( GET_VARIABLE( g_variables.m_hands_chams, bool ), GET_VARIABLE( g_variables.m_hands_chams_material, int ),
							                     GET_VARIABLE( g_variables.m_hands_chams_color, c_color ), GET_VARIABLE( g_variables.m_hands_chams_ignorez, bool ),
							                     GET_VARIABLE( g_variables.m_hands_chams_wireframe, bool ), GET_VARIABLE( g_variables.m_hands_chams_cover, bool ),
							                     "##hands_chams_color" );
							break;
						case 8:
							draw_chams_settings( GET_VARIABLE( g_variables.m_sleeves_chams, bool ), GET_VARIABLE( g_variables.m_sleeves_chams_material, int ),
							                     GET_VARIABLE( g_variables.m_sleeves_chams_color, c_color ), GET_VARIABLE( g_variables.m_sleeves_chams_ignorez, bool ),
							                     GET_VARIABLE( g_variables.m_sleeves_chams_wireframe, bool ), GET_VARIABLE( g_variables.m_sleeves_chams_cover, bool ),
							                     "##sleeves_chams_color" );
							break;
						}
					} );
				} );
							ImGui::SetCursorPos( ImVec2( 215.0f, -5.0f + 380.f ) );
			child( "Glow", ImVec2( 300.0f, 60.0f ), []( ) {
					checkbox( "Player glow", &GET_VARIABLE( g_variables.m_glow_enable, bool ), false, true, { 300, -1 }, []( ) {
						ImGui::Text( "Player visible glow color" );
						ImGui::ColorEdit4( "##player vis glow color", &GET_VARIABLE( g_variables.m_glow_vis_color, c_color ),
						                   color_picker_alpha_flags );
						ImGui::Spacing( );
						ImGui::Text( "Player invisible glow color" );
						ImGui::ColorEdit4( "##player invis glow color", &GET_VARIABLE( g_variables.m_glow_invis_color, c_color ),
						                   color_picker_alpha_flags );
					} );
				} );
		}
		if ( display_tab_number == 2 ) {
					ImGui::SetCursorPos( ImVec2( 215.0f, -5.0f ) );
		child( "World", ImVec2( 300.0f, 515.0f ), []( ) {
				checkbox( "Precipitation", &GET_VARIABLE( g_variables.m_precipitation, bool ), false, true, { 300, -1 }, []( ) {
					const char* precipitation_types[] = { "Rain", "Ash", "Rain Storm", "Snow" };
					popup_combo( "Type", GET_VARIABLE( g_variables.m_precipitation_type, int ), precipitation_types, 4 );
				} );
				checkbox( "Disable post processing", &GET_VARIABLE( g_variables.m_disable_post_processing, bool ) );
				checkbox( "Remove panorama blur", &GET_VARIABLE( g_variables.m_remove_panorama_blur, bool ) );
#ifdef _DEBUG
				checkbox( "Disable interp", &GET_VARIABLE( g_variables.m_disable_interp, bool ) );
#endif
				checkbox( "Fog", &GET_VARIABLE( g_variables.m_fog, bool ), false, true, { 300, 90 }, []( ) {
					ImGui::Text( "Fog color" );
					ImGui::ColorEdit4( "##fog color picker", &GET_VARIABLE( g_variables.m_fog_color, c_color ), color_picker_alpha_flags );
					popup_slider_float( "Start", &GET_VARIABLE( g_variables.m_fog_start, float ), 0.f, 5000.f, "%.0f u" );
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.0f);
					popup_slider_float( "End", &GET_VARIABLE( g_variables.m_fog_end, float ), 0.f, 5000.f, "%.0f u" );
				} );
				checkbox( "Sunset", &GET_VARIABLE( g_variables.m_sun_set, bool ), false, true, { 300, 120 }, []( ) {
					if ( ImGui::IsItemHovered( ) ) {
						ImGui::SetTooltip( "If you want restore shadows setup all on zero" );
					}
					popup_slider_float( "Sunset rot X", &GET_VARIABLE( g_variables.m_sun_set_rot_x, float ), -1000.f, 1000.f, "%.2f" );
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.0f);
					popup_slider_float( "Sunset rot Y", &GET_VARIABLE( g_variables.m_sun_set_rot_y, float ), -1000.f, 1000.f, "%.2f" );
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.0f);
					popup_slider_float( "Sunset rot Z", &GET_VARIABLE( g_variables.m_sun_set_rot_z, float ), -1000.f, 1000.f, "%.2f" );
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.0f);
					popup_slider_float( "Sunset rotation speed", &GET_VARIABLE( g_variables.m_sun_set_rotation_speed, float ), 0.0f, 1.0f, "%.2f" );
				} );
				checkbox_with_color_picker( "Custom smoke color", &GET_VARIABLE( g_variables.m_custom_smoke, bool ), false, true, { 300, -1 }, []( ) {
					ImGui::Text( "Custom smoke color" );
					popup_color_picker_button( "##custom smoke color picker", &GET_VARIABLE( g_variables.m_custom_smoke_color, c_color ) );
				}, &GET_VARIABLE( g_variables.m_custom_smoke_color, c_color ) );
				checkbox_with_color_picker( "Custom molotov color", &GET_VARIABLE( g_variables.m_custom_molotov, bool ), false, true, { 300, -1 }, []( ) {
					ImGui::Text( "Custom molotov color" );
					popup_color_picker_button( "##custom molotov color picker", &GET_VARIABLE( g_variables.m_custom_molotov_color, c_color ) );
				}, &GET_VARIABLE( g_variables.m_custom_molotov_color, c_color ) );
				checkbox_with_color_picker( "Custom blood color", &GET_VARIABLE( g_variables.m_custom_blood, bool ), false, true, { 300, -1 }, []( ) {
					ImGui::Text( "Custom blood color" );
					popup_color_picker_button( "##custom blood color picker", &GET_VARIABLE( g_variables.m_custom_blood_color, c_color ) );
				}, &GET_VARIABLE( g_variables.m_custom_blood_color, c_color ) );
				checkbox( "Custom precipitation color", &GET_VARIABLE( g_variables.m_custom_precipitation, bool ), false, true, { 300, -1 },
				          []( ) {
							  ImGui::Text( "Custom precipitation color" );
							  ImGui::ColorEdit4( "##custom precipitation color picker",
					                             &GET_VARIABLE( g_variables.m_custom_precipitation_color, c_color ), color_picker_alpha_flags );
						  } );
				checkbox( "Motion blur", &GET_VARIABLE( g_variables.m_motion_blur, bool ), false, true, { 300, 140 }, []( ) {
					popup_checkbox( "Forward enabled", &GET_VARIABLE( g_variables.m_motion_forward_move_blur, bool ) );
					popup_slider_float( "Strength", &GET_VARIABLE( g_variables.m_motion_strength, float ), 0.0f, 10.0f, "%.3f" );
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.0f);
				popup_slider_float( "Falling intensity", &GET_VARIABLE( g_variables.m_motion_falling_intensity, float ), 0.0f, 20.0f, "%.3f" );
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.0f);
				popup_slider_float( "Rotation intensity", &GET_VARIABLE( g_variables.m_motion_rotate_intensity, float ), 0.0f, 20.0f, "%.3f" );
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.0f);
				popup_slider_float( "Falling min", &GET_VARIABLE( g_variables.m_motion_falling_minimum, float ), 0.0f, 50.0f, "%.3f" );
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.0f);
				popup_slider_float( "Falling max", &GET_VARIABLE( g_variables.m_motion_falling_maximum, float ), 0.0f, 50.0f, "%.3f" );
				} );
				combo( "Skybox", GET_VARIABLE( g_variables.m_sky_box, int ), (const char**)skyboxList.data( ), skyboxList.size( ) );
				slider_float( "Aspect ratio", &GET_VARIABLE( g_variables.m_aspect_ratio, float ), 0.0f, 5.0f, "%.2f", 75.f, 46.f );
				if ( GET_VARIABLE( g_variables.m_sky_box, int ) == 23 ) {
					static char buf[ 256 ];
					ImGui::Spacing( );
					ImGui::Spacing( );
					ImGui::Spacing( );
					ImGui::InputText( "Skybox filename", buf, sizeof( buf ) );
					g_variables.m_custom_sky_box = buf;
					if ( ImGui::IsItemHovered( ) )
						ImGui::SetTooltip( "Skybox files must be put in csgo/materials/skybox/ " );
				}
				ImGui::Spacing( );
			} );
		}
		if ( display_tab_number == 3 ) {
							ImGui::SetCursorPos( ImVec2( 215.0f, -5.0f ) );
			child( "Game", ImVec2( 300.0f, 515.0f ), []( ) {
					checkbox( "Mouse fix", &GET_VARIABLE( g_variables.m_mouse_fix, bool ) );
				checkbox( "Camera fix", &GET_VARIABLE( g_variables.m_camera_fix, bool ), false, true, ImVec2( 300, -1 ), []( ) {
						popup_checkbox( "Enable smoothing", &GET_VARIABLE( g_variables.m_camera_enable_smoothing, bool ), false, false );
						if ( GET_VARIABLE( g_variables.m_camera_enable_smoothing, bool ) ) {
							popup_slider_float( "Smoothing", &GET_VARIABLE( g_variables.m_camera_smoothing, float ), 0.0f, 1.0f, "%.3f" );
						}
						popup_checkbox( "Angle fix", &GET_VARIABLE( g_variables.m_camera_angle_fix, bool ), false, false );
						popup_checkbox( "Teleport fix", &GET_VARIABLE( g_variables.m_camera_teleport_fix, bool ), false, false );
					} );
					checkbox( "Practice window", &GET_VARIABLE( g_variables.m_practice_window, bool ), false, true, ImVec2( 300, -1 ), []( ) {
						text_with_keybind( "Practice checkpoint key" );
						keybind( "practice cp key", &GET_VARIABLE( g_variables.m_practice_cp_key, key_bind_t ) );
						text_with_keybind( "Practice teleport key" );
						keybind( "practice tp key", &GET_VARIABLE( g_variables.m_practice_tp_key, key_bind_t ) );
						text_with_keybind( "Practice noclip key" );
						keybind( "practice noclip key", &GET_VARIABLE( g_variables.m_practice_noclip_key, key_bind_t ) );
						popup_checkbox( "Bold font", &GET_VARIABLE( g_variables.m_practice_window_bold_font, bool ), false, false );
						popup_checkbox( "Shadow in checkpoints", &GET_VARIABLE( g_variables.m_practice_window_shadow, bool ), false, false );
						popup_checkbox( "Custom title", &GET_VARIABLE( g_variables.m_practice_window_custom_title, bool ), false, false );
						if ( GET_VARIABLE( g_variables.m_practice_window_custom_title, bool ) ) {
							static char title_buffer_checkpoints[64] = "";
							static bool first_open_title_checkpoints = true;
							if ( first_open_title_checkpoints ) {
								strncpy_s( title_buffer_checkpoints, GET_VARIABLE( g_variables.m_practice_window_custom_title_text, std::string ).c_str( ), sizeof( title_buffer_checkpoints ) - 1 );
								first_open_title_checkpoints = false;
							}
							if ( ImGui::InputTextWithHint( "##checkpoints_custom_title", "Checkpoints", title_buffer_checkpoints, sizeof( title_buffer_checkpoints ) ) ) {
								GET_VARIABLE( g_variables.m_practice_window_custom_title_text, std::string ) = std::string( title_buffer_checkpoints );
							}
						}
						popup_checkbox( "Inner stroke outline", &GET_VARIABLE( g_variables.m_practice_window_inner_stroke, bool ), false, false );
						if ( GET_VARIABLE( g_variables.m_practice_window_inner_stroke, bool ) ) {
							popup_checkbox( "Sync with watermark outline", &GET_VARIABLE( g_variables.m_practice_window_inner_stroke_sync, bool ), false, false );
						}
						popup_checkbox( "Rounding settings", &GET_VARIABLE( g_variables.m_practice_window_rounding_enable, bool ), false, false );
						if ( GET_VARIABLE( g_variables.m_practice_window_rounding_enable, bool ) ) {
							popup_slider_float( "Rounding", &GET_VARIABLE( g_variables.m_practice_window_rounding, float ), 0.0f, 15.0f, "%.1f" );
						}
						popup_checkbox( "Outline sync with watermark", &GET_VARIABLE( g_variables.m_practice_window_outline_sync, bool ), false, false );
						if ( GET_VARIABLE( g_variables.m_practice_window_outline_sync, bool ) ) {
							popup_checkbox( "Glow", &GET_VARIABLE( g_variables.m_practice_window_glow, bool ), false, false );
							if ( GET_VARIABLE( g_variables.m_practice_window_glow, bool ) ) {
								popup_slider_int( "Glow strength", &GET_VARIABLE( g_variables.m_practice_window_glow_strength, int ), 1, 10, "%d" );
								popup_slider_int( "Glow spread", &GET_VARIABLE( g_variables.m_practice_window_glow_spread, int ), 1, 10, "%d" );
							}
						}
					} );
					const char* displayed_logs[] = { "Damage", "Team damage", "Purchase", "Votes" };
					multi_combo( "Displayed logs", GET_VARIABLE( g_variables.m_log_types, std::vector< bool > ), displayed_logs,
					             GET_VARIABLE( g_variables.m_log_types, std::vector< bool > ).size( ) );
					checkbox( "Reveal money", &GET_VARIABLE( g_variables.m_reveal_money, bool ) );
					checkbox( "Reveal suspect", &GET_VARIABLE( g_variables.m_reveal_suspect, bool ) );
					checkbox( "Reveal votes", &GET_VARIABLE( g_variables.m_reveal_votes, bool ) );
					checkbox( "Killfeed changer", &GET_VARIABLE( g_variables.m_killfeed_changer, bool ), false, true, ImVec2( 300, -1 ), []( ) {
						popup_checkbox( "Headshot", &GET_VARIABLE( g_variables.m_killfeed_headshot, bool ) );
						popup_checkbox( "Dominated", &GET_VARIABLE( g_variables.m_killfeed_dominated, bool ) );
						popup_checkbox( "Revenge", &GET_VARIABLE( g_variables.m_killfeed_revenge, bool ) );
						popup_checkbox( "Penetrated", &GET_VARIABLE( g_variables.m_killfeed_penetrated, bool ) );
						popup_checkbox( "Noscope", &GET_VARIABLE( g_variables.m_killfeed_noscope, bool ) );
						popup_checkbox( "Thrusmoke", &GET_VARIABLE( g_variables.m_killfeed_thrusmoke, bool ) );
						popup_checkbox( "Attackerblind", &GET_VARIABLE( g_variables.m_killfeed_attackerblind, bool ) );
					} );
					checkbox( "Hit sound", &GET_VARIABLE( g_variables.m_hit_sound, bool ) );
					checkbox( "Hit marker", &GET_VARIABLE( g_variables.m_hit_marker, bool ) );
					checkbox( "Scaleform", &GET_VARIABLE( g_variables.m_scaleform, bool ) );
					checkbox( "PTS System settings", &GET_VARIABLE( g_variables.m_pts_system_settings, bool ), false, true, ImVec2( 300, -1 ), []( ) {
						popup_checkbox( "PTS display", &GET_VARIABLE( g_variables.m_pts_display, bool ), false, false );
						if ( GET_VARIABLE( g_variables.m_pts_display, bool ) ) {
							popup_checkbox( "Appear animation speed", &GET_VARIABLE( g_variables.m_pts_appear_animation_speed_enable, bool ), false, false );
							if ( GET_VARIABLE( g_variables.m_pts_appear_animation_speed_enable, bool ) ) {
								popup_slider_float( "Animation speed", &GET_VARIABLE( g_variables.m_pts_appear_animation_speed, float ), 0.5f, 20.0f, "x%.2f" );
							}
						}
						if ( GET_VARIABLE( g_variables.m_pts_display, bool ) ) {
							popup_checkbox( "PTS x coordinates", &GET_VARIABLE( g_variables.m_pts_x_coordinates, bool ), false, false );
							if ( GET_VARIABLE( g_variables.m_pts_x_coordinates, bool ) ) {
								static char pts_x_buffer[16] = {};
								static bool first_open_pts_x = true;
								if ( first_open_pts_x ) {
									snprintf( pts_x_buffer, sizeof( pts_x_buffer ), "%.0f", GET_VARIABLE( g_variables.m_pts_x_offset, float ) );
									first_open_pts_x = false;
								}
								if ( ImGui::InputTextWithHint( "##pts_x_offset", "PTS x offset", pts_x_buffer, sizeof( pts_x_buffer ), ImGuiInputTextFlags_CharsDecimal ) ) {
									float value = static_cast<float>( atof( pts_x_buffer ) );
									if ( value < -500.0f ) value = -500.0f;
									if ( value > 500.0f ) value = 500.0f;
									GET_VARIABLE( g_variables.m_pts_x_offset, float ) = value;
								}
							}
						}
						popup_checkbox( "Show Picture PTS", &GET_VARIABLE( g_variables.m_pts_show_picture, bool ), false, false );
						if ( GET_VARIABLE( g_variables.m_pts_show_picture, bool ) ) {
							popup_checkbox( "Picture x coordinates", &GET_VARIABLE( g_variables.m_pts_picture_x_coordinates, bool ), false, false );
							if ( GET_VARIABLE( g_variables.m_pts_picture_x_coordinates, bool ) ) {
								static char picture_x_buffer[16] = {};
								static bool first_open_picture_x = true;
								if ( first_open_picture_x ) {
									snprintf( picture_x_buffer, sizeof( picture_x_buffer ), "%.0f", GET_VARIABLE( g_variables.m_pts_picture_x_offset, float ) );
									first_open_picture_x = false;
								}
								if ( ImGui::InputTextWithHint( "##picture_x_offset", "Picture x offset", picture_x_buffer, sizeof( picture_x_buffer ), ImGuiInputTextFlags_CharsDecimal ) ) {
									float value = static_cast<float>( atof( picture_x_buffer ) );
									if ( value < -500.0f ) value = -500.0f;
									if ( value > 500.0f ) value = 500.0f;
									GET_VARIABLE( g_variables.m_pts_picture_x_offset, float ) = value;
								}
							}
							popup_checkbox( "Bebra", &GET_VARIABLE( g_variables.m_pts_picture_bebra, bool ), false, false );
							popup_checkbox( "Bebra cool", &GET_VARIABLE( g_variables.m_pts_picture_bebra_cool, bool ), false, false );
							popup_checkbox( "Always default position", &GET_VARIABLE( g_variables.m_pts_picture_always_default, bool ), false, false );
						}
						if ( GET_VARIABLE( g_variables.m_pts_display, bool ) ) {
							popup_checkbox( "PTS speed", &GET_VARIABLE( g_variables.m_pts_speed_enable, bool ), false, false );
							if ( GET_VARIABLE( g_variables.m_pts_speed_enable, bool ) ) {
								popup_slider_float( "PTS speed factor", &GET_VARIABLE( g_variables.m_pts_speed, float ), 0.1f, 10.0f, "x%.2f" );
							}
											popup_slider_float( "PTS Size", &GET_VARIABLE( g_variables.m_pts_thin_font_size, float ), 8.0f, 64.0f, "%.1f px" );
											popup_checkbox( "Original font", &GET_VARIABLE( g_variables.m_pts_original_font, bool ), false, false );
											if ( GET_VARIABLE( g_variables.m_pts_original_font, bool ) ) {
												popup_slider_float( "Y width", &GET_VARIABLE( g_variables.m_pts_original_font_y_width, float ), -50.0f, 50.0f, "%.1f" );
											}
						}
					} );
					checkbox( "Spectator list", &GET_VARIABLE( g_variables.m_spectators_list, bool ), false, true, ImVec2( 300, -1 ), []( ) {
						popup_checkbox( "Show all players", &GET_VARIABLE( g_variables.m_spectators_show_all, bool ), false, false );
						popup_checkbox( "Animate new entries", &GET_VARIABLE( g_variables.m_spectators_animate_new_entries, bool ), false, false );
					
				
						
						popup_checkbox( "Custom title", &GET_VARIABLE( g_variables.m_spectators_custom_title, bool ), false, false );
						if ( GET_VARIABLE( g_variables.m_spectators_custom_title, bool ) ) {
							static char title_buffer[64] = "";
							static bool first_open_title = true;
							if ( first_open_title ) {
								strncpy_s( title_buffer, GET_VARIABLE( g_variables.m_spectators_custom_title_text, std::string ).c_str( ), sizeof( title_buffer ) - 1 );
								first_open_title = false;
							}
							if ( ImGui::InputTextWithHint( "##spectators_custom_title", "Spectators", title_buffer, sizeof( title_buffer ) ) ) {
								GET_VARIABLE( g_variables.m_spectators_custom_title_text, std::string ) = std::string( title_buffer );
							}
						}
						
						popup_checkbox( "Bebra GIF", &GET_VARIABLE( g_variables.m_spectators_gif_enabled, bool ), false, false );
						if ( GET_VARIABLE( g_variables.m_spectators_gif_enabled, bool ) ) {
							static const char* gif_types[] = { "Original", "Shit" };
							int& gif_type_ref = GET_VARIABLE( g_variables.m_spectators_gif_type, int );
							popup_combo( "GIF Type", gif_type_ref, gif_types, IM_ARRAYSIZE( gif_types ) );
							popup_slider_float( "GIF size", &GET_VARIABLE( g_variables.m_spectators_gif_width, float ), 32.0f, 128.0f, "%.0f px" );
						}
					} );
					checkbox( "Watermark", &GET_VARIABLE( g_variables.m_watermark, bool ), false, true, ImVec2( 300, -1 ), []( ) {
						popup_checkbox( "Watermark Outline", &GET_VARIABLE( g_variables.m_watermark_outline, bool ), false, false );
						if ( GET_VARIABLE( g_variables.m_watermark_outline, bool ) ) {
							
							popup_checkbox( "To synchronize with pts", &GET_VARIABLE( g_variables.m_watermark_outline_sync_with_pts, bool ), false, false );
							ImGui::Text( "Outline color:" );
							ImGui::ColorEdit4( "##watermark_outline_color", &GET_VARIABLE( g_variables.m_watermark_outline_color, c_color ), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar );
							
						}
						
						popup_checkbox( "Custom text", &GET_VARIABLE( g_variables.m_watermark_custom_text, bool ), false, false );
						if ( GET_VARIABLE( g_variables.m_watermark_custom_text, bool ) ) {
							static char input_buffer[21] = "";
							static bool first_open = true;
							if ( first_open ) {
								strncpy_s( input_buffer, GET_VARIABLE( g_variables.m_watermark_custom_text_value, std::string ).c_str( ), sizeof( input_buffer ) - 1 );
								first_open = false;
							}
							if ( ImGui::InputTextWithHint( "##watermark_custom_text", "type shit here...", input_buffer, sizeof( input_buffer ) ) ) {
								GET_VARIABLE( g_variables.m_watermark_custom_text_value, std::string ) = std::string( input_buffer );
							}
							
						}
					
					
						
						bool custom_text_enabled = GET_VARIABLE( g_variables.m_watermark_custom_text, bool );
					
					} );
					checkbox( "Clantag", &GET_VARIABLE( g_variables.m_clantag, bool ), false, true, ImVec2( 300, -1 ), []( ) {
						popup_checkbox( "Custom text", &GET_VARIABLE( g_variables.m_clantag_custom_text, bool ), false, false );
						if ( GET_VARIABLE( g_variables.m_clantag_custom_text, bool ) ) {
							static char buffer[64];
							std::string& custom_text = GET_VARIABLE( g_variables.m_clantag_custom_text_value, std::string );
							strncpy_s( buffer, custom_text.c_str(), sizeof(buffer) - 1 );
							buffer[sizeof(buffer) - 1] = '\0';
							if ( ImGui::InputTextWithHint( "##clantag_custom_text", "type shit here...", buffer, sizeof( buffer ) ) ) {
								custom_text = buffer;
							}
						}
						const char* animation_types[] = { "Bebra Type 1", "Bebra Type 2" };
						int& anim_type_ref = GET_VARIABLE( g_variables.m_clantag_animation_type, int );
						popup_combo( "Animation type", anim_type_ref, animation_types, 2 );
					} );
					
					checkbox( "Media player", &GET_VARIABLE( g_variables.trackdispay, bool ) );
					if ( button( "Force HUD update" ) )
						g_ctx.force_full_update = true;
#ifdef _DEBUG
#endif
				} );
		}
		if ( display_tab_number == 4 ) {
			movement_section_tab = 0;
			ImGui::SetCursorPos( ImVec2( 215.0f, 26.0f ) );
			if ( movement_section_tab == 0 )
				scrollable_child( "Main", ImVec2( 300.0f, 515.0f ), []( ) {
				checkbox( "Bunny hop", &GET_VARIABLE( g_variables.m_bunny_hop, bool ) );
				checkbox( "Fast duck", &GET_VARIABLE( g_variables.fast_quak, bool ) );	
				checkbox( "Block bot", &GET_VARIABLE( g_variables.m_blockbot, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Block bot key" );
					keybind( "block bot key", &GET_VARIABLE( g_variables.m_blockbot_key, key_bind_t ) );
					const char* key_types[] = { "Hold", "Toggle" };
					popup_combo( "Key type", GET_VARIABLE( g_variables.m_blockbot_key_type, int ), key_types, 2 );
				} );
				checkbox( "Edge jump", &GET_VARIABLE( g_variables.m_edge_jump, bool ), false, true, { 300, -1 }, []( ) {
					popup_checkbox( "On ladders", &GET_VARIABLE( g_variables.m_edge_jump_ladder, bool ) );
					text_with_keybind( "Edge jump key" );
					keybind( "edge jump key ", &GET_VARIABLE( g_variables.m_edge_jump_key, key_bind_t ) );
				} );
				checkbox( "Long jump", &GET_VARIABLE( g_variables.m_long_jump, bool ), false, true, { 300, -1 }, []( ) {
					popup_checkbox( "Block w/s while longjump", &GET_VARIABLE( g_variables.m_long_jump_block_ws, bool ) );
					text_with_keybind( "Long jump key" );
					keybind( "long jump key ", &GET_VARIABLE( g_variables.m_long_jump_key, key_bind_t ) );
				} );
				checkbox( "Fire man", &GET_VARIABLE( g_variables.m_fire_man, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Fire man key" );
					keybind( "fire man key ", &GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ) );
					popup_checkbox( "Manual jump", &GET_VARIABLE( g_variables.m_fire_man_manual_jump, bool ) );
					popup_checkbox( "+jump activates early", &GET_VARIABLE( g_variables.m_fire_man_plus_jump_activates_early, bool ) );
				} );
				checkbox( "Mini jump", &GET_VARIABLE( g_variables.m_mini_jump, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Mini jump key" );
					keybind( "mini jump key ", &GET_VARIABLE( g_variables.m_mini_jump_key, key_bind_t ) );
				} );
				checkbox( "Jump bug", &GET_VARIABLE( g_variables.m_jump_bug, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Jump bug key" );
					keybind( "jump bug key ", &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) );
				} );
				checkbox( "Edge bug", &GET_VARIABLE( g_variables.edge_bug, bool ), false, true, { 300, -1 }, []( ) {
					const char* edgebug_modes[] = { "Original", "Enchanced" };
					popup_combo( "Detection mode", GET_VARIABLE( g_variables.m_edgebug_detection_mode, int ), edgebug_modes, 2 );
					const int edgebug_mode = GET_VARIABLE( g_variables.m_edgebug_detection_mode, int );
					if ( ImGui::IsItemHovered( ) )
						ImGui::SetTooltip( "If lag in-game: net_graph 0, cl_showpos 0, lower rate" );
					text_with_keybind( "Edge bug key" );
					keybind( "edge bug key", &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) );
					popup_checkbox( "Continue after bind release", &GET_VARIABLE( g_variables.edgebug_continue_after_bind_release, bool ) );
					checkbox( "Advanced search", &GET_VARIABLE( g_variables.EdgeBugAdvanceSearch, bool ) );
					checkbox( "Silent edgebug", &GET_VARIABLE( g_variables.SiletEdgeBug, bool ) );
					checkbox( "Extra advanced search", &GET_VARIABLE( g_variables.MegaEdgeBug, bool ) );
					if ( GET_VARIABLE( g_variables.EdgeBugAdvanceSearch, bool ) ) {
						checkbox( "Autostrafe to edge", &GET_VARIABLE( g_variables.AutoStrafeEdgeBug, bool ) );
						if ( !GET_VARIABLE( g_variables.AutoStrafeEdgeBug, bool ) ) {
							popup_slider_float( "Strafe angle", &GET_VARIABLE( g_variables.deltaScaler, float ), 1.f, 179.f, "%.0f" );
							const char* delta_types[] = { "Full", "Two-fifths", "Half", "Quarter" };
							popup_combo( "Delta scale", GET_VARIABLE( g_variables.DeltaType, int ), delta_types, 4 );
						}
					}
					if ( edgebug_mode == 0 ) {
						popup_slider_int( "Edgebug ticks", &GET_VARIABLE( g_variables.EdgeBugTicks, int ), 1, 128, "%d" );
					} else {
						popup_slider_int( "Search amount", &GET_VARIABLE( g_variables.m_edgebug_enhanced_search_amount, int ), 1, 8, "%d" );
						popup_slider_float( "Search time", &GET_VARIABLE( g_variables.m_edgebug_enhanced_time, float ), 0.05f, 1.0f, "%.2fs" );
						popup_checkbox( "Strafing", &GET_VARIABLE( g_variables.m_edgebug_enhanced_strafing, bool ) );
						popup_checkbox( "Autostrafe to edge", &GET_VARIABLE( g_variables.m_edgebug_enhanced_autostrafe_to_edge, bool ) );
					}
					popup_slider_float( "Edgebug lock", &GET_VARIABLE( g_variables.EdgeBugMouseLock, float ), 0.0f, 10.0f, "%.3f" );
				} );
				checkbox( "Texture bug", &GET_VARIABLE( g_variables.m_antoha_bug, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Texture bug key" );
					keybind( "Texture bug key ", &GET_VARIABLE( g_variables.m_antoha_bug_key, key_bind_t ) );
					popup_checkbox( "Texture bug render", &GET_VARIABLE( g_variables.m_antoha_bug_wall_debug_render, bool ) );
					popup_checkbox( "No wall slow on hit", &GET_VARIABLE( g_variables.m_antoha_bug_no_wall_slow, bool ) );
					popup_checkbox( "Fix air stucks", &GET_VARIABLE( g_variables.m_fix_air_stucks, bool ) );
					popup_slider_int( "Max stamina", &GET_VARIABLE( g_variables.m_antoha_bug_max_stamina, int ), 0, 40, "%d" );
					popup_slider_float( "Wall reach", &GET_VARIABLE( g_variables.m_antoha_bug_extra_reach, float ), 0.01f, 2.f, "%.2f" );
					popup_checkbox( "Debug info", &GET_VARIABLE( g_variables.m_antoha_bug_debug, bool ) );
					popup_checkbox( "Activate near pixel jump point", &GET_VARIABLE( g_variables.m_antoha_bug_pixel_assist, bool ) );
				} );
				checkbox( "Pixeljump helper", &GET_VARIABLE( g_variables.m_antoha_bug_land, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Pixeljump helper key" );
					keybind( "Pixeljump helper key", &GET_VARIABLE( g_variables.m_antoha_bug_land_key, key_bind_t ) );
					popup_slider_int( "Max stamina", &GET_VARIABLE( g_variables.m_antoha_bug_max_stamina, int ), 0, 40, "%d" );
				} );
				checkbox( "Test", &GET_VARIABLE( g_variables.test, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Test key" );
					keybind( "Test key", &GET_VARIABLE( g_variables.test_key, key_bind_t ) );
				} );
				checkbox( "Pixel surf", &GET_VARIABLE( g_variables.m_pixel_surf, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Pixel surf key" );
					keybind( "pixel surf key ", &GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ) );
					popup_checkbox( "Pixel surf fix", &GET_VARIABLE( g_variables.m_pixel_surf_fix, bool ) );
					popup_checkbox( "Fix air stucks", &GET_VARIABLE( g_variables.m_fix_air_stucks, bool ) );
					popup_checkbox( "Pixelsurf line render", &GET_VARIABLE( g_variables.m_pixel_surf_line_render, bool ) );
					if ( GET_VARIABLE( g_variables.m_pixel_surf_line_render, bool ) ) {
						ImGui::Text( "Line color" );
						popup_color_picker_button( "##pixelsurf line color", &GET_VARIABLE( g_variables.m_pixel_surf_line_color, c_color ) );
						popup_slider_float( "Line thickness", &GET_VARIABLE( g_variables.m_pixel_surf_line_thickness, float ), 1.0f, 5.0f, "%.1f" );
						popup_checkbox( "Show only when pixelsurf", &GET_VARIABLE( g_variables.m_pixel_surf_line_show_only_active, bool ) );
						popup_checkbox( "Full trajectory", &GET_VARIABLE( g_variables.m_pixel_surf_line_full_trajectory, bool ) );
						if ( button( "Clear all lines" ) ) {
							g_movement.m_pixelsurf_data.m_trajectory_points.clear( );
							g_movement.m_pixelsurf_data.m_trajectory_segments.clear( );
						}
						popup_checkbox( "Delete mode", &g_movement.m_pixelsurf_data.m_delete_mode );
						if ( g_movement.m_pixelsurf_data.m_delete_mode ) {
							ImGui::TextWrapped( "Click on line to delete segment" );
						}
					}
					popup_checkbox( "Auto align", &GET_VARIABLE( g_variables.m_auto_align, bool ) );
					if ( GET_VARIABLE( g_variables.m_auto_align, bool ) ) {
						const char* align_types[] = { "Original", "LB Recode", "LB Original" };
						popup_combo( "Align type", GET_VARIABLE( g_variables.m_auto_align_type, int ), align_types, 3 );
						if ( GET_VARIABLE( g_variables.m_auto_align_type, int ) == 1 ) {
							popup_slider_int( "Predict ticks", &GET_VARIABLE( g_variables.m_auto_align_predict_ticks, int ), 1, 12, "%d" );
						}
					}
				} );
checkbox( "Ladder glide jump", &GET_VARIABLE( g_variables.m_ladder_bug, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Ladder glide jump key" );
					keybind( "ladder bug key ", &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) );
					popup_checkbox( "+jetpack", &GET_VARIABLE( g_variables.m_ladder_glide_jetpack, bool ), false, false );
					popup_checkbox( "Podlet (jump impulse)", &GET_VARIABLE( g_variables.m_podlet, bool ), false, false );
				} );
				checkbox( "Ladder Freelook Climb", &GET_VARIABLE( g_variables.m_ladder_freelook_climb, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Ladder Freelook Climb key" );
					keybind( "ladder freelook climb key ", &GET_VARIABLE( g_variables.m_ladder_freelook_climb_key, key_bind_t ) );
				} );
				
				checkbox( "Wall stuck", &GET_VARIABLE( g_variables.m_wall_stuck, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Wall stuck Key" );
					keybind( "##wall stuck key ", &GET_VARIABLE( g_variables.m_wall_stuck_key, key_bind_t ) );
					popup_checkbox( "Use movement fix", &GET_VARIABLE( g_variables.m_movement_fix, bool ) );
					popup_checkbox( "Display screen", &GET_VARIABLE( g_variables.m_wall_stuck_display_screen, bool ) );
					popup_checkbox( "Show Window", &GET_VARIABLE( g_variables.m_air_stuck_debug_window, bool ) );
				} );
				checkbox( "Wall kiss", &GET_VARIABLE( g_variables.m_air_stuck, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Wall kiss Key" );
					keybind( "##wall kiss key ", &GET_VARIABLE( g_variables.m_air_stuck_key, key_bind_t ) );
					popup_checkbox( "Use movement fix", &GET_VARIABLE( g_variables.m_movement_fix, bool ) );
					popup_checkbox( "Display screen", &GET_VARIABLE( g_variables.m_air_stuck_display_screen, bool ) );
					popup_checkbox( "Show Window", &GET_VARIABLE( g_variables.m_air_stuck_debug_window, bool ) );
				} );
				checkbox( "Lirili larila", &GET_VARIABLE( g_variables.m_lirili_larila, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Lirili larila key" );
					keybind( "##lirili larila key", &GET_VARIABLE( g_variables.m_lirili_larila_key, key_bind_t ) );
					popup_slider_float( "Assist render height", &GET_VARIABLE( g_variables.m_pixel_surf_assist_render_height, float ), -100.f, 100.f,
					                    "%.1f" );
				} );
				checkbox( "Autostrafe", &GET_VARIABLE( g_variables.m_auto_strafe, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Autostrafe Key" );
					keybind( "##autostrafe key ", &GET_VARIABLE( g_variables.m_auto_strafe_key, key_bind_t ) );
				} );
				checkbox( "Auto Crouch", &GET_VARIABLE( g_variables.m_auto_crouch, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Auto Crouch Key" );
					keybind( "##auto_crouch_key", &GET_VARIABLE( g_variables.m_auto_crouch_key, key_bind_t ) );
					popup_slider_int( "Ticks", &GET_VARIABLE( g_variables.m_auto_crouch_ticks, int ), 1, 20, "%d" );
					popup_checkbox( "No crouch cooldown", &GET_VARIABLE( g_variables.m_no_crouch_cooldown, bool ), false, false );
				} );
				checkbox( "Pixelsurf Calculator", &GET_VARIABLE( g_variables.m_pixel_calc, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Set point Key" );
					keybind( "##pixel_calc_aim", &GET_VARIABLE( g_variables.m_pixel_calc_aim_key, key_bind_t ) );
					text_with_keybind( "Calculate Key" );
					keybind( "##pixel_calc_solve", &GET_VARIABLE( g_variables.m_pixel_calc_solve_key, key_bind_t ) );
					popup_checkbox( "Show Point", &GET_VARIABLE( g_variables.m_pixel_calc_show_point, bool ) );
				} );
			} );
			if ( movement_section_tab == 0 ) {
				ImGui::SetCursorPos( ImVec2( 215.0f + 315.f, 26.0f ) );
				child( "Indicators", ImVec2( 300.0f, 180.0f ), []( ) {
				checkbox( "Velocity Indicator", &GET_VARIABLE( g_variables.m_velocity_indicator, bool ), false, true, { 300, -1 }, []( ) {
					popup_checkbox( "Italic font", &GET_VARIABLE( g_variables.m_velocity_indicator_italic_font, bool ), false, false );
					popup_checkbox( "Show Pre Speed", &GET_VARIABLE( g_variables.m_velocity_indicator_show_pre_speed, bool ) );
					if ( GET_VARIABLE( g_variables.m_velocity_indicator_show_pre_speed, bool ) ) {
						popup_checkbox( "Without new animation", &GET_VARIABLE( g_variables.m_velocity_indicator_without_animation, bool ), false,
						                false );
						popup_checkbox( "Reverse animation when disappearing",
						                &GET_VARIABLE( g_variables.m_velocity_indicator_reverse_disappear_animation, bool ), false, false );
					}
					popup_checkbox( "Fade Alpha", &GET_VARIABLE( g_variables.m_velocity_indicator_fade_alpha, bool ) );
					popup_slider_int( "Padding", &GET_VARIABLE( g_variables.m_velocity_indicator_padding, int ), 5, 100, "%d%%" );
					popup_checkbox( "Orbs in velocity", &GET_VARIABLE( g_variables.m_velocity_indicator_orbs, bool ), false, false );
					if ( GET_VARIABLE( g_variables.m_velocity_indicator_orbs, bool ) )
						popup_slider_float( "Orbs survival time", &GET_VARIABLE( g_variables.m_velocity_orbs_survival_time, float ), 0.2f, 5.0f,
						                    "%.2f s" );
					popup_checkbox( "Ease out velocity", &GET_VARIABLE( g_variables.m_velocity_indicator_ease_out, bool ), false, false );
					if ( GET_VARIABLE( g_variables.m_velocity_indicator_ease_out, bool ) ) {
						popup_slider_float( "Ease out speed", &GET_VARIABLE( g_variables.m_velocity_indicator_ease_out_speed, float ), 10.0f, 100.0f,
						                    "%.1f" );
					}
					popup_checkbox( "Custom shadow color", &GET_VARIABLE( g_variables.m_velocity_indicator_custom_shadow_color, bool ), false,
					                false );
					if ( GET_VARIABLE( g_variables.m_velocity_indicator_custom_shadow_color, bool ) ) {
						ImGui::Text( "Custom color:" );
						ImGui::ColorEdit4( "##velocity_shadow_color", &GET_VARIABLE( g_variables.m_velocity_indicator_shadow_color, c_color ),
						                   ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar );
					}
				} );

				checkbox( "Keybind Indicators", &GET_VARIABLE( g_variables.m_key_indicators_enable, bool ), false, true, { 300, 560 }, []( ) {
					popup_checkbox( "Italic font", &GET_VARIABLE( g_variables.m_keybind_indicators_italic_font, bool ), false, false );
					if ( !GET_VARIABLE( g_variables.m_key_indicators_lock_to_addwindow, bool ) )
						popup_slider_int( "Position", &GET_VARIABLE( g_variables.m_key_indicators_position, int ), 30, g_ctx.m_height, "%d" );
					auto& displayed_keybinds = GET_VARIABLE( g_variables.m_key_indicators, std::vector< bool > );
					if ( displayed_keybinds.size( ) < e_keybind_indicators::key_max )
						displayed_keybinds.resize( e_keybind_indicators::key_max, false );
					std::vector< bool > visible_keybinds = {
						displayed_keybinds[ e_keybind_indicators::key_eb ],
						displayed_keybinds[ e_keybind_indicators::key_ps ],
						displayed_keybinds[ e_keybind_indicators::key_lj ],
						displayed_keybinds[ e_keybind_indicators::key_mj ],
						displayed_keybinds[ e_keybind_indicators::key_jb ],
						displayed_keybinds[ e_keybind_indicators::key_ast ],
						displayed_keybinds[ e_keybind_indicators::key_bast ],
						displayed_keybinds[ e_keybind_indicators::key_lb ],
						displayed_keybinds[ e_keybind_indicators::key_air ],
						displayed_keybinds[ e_keybind_indicators::key_ws ],
						displayed_keybinds[ e_keybind_indicators::key_tb ],
						displayed_keybinds[ e_keybind_indicators::key_ac ],
						displayed_keybinds[ e_keybind_indicators::key_lg ],
						displayed_keybinds[ e_keybind_indicators::key_wc ],
						displayed_keybinds[ e_keybind_indicators::key_lirili ],
						displayed_keybinds[ e_keybind_indicators::key_tp ],
						displayed_keybinds[ e_keybind_indicators::key_god ]
					};
					const char* keybind_indicators[] = { "Edgebug", "Pixelsurf", "Longjump", "Minijump", "Jumpbug", "Assistants", "Bounce Assist", "Ladder glide jump", "Wall kiss", "Wall stuck", "Texture bug", "Auto Crouch", "Ladder Glide", "Wall climb", "Lirili larila", "Thirdperson", "God mode" };
					popup_multi_combo( "Displayed Keybinds", visible_keybinds, keybind_indicators, static_cast<int>( visible_keybinds.size( ) ) );
					displayed_keybinds[ e_keybind_indicators::key_eb ] = visible_keybinds[ 0 ];
					displayed_keybinds[ e_keybind_indicators::key_ps ] = visible_keybinds[ 1 ];
					displayed_keybinds[ e_keybind_indicators::key_lj ] = visible_keybinds[ 2 ];
					displayed_keybinds[ e_keybind_indicators::key_mj ] = visible_keybinds[ 3 ];
					displayed_keybinds[ e_keybind_indicators::key_jb ] = visible_keybinds[ 4 ];
					displayed_keybinds[ e_keybind_indicators::key_ast ] = visible_keybinds[ 5 ];
					displayed_keybinds[ e_keybind_indicators::key_bast ] = visible_keybinds[ 6 ];
					displayed_keybinds[ e_keybind_indicators::key_lb ] = visible_keybinds[ 7 ];
					displayed_keybinds[ e_keybind_indicators::key_air ] = visible_keybinds[ 8 ];
					displayed_keybinds[ e_keybind_indicators::key_ws ] = visible_keybinds[ 9 ];
					displayed_keybinds[ e_keybind_indicators::key_tb ] = visible_keybinds[ 10 ];
					displayed_keybinds[ e_keybind_indicators::key_ac ] = visible_keybinds[ 11 ];
					displayed_keybinds[ e_keybind_indicators::key_lg ] = visible_keybinds[ 12 ];
					displayed_keybinds[ e_keybind_indicators::key_wc ] = visible_keybinds[ 13 ];
					displayed_keybinds[ e_keybind_indicators::key_lirili ] = visible_keybinds[ 14 ];
					displayed_keybinds[ e_keybind_indicators::key_tp ] = visible_keybinds[ 15 ];
					displayed_keybinds[ e_keybind_indicators::key_god ] = visible_keybinds[ 16 ];
					const char* ind_size[] = { "Default", "Small" };
					popup_combo( "Keybind Indicator Size", GET_VARIABLE( g_variables.m_key_indicators_size, int ), ind_size, 2 );
					popup_checkbox( "Custom shadow color", &GET_VARIABLE( g_variables.m_indicators_custom_shadow_color, bool ), false, false );
					if ( GET_VARIABLE( g_variables.m_indicators_custom_shadow_color, bool ) ) {
						ImGui::Text( "Custom color:" );
						ImGui::ColorEdit4( "##indicators_shadow_color", &GET_VARIABLE( g_variables.m_indicators_shadow_color, c_color ),
						                   ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar );
					}
					popup_checkbox( "Custom detect color", &GET_VARIABLE( g_variables.m_indicators_custom_detect_color, bool ), false, false );
					if ( GET_VARIABLE( g_variables.m_indicators_custom_detect_color, bool ) ) {
						ImGui::Text( "Detect color:" );
						ImGui::ColorEdit4( "##indicators_detect_color", &GET_VARIABLE( g_variables.m_indicators_detect_color, c_color ),
						                   ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar );
					}
					popup_checkbox( "Allow color detection", &GET_VARIABLE( g_variables.m_indicators_allow_color_detection, bool ), false, false );
					
				} );

				// ========================================== //
				// ������ DETECTIONS                          //
				// ========================================== //
				checkbox( "Detections", &GET_VARIABLE( g_variables.m_detections, bool ), false, true, { 300, -1 }, []( ) {
					popup_checkbox( "Pixelsurf", &GET_VARIABLE( g_variables.m_pixelsurf_detect, bool ), false, false );
					popup_checkbox( "Airstuck", &GET_VARIABLE( g_variables.m_detect_air_stuck, bool ), false, false );
					popup_checkbox( "Edgebug", &GET_VARIABLE( g_variables.m_detect_edgebug, bool ), false, false );
					// popup_checkbox( "Jumpbug", &GET_VARIABLE( g_variables.m_jumpbug_detect, bool ), false, false );
					// popup_checkbox( "Longjump", &GET_VARIABLE( g_variables.m_longjump_detect, bool ), false, false );
					// popup_checkbox( "Minijump", &GET_VARIABLE( g_variables.m_minijump_detect, bool ), false, false );
				} );
				} );

				ImGui::SetCursorPos( ImVec2( 215.0f + 315.f, 216.0f ) );
				child( "Keybind Animation", ImVec2( 300.0f, 310.0f ), []( ) {
					const char* keybind_animation_modes[] = { "Animated", "Static" };
					int keybind_animation_mode = GET_VARIABLE( g_variables.m_indicators_without_animation, bool ) ? 1 : 0;
					combo( "Mode", keybind_animation_mode, keybind_animation_modes, 2 );
					GET_VARIABLE( g_variables.m_indicators_without_animation, bool ) = keybind_animation_mode == 1;
					checkbox( "Lock to addwindow", &GET_VARIABLE( g_variables.m_key_indicators_lock_to_addwindow, bool ) );
					slider_float( "Fade speed", &GET_VARIABLE( g_variables.m_key_indicators_show_lerp, float ), 0.5f, 20.0f, "%.2f", 75.f, 36.f );
					GET_VARIABLE( g_variables.m_key_indicators_hide_lerp, float ) = GET_VARIABLE( g_variables.m_key_indicators_show_lerp, float );
					slider_int( "Move speed", &GET_VARIABLE( g_variables.m_key_indicators_shift_lerp, int ), 50, 3000, "%d px/s", 75.f, 36.f );
					slider_int( "Shift speed", &GET_VARIABLE( g_variables.m_key_indicators_reposition_lerp, int ), 50, 3000, "%d px/s", 75.f, 36.f );
					slider_int( "Spacing", &GET_VARIABLE( g_variables.m_key_indicators_spacing, int ), 0, 64, "%d px", 75.f, 36.f );
					slider_int( "Remove distance", &GET_VARIABLE( g_variables.m_key_indicators_remove_distance, int ), 0, 400, "%d px/s", 75.f, 36.f );
					slider_int( "Overlap", &GET_VARIABLE( g_variables.m_key_indicators_overlap, int ), -6, 6, "%d px", 75.f, 36.f );
				} );
			}
		}
		
		if ( display_tab_number == 5 ) {
			ImGui::SetCursorPos( ImVec2( 215.0f, -5.0f ) );
			child( "Assist", ImVec2( 300.0f, 515.0f ), []( ) {
				checkbox( "Assistants", &GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ) );
				checkbox( "Stamina hop", &GET_VARIABLE( g_variables.m_pixel_surf_assist_brokehop, bool ) );
				checkbox( "Render pixelsurf assist", &GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) );
				checkbox( "Bounce assistant", &GET_VARIABLE( g_variables.m_bouncee_assist, bool ) );
				checkbox( "Bounce stamina hop", &GET_VARIABLE( g_variables.m_bouncee_assist_brokehop, bool ) );
				checkbox( "Render bounce assist", &GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) );
				text_with_keybind( "Assist key:" );
				keybind( "assist key", &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) );
				text_with_keybind( "Pixel finder:" );
				keybind( "pixel finder key", &GET_VARIABLE( g_variables.m_pixel_finder_key, key_bind_t ) );
				slider_int( "Scan ticks", &GET_VARIABLE( g_variables.m_pixel_surf_assist_ticks, int ), 1, 64, "%d", 75.f, 36.f );
				text_with_keybind( "Bug point:" );
				keybind( "bounce assistant point key", &GET_VARIABLE( g_variables.m_bounce_assist_point_key, key_bind_t ) );
				text_with_keybind( "Free point:" );
				keybind( "free point key", &GET_VARIABLE( g_variables.m_free_point_key, key_bind_t ) );
				key_bind_t delete_point_key;
				delete_point_key.m_key = VK_BACK;
				text_with_keybind( "Delete point:" );
				keybind( "delete point key", &delete_point_key, false );
				slider_int( "Prediction ticks (points)", &GET_VARIABLE( g_variables.m_pixel_surf_assist_ground_prediction_ticks, int ), 1, 64, "%d", 75.f, 36.f );
			} );
		}

		if ( display_tab_number == 6 ) {
			ImGui::SetCursorPos( ImVec2( 215.0f, 26.0f ) );
			child( "Style", ImVec2( 300.0f, 515.0f ), []( ) {
				checkbox( "LESTER MODE", &GET_VARIABLE( g_variables.m_lester_mode, bool ) );
			} );
		}

		if ( display_tab_number == 7 ) {
			static std::string selected_config_name = { };

			ImGui::SetCursorPos( ImVec2( 215.0f, -5.0f ) );
			child( "Settings", ImVec2( 295.0f, 90.0f ), []( ) {
				if ( button( "Refresh", ImVec2( 275.f, 22.f ) ) ) {
					g_config.refresh( );
				}
			} );

			ImGui::SetCursorPos( ImVec2( 215.0f + 310.f, -5.0f ) );
			child( "Create Config", ImVec2( 305.0f, 90.0f ), []( ) {
				ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.6f, 0.6f, 0.6f, 1.f ) );
				ImGui::Text( "Config name" );
				ImGui::PopStyleColor( );
				ImGui::SameLine( );
				ImGui::SetCursorPosX( 130.f );
				input_text( "##cfg_name", g_menu.m_config_file, g_menu.m_config_file, sizeof( g_menu.m_config_file ), 150.f, 22.f, NULL );
				ImGui::SetCursorPosY( ImGui::GetCursorPosY( ) - 5.0f );
				std::string converted_file_name = g_menu.m_config_file;
				if ( button( "Create", ImVec2( 285.f, 22.f ) ) ) {
					if ( !converted_file_name.empty( ) ) {
						if ( !g_config.save( converted_file_name ) )
							g_console.print( std::vformat( "failed to create {:s}", std::make_format_args( converted_file_name ) ).c_str( ) );
						g_menu.m_config_file[0] = '\0';
						g_config.refresh( );
					}
				}
			} );

			{
				ImGui::SetCursorPos( ImVec2( 215.0f, 115.0f ) );
				ImVec2 list_origin = ImGui::GetWindowPos( ) + ImVec2( 215.0f, 115.0f );
				ImVec2 list_size( 615.0f, 400.0f );

				auto* draw_list = ImGui::GetWindowDrawList( );
				auto* text_font = g_render.m_custom_fonts[e_custom_font_names::custom_font_name_googlesans_addwindow];
				float font_size = text_font->FontSize;

				selected_config_name = !g_config.m_file_names.empty( ) ? g_config.m_file_names[ g_menu.m_selected_config ] : "";

				constexpr float item_h = 30.0f;
				constexpr float item_pad = 4.0f;
				constexpr float btn_w = 40.0f;
				constexpr float btn_h = 20.0f;
				constexpr float btn_spacing = 6.0f;

				int total_items = (int)g_config.m_file_names.size( );
				static float cfg_scroll = 0.0f;
				int max_visible = (int)( list_size.y / ( item_h + item_pad ) );
				int max_scroll = std::max( 0, total_items - max_visible );

				if ( ImGui::IsMouseHoveringRect( list_origin, list_origin + list_size ) ) {
					float wheel = ImGui::GetIO( ).MouseWheel;
					cfg_scroll -= wheel;
					cfg_scroll = std::clamp( cfg_scroll, 0.0f, (float)max_scroll );
				}

				int start_idx = (int)cfg_scroll;
				for ( int vi = 0; vi < max_visible && ( start_idx + vi ) < total_items; vi++ ) {
					int idx = start_idx + vi;
					ImVec2 row_min = list_origin + ImVec2( 0, vi * ( item_h + item_pad ) );
					ImVec2 row_max = row_min + ImVec2( list_size.x, item_h );

					bool is_selected = ( g_menu.m_selected_config == idx );
					bool is_hovered = ImGui::IsMouseHoveringRect( row_min, row_max );

					if ( is_hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
						g_menu.m_selected_config = idx;
						is_selected = true;
					}

					(void)is_selected;

					draw_list->AddRectFilled( row_min, row_max, ImColor( 16, 16, 16, (int)( 255 * g_menu.m_animation_progress ) ), 4.0f );
					draw_list->AddRect( row_min, row_max, ImColor( 11, 11, 11, (int)( 255 * g_menu.m_animation_progress ) ), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f );

					ImVec2 text_pos = row_min + ImVec2( 12, ( item_h - font_size ) * 0.5f );
					draw_list->AddText( text_font, font_size, text_pos,
						ImColor( 200, 200, 200, 255 ),
						g_config.m_file_names[idx].c_str( ) );

					float btns_total = btn_w * 3 + btn_spacing * 2;
					float btns_x = row_max.x - btns_total - 12.0f;
					float btn_y = row_min.y + ( item_h - btn_h ) * 0.5f;

					auto draw_inline_btn = [&]( const char* label, float x ) -> bool {
						ImVec2 bmin( x, btn_y );
						ImVec2 bmax( x + btn_w, btn_y + btn_h );
						bool bh = ImGui::IsMouseHoveringRect( bmin, bmax );
						bool bp = bh && ImGui::IsMouseClicked( ImGuiMouseButton_Left );

						draw_list->AddRect( bmin, bmax, ImColor( 33, 33, 33, (int)( 255 * g_menu.m_animation_progress ) ), 3.0f, 0, 1.0f );

						ImVec2 ts = text_font->CalcTextSizeA( font_size, FLT_MAX, 0.f, label );
						ImVec2 tp( bmin.x + ( btn_w - ts.x ) * 0.5f, bmin.y + ( btn_h - ts.y ) * 0.5f );
						draw_list->AddText( text_font, font_size, tp,
							bh ? ImColor( 255, 255, 255, 255 ) : ImColor( 180, 180, 180, 255 ), label );
						return bp;
					};

					if ( draw_inline_btn( "Load", btns_x ) ) {
						std::string cfg_name = g_config.m_file_names[idx];
						g_menu.m_selected_config = idx;
					if ( g_config.load( cfg_name ) ) {
						g_render.m_reload_fonts = true;
					}
					movement_add_window( 5, std::vformat( "Config {} loaded.", std::make_format_args( cfg_name ) ) );
				}
					if ( draw_inline_btn( "Save", btns_x + btn_w + btn_spacing ) ) {
					std::string cfg_name = g_config.m_file_names[idx];
					g_menu.m_selected_config = idx;
					g_config.save( cfg_name );
					movement_add_window( 5, std::vformat( "Config {} saved.", std::make_format_args( cfg_name ) ) );
				}
					if ( draw_inline_btn( "Delete", btns_x + ( btn_w + btn_spacing ) * 2 ) ) {
						std::string cfg_name = g_config.m_file_names[idx];
						g_config.remove( idx );
						if ( g_menu.m_selected_config >= (int)g_config.m_file_names.size( ) )
							g_menu.m_selected_config = std::max( 0, (int)g_config.m_file_names.size( ) - 1 );
						movement_add_window( 5, std::vformat( "Config {} deleted.", std::make_format_args( cfg_name ) ) );
					}
				}
			}
		}
		if ( display_tab_number == 8 ) {
			content_subtabs( { { "Players", 0 }, { "World", 1 }, { "Local", 2 }, { "Utils", 3 } }, server_section_tab, server_section_tab );
			ImGui::SetCursorPos( ImVec2( 215.0f, 26.0f ) );
			child( "Server", ImVec2( 615.0f, 484.0f ), []( ) {
				struct player_entry_t {
					int index;
					std::string real_name;
					std::string display_name;
					int team;
					bool is_alive;
				};

				std::vector< player_entry_t > server_players;
				const int max_clients = g_interfaces.m_engine_client ? g_interfaces.m_engine_client->get_max_clients( ) : 64;
				for ( int i = 1; i <= max_clients; ++i ) {
					if ( i == g_interfaces.m_engine_client->get_local_player( ) )
						continue;

					auto entity = g_interfaces.m_client_entity_list->get< c_base_entity >( i );
					if ( !entity )
						continue;

					player_info_t info{ };
					if ( !g_interfaces.m_engine_client->get_player_info( i, &info ) )
						continue;

					std::string display_name = g_menu.get_server_player_name( i, info.m_name );
					std::string alias_name = "";
					if ( !alias_name.empty( ) )
						display_name += " (" + alias_name + ")";
					server_players.push_back( { i, std::string( info.m_name ), display_name, entity->get_team( ), entity->is_alive( ) } );
				}

				if ( g_menu.m_selected_server_player == -1 && !server_players.empty( ) )
					g_menu.m_selected_server_player = server_players.front( ).index;

				n_menu::server_player_settings_t* selected_settings = g_menu.find_server_player_settings( g_menu.m_selected_server_player, true );
				player_info_t selected_player_info{ };
				bool has_selected_player_info = g_interfaces.m_engine_client && g_interfaces.m_engine_client->get_player_info( g_menu.m_selected_server_player, &selected_player_info );
				if ( selected_settings ) {
					strncpy_s( g_menu.m_server_custom_name_buffer.data( ), g_menu.m_server_custom_name_buffer.size( ), selected_settings->m_custom_name.c_str( ), _TRUNCATE );
					if ( has_selected_player_info ) {
						std::string alias_name = "";
						strncpy_s( g_menu.m_server_alias_buffer.data( ), g_menu.m_server_alias_buffer.size( ), alias_name.c_str( ), _TRUNCATE );
					}
				}

				if ( server_section_tab == 0 ) {
				ImGui::SetCursorPos( ImVec2( 8.0f, 8.0f ) );
				scrollable_child( "Server Players", ImVec2( 285.0f, 458.0f ), [ & ]( ) {
					const float item_height = ImGui::GetTextLineHeightWithSpacing( ) + 6.0f;
					for ( const auto& player : server_players ) {
						bool selected = player.index == g_menu.m_selected_server_player;
						ImVec2 cursor = ImGui::GetCursorScreenPos( );
						ImVec2 item_size( 250.0f, item_height );
						ImRect item_rect( cursor, cursor + item_size );
						bool hovered = ImGui::IsMouseHoveringRect( item_rect.Min, item_rect.Max );
						ImGui::GetWindowDrawList( )->AddRectFilled( item_rect.Min, item_rect.Max, selected ? ImColor( 24, 24, 24, 255 ) : hovered ? ImColor( 19, 19, 19, 255 ) : ImColor( 15, 15, 15, 210 ), 4.0f, ImDrawFlags_RoundCornersAll );
						ImGui::GetWindowDrawList( )->AddRect( item_rect.Min, item_rect.Max, selected ? ImColor( 255, 255, 255, 20 ) : ImColor( 33, 33, 33, 255 ), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f );
						const char* team_text = "SPEC";
						if ( player.team == e_team_id::team_ct )
							team_text = "CT";
						else if ( player.team == e_team_id::team_tt )
							team_text = "T";
						const char* life_text = player.is_alive ? "Alive" : "Dead";
						std::string state_text = std::string( team_text ) + ", " + life_text;
						ImGui::GetWindowDrawList( )->AddText( g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ], g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ]->FontSize, item_rect.Min + ImVec2( 10.0f, 3.0f ), ImColor( 225, 225, 225, 255 ), player.display_name.c_str( ) );
						ImVec2 state_size = ImGui::CalcTextSize( state_text.c_str( ) );
						ImGui::GetWindowDrawList( )->AddText( g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ],
						                                    g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ]->FontSize,
						                                    ImVec2( item_rect.Max.x - state_size.x - 10.0f, item_rect.Min.y + 3.0f ),
						                                    ImColor( 150, 150, 150, 255 ), state_text.c_str( ) );
						ImGui::InvisibleButton( ( "##server_player_" + std::to_string( player.index ) ).c_str( ), item_size );
						if ( ImGui::IsItemClicked( ) ) {
							g_menu.m_selected_server_player = player.index;
							selected_settings = g_menu.find_server_player_settings( player.index, true );
							if ( selected_settings ) {
								strncpy_s( g_menu.m_server_custom_name_buffer.data( ), g_menu.m_server_custom_name_buffer.size( ), selected_settings->m_custom_name.c_str( ), _TRUNCATE );
								player_info_t clicked_player_info{ };
								if ( g_interfaces.m_engine_client->get_player_info( player.index, &clicked_player_info ) ) {
									std::string alias_name = "";
									strncpy_s( g_menu.m_server_alias_buffer.data( ), g_menu.m_server_alias_buffer.size( ), alias_name.c_str( ), _TRUNCATE );
								}
							}
						}
					}
				} );

				ImGui::SetCursorPos( ImVec2( 300.0f, 8.0f ) );
				child( "Server Actions", ImVec2( 300.0f, 458.0f ), [ & ]( ) {
					if ( !selected_settings ) {
						ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.7f, 1.0f ), "Select player" );
						return;
					}

					ImGui::SetNextItemWidth( 280.f );
					ImGui::InputTextWithHint( "##server_custom_name", "Custom Name", g_menu.m_server_custom_name_buffer.data( ), g_menu.m_server_custom_name_buffer.size( ) );
					selected_settings->m_custom_name = g_menu.m_server_custom_name_buffer.data( );
					ImGui::Spacing( );
					if ( smbutton_hh( "Player Actions" ) )
						ImGui::OpenPopup( "##server_player_actions_popup" );
					checkbox( "Whitelist", &selected_settings->m_whitelist, false, true, { 280, -1 }, [ & ]( ) {
						const bool was_whitelisted = selected_settings->m_whitelist;
						if ( was_whitelisted && !selected_settings->m_whitelist ) {
							selected_settings->m_whitelist_visuals = false;
							selected_settings->m_whitelist_aimbot = false;
						}
						popup_checkbox( "Visuals", &selected_settings->m_whitelist_visuals );
						popup_checkbox( "Aimbot", &selected_settings->m_whitelist_aimbot );
					} );

					ImGui::SetNextWindowSize( ImVec2( 300.0f, 132.0f ) );
					if ( ImGui::BeginPopup( "##server_player_actions_popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) ) {
						ImVec2 pos = ImGui::GetWindowPos( );
						ImVec2 size = ImGui::GetWindowSize( );
						ImGui::GetWindowDrawList( )->AddRectFilled( pos, pos + size, ImColor( 13, 13, 13, 255 ), 12.0f );
						ImGui::GetWindowDrawList( )->AddRect( pos, pos + size, ImColor( 33, 33, 33, 255 ), 12.0f, 0, 1.0f );
						popup_checkbox( "Auto respawn", &selected_settings->m_auto_respawn );
						const char* team_types[] = { "None", "CT", "T", "SPEC" };
						popup_combo( "Team", selected_settings->m_team_action, team_types, 4 );
						popup_checkbox( "Target Switch", &selected_settings->m_target_switch );
						ImGui::EndPopup( );
					}
				} );
				}

				if ( server_section_tab == 1 ) {
					ImGui::SetCursorPos( ImVec2( 8.0f, 8.0f ) );
					child( "World", ImVec2( 300.0f, 458.0f ), [ & ]( ) {
						checkbox( "Auto respawn", &GET_VARIABLE( g_variables.m_auto_respawn, bool ) );
						{
							bool& god = GET_VARIABLE( g_variables.m_god_mode, bool );
							bool prev_god = god;
							checkbox( "God mode", &god );
							if ( god != prev_god && g_interfaces.m_engine_client ) {
								if ( god )
									g_interfaces.m_engine_client->execute_client_cmd( "sm_god @me 1" );
								else
									g_interfaces.m_engine_client->execute_client_cmd( "sm_god @me 0" );
							}
						}
						checkbox( "Detect illegal airstuck", &GET_VARIABLE( g_variables.m_server_detect_illegal_airstuck, bool ) );
						checkbox( "LESTER", &GET_VARIABLE( g_variables.m_lester_kill_notify, bool ), false, true, { 300, 260 }, []( ) {
							popup_slider_float( "Total time", &GET_VARIABLE( g_variables.m_lester_notify_duration, float ), 0.25f, 15.f, "%.2f s" );
							popup_slider_float( "Sound time", &GET_VARIABLE( g_variables.m_lester_sound_duration, float ), 0.f, 15.f, "%.2f s (0 = full)" );
							popup_slider_float( "Pos X", &GET_VARIABLE( g_variables.m_lester_pos_offset_x, float ), -400.f, 400.f, "%.0f px" );
							popup_slider_float( "Pos Y", &GET_VARIABLE( g_variables.m_lester_pos_offset_y, float ), -400.f, 400.f, "%.0f px" );
							popup_slider_float( "Slide in", &GET_VARIABLE( g_variables.m_lester_slide_in, float ), 0.02f, 3.f, "%.2f s" );
							popup_slider_float( "Slide out", &GET_VARIABLE( g_variables.m_lester_slide_out, float ), 0.02f, 3.f, "%.2f s" );
							popup_slider_float( "Image height", &GET_VARIABLE( g_variables.m_lester_image_max_height_frac, float ), 0.12f, 0.85f,
							                    "%.2f of screen" );
						} );
					} );

					ImGui::SetCursorPos( ImVec2( 315.0f, 8.0f ) );
					child( "Knife changer", ImVec2( 292.0f, 458.0f ), [ & ]( ) {
						checkbox( "Enable knife changer", &GET_VARIABLE( g_variables.m_knife_changer_enabled, bool ) );

						static std::vector< const char* > knife_labels;
						if ( knife_labels.empty( ) ) {
							for ( const auto& k : g_skinchanger_knife_names )
								knife_labels.push_back( k.name );
						}

						auto& t_index  = GET_VARIABLE( g_variables.m_knife_changer_t_index, int );
						auto& ct_index = GET_VARIABLE( g_variables.m_knife_changer_ct_index, int );
						if ( t_index < 0 || t_index >= ( int )knife_labels.size( ) ) t_index = 0;
						if ( ct_index < 0 || ct_index >= ( int )knife_labels.size( ) ) ct_index = 0;

						combo( "T knife", t_index, knife_labels.data( ), knife_labels.size( ) );
						combo( "CT knife", ct_index, knife_labels.data( ), knife_labels.size( ) );

						checkbox( "Animation fix (compat)", &GET_VARIABLE( g_variables.m_knife_changer_anim_fix, bool ) );

						ImGui::Spacing( );
						if ( button( "Apply / force update", ImVec2( 260.f, 22.f ) ) )
							g_knife_changer.request_full_update( );

						ImGui::Spacing( );
						ImGui::TextColored( ImVec4( 0.55f, 0.55f, 0.55f, 1.0f ),
						                    "Re-equip knife in-game for changes\n"
						                    "to fully apply (quick switch: Q).\n"
						                    "Enable 'Animation fix' if server has\n"
						                    "its own skinchanger plugin." );
					} );
				}

				if ( server_section_tab == 2 ) {
					if ( g_invetory_changer.m_custom_models.empty( ) )
						g_invetory_changer.ScanCustomModels( );

					ImGui::SetCursorPos( ImVec2( 8.0f, 8.0f ) );
					child( "Local Visuals", ImVec2( 300.0f, 458.0f ), [ & ]( ) {
						ImGui::TextColored( ImVec4( 0.65f, 0.65f, 0.65f, 1.0f ), "Current custom player model" );
						ImGui::Spacing( );
						const std::string current_model = g_invetory_changer.m_selected_custom_model.empty( ) ? "No custom model selected" : g_invetory_changer.m_selected_custom_model;
						ImGui::TextWrapped( "%s", current_model.c_str( ) );
					} );

					ImGui::SetCursorPos( ImVec2( 315.0f, 8.0f ) );
					child( "Local Extra", ImVec2( 292.0f, 458.0f ), [ & ]( ) {
						checkbox( "Thirdperson", &GET_VARIABLE( g_variables.m_thirdperson, bool ), false, true, { 280, -1 }, []( ) {
							keybind( "Thirdperson key", &GET_VARIABLE( g_variables.m_thirdperson_key, key_bind_t ) );
							popup_slider_float( "Distance", &GET_VARIABLE( g_variables.m_thirdperson_distance, float ), 50.f, 300.f, "%.0f" );
						} );

						ImGui::Spacing( );
						checkbox( "Model changer", &GET_VARIABLE( g_variables.m_custom_model_changer, bool ) );
						ImGui::TextColored( ImVec4( 0.55f, 0.55f, 0.55f, 1.0f ), "Model switches back automatically in first person" );
						ImGui::Spacing( );
						ImGui::TextColored( ImVec4( 0.55f, 0.55f, 0.55f, 1.0f ), "Custom player model" );

						std::vector< const char* > model_names;
						for ( const auto& model : g_invetory_changer.m_custom_models )
							model_names.push_back( model.c_str( ) );

						auto& custom_model_index = GET_VARIABLE( g_variables.m_custom_model_index, int );
						if ( custom_model_index < 0 || custom_model_index >= static_cast< int >( model_names.size( ) ) )
							custom_model_index = 0;

						if ( !model_names.empty( ) ) {
							combo( "Model", custom_model_index, model_names.data( ), model_names.size( ) );
							if ( custom_model_index >= 0 && custom_model_index < static_cast< int >( g_invetory_changer.m_custom_models.size( ) ) )
								g_invetory_changer.m_selected_custom_model = g_invetory_changer.m_custom_models[ custom_model_index ];
						}

						if ( button( "Refresh scanned models", ImVec2( 260.f, 22.f ) ) ) {
							g_invetory_changer.ScanCustomModels( );
							if ( custom_model_index < 0 || custom_model_index >= static_cast< int >( g_invetory_changer.m_custom_models.size( ) ) )
								custom_model_index = 0;
						}

						if ( button( "Use scanned model", ImVec2( 260.f, 22.f ) ) ) {
							if ( custom_model_index >= 0 && custom_model_index < static_cast< int >( g_invetory_changer.m_custom_models.size( ) ) ) {
								g_invetory_changer.m_selected_custom_model = g_invetory_changer.m_custom_models[ custom_model_index ];
								GET_VARIABLE( g_variables.m_custom_model_changer, bool ) = !g_invetory_changer.m_selected_custom_model.empty( );
								g_ctx.m_bNeedFullUpdate = true;
							}
						}

						if ( button( "Clear custom model", ImVec2( 260.f, 22.f ) ) ) {
							g_invetory_changer.m_selected_custom_model.clear( );
							GET_VARIABLE( g_variables.m_custom_model_changer, bool ) = false;
							g_ctx.m_bNeedFullUpdate = true;
						}
					} );
				}

				if ( server_section_tab == 3 ) {
					ImGui::SetCursorPos( ImVec2( 8.0f, 8.0f ) );
					child( "Maps", ImVec2( 300.0f, 140.0f ), []( ) {
						const char* level_name = g_interfaces.m_engine_client->get_level_name_short( );
						ImGui::Text( "current map: %s", ( level_name && *level_name ) ? level_name : "none" );
						ImGui::Spacing( );

						static const char* maps[] = {
							"de_mirage", "de_nuke", "de_train", "de_mirage_2014", "de_mirage_winter", "de_mirage_mod",
							"de_nuke_mod", "de_nuke_2017", "de_nuke_2013_tix", "de_nuke_2013_winter", "de_nukefun",
							"de_seaside_2014", "de_seaside_2014_cheddabob", "de_seasonNEW", "de_season_chedbob",
							"de_shortdust", "de_tuscan", "de_vertigo", "cs_insertion2", "cs_assault", "cs_agency",
							"de_bank", "de_dust2", "de_dust2old", "de_inferno", "de_inferno_winter_2019", "de_inferno_old"
						};
						combo( "Select map", GET_VARIABLE( g_variables.m_server_utils_map, int ), maps, 27 );
						if ( button( "Change map", ImVec2( 280.f, 22.f ) ) ) {
							std::string cmd = "sm_map " + std::string( maps[ GET_VARIABLE( g_variables.m_server_utils_map, int ) ] );
							g_interfaces.m_engine_client->execute_client_cmd( cmd.c_str( ) );
						}
					} );

					ImGui::SetCursorPos( ImVec2( 315.0f, 8.0f ) );
					child( "Weapons", ImVec2( 292.0f, 220.0f ), []( ) {
						static const char* rifles[] = { "None", "ak_47", "m4a1", "m4a1_silencer", "galilar", "famas", "aug", "sg556", "awp", "ssg08", "scar20", "g3sg1", "negev", "m249" };
						static const char* shotguns[] = { "None", "nova", "xm1014", "mag7", "sawedoff" };
						static const char* pistols[] = { "None", "glock", "usp_silencer", "hkp2000", "deagle", "elite", "fiveseven", "tec9", "p250", "cz75a", "revolver", "mac10", "mp9", "mp7", "mp5sd", "ump45", "p90", "bizon" };
						static const char* knives[] = { "None", "knife", "knifegg" };
						static const char* grenades[] = { "hegrenade", "flashbang", "smokegrenade", "molotov", "incgrenade", "decoy", "taser", "vesthelm", "healthshot", "c4" };

						combo( "Rifles", GET_VARIABLE( g_variables.m_server_utils_rifle, int ), rifles, 14 );
						combo( "Shotguns", GET_VARIABLE( g_variables.m_server_utils_shotgun, int ), shotguns, 5 );
						combo( "Pistols", GET_VARIABLE( g_variables.m_server_utils_pistol, int ), pistols, 18 );
						combo( "Knife", GET_VARIABLE( g_variables.m_server_utils_knife, int ), knives, 3 );
						multi_combo( "Grenades", GET_VARIABLE( g_variables.m_server_utils_grenades, std::vector< bool > ), grenades, 10 );

						int total_selected = 0;
						std::vector< std::string > to_give;
						if ( GET_VARIABLE( g_variables.m_server_utils_rifle, int ) > 0 ) {
							total_selected++;
							to_give.push_back( rifles[ GET_VARIABLE( g_variables.m_server_utils_rifle, int ) ] );
						}
						if ( GET_VARIABLE( g_variables.m_server_utils_shotgun, int ) > 0 ) {
							total_selected++;
							to_give.push_back( shotguns[ GET_VARIABLE( g_variables.m_server_utils_shotgun, int ) ] );
						}
						if ( GET_VARIABLE( g_variables.m_server_utils_pistol, int ) > 0 ) {
							total_selected++;
							to_give.push_back( pistols[ GET_VARIABLE( g_variables.m_server_utils_pistol, int ) ] );
						}
						if ( GET_VARIABLE( g_variables.m_server_utils_knife, int ) > 0 ) {
							total_selected++;
							to_give.push_back( knives[ GET_VARIABLE( g_variables.m_server_utils_knife, int ) ] );
						}
						auto& gren_vec = GET_VARIABLE( g_variables.m_server_utils_grenades, std::vector< bool > );
						for ( int i = 0; i < 10; ++i ) {
							if ( gren_vec[ i ] ) {
								total_selected++;
								to_give.push_back( grenades[ i ] );
							}
						}

						const char* btn_text = total_selected > 1 ? "Give set" : "Give weapon";
						if ( button( btn_text, ImVec2( 270.f, 22.f ) ) ) {
							if ( total_selected > 1 )
								g_interfaces.m_engine_client->execute_client_cmd( "sm_disarm @me" );

							for ( const auto& weapon_name : to_give ) {
								std::string cmd = "sm_give @me " + weapon_name;
								g_interfaces.m_engine_client->execute_client_cmd( cmd.c_str( ) );
							}
						}
					} );

					ImGui::SetCursorPos( ImVec2( 8.0f, 156.0f ) );
					child( "Helper", ImVec2( 300.0f, 315.0f ), []( ) {
						auto helper_btn = []( const char* label, const char* cmd ) {
							if ( button( label, ImVec2( 280.f, 22.f ) ) )
								g_interfaces.m_engine_client->execute_client_cmd( cmd );
						};

						helper_btn( "Respawn @all", "sm_respawn @all" );
						helper_btn( "Respawn @dead", "sm_respawn @dead" );
						helper_btn( "Respawn @me", "sm_respawn @me" );
						helper_btn( "Disarm @all", "sm_disarm @all" );
						helper_btn( "Disarm @me", "sm_disarm @me" );
						helper_btn( "Teleport ct @me", "sm_tp ct @me" );
						helper_btn( "Teleport t @me", "sm_tp t @me" );
					} );

					ImGui::SetCursorPos( ImVec2( 315.0f, 236.0f ) );
					child( "Connect to y6OuHbIu", ImVec2( 292.0f, 60.0f ), []( ) {
						if ( button( "y6OuHbIu", ImVec2( 275.f, 22.f ) ) ) {
							g_menu.connect_to_server = true;
						}
					} );
					ImGui::SetCursorPos( ImVec2( 315.0f, 302.0f ) );
					child( "Connect to direct", ImVec2( 292.0f, 60.0f ), []( ) {
						if ( button( "Direct", ImVec2( 275.f, 22.f ) ) ) {
							g_menu.connect_to_direct = true;
						}
					} );
				}
			} );
		}
		if ( display_tab_number == 9 ) {
			ImGui::SetCursorPos( ImVec2( 245.0f, -5.0f ) );
			child( "Convar Settings", ImVec2( 360.0f, 515.0f ), []( ) {
				static char convar_command[ 256 ] = "";
				auto& saved_cmds = GET_VARIABLE( g_variables.m_convar_commands, std::vector< std::string > );
				auto execute_convar_cmd = []( const char* cmd_text ) {
					if ( !cmd_text || !*cmd_text || !g_interfaces.m_engine_client )
						return;
					auto sv_cheats = g_convars[ HASH_BT( "sv_cheats" ) ];
					if ( sv_cheats ) {
						sv_cheats->set_value( 1 );
					}
					g_interfaces.m_engine_client->client_cmd_unrestricted( cmd_text );
				};
				auto* section_font = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ];

				if ( section_font )
					ImGui::PushFont( section_font );
				ImGui::TextColored( ImVec4( 0.78f, 0.78f, 0.78f, 1.0f ), "Convar list" );
				if ( section_font )
					ImGui::PopFont( );
				ImGui::Spacing( );

				input_text( "", convar_command, convar_command, sizeof( convar_command ), 340.f, 22.f, NULL );
				ImGui::Spacing( );
				bool submit_convar = smbutton_hh( "Add convar" );
				if ( submit_convar && strlen( convar_command ) > 0 ) {
					std::string new_cmd = convar_command;
					bool exists = false;
					for ( const auto& cmd : saved_cmds ) {
						if ( cmd == new_cmd ) {
							exists = true;
							break;
						}
					}
					if ( !exists )
						saved_cmds.push_back( new_cmd );
					execute_convar_cmd( new_cmd.c_str( ) );
					convar_command[ 0 ] = '\0';
				}

				ImGui::Spacing( );
				ImGui::Spacing( );
				if ( section_font )
					ImGui::PushFont( section_font );
				ImGui::TextColored( ImVec4( 0.55f, 0.55f, 0.55f, 1.0f ), "Saved entries" );
				if ( section_font )
					ImGui::PopFont( );
				ImGui::Spacing( );

				const float item_height = ImGui::GetTextLineHeightWithSpacing( );
				const ImVec2 size = ImVec2( 340, item_height * 10 );
				ImVec2 listbox_pos = ImGui::GetCursorPos( );
				ImVec2 listbox_min = ImGui::GetWindowPos( ) + listbox_pos;
				ImVec2 listbox_max = listbox_min + size;
				ImGui::GetWindowDrawList( )->AddRectFilled( listbox_min, listbox_max, ImColor( 13, 13, 13, 255 ), 4.0f, ImDrawFlags_RoundCornersAll );
				ImGui::GetWindowDrawList( )->AddRect( listbox_min, listbox_max, ImColor( 33, 33, 33, 255 ), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f );

				static float scroll_offset = 0.0f;
				const int max_visible_items = 10;
				const int total_items = ( int )saved_cmds.size( );
				const int max_scroll = std::max( 0, total_items - max_visible_items );
				if ( ImGui::IsMouseHoveringRect( listbox_min, listbox_max ) ) {
					float wheel = ImGui::GetIO( ).MouseWheel;
					scroll_offset -= wheel * item_height;
					scroll_offset = std::clamp( scroll_offset, 0.0f, ( float )max_scroll * item_height );
				}

				ImGui::SetCursorPos( listbox_pos );
				ImGui::InvisibleButton( "##convar_list_area", size );

				int remove_idx = -1;
				for ( int i = 0; i < max_visible_items && i < total_items; ++i ) {
					int item_index = i + ( int )( scroll_offset / item_height );
					if ( item_index >= total_items )
						break;

					ImVec2 item_min = listbox_min + ImVec2( 4, 4 + i * item_height );
					ImVec2 item_max = item_min + ImVec2( size.x - 8, item_height );
					bool is_hovered = ImGui::IsMouseHoveringRect( item_min, item_max );
					ImGui::GetWindowDrawList( )->AddRectFilled( item_min, item_max, is_hovered ? ImColor( 21, 21, 21, 255 ) : ImColor( 16, 16, 16, 220 ), 4.0f, ImDrawFlags_RoundCornersAll );
					ImGui::GetWindowDrawList( )->AddRect( item_min, item_max, ImColor( 33, 33, 33, 255 ), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f );
					if ( is_hovered ) {
						ImGui::GetWindowDrawList( )->AddRect( item_min, item_max, ImColor( 255, 255, 255, 18 ), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f );
					}

					ImGui::GetWindowDrawList( )->AddText(
						g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ],
						g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ]->FontSize,
						item_min + ImVec2( 10, 2 ),
						ImColor( 220, 220, 220, 255 ),
						saved_cmds[ item_index ].c_str( )
					);

					const ImVec2 remove_min = ImVec2( item_max.x - 20.0f, item_min.y + 0.0f );
					const ImVec2 remove_max = remove_min + ImVec2( 16.0f, 16.0f );
					const bool remove_hovered = ImGui::IsMouseHoveringRect( remove_min, remove_max );
					ImGui::GetWindowDrawList( )->AddRectFilled( remove_min, remove_max,
					                                           remove_hovered ? ImColor( 155, 30, 30, 255 ) : ImColor( 110, 22, 22, 255 ),
					                                           3.0f, ImDrawFlags_RoundCornersAll );
					ImGui::GetWindowDrawList( )->AddRect( remove_min, remove_max, ImColor( 255, 255, 255, 45 ), 3.0f, ImDrawFlags_RoundCornersAll, 1.0f );
					ImGui::GetWindowDrawList( )->AddText(
						g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ],
						g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ]->FontSize,
						ImVec2( remove_min.x + 4.7f, remove_min.y - 0.4f ),
						ImColor( 255, 255, 255, 255 ),
						"x"
					);

					if ( remove_hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
						remove_idx = item_index;
					} else if ( is_hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
						execute_convar_cmd( saved_cmds[ item_index ].c_str( ) );
						strncpy_s( convar_command, saved_cmds[ item_index ].c_str( ), sizeof( convar_command ) - 1 );
					}
				}

				if ( remove_idx >= 0 )
					saved_cmds.erase( saved_cmds.begin( ) + remove_idx );
			} );
		}

		if ( display_tab_number == 10 ) {
			ImGui::SetCursorPos( ImVec2( 215.0f, -5.0f ) );
			scrollable_child( "Settings", ImVec2( 295.0f, 150.0f ), []( ) {
				const bool tickrate_ok = !g_interfaces.m_engine_client->is_connected( ) ||
				                         static_cast< int >( 1.f / g_interfaces.m_global_vars_base->m_interval_per_tick ) == 64;

				if ( !tickrate_ok ) {
					GET_VARIABLE( g_variables.m_movement_rec, bool ) = false;
					ImGui::TextColored( ImVec4( 1.f, 0.3f, 0.3f, 1.f ), "Available on 64-tick servers only" );
					return;
				}

				checkbox( "Enable recorder", &GET_VARIABLE( g_variables.m_movement_rec, bool ), false, true, { 300, 185 }, []( ) {
					checkbox( "Show recording line", &GET_VARIABLE( g_variables.m_movement_rec_show_line, bool ) );
					checkbox( "Show eye start", &GET_VARIABLE( g_variables.m_movement_rec_show_eye_start, bool ) );
					checkbox( "Show recorder timer", &GET_VARIABLE( g_variables.m_movement_rec_render, bool ), false, true, { 300, -1 }, []( ) {
						static const char* positions[] = { "Top-left", "Bottom-left", "Bottom-right" };
						popup_combo( "Indicator position", GET_VARIABLE( g_variables.m_movement_rec_position, int ), positions, 3 );
					} );
					checkbox( "Show clipper box", &GET_VARIABLE( g_variables.m_movement_rec_clipper_box, bool ) );
					checkbox( "Original playback viewangles", &GET_VARIABLE( g_variables.m_movement_rec_lockva, bool ) );
					checkbox( "Lock while aiming to position", &GET_VARIABLE( g_variables.m_movement_rec_lockgoingtostart, bool ) );
				} );
				if ( button( "Refresh", ImVec2( 275.f, 22.f ) ) )
					g_movement_recorder.refresh_clips( );
				if ( !GET_VARIABLE( g_variables.m_movement_rec, bool ) )
					return;
			} );

			ImGui::SetCursorPos( ImVec2( 215.0f + 310.f, -5.0f ) );
			scrollable_child( "Binds", ImVec2( 305.0f, 150.0f ), []( ) {
				if ( !GET_VARIABLE( g_variables.m_movement_rec, bool ) ) {
					ImGui::TextColored( ImVec4( 0.6f, 0.6f, 0.6f, 1.f ), "Enable recorder first" );
					return;
				}

				slider_float( "Smoothing", &GET_VARIABLE( g_variables.m_movement_rec_smoothing, float ), 1.f, 100.f, "%.1f", 75.f, 36.f );
				text_with_keybind( "Save route" );
				keybind( "##mr_clip",       &GET_VARIABLE( g_variables.m_movement_rec_keyclip,      key_bind_t ) );
				text_with_keybind( "Start playback" );
				keybind( "##mr_start_play", &GET_VARIABLE( g_variables.m_movement_rec_keystartplay, key_bind_t ) );
				text_with_keybind( "Stop playback" );
				keybind( "##mr_stop_play",  &GET_VARIABLE( g_variables.m_movement_rec_keystopplay,  key_bind_t ) );
				text_with_keybind( "Clear record" );
				keybind( "##mr_clear_record", &GET_VARIABLE( g_variables.m_movement_rec_keyclearrecord, key_bind_t ) );
			} );

			{
				static bool first_list = true;
				if ( first_list ) {
					g_movement_recorder.refresh_clips( );
					first_list = false;
				}

				auto& mr = g_movement_recorder;
				ImGui::SetCursorPos( ImVec2( 215.0f, 165.0f ) );
				ImVec2 list_origin = ImGui::GetWindowPos( ) + ImVec2( 215.0f, 165.0f );
				ImVec2 list_size( 615.0f, 350.0f );

				auto* draw_list = ImGui::GetWindowDrawList( );
				auto* text_font = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ];
				if ( !text_font )
					text_font = ImGui::GetFont( );
				float font_size = text_font ? text_font->FontSize : ImGui::GetFontSize( );

				constexpr float item_h = 30.0f;
				constexpr float item_pad = 4.0f;
				constexpr float btn_w = 40.0f;
				constexpr float btn_h = 20.0f;

				int total_items = static_cast< int >( mr.m_clips.size( ) );
				static float mr_scroll = 0.0f;
				int max_visible = static_cast< int >( list_size.y / ( item_h + item_pad ) );
				int max_scroll = std::max( 0, total_items - max_visible );

				if ( ImGui::IsMouseHoveringRect( list_origin, list_origin + list_size ) ) {
					mr_scroll -= ImGui::GetIO( ).MouseWheel;
					mr_scroll = std::clamp( mr_scroll, 0.0f, static_cast< float >( max_scroll ) );
				}

				int start_idx = static_cast< int >( mr_scroll );
				int delete_idx = -1;

				for ( int vi = 0; vi < max_visible && ( start_idx + vi ) < total_items; ++vi ) {
					int idx = start_idx + vi;
					ImVec2 row_min = list_origin + ImVec2( 0, vi * ( item_h + item_pad ) );
					ImVec2 row_max = row_min + ImVec2( list_size.x, item_h );
					bool is_hovered = ImGui::IsMouseHoveringRect( row_min, row_max );

					if ( is_hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
						mr.m_selected_clip = idx;

					draw_list->AddRectFilled( row_min, row_max, ImColor( 16, 16, 16, static_cast< int >( 255 * g_menu.m_animation_progress ) ), 4.0f );
					draw_list->AddRect( row_min, row_max, ImColor( 11, 11, 11, static_cast< int >( 255 * g_menu.m_animation_progress ) ), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f );

					const std::string file_name = mr.m_clips[ idx ].filename.empty( ) ? mr.m_clips[ idx ].display_name + ".mr" : mr.m_clips[ idx ].filename;
					ImVec2 text_pos = row_min + ImVec2( 12, ( item_h - font_size ) * 0.5f );
					draw_list->AddText( text_font, font_size, text_pos,
						ImColor( 200, 200, 200, 255 ),
						file_name.c_str( ) );

					float btns_x = row_max.x - btn_w - 12.0f;
					float btn_y = row_min.y + ( item_h - btn_h ) * 0.5f;
					auto draw_inline_btn = [&]( const char* label, float x ) -> bool {
						ImVec2 bmin( x, btn_y );
						ImVec2 bmax( x + btn_w, btn_y + btn_h );
						bool bh = ImGui::IsMouseHoveringRect( bmin, bmax );
						bool bp = bh && ImGui::IsMouseClicked( ImGuiMouseButton_Left );

						draw_list->AddRect( bmin, bmax, ImColor( 33, 33, 33, static_cast< int >( 255 * g_menu.m_animation_progress ) ), 3.0f, 0, 1.0f );

						ImVec2 ts = text_font->CalcTextSizeA( font_size, FLT_MAX, 0.f, label );
						ImVec2 tp( bmin.x + ( btn_w - ts.x ) * 0.5f, bmin.y + ( btn_h - ts.y ) * 0.5f );
						draw_list->AddText( text_font, font_size, tp,
							bh ? ImColor( 255, 255, 255, 255 ) : ImColor( 180, 180, 180, 255 ), label );
						return bp;
					};

					if ( draw_inline_btn( "Delete", btns_x ) )
						delete_idx = idx;
				}

				if ( delete_idx >= 0 )
					mr.delete_clip( static_cast< size_t >( delete_idx ) );
			}
		}

		if ( display_tab_number == 11 && is_dev_hwid_allowed( ) ) {
			ImGui::SetCursorPos( ImVec2( 215.0f, -5.0f ) );
			child( "Dev", ImVec2( 615.0f, 515.0f ), []( ) {
				checkbox( "Anti server chat", &GET_VARIABLE( g_variables.m_anti_server_chat, bool ) );
checkbox( "Ladder glide jump", &GET_VARIABLE( g_variables.m_ladder_bug, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Ladder glide jump key" );
					keybind( "ladder bug key ", &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) );
					popup_checkbox( "+jetpack", &GET_VARIABLE( g_variables.m_ladder_glide_jetpack, bool ), false, true, { 300, -1 }, []( ) {
						popup_slider_int( "Ticks interval", &GET_VARIABLE( g_variables.m_ladder_glide_jetpack_interval, int ), 1, 70, "%d" );
					} );
				} );
				
				checkbox( "Extended texture bug", &GET_VARIABLE( g_variables.m_extended_texture_bug, bool ), false, true, { 300, -1 }, []( ) {
					text_with_keybind( "Extended texture bug key" );
					keybind( "extended texture bug key ", &GET_VARIABLE( g_variables.m_extended_texture_bug_key, key_bind_t ) );
				} );
			} );
		}
		
		ImGui::PopFont( );
	}
	ImGui::End( );
	ImGui::PopStyleVar( );
}
void n_menu::impl_t::render_point_settings_popup( )
{
	for ( size_t i = 0; i < m_bounce_points_check.size( ); ++i ) {
		points_check_t& point = m_bounce_points_check[ i ];
		if ( point.open_settings ) {
			custom_point_popup( "bounce point settings", ImVec2( 300.f, -1.f ), [ & ]( ) {
				ImGui::PushFont( g_render.m_custom_fonts[e_custom_font_names::custom_font_name_googlesans_addwindow] );
				ImGui::SetCursorPosX( (ImGui::GetWindowWidth() - ImGui::CalcTextSize("Bounce Point Settings").x) * 0.5f );
				ImGui::Text( "Bounce Point Settings" );
				if ( button( "Add" ) ) {
					point.active = !point.active;
				}
				popup_slider_float( "Radius", &point.radius, 50.f, 1000.f, "%.0f" );
				popup_checkbox( "Show 3D Radius", &point.show_3d_radius );
				const char* presets[] = { "All", "Only Stand", "Only Crouch", "Custom" };
				popup_combo( "Preset", point.selected_preset, presets, 4 );
				if ( point.selected_preset == 0 ) {
					point.jump = true; point.c_jump = true;
					point.minijump = true; point.c_minijump = true;
					point.longjump = true; point.c_longjump = true;
					point.jumpbug = true; point.c_jumpbug = true;
					point.crouch_hop = true; point.c_crouch_hop = true;
					point.high_crouch_jump = true; point.c_high_crouch_jump = true;
				} else if ( point.selected_preset == 1 ) {
					point.jump = true; point.c_jump = false;
					point.minijump = true; point.c_minijump = false;
					point.longjump = true; point.c_longjump = false;
					point.jumpbug = true; point.c_jumpbug = false;
					point.crouch_hop = true; point.c_crouch_hop = false;
					point.high_crouch_jump = true; point.c_high_crouch_jump = false;
				} else if ( point.selected_preset == 2 ) {
					point.jump = false; point.c_jump = true;
					point.minijump = false; point.c_minijump = true;
					point.longjump = false; point.c_longjump = true;
					point.jumpbug = false; point.c_jumpbug = true;
					point.crouch_hop = false; point.c_crouch_hop = true;
					point.high_crouch_jump = false; point.c_high_crouch_jump = true;
				}
				popup_checkbox( "Advanced", &point.advanced_enabled );
				if ( point.advanced_enabled ) {
					const char* advanced_options[] = { "Custom", "Delta Strafe", "Mixed" };
					popup_combo( "Advanced Mode", point.selected_advanced, advanced_options, 3 );
					if ( point.selected_advanced == 1 ) {
						popup_slider_float( "Delta Strafe Value", &point.delta_strafe, 0.0f, 5.0f, "%.0f" );
					}
				}
				popup_checkbox( "Enable Restricted Binds", &point.enable_restricted_binds );
				if ( point.enable_restricted_binds ) {
					const char* restricted_bind_types[] = { "Crouch", "Stand" };
					popup_combo( "Restricted Bind Type", point.selected_restricted_bind, restricted_bind_types, 2 );
					popup_slider_float( "Restricted Binds Radius", &point.restricted_binds_radius, 50.f, 1000.f, "%.0f" );
				}
				if ( point.selected_preset == 3 ) {
					popup_checkbox( "jump (stand)", &point.jump );
					popup_checkbox( "jump (crouch)", &point.c_jump );
					popup_checkbox( "minijump (stand)", &point.minijump );
					popup_checkbox( "minijump (crouch)", &point.c_minijump );
					popup_checkbox( "longjump (stand)", &point.longjump );
					popup_checkbox( "longjump (crouch)", &point.c_longjump );
					popup_checkbox( "jumpbug (stand)", &point.jumpbug );
					popup_checkbox( "jumpbug (crouch)", &point.c_jumpbug );
					popup_checkbox( "crouch hop (stand)", &point.crouch_hop );
					popup_checkbox( "crouch hop (crouch)", &point.c_crouch_hop );
					popup_checkbox( "crouch jb (stand)", &point.high_crouch_jump );
					popup_checkbox( "crouch jb (crouch)", &point.c_high_crouch_jump );
				}
				ImGui::PopFont( );
				if ( isEnterPressed( ) ) {
					point.open_settings = false;
					if ( !point_menu_is_opened( ) ) {
						g_menu.m_opened = false;
					}
				}
			} );
		}
	}
	for ( size_t i = 0; i < m_points_check.size( ); ++i ) {
		points_check_t& point = m_points_check[ i ];
		if ( point.open_settings ) {
			custom_point_popup( "pixelsurf point settings", ImVec2( 300.f, -1.f ), [ & ]( ) {
				ImGui::PushFont( g_render.m_custom_fonts[e_custom_font_names::custom_font_name_googlesans_addwindow] );
				ImGui::SetCursorPosX( (ImGui::GetWindowWidth() - ImGui::CalcTextSize("Pixelsurf Point Settings").x) * 0.5f );
				ImGui::Text( "Pixelsurf Point Settings" );
				if ( button( "Add" ) ) {
					point.active = !point.active;
				}
				popup_slider_float( "Radius", &point.radius, 50.f, 1000.f, "%.0f" );
				popup_checkbox( "Show 3D Radius", &point.show_3d_radius );
				popup_checkbox( "Pixelsurf line render", &GET_VARIABLE( g_variables.m_pixel_surf_line_render, bool ) );
				if ( GET_VARIABLE( g_variables.m_pixel_surf_line_render, bool ) ) {
					ImGui::Text( "Line color" );
					popup_color_picker_button( "##pixelsurf line color", &GET_VARIABLE( g_variables.m_pixel_surf_line_color, c_color ) );
					popup_slider_float( "Line thickness", &GET_VARIABLE( g_variables.m_pixel_surf_line_thickness, float ), 1.0f, 5.0f, "%.1f" );
					if ( button( "Clear trajectory" ) ) {
						g_movement.m_pixelsurf_data.m_trajectory_points.clear( );
					}
				}
				const char* presets[] = { "All", "Only Stand", "Only Crouch", "Custom" };
				popup_combo( "Preset", point.selected_preset, presets, 4 );
				if ( point.selected_preset == 0 ) {
					point.jump = true; point.c_jump = true;
					point.minijump = true; point.c_minijump = true;
					point.longjump = true; point.c_longjump = true;
					point.jumpbug = true; point.c_jumpbug = true;
					point.crouch_hop = true; point.c_crouch_hop = true;
					point.high_crouch_jump = true; point.c_high_crouch_jump = true;
				} else if ( point.selected_preset == 1 ) {
					point.jump = true; point.c_jump = false;
					point.minijump = true; point.c_minijump = false;
					point.longjump = true; point.c_longjump = false;
					point.jumpbug = true; point.c_jumpbug = false;
					point.crouch_hop = true; point.c_crouch_hop = false;
					point.high_crouch_jump = true; point.c_high_crouch_jump = false;
				} else if ( point.selected_preset == 2 ) {
					point.jump = false; point.c_jump = true;
					point.minijump = false; point.c_minijump = true;
					point.longjump = false; point.c_longjump = true;
					point.jumpbug = false; point.c_jumpbug = true;
					point.crouch_hop = false; point.c_crouch_hop = true;
					point.high_crouch_jump = false; point.c_high_crouch_jump = true;
				}
				popup_checkbox( "Advanced", &point.advanced_enabled );
				if ( point.advanced_enabled ) {
					const char* advanced_options[] = { "Custom", "Delta Strafe", "Mixed" };
					popup_combo( "Advanced Mode", point.selected_advanced, advanced_options, 3 );
					if ( point.selected_advanced == 1 ) {
						popup_slider_float( "Delta Strafe Value", &point.delta_strafe, 0.0f, 5.0f, "%.0f" );
					}
				}
				popup_checkbox( "Enable Restricted Binds", &point.enable_restricted_binds );
				if ( point.enable_restricted_binds ) {
					const char* restricted_bind_types[] = { "Crouch", "Stand" };
					popup_combo( "Restricted Bind Type", point.selected_restricted_bind, restricted_bind_types, 2 );
					popup_slider_float( "Restricted Binds Radius", &point.restricted_binds_radius, 50.f, 1000.f, "%.0f" );
				}
				if ( point.selected_preset == 3 ) {
					popup_checkbox( "jump (stand)", &point.jump );
					popup_checkbox( "jump (crouch)", &point.c_jump );
					popup_checkbox( "minijump (stand)", &point.minijump );
					popup_checkbox( "minijump (crouch)", &point.c_minijump );
					popup_checkbox( "longjump (stand)", &point.longjump );
					popup_checkbox( "longjump (crouch)", &point.c_longjump );
					popup_checkbox( "jumpbug (stand)", &point.jumpbug );
					popup_checkbox( "jumpbug (crouch)", &point.c_jumpbug );
					popup_checkbox( "crouch hop (stand)", &point.crouch_hop );
					popup_checkbox( "crouch hop (crouch)", &point.c_crouch_hop );
					popup_checkbox( "crouch jb (stand)", &point.high_crouch_jump );
					popup_checkbox( "crouch jb (crouch)", &point.c_high_crouch_jump );
				}
				ImGui::PopFont( );
				if ( isEnterPressed( ) ) {
					point.open_settings = false;
					if ( !point_menu_is_opened( ) ) {
						g_menu.m_opened = false;
					}
				}
			} );
		}
	}
}
