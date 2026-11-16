#pragma once
#include "players.h"
#include "../../../game/sdk/classes/c_csgo_hud_radar.h"
#include "../../../game/sdk/includes/includes.h"
#include "../../../globals/includes/includes.h"
#include "../../../globals/render/draw.h"
#include "../../../instalisation fonts/fonts2.h"
#include "../../avatar_cache/avatar_cache.h"
#include "../../entity_cache/entity_cache.h"
#include "dormancy/dormancy.h"
void n_players::impl_t::on_paint_traverse( )
{
	if ( !g_interfaces.m_engine_client->is_connected_safe( ) || g_interfaces.m_engine_client->is_hltv( ) )
		return;

	if ( !g_ctx.m_local )
		return;

	if ( GET_VARIABLE( g_variables.m_players, bool ) )
		this->players( );
}
constexpr float anim_time = 255 / 2.0f;
void n_players::impl_t::players( )
{
	static unsigned long radar_base{ };
	static c_csgo_hud_radar* hud_radar{ };
	[]( ) {
		g_dormancy.think( );
	}( );
	const float delta_time = 1.f * g_interfaces.m_global_vars_base->m_frame_time;
	c_angle view_angles = { };
	g_interfaces.m_engine_client->get_view_angles( view_angles );
	const float yaw = deg2rad( view_angles.m_y );
	const auto display_size = ImGui::GetIO( ).DisplaySize;
	g_entity_cache.enumerate( e_enumeration_type::type_players, [ & ]( c_base_entity* entity ) {
		if ( !entity || entity == g_ctx.m_local || !g_ctx.m_local->is_enemy( entity ) || !entity->is_alive( ) )
			return;
		const auto index = entity->get_index( );
		const bool whitelist_visuals = g_menu.is_server_player_visuals_whitelisted( index );
		bool valid_dormant       = false;
		const auto backup_flags  = entity->get_flags( );
		const auto backup_origin = entity->get_abs_origin( );
		if ( entity->is_dormant( ) )
			valid_dormant = g_dormancy.adjust_player_sound( entity );
		else
			g_dormancy.m_sound_players[ index ].reset( true, entity->get_abs_origin( ), entity->get_flags( ) );
		if ( entity->is_dormant( ) ) {
			this->m_fading_alpha[ index ] -= delta_time;
		} else if ( this->m_fading_alpha[ index ] < 1.f ) {
			this->m_fading_alpha[ index ] += delta_time;
		}
		const auto client_renderable = entity->get_client_renderable( );
		if ( !client_renderable )
			return;
		const auto client_unknown = client_renderable->get_client_unknown( );
		if ( !client_unknown )
			return;
		const auto collideable = client_unknown->get_collideable( );
		if ( !collideable )
			return;
		const auto client_entity = client_unknown->get_client_entity( );
		if ( !client_entity )
			return;
		const bool can_see_player = g_ctx.m_local->can_see( entity->get_eye_position( false ) );
		if ( this->m_fading_alpha[ index ] >= 1.0f )
			this->m_fading_alpha[ index ] = 1.0f;
		if ( this->m_fading_alpha[ index ] <= 0.0f ) {
			this->m_fading_alpha[ index ] = 0.0f;
			return;
		}
		const bool in_view_frustrum = !g_interfaces.m_engine_client->cull_box( collideable->get_obb_mins( ) + client_entity->get_abs_origin( ),
		                                                                       collideable->get_obb_maxs( ) + client_entity->get_abs_origin( ) );
		const auto client_networkable = client_unknown->get_client_networkable( );
		if ( !client_networkable )
			return;
		[ & ]( ) {
			if ( !GET_VARIABLE( g_variables.m_out_of_fov_arrows, bool ) || in_view_frustrum )
				return;

			const c_vector position_difference = g_ctx.m_local->get_abs_origin( ) - client_entity->get_abs_origin( );
			float x = std::cos( yaw ) * position_difference.m_y - std::sin( yaw ) * position_difference.m_x;
			float y = std::cos( yaw ) * position_difference.m_x + std::sin( yaw ) * position_difference.m_y;
			if ( const auto len = std::sqrt( x * x + y * y ); len ) {
				x /= len;
				y /= len;
			}

			const float avatar_size = std::max( 18.0f, GET_VARIABLE( g_variables.m_out_of_fov_arrows_size, float ) * 1.8f );
			const int avatar_screen_position = GET_VARIABLE( g_variables.m_out_of_fov_arrows_distance, int );
			const c_vector_2d avatar_center(
				display_size.x / 2.f + ( x * avatar_screen_position ),
				display_size.y / 2.f + ( y * avatar_screen_position )
			);
			const c_vector_2d avatar_min( avatar_center.m_x - avatar_size * 0.5f, avatar_center.m_y - avatar_size * 0.5f );
			const c_vector_2d avatar_max( avatar_center.m_x + avatar_size * 0.5f, avatar_center.m_y + avatar_size * 0.5f );
			const c_color accent_color = GET_VARIABLE( g_variables.m_out_of_fov_arrows_color, c_color );
			const auto avatar_alpha = this->m_fading_alpha[ index ];
			const auto shadow_color = c_color( 0.f, 0.f, 0.f, avatar_alpha * 0.55f ).get_u32( avatar_alpha * 0.55f );

			player_info_t player_info{ };
			if ( !g_interfaces.m_engine_client->get_player_info( index, &player_info ) )
				return;

			const int team = entity->get_team( );
			IDirect3DTexture9* avatar_texture = whitelist_visuals ? g_render.m_whitelist_avatar :
				( player_info.m_fake_player ?
					( team == e_team_id::team_tt ? g_render.m_terrorist_avatar : team == e_team_id::team_ct ? g_render.m_counter_terrorist_avatar : nullptr ) :
					g_avatar_cache[ index ] );

			filled_circle_draw_object_t shadow_circle{ };
			shadow_circle.m_center = avatar_center;
			shadow_circle.m_radius = avatar_size * 0.62f;
			shadow_circle.m_color = shadow_color;
			shadow_circle.m_segments = 32;
			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_filled_circle,
				std::make_any< filled_circle_draw_object_t >( shadow_circle ) );

			if ( avatar_texture ) {
				texture_draw_object_t avatar_draw{ };
				avatar_draw.m_position = avatar_min;
				avatar_draw.m_size = c_vector_2d( avatar_size, avatar_size );
				avatar_draw.m_color = IM_COL32( 255, 255, 255, static_cast< int >( 255.f * avatar_alpha ) );
				avatar_draw.m_texture_id = avatar_texture;
				avatar_draw.m_rounding = avatar_size * 0.28f;
				avatar_draw.m_draw_flags = ImDrawFlags_RoundCornersAll;
				g_render.m_draw_data.emplace_back(
					e_draw_type::draw_type_texture,
					std::make_any< texture_draw_object_t >( avatar_draw ) );
			}

		}( );
		if ( whitelist_visuals ) {
			if ( !can_see_player )
				return;
			c_vector top_origin( entity->get_abs_origin( ).m_x, entity->get_abs_origin( ).m_y, entity->get_abs_origin( ).m_z + 72.0f );
			c_vector middle_origin( entity->get_abs_origin( ).m_x, entity->get_abs_origin( ).m_y, entity->get_abs_origin( ).m_z + 38.0f );
			c_vector_2d screen_top{ };
			c_vector_2d screen_bottom{ };
			c_vector_2d screen_middle{ };
			if ( g_render.world_to_screen( top_origin, screen_top ) && g_render.world_to_screen( entity->get_abs_origin( ), screen_bottom ) &&
				 g_render.world_to_screen( middle_origin, screen_middle ) ) {
				const float model_h = std::max( 72.0f, std::fabs( screen_bottom.m_y - screen_top.m_y ) );
				const float sprite_h = model_h * 1.08f;
				const float sprite_w = sprite_h * 0.88f;
				if ( g_render.m_whitelist_avatar ) {
					texture_draw_object_t whitelist_draw{ };
					whitelist_draw.m_position = c_vector_2d( screen_middle.m_x - sprite_w * 0.5f, screen_middle.m_y - sprite_h * 0.56f );
					whitelist_draw.m_size = c_vector_2d( sprite_w, sprite_h );
					whitelist_draw.m_color = IM_COL32( 255, 255, 255, static_cast< int >( 255.f * this->m_fading_alpha[ index ] ) );
					whitelist_draw.m_texture_id = g_render.m_whitelist_avatar;
					whitelist_draw.m_rounding = 12.0f;
					whitelist_draw.m_draw_flags = ImDrawFlags_RoundCornersAll;
					g_render.m_draw_data.emplace_back( e_draw_type::draw_type_texture, std::make_any< texture_draw_object_t >( whitelist_draw ) );
				}
			}
			return;
		}
		bounding_box_t box{ };
		if ( !entity->get_bounding_box( &box ) )
			return;
		if ( GET_VARIABLE( g_variables.m_players_visibility_range, bool ) ) {
			float distance = g_ctx.m_local->get_abs_origin().dist_to( entity->get_abs_origin() );
			float max_distance = GET_VARIABLE( g_variables.m_players_visibility_range_distance, float );
			if ( distance > max_distance )
				return;
		}
		player_info_t player_info{ };
		if ( !g_interfaces.m_engine_client->get_player_info( index, &player_info ) )
			return;
		std::string hp_text = std::to_string( entity->get_health( ) );
		if ( GET_VARIABLE( g_variables.m_players_health_suffix, bool ) )
			hp_text.append( "hp" );
		const int team = entity->get_team( );
		float alpha = 1.0f;
		bool can_see = can_see_player;
		if ( this->m_backup_player_data[ index ].m_animated_health > entity->get_health( ) )
			this->m_backup_player_data[ index ].m_animated_health -=
				( 100.f * g_interfaces.m_global_vars_base->m_frame_time ) ;
		else
			this->m_backup_player_data[ index ].m_animated_health = entity->get_health( );
		const float factor = static_cast< float >( this->m_backup_player_data[ index ].m_animated_health ) / entity->get_max_health( );
		const float hue    = ( factor * 120.f ) / 360.f;
		float padding[ e_padding_direction::padding_direction_max ] = { 0.f, 0.f, 0.f, 0.f };
		if ( GET_VARIABLE( g_variables.m_players_box, bool ) ) {
			if ( !GET_VARIABLE( g_variables.m_players_box_corner, bool ) ) {
				g_render.m_draw_data.emplace_back(
					e_draw_type::draw_type_rect,
					std::make_any< rect_draw_object_t >( c_vector_2d( box.m_left, box.m_top ), c_vector_2d( box.m_right, box.m_bottom ),
				                                         GET_VARIABLE( g_variables.m_players_box_color, c_color ).get_u32( alpha ),
				                                         c_color( 0.f, 0.f, 0.f, alpha ).get_u32( alpha ), false,
				                                         0.f, ImDrawFlags_::ImDrawFlags_None, 1.f,
				                                         e_rect_flags::rect_flag_inner_outline | e_rect_flags::rect_flag_outer_outline ) );
			} else {
				const float corner_length = GET_VARIABLE( g_variables.m_players_box_corner_length, float );
				g_render.corner_rect( box.m_left, box.m_top, box.m_right, box.m_bottom,
				                      c_color( 0.f, 0.f, 0.f, alpha ).get_u32( alpha ), 2.f, corner_length );
				g_render.corner_rect( box.m_left, box.m_top, box.m_right, box.m_bottom,
				                      c_color( GET_VARIABLE( g_variables.m_players_box_color, c_color ) ).get_u32( alpha ), 1.f, corner_length );
			}
		}
		if ( GET_VARIABLE( g_variables.m_players_name, bool ) ) {
			std::string converted_name = g_menu.get_server_player_name( index, player_info.m_name );
			const auto text_size = fonts_for_gui::verdanarus_esp_font->CalcTextSizeA( fonts_for_gui::verdanarus_esp_font->FontSize, FLT_MAX, 0.f,
			                                                                          converted_name.c_str( ) );
			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_text,
				std::make_any< text_draw_object_t >( fonts_for_gui::verdanarus_esp_font,
			                                         c_vector_2d( box.m_left + box.m_width * 0.5f - text_size.x * 0.5f, box.m_top - 3 - text_size.y ),
			                                         converted_name, GET_VARIABLE( g_variables.m_players_name_color, c_color ).get_u32( alpha ),
			                                         c_color( 0.f, 0.f, 0.f, alpha ).get_u32( alpha ), 
			                                         static_cast<e_text_flags>(text_flag_outline | text_flag_dropshadow) ) );
			padding[ e_padding_direction::padding_direction_top ] -= text_size.y - 20;
		}
		const auto active_weapon = reinterpret_cast< c_base_entity* >(
			g_interfaces.m_client_entity_list->get_client_entity_from_handle( entity->get_active_weapon_handle( ) ) );
		if ( active_weapon )
			this->m_backup_player_data[ index ].m_active_weapon = active_weapon;
		if ( this->m_backup_player_data[ index ].m_active_weapon ) {
			const auto item_definition_index = this->m_backup_player_data[ index ].m_active_weapon->get_item_definition_index( );
			const auto weapon_data           = g_interfaces.m_weapon_system->get_weapon_data( item_definition_index );
			if ( weapon_data )
				this->m_backup_player_data[ index ].m_weapon_data = weapon_data;
			if ( this->m_backup_player_data[ index ].m_weapon_data ) {
				[ & ]( ) {
					if ( GET_VARIABLE( g_variables.m_player_ammo_bar, bool ) ) {
						if ( !this->m_backup_player_data[ index ].m_weapon_data->is_gun( ) )
							return;
						const float ammo = static_cast< float >( this->m_backup_player_data[ index ].m_active_weapon->get_ammo( ) );
						if ( this->m_backup_player_data[ index ].m_ammo > ammo )
							this->m_backup_player_data[ index ].m_ammo -= ( 2.f * g_interfaces.m_global_vars_base->m_frame_time );
						else
							this->m_backup_player_data[ index ].m_ammo = ammo;
						const int max_ammo = this->m_backup_player_data[ index ].m_weapon_data->m_max_clip1;
						float factor = this->m_backup_player_data[ index ].m_ammo / static_cast< float >( max_ammo );
						factor       = std::clamp< float >( factor, 0.f, 1.f );
						int position = GET_VARIABLE( g_variables.m_player_ammo_bar_position, int );
						c_vector_2d bar_start, bar_end, bar_fill_end;
						switch ( position ) {
							case 0:
								bar_start = c_vector_2d( box.m_left, box.m_top - 5.f + padding[ e_padding_direction::padding_direction_top ] );
								bar_end = c_vector_2d( box.m_left + box.m_width, box.m_top - 3.f + padding[ e_padding_direction::padding_direction_top ] );
								bar_fill_end = c_vector_2d( box.m_left + box.m_width * factor, box.m_top - 3.f + padding[ e_padding_direction::padding_direction_top ] );
								padding[ e_padding_direction::padding_direction_top ] -= 6.f;
								break;
							case 1:
								bar_start = c_vector_2d( box.m_left - 5.f, box.m_top );
								bar_end = c_vector_2d( box.m_left - 3.f, box.m_top + box.m_height );
								bar_fill_end = c_vector_2d( box.m_left - 3.f, box.m_top + box.m_height * ( 1.f - factor ) );
								break;
							case 2:
								bar_start = c_vector_2d( box.m_right + 3.f, box.m_top );
								bar_end = c_vector_2d( box.m_right + 5.f, box.m_top + box.m_height );
								bar_fill_end = c_vector_2d( box.m_right + 5.f, box.m_top + box.m_height * ( 1.f - factor ) );
								break;
							case 3:
								bar_start = c_vector_2d( box.m_left, box.m_bottom + 3.f + padding[ e_padding_direction::padding_direction_bottom ] );
								bar_end = c_vector_2d( box.m_left + box.m_width, box.m_bottom + 5.f + padding[ e_padding_direction::padding_direction_bottom ] );
								bar_fill_end = c_vector_2d( box.m_left + box.m_width * factor, box.m_bottom + 5.f + padding[ e_padding_direction::padding_direction_bottom ] );
								padding[ e_padding_direction::padding_direction_bottom ] += 6.f;
								break;
						}
						g_render.m_draw_data.emplace_back(
							e_draw_type::draw_type_rect,
							std::make_any< rect_draw_object_t >( bar_start, bar_end,
						                                         ImColor( 0.f, 0.f, 0.f, alpha ), ImColor( 0.f, 0.f, 0.f, alpha ), false, 0.f,
						                                         ImDrawFlags_::ImDrawFlags_None, 1.f, rect_flag_outer_outline ) );
						if ( position == 1 || position == 2 ) {
							g_render.m_draw_data.emplace_back(
								e_draw_type::draw_type_rect,
								std::make_any< rect_draw_object_t >(
									c_vector_2d( bar_start.m_x, bar_start.m_y + box.m_height * ( 1.f - factor ) ), 
									c_vector_2d( bar_end.m_x, bar_end.m_y ),
									GET_VARIABLE( g_variables.m_player_ammo_bar_color, c_color ).get_u32( alpha ), ImColor( 0.f, 0.f, 0.f, alpha ), false,
									0.f, ImDrawFlags_::ImDrawFlags_None, 1.f, rect_flag_outer_outline ) );
						} else {
							g_render.m_draw_data.emplace_back(
								e_draw_type::draw_type_rect,
								std::make_any< rect_draw_object_t >(
									bar_start, bar_fill_end,
									GET_VARIABLE( g_variables.m_player_ammo_bar_color, c_color ).get_u32( alpha ), ImColor( 0.f, 0.f, 0.f, alpha ), false,
									0.f, ImDrawFlags_::ImDrawFlags_None, 1.f, rect_flag_outer_outline ) );
						}
					};
				}( );
				[ & ]( ) {
					if ( GET_VARIABLE( g_variables.m_weapon_name, bool ) ) {
						c_weapon_data* wd = this->m_backup_player_data[ index ].m_weapon_data;
						if ( !wd )
							return;
						std::string display_name;
						
						// Get weapon name without prefix
						if ( wd->m_weapon_name && *wd->m_weapon_name ) {
							display_name = wd->m_weapon_name;
							if ( display_name.size( ) > 7 && display_name.substr( 0, 7 ) == "weapon_" )
								display_name = display_name.substr( 7 );
						}
						
						// Map to proper English names
						static std::map<std::string, std::string> weapon_names = {
							{"ak47", "AK-47"}, {"aug", "AUG"}, {"awp", "AWP"}, {"bizon", "PP-Bizon"},
							{"cz75a", "CZ75-Auto"}, {"deagle", "Desert Eagle"}, {"elite", "Dual Berettas"},
							{"famas", "FAMAS"}, {"fiveseven", "Five-SeveN"}, {"g3sg1", "G3SG1"},
							{"galilar", "Galil AR"}, {"glock", "Glock-18"}, {"hkp2000", "P2000"},
							{"m249", "M249"}, {"m4a1", "M4A4"}, {"m4a1_silencer", "M4A1-S"},
							{"mac10", "MAC-10"}, {"mag7", "MAG-7"}, {"mp5sd", "MP5-SD"},
							{"mp7", "MP7"}, {"mp9", "MP9"}, {"negev", "Negev"},
							{"nova", "Nova"}, {"p250", "P250"}, {"p90", "P90"},
							{"revolver", "R8 Revolver"}, {"sawedoff", "Sawed-Off"}, {"scar20", "SCAR-20"},
							{"sg556", "SG 553"}, {"ssg08", "SSG 08"}, {"tec9", "Tec-9"},
							{"ump45", "UMP-45"}, {"usp_silencer", "USP-S"}, {"xm1014", "XM1014"},
							{"knife", "Knife"}, {"knife_m9_bayonet", "M9 Bayonet"}, {"bayonet", "Bayonet"},
							{"knife_flip", "Flip Knife"}, {"knife_gut", "Gut Knife"}, {"knife_karambit", "Karambit"},
							{"knife_tactical", "Huntsman Knife"}, {"knife_falchion", "Falchion Knife"},
							{"knife_survival_bowie", "Bowie Knife"}, {"knife_butterfly", "Butterfly Knife"},
							{"knife_push", "Shadow Daggers"}, {"knife_ursus", "Ursus Knife"},
							{"knife_gypsy_jackknife", "Navaja Knife"}, {"knife_stiletto", "Stiletto Knife"},
							{"knife_widowmaker", "Talon Knife"}, {"knife_css", "Classic Knife"},
							{"knife_cord", "Paracord Knife"}, {"knife_canis", "Survival Knife"},
							{"knife_outdoor", "Nomad Knife"}, {"knife_skeleton", "Skeleton Knife"},
							{"hegrenade", "HE Grenade"}, {"flashbang", "Flashbang"}, {"smokegrenade", "Smoke Grenade"},
							{"molotov", "Molotov"}, {"incgrenade", "Incendiary Grenade"}, {"decoy", "Decoy Grenade"},
							{"c4", "C4 Explosive"}, {"taser", "Zeus x27"}
						};
						
						auto it = weapon_names.find(display_name);
						if (it != weapon_names.end()) {
							display_name = it->second;
						}
						
						if ( display_name.empty( ) )
							return;
						const auto text_size = fonts_for_gui::verdanarus_esp_font->CalcTextSizeA( fonts_for_gui::verdanarus_esp_font->FontSize,
						                                                                          FLT_MAX, 0.f, display_name.c_str( ) );
						int position = GET_VARIABLE( g_variables.m_weapon_name_position, int );
						c_vector_2d text_pos;
						switch ( position ) {
							case 0:
								text_pos = c_vector_2d( box.m_left + ( box.m_width - text_size.x ) / 2,
								                       box.m_top - text_size.y - 2 + padding[ e_padding_direction::padding_direction_top ] );
								padding[ e_padding_direction::padding_direction_top ] -= ( text_size.y + 1.f );
								break;
							case 1:
								text_pos = c_vector_2d( box.m_left - text_size.x - 2,
								                       box.m_top + ( box.m_height - text_size.y ) / 2 );
								break;
							case 2:
								text_pos = c_vector_2d( box.m_right + 2,
								                       box.m_top + ( box.m_height - text_size.y ) / 2 );
								break;
							case 3:
								text_pos = c_vector_2d( box.m_left + ( box.m_width - text_size.x ) / 2,
								                       box.m_bottom + 2 + padding[ e_padding_direction::padding_direction_bottom ] );
								padding[ e_padding_direction::padding_direction_bottom ] += ( text_size.y + 1.f );
								break;
						}
						g_render.m_draw_data.emplace_back(
							e_draw_type::draw_type_text,
							std::make_any< text_draw_object_t >(
								fonts_for_gui::verdanarus_esp_font,
								text_pos,
								display_name, GET_VARIABLE( g_variables.m_weapon_name_color, c_color ).get_u32( alpha ),
								ImColor( 0.f, 0.f, 0.f, alpha ), 
								static_cast<e_text_flags>(text_flag_outline | text_flag_dropshadow) ) );
					}
				}( );
				if ( GET_VARIABLE( g_variables.m_weapon_icon, bool ) ) {
					if ( !( item_definition_index == e_item_definition_index::weapon_shield ||
					        item_definition_index == e_item_definition_index::weapon_breachcharge ||
					        item_definition_index == e_item_definition_index::weapon_bumpmine ) ) {
						const auto text      = reinterpret_cast< const char* >( g_utilities.get_weapon_icon( item_definition_index ) );
						const auto text_size = g_render.m_fonts[ e_font_names::font_name_icon_12 ]->CalcTextSizeA(
							g_render.m_fonts[ e_font_names::font_name_icon_12 ]->FontSize, FLT_MAX, 0.f, text );
						g_render.m_draw_data.emplace_back(
							e_draw_type::draw_type_text,
							std::make_any< text_draw_object_t >(
								g_render.m_fonts[ e_font_names::font_name_icon_12 ],
								c_vector_2d( box.m_left + ( box.m_width - text_size.x ) / 2,
						                     box.m_bottom + 2 + padding[ e_padding_direction::padding_direction_bottom ] ),
								text, GET_VARIABLE( g_variables.m_weapon_icon_color, c_color ).get_u32( alpha ), ImColor( 0.f, 0.f, 0.f, alpha ),
								static_cast<e_text_flags>(text_flag_outline | text_flag_dropshadow) ) );
						padding[ e_padding_direction::padding_direction_bottom ] += text_size.y;
					}
				}
			}
		}
		if ( GET_VARIABLE( g_variables.m_players_avatar, bool ) ) {
			constexpr const static auto avatar_size = 14.f;
			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_texture,
			                                   std::make_any< texture_draw_object_t >(
												   c_vector_2d( box.m_left + ( box.m_width - avatar_size ) * 0.5f,
			                                                    box.m_top - 5 - avatar_size - padding[ e_padding_direction::padding_direction_top ] ),
												   c_vector_2d( avatar_size, avatar_size ), ImColor( 1.f, 1.f, 1.f, alpha ),
												   player_info.m_fake_player ? team == e_team_id::team_tt   ? g_render.m_terrorist_avatar
			                                                                   : team == e_team_id::team_ct ? g_render.m_counter_terrorist_avatar
			                                                                                                : nullptr
																			 : g_avatar_cache[ index ],
												   0.f, ImDrawFlags_::ImDrawFlags_None ) );
			padding[ e_padding_direction::padding_direction_top ] -= avatar_size;
		}
		if ( GET_VARIABLE( g_variables.health_bar_enable, bool ) ) {
			static float prev_player_hp[ 65 ];
			int idx = entity->get_index( );
			prev_player_hp[ idx ] = entity->get_health( );
			int g = static_cast< int >( entity->get_health( ) * 2.55f );
			int r = 255 - g;
			unsigned int rgb_health_color = c_color::from_hsb( hue, 1.f, 1.f, 1.f ).get_u32( alpha );
			unsigned int health_bar         = GET_VARIABLE( g_variables.colors_health_bar, c_color ).get_u32( alpha );
			unsigned int health_bar_upper   = GET_VARIABLE( g_variables.colors_health_bar_upper, c_color ).get_u32( alpha );
			unsigned int health_bar_lower   = GET_VARIABLE( g_variables.colors_health_bar_lower, c_color ).get_u32( alpha );
			unsigned int health_bar_outline = GET_VARIABLE( g_variables.colors_health_bar_outline, c_color ).get_u32( alpha );
			const int health_bar_position = GET_VARIABLE( g_variables.health_bar_position, int );
			const int bar_size = GET_VARIABLE( g_variables.health_bar_size, int );
			const float health_fraction = prev_player_hp[ idx ] / 100.f;
			auto draw_vertical_bar = [&]( float x_pos ) {
				if ( GET_VARIABLE( g_variables.health_bar_background, bool ) )
					g_draw.rect_filled( x_pos - 1, box.m_top - 1, bar_size + 2, box.m_height + 2, health_bar_outline );
				else
					g_draw.rect_outlined( x_pos, box.m_top - 1, bar_size, box.m_height + 2, health_bar_outline );

				if ( !GET_VARIABLE( g_variables.health_bar_baseonhealth, bool ) && !GET_VARIABLE( g_variables.health_bar_gradient, bool ) )
					g_draw.rect_filled( x_pos, box.m_top + ( box.m_height - box.m_height * health_fraction ), bar_size, box.m_height * health_fraction, health_bar );
				if ( GET_VARIABLE( g_variables.health_bar_baseonhealth, bool ) )
					g_draw.rect_filled( x_pos, box.m_top + ( box.m_height - box.m_height * health_fraction ), bar_size, box.m_height * health_fraction, rgb_health_color );
				if ( GET_VARIABLE( g_variables.health_bar_gradient, bool ) )
					g_draw.rect_gradient( x_pos, box.m_top + ( box.m_height - box.m_height * health_fraction ), bar_size, box.m_height * health_fraction, health_bar_upper, health_bar_upper, health_bar_lower, health_bar_lower );
			};
			auto draw_horizontal_bar = [&]( float y_pos ) {
				if ( GET_VARIABLE( g_variables.health_bar_background, bool ) )
					g_draw.rect_filled( box.m_left - 1, y_pos - 1, box.m_width + 2, bar_size + 2, health_bar_outline );
				else
					g_draw.rect_outlined( box.m_left - 1, y_pos, box.m_width + 2, bar_size, health_bar_outline );

				if ( !GET_VARIABLE( g_variables.health_bar_baseonhealth, bool ) && !GET_VARIABLE( g_variables.health_bar_gradient, bool ) )
					g_draw.rect_filled( box.m_left, y_pos, box.m_width * health_fraction, bar_size, health_bar );
				if ( GET_VARIABLE( g_variables.health_bar_baseonhealth, bool ) )
					g_draw.rect_filled( box.m_left, y_pos, box.m_width * health_fraction, bar_size, rgb_health_color );
				if ( GET_VARIABLE( g_variables.health_bar_gradient, bool ) )
					g_draw.rect_gradient( box.m_left, y_pos, box.m_width * health_fraction, bar_size, health_bar_upper, health_bar_lower, health_bar_upper, health_bar_lower );
			};
			switch ( health_bar_position ) {
			case 0: draw_horizontal_bar( box.m_top - bar_size - 3 ); break;
			case 1: draw_horizontal_bar( box.m_bottom + 3 ); break;
			case 3: draw_vertical_bar( box.m_left - bar_size - 3 ); break;
			case 2:
			default: draw_vertical_bar( box.m_right + 3 ); break;
			}
			if ( GET_VARIABLE( g_variables.health_bar_text, bool ) && entity->get_health( ) < 99 ) {
				ImVec2 size = fonts_for_gui::verdanarus_esp_font->CalcTextSizeA( fonts_for_gui::verdanarus_esp_font->FontSize, FLT_MAX, NULL,
				                                                        std::to_string( entity->get_health( ) ).c_str( ) );
				text_draw_object_t txt{ };
				txt.m_position = c_vector_2d( ( box.m_left - ( size.x + ( float )GET_VARIABLE( g_variables.health_bar_size, int ) ) ) - size.x / 2.f,
				                              box.m_top + ( box.m_height - box.m_height * prev_player_hp[ idx ] / 100.f ) );
				txt.m_color    = c_color( 1.f, 1.f, 1.f ).get_u32( alpha );
				txt.m_font          = fonts_for_gui::verdanarus_esp_font;
				txt.m_outline_color = c_color( 0.f, 0.f, 0.f, alpha ).get_u32( alpha );
				txt.m_draw_flags    = static_cast<e_text_flags>(text_flag_outline | text_flag_dropshadow);
				txt.m_text          = std::to_string( entity->get_health( ) );
				g_render.m_draw_data.emplace_back( e_draw_type::draw_type_text, std::make_any< text_draw_object_t >( txt ) );
			}
			if ( GET_VARIABLE( g_variables.health_bar_hp_indicator, bool ) && fonts_for_gui::silkscreen_font ) {
				std::string hp_text = std::to_string( entity->get_health( ) ) + " hp";
				ImVec2 text_size = fonts_for_gui::silkscreen_font->CalcTextSizeA( fonts_for_gui::silkscreen_font->FontSize, FLT_MAX, 0.f, hp_text.c_str( ) );
				unsigned int text_color;
				if ( GET_VARIABLE( g_variables.health_bar_baseonhealth, bool ) ) {
					text_color = rgb_health_color;
				} else if ( GET_VARIABLE( g_variables.health_bar_gradient, bool ) ) {
					text_color = health_bar_upper;
				} else {
					text_color = health_bar;
				}
				float bar_top = box.m_top + ( box.m_height - box.m_height * prev_player_hp[ idx ] / 100.f );
				g_render.m_draw_data.emplace_back(
					e_draw_type::draw_type_text,
					std::make_any< text_draw_object_t >( 
						fonts_for_gui::silkscreen_font,
						c_vector_2d( box.m_right + 3, bar_top - text_size.y / 2 ),
						hp_text,
						text_color,
						c_color( 0.f, 0.f, 0.f, alpha ).get_u32( alpha ),
						static_cast<e_text_flags>(text_flag_outline | text_flag_dropshadow) ) );
			}
		}
		if ( GET_VARIABLE( g_variables.m_players_health_text, bool ) && fonts_for_gui::silkscreen_font ) {
			std::string hp_text = std::to_string( entity->get_health( ) ) + " hp";
			ImVec2 text_size = fonts_for_gui::silkscreen_font->CalcTextSizeA( fonts_for_gui::silkscreen_font->FontSize, FLT_MAX, 0.f, hp_text.c_str( ) );
			int position = GET_VARIABLE( g_variables.m_players_health_text_position, int );
			c_vector_2d text_pos;
			switch ( position ) {
				case 0:
					text_pos = c_vector_2d( box.m_left + ( box.m_width - text_size.x ) / 2, 
					                       box.m_top - text_size.y - 2 - padding[ e_padding_direction::padding_direction_top ] );
					padding[ e_padding_direction::padding_direction_top ] += ( text_size.y + 2.f );
					break;
				case 1:
					text_pos = c_vector_2d( box.m_left - text_size.x - 2 - padding[ e_padding_direction::padding_direction_left ], 
					                       box.m_top );
					padding[ e_padding_direction::padding_direction_left ] += ( text_size.x + 2.f );
					break;
				case 2:
					text_pos = c_vector_2d( box.m_right + GET_VARIABLE( g_variables.health_bar_size, int ) + 8 + padding[ e_padding_direction::padding_direction_right ], 
					                       box.m_top );
					padding[ e_padding_direction::padding_direction_right ] += ( text_size.x + 2.f );
					break;
				case 3:
					text_pos = c_vector_2d( box.m_left + ( box.m_width - text_size.x ) / 2, 
					                       box.m_bottom + 2 + padding[ e_padding_direction::padding_direction_bottom ] );
					padding[ e_padding_direction::padding_direction_bottom ] += ( text_size.y + 2.f );
					break;
			}
			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_text,
				std::make_any< text_draw_object_t >( fonts_for_gui::silkscreen_font,
				                                     text_pos,
				                                     hp_text, 
				                                     c_color( 1.f, 1.f, 1.f ).get_u32( alpha ),
				                                     c_color( 0.f, 0.f, 0.f, alpha ).get_u32( alpha ), 
				                                     static_cast<e_text_flags>(text_flag_outline | text_flag_dropshadow) ) );
		}
		if ( GET_VARIABLE( g_variables.m_player_ammo_text, bool ) && fonts_for_gui::silkscreen_font ) {
			const auto active_weapon = this->m_backup_player_data[ index ].m_active_weapon;
			if ( active_weapon && this->m_backup_player_data[ index ].m_weapon_data ) {
				const int ammo = active_weapon->get_ammo( );
				const int max_ammo = this->m_backup_player_data[ index ].m_weapon_data->m_max_clip1;
				if ( max_ammo > 0 ) {
					std::string ammo_text = std::to_string( ammo ) + "/" + std::to_string( max_ammo );
					ImVec2 text_size = fonts_for_gui::silkscreen_font->CalcTextSizeA( fonts_for_gui::silkscreen_font->FontSize, FLT_MAX, 0.f, ammo_text.c_str( ) );
					int position = GET_VARIABLE( g_variables.m_player_ammo_text_position, int );
					c_vector_2d text_pos;
					switch ( position ) {
						case 0:
							text_pos = c_vector_2d( box.m_left + ( box.m_width - text_size.x ) / 2, 
							                       box.m_top - text_size.y - 2 - padding[ e_padding_direction::padding_direction_top ] );
							padding[ e_padding_direction::padding_direction_top ] += ( text_size.y + 2.f );
							break;
						case 1:
							text_pos = c_vector_2d( box.m_left - text_size.x - 2 - padding[ e_padding_direction::padding_direction_left ], 
							                       box.m_top );
							padding[ e_padding_direction::padding_direction_left ] += ( text_size.x + 2.f );
							break;
						case 2:
							text_pos = c_vector_2d( box.m_right + GET_VARIABLE( g_variables.health_bar_size, int ) + 8 + padding[ e_padding_direction::padding_direction_right ], 
							                       box.m_top );
							padding[ e_padding_direction::padding_direction_right ] += ( text_size.x + 2.f );
							break;
						case 3:
							text_pos = c_vector_2d( box.m_left + ( box.m_width - text_size.x ) / 2, 
							                       box.m_bottom + 2 + padding[ e_padding_direction::padding_direction_bottom ] );
							padding[ e_padding_direction::padding_direction_bottom ] += ( text_size.y + 2.f );
							break;
					}
					g_render.m_draw_data.emplace_back(
						e_draw_type::draw_type_text,
						std::make_any< text_draw_object_t >( fonts_for_gui::silkscreen_font,
						                                     text_pos,
						                                     ammo_text, 
						                                     c_color( 1.f, 1.f, 1.f ).get_u32( alpha ),
						                                     c_color( 0.f, 0.f, 0.f, alpha ).get_u32( alpha ), 
						                                     static_cast<e_text_flags>(text_flag_outline | text_flag_dropshadow) ) );
				}
			}
		}
		[ & ]( const bool draw_skeleton ) {
			if ( !draw_skeleton )
				return;
			auto draw_skeleton_matrix = [ & ]( matrix3x4_t* bone_matrix ) {
				if ( !bone_matrix )
					return;
				const auto model = client_renderable->get_model( );
				if ( model ) {
					const auto studio_model = g_interfaces.m_model_info->get_studio_model( model );
					if ( studio_model ) {
						c_vector child_position = { }, parent_position = { };
						c_vector_2d child_screen_position = { }, parent_screen_position = { };
						c_vector upper_direction = c_vector( bone_matrix[ 7 ][ 0 ][ 3 ], bone_matrix[ 7 ][ 1 ][ 3 ], bone_matrix[ 7 ][ 2 ][ 3 ] ) -
						                           c_vector( bone_matrix[ 6 ][ 0 ][ 3 ], bone_matrix[ 6 ][ 1 ][ 3 ], bone_matrix[ 6 ][ 2 ][ 3 ] );
						c_vector breast_bone =
							c_vector( bone_matrix[ 6 ][ 0 ][ 3 ], bone_matrix[ 6 ][ 1 ][ 3 ], bone_matrix[ 6 ][ 2 ][ 3 ] ) + upper_direction * 0.5f;
						for ( int i = 0; i < studio_model->n_bones; i++ ) {
							mstudiobone_t* bone = studio_model->get_bone( i );
							if ( !bone )
								continue;
							if ( bone->m_parent == -1 )
								continue;
							if ( !( bone->m_flags & 0x00000100  ) )
								continue;
							child_position  = c_vector( bone_matrix[ i ][ 0 ][ 3 ], bone_matrix[ i ][ 1 ][ 3 ], bone_matrix[ i ][ 2 ][ 3 ] );
							parent_position = c_vector( bone_matrix[ bone->m_parent ][ 0 ][ 3 ], bone_matrix[ bone->m_parent ][ 1 ][ 3 ],
							                            bone_matrix[ bone->m_parent ][ 2 ][ 3 ] );
							c_vector delta_child  = child_position - breast_bone;
							c_vector delta_parent = parent_position - breast_bone;
							if ( delta_parent.length( ) < 9.0f && delta_child.length( ) < 9.0f )
								parent_position = breast_bone;
							if ( i == 5 )
								child_position = breast_bone;
							if ( fabs( delta_child.m_z ) < 5.0f && delta_parent.length( ) < 5.0f && delta_child.length( ) < 5.0f || i == 6 )
								continue;
							if ( g_render.world_to_screen( child_position, child_screen_position ) &&
							     g_render.world_to_screen( parent_position, parent_screen_position ) ) {
								if ( GET_VARIABLE( g_variables.m_players_skeleton_outline, bool ) ) {
									c_color outline_color = GET_VARIABLE( g_variables.m_players_skeleton_outline_color, c_color );
									for ( int ox = -1; ox <= 1; ox++ ) {
										for ( int oy = -1; oy <= 1; oy++ ) {
											if ( ox == 0 && oy == 0 ) continue;
											g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line,
											                                   std::make_any< line_draw_object_t >(
																				   c_vector_2d( child_screen_position.m_x + ox, child_screen_position.m_y + oy ),
																				   c_vector_2d( parent_screen_position.m_x + ox, parent_screen_position.m_y + oy ),
																				   outline_color.get_u32( alpha ),
																				   1.f ) );
										}
									}
								}
								g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line,
								                                   std::make_any< line_draw_object_t >(
																	   child_screen_position, parent_screen_position,
																	   GET_VARIABLE( g_variables.m_players_skeleton_color, c_color ).get_u32( alpha ),
																	   1.f ) );
							}
						}
					}
				}
			};
			if ( GET_VARIABLE( g_variables.m_players_skeleton_type, int ) == 1 && GET_VARIABLE( g_variables.m_backtrack_enable, bool ) ) {
				auto record = &Backtrack::records[ entity->get_index( ) ];
				if ( !Backtrack::valid( record->front( ).simulationTime ) )
					return;
				auto bone_matrix = record->front( ).matrix;
				draw_skeleton_matrix( bone_matrix );
			} else {
				matrix3x4_t bone_matrix[ 128 ]{ };
				memcpy( bone_matrix, entity->get_cached_bone_data( ).get_elements( ),
				        entity->get_cached_bone_data( ).count( ) * sizeof( matrix3x4_t ) );
				draw_skeleton_matrix( bone_matrix );
			}
		}( GET_VARIABLE( g_variables.m_players_skeleton, bool ) );
		if ( entity->is_dormant( ) ) {
			entity->get_flags( ) = backup_flags;
			entity->set_abs_origin( backup_origin );
		}
	} );
}
