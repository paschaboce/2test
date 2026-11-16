#include "misc.h"
#include "../../dependencies/imgui/helpers/easing.h"
#include "../../elements/animations.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../../globals/logger/logger.h"
#include "../../globals/crash_logger/crash_logger.h"
#include "../avatar_cache/avatar_cache.h"
#include "../entity_cache/entity_cache.h"
#include "../instalisation fonts/fonts2.h"
#include "../menu/menu.h"
#include "../movement/movement.h"
#include "../movement/edgebug.h"
#include "image_display.h"
#include "../../chat_hud.h"
#include "spectator_gif_data.h"
#include "spectator_gif_gdiplus.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <deque>
#include <algorithm>
#include <string>
#include <vector>
#include <thread>
#include <wininet.h>
#include <cmath>
#include <fstream>
#include "../prediction/prediction.h"
void n_misc::impl_t::on_create_move_pre( )
{
	this->disable_post_processing( );
	this->remove_panorama_blur( );
	this->practice_window_think( );
	this->mouse_fix();
	this->camera_fix();
}
void n_misc::impl_t::on_paint_traverse( )
{
	if ( !g_interfaces.m_engine_client || !g_interfaces.m_panel ) {
		return;
	}
	this->check_and_add_points_for_detects( );
	static std::string last_map = "";
	static bool was_connected   = false;
	if ( g_interfaces.m_engine_client ) {
		std::string current_map     = g_interfaces.m_engine_client->get_level_name( );
		bool is_connected           = g_interfaces.m_engine_client->is_connected( );
		was_connected = is_connected;
	}
	this->draw_spectating_local( );
	if ( GET_VARIABLE( g_variables.m_clantag, bool ) ) {
		this->update_clantag( );
	}
}

void n_misc::impl_t::killfeed_changer( game_event_t* event )
{
	if ( !event || !GET_VARIABLE( g_variables.m_killfeed_changer, bool ) || !g_interfaces.m_engine_client )
		return;

	const int local_index = g_interfaces.m_engine_client->get_local_player( );
	const int attacker    = g_interfaces.m_engine_client->get_player_for_user_id( event->get_int( "attacker" ) );
	const int victim      = g_interfaces.m_engine_client->get_player_for_user_id( event->get_int( "userid" ) );
	if ( attacker != local_index || victim == local_index )
		return;

	if ( GET_VARIABLE( g_variables.m_killfeed_headshot, bool ) )
		event->set_bool( "headshot", true );
	if ( GET_VARIABLE( g_variables.m_killfeed_dominated, bool ) )
		event->set_int( "Dominated", 1 );
	if ( GET_VARIABLE( g_variables.m_killfeed_revenge, bool ) )
		event->set_int( "Revenge", 1 );
	if ( GET_VARIABLE( g_variables.m_killfeed_penetrated, bool ) )
		event->set_int( "penetrated", 1 );
	if ( GET_VARIABLE( g_variables.m_killfeed_noscope, bool ) )
		event->set_int( "noscope", 1 );
	if ( GET_VARIABLE( g_variables.m_killfeed_thrusmoke, bool ) )
		event->set_int( "thrusmoke", 1 );
	if ( GET_VARIABLE( g_variables.m_killfeed_attackerblind, bool ) )
		event->set_int( "attackerblind", 1 );
}

void n_misc::impl_t::vote_revealer( game_event_t* event )
{
	if ( !event || !GET_VARIABLE( g_variables.m_reveal_votes, bool ) || !n_interfaces::chat_element )
		return;

	const int entity_index = event->get_int( "entityid" );
	auto* entity = g_interfaces.m_client_entity_list->get< c_base_entity >( entity_index );
	if ( !entity || !entity->is_player( ) )
		return;

	player_info_t info{ };
	std::string name = "unknown";
	if ( g_interfaces.m_engine_client->get_player_info( entity_index, &info ) )
		name = g_menu.get_server_player_name( entity_index, info.m_name );

	const bool voted_yes = event->get_int( "vote_option" ) == 0;
	n_interfaces::chat_element->chatprintf( 0, 0, "%s", make_kami_chat_message( GET_VARIABLE( g_variables.m_accent, c_color ),
		std::format( "{} voted {}", name, voted_yes ? "Yes" : "No" ).c_str( ) ).c_str( ) );
}

void n_misc::impl_t::add_points_on_detect( )
{
}
void n_misc::impl_t::reset_detect_states( )
{
	m_points_system.m_jb_detected        = false;
	m_points_system.m_eb_detected        = false;
	m_points_system.m_ast_detected       = false;
	m_points_system.m_pixelsurf_detected = false;
	m_points_system.m_fm_detected        = false;
	m_points_system.m_head_ceiling_detected = false;
	m_points_system.m_tb_detected              = false;
	m_points_system.m_jb_points_given        = 0;
	m_points_system.m_eb_points_given        = 0;
	m_points_system.m_ast_points_given       = 0;
	m_points_system.m_pixelsurf_points_given = 0;
	m_points_system.m_fm_points_given        = 0;
	m_points_system.m_head_ceiling_points_given = 0;
	m_points_system.m_tb_points_given          = 0;
	m_points_system.m_pixelsurf_active          = false;
	m_points_system.m_pixelsurf_start_time      = 0.0f;
	m_points_system.m_last_pixelsurf_bonus_time = 0.0f;
	m_points_system.m_last_detect_time          = 0.0f;
	m_points_system.m_previous_points           = 0;
}
void n_misc::impl_t::update_pixelsurf_bonus_points( )
{
	if ( !m_points_system.m_pixelsurf_active )
		return;
	float current_time          = ImGui::GetTime( );
	float time_since_last_bonus = current_time - m_points_system.m_last_pixelsurf_bonus_time;
	if ( time_since_last_bonus >= 0.35f ) {
		m_points_system.m_current_points += 25;
		m_points_system.m_last_pixelsurf_bonus_time = current_time;
		m_points_system.m_last_detect_time = current_time;
	}
}
void n_misc::impl_t::check_and_add_points_for_detects( )
{
	if ( g_menu.m_opened ) {
		m_points_system.m_jb_detected = false;
		m_points_system.m_eb_detected = false;
		m_points_system.m_ast_detected = false;
		m_points_system.m_pixelsurf_detected = false;
		m_points_system.m_fm_detected = false;
		m_points_system.m_head_ceiling_detected = false;
		m_points_system.m_tb_detected              = false;
		return;
	}
	if ( !g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	if ( !GET_VARIABLE( g_variables.m_pts_display, bool ) && !GET_VARIABLE( g_variables.m_pts_show_picture, bool ) )
		return;
	if ( !g_ctx.m_cmd )
		return;
	const bool any_action = ( g_ctx.m_cmd->m_buttons & ( in_jump | in_duck ) ) ||
		g_input.check_input( &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) ) ||
		g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) ||
		g_input.check_input( &GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ) ) ||
		g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ) ) ||
		g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) );
	const bool any_active_state = g_movement.m_pixelsurf_data.m_predicted_succesful ||
		g_movement.m_pixelsurf_data.m_in_pixel_surf || g_edgebug.m_found || g_movement.m_fireman_data.is_ladder ||
		g_movement.is_head_ceiling_detected( ) || g_movement.m_air_stuck_data.m_antoha_bug_detect;
	if ( !any_action && !any_active_state )
		return;
	if ( !GET_VARIABLE( g_variables.m_pts_display, bool ) && !GET_VARIABLE( g_variables.m_pts_show_picture, bool ) )
		return;
	float current_time = ImGui::GetTime( );
	static bool prev_jb_state = false;
	bool current_jb_state = g_movement.m_jumpbug_data.m_can_jb;
	const bool jb_input = g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) );
	bool is_in_air = !(g_ctx.m_local->get_flags() & 1);
	if ( current_jb_state && !prev_jb_state && jb_input && is_in_air ) {
		m_points_system.m_current_points += 50;
		m_points_system.m_last_detect_time = current_time;
		m_points_system.m_jb_points_given += 50;
	}
	prev_jb_state = current_jb_state;
	if ( g_edgebug.m_found && !m_points_system.m_eb_detected ) {
		m_points_system.m_eb_detected = true;
		m_points_system.m_current_points += 125;
		m_points_system.m_last_detect_time = current_time;
		m_points_system.m_eb_points_given += 125;
	} else if ( !g_edgebug.m_found ) {
		m_points_system.m_eb_detected = false;
		m_points_system.m_eb_points_given = 0;
	}
	extern bool HITGODA, HITGODA2;
	if ( ( HITGODA || HITGODA2 ) && !m_points_system.m_ast_detected ) {
		m_points_system.m_ast_detected     = true;
		m_points_system.m_last_detect_time = current_time;
	} else if ( !HITGODA && !HITGODA2 ) {
		m_points_system.m_ast_detected = false;
	}
	bool pixelsurf_active = g_movement.m_pixelsurf_data.m_predicted_succesful || g_movement.m_pixelsurf_data.m_in_pixel_surf;
	if ( pixelsurf_active && !m_points_system.m_pixelsurf_detected ) {
		m_points_system.m_pixelsurf_detected        = true;
		m_points_system.m_pixelsurf_active          = true;
		m_points_system.m_pixelsurf_start_time      = current_time;
		m_points_system.m_last_pixelsurf_bonus_time = current_time;
		m_points_system.m_current_points += 125;
		m_points_system.m_last_detect_time = current_time;
		m_points_system.m_pixelsurf_points_given += 125;
	} else if ( !pixelsurf_active ) {
		m_points_system.m_pixelsurf_detected     = false;
		m_points_system.m_pixelsurf_active       = false;
		m_points_system.m_pixelsurf_points_given = 0;
	}
	if ( g_movement.m_fireman_data.is_ladder && !m_points_system.m_fm_detected ) {
		m_points_system.m_fm_detected = true;
		m_points_system.m_current_points += 275;
		m_points_system.m_last_detect_time = current_time;
		m_points_system.m_fm_points_given += 275;
	} else if ( !g_movement.m_fireman_data.is_ladder ) {
		m_points_system.m_fm_detected = false;
		m_points_system.m_fm_points_given = 0;
	}
	static bool prev_head_ceiling_state = false;
	bool current_head_ceiling_state = g_movement.is_head_ceiling_detected( );
	if ( current_head_ceiling_state && !prev_head_ceiling_state && !m_points_system.m_head_ceiling_detected ) {
		m_points_system.m_head_ceiling_detected = true;
		m_points_system.m_current_points += 50;
		m_points_system.m_last_detect_time = current_time;
		m_points_system.m_head_ceiling_points_given += 50;
	} else if ( !current_head_ceiling_state ) {
		m_points_system.m_head_ceiling_detected = false;
		m_points_system.m_head_ceiling_points_given = 0;
	}
	prev_head_ceiling_state = current_head_ceiling_state;
	bool current_tb_state = g_movement.m_air_stuck_data.m_antoha_bug_detect;
	if ( current_tb_state && !m_points_system.m_tb_detected ) {
		m_points_system.m_tb_detected = true;
		m_points_system.m_current_points += 50;
		m_points_system.m_last_detect_time = current_time;
		m_points_system.m_tb_points_given += 50;
	} else if ( !current_tb_state ) {
		m_points_system.m_tb_detected = false;
		m_points_system.m_tb_points_given = 0;
	}
	update_pixelsurf_bonus_points( );
}
static std::chrono::steady_clock::time_point progressStartTime = std::chrono::steady_clock::now( );
static double calculatedPositionMs                             = 0.0;
static std::string lastTitle                                   = "";
void UpdateCalculatedTrackPosition( Player& player )
{
	if ( lastTitle != player.Title ) {
		calculatedPositionMs = 0.0;
		progressStartTime    = std::chrono::steady_clock::now( );
		lastTitle            = player.Title;
	}
	if ( player.isPlaying ) {
		auto now     = std::chrono::steady_clock::now( );
		auto deltaMs = std::chrono::duration_cast< std::chrono::milliseconds >( now - progressStartTime ).count( );
		calculatedPositionMs += deltaMs;
		progressStartTime = now;
	}
}
void RenderMediaPlayer( )
{
	UpdateCalculatedTrackPosition( player );
	float progress = 0.0f;
	if ( player.TotalTime > 0 ) {
		progress = static_cast< float >( calculatedPositionMs ) / static_cast< float >( player.TotalTime );
		if ( progress > 1.0f )
			progress = 1.0f;
	}
	static float smoothProgress = 0.0f;
	if ( progress < smoothProgress )
		smoothProgress = progress;
	smoothProgress += ( progress - smoothProgress ) * 0.1f;
	static ImVec2 sz{ };
	int x = g_ctx.m_width;
	int y = g_ctx.m_height;
	if ( GET_VARIABLE( g_variables.m_watermark, bool ) )
		ImGui::SetNextWindowPos( { x - sz.x - 10, 45 } );
	else
		ImGui::SetNextWindowPos( { x - sz.x - 10, 10 } );
	ImGui::Begin( "Media Player", nullptr,
	              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize );
	ImGui::PushFont( fonts_for_gui::regular_11 );
	float padding    = 10.0f;
	float imageWidth = 30.0f;
	auto sizex1       = ImGui::CalcTextSize( strartist.c_str( ) ).x;
	auto sizex2       = ImGui::CalcTextSize( strtitle.c_str( ) ).x;
	float windowWidth = ImGui::GetWindowSize( ).x;
	if ( albumArtTexture ) {
		ImGui::SetCursorPos( ImVec2( windowWidth - imageWidth - padding, 3 ) );
		ImGui::Image( albumArtTexture, ImVec2( imageWidth, imageWidth ) );
	}
	if ( albumArtTexture )
		ImGui::SetCursorPos( { windowWidth - imageWidth - padding - sizex1 - padding, 20 } );
	else
		ImGui::SetCursorPos( { windowWidth - padding - sizex1 - padding, 20 } );
	ImGui::TextColored( ImVec4( 0.f, 0.f, 0.f, 1.f ), strartist.c_str( ) );
	if ( albumArtTexture )
		ImGui::SetCursorPos( { windowWidth - imageWidth - padding - sizex1 - padding + 1, 19 } );
	else
		ImGui::SetCursorPos( { windowWidth - padding - sizex1 - padding + 1, 19 } );
	ImGui::TextColored( ImVec4( 0.6f, 0.6f, 0.6f, 1.f ), strartist.c_str( ) );
	if ( albumArtTexture )
		ImGui::SetCursorPos( { windowWidth - imageWidth - padding - sizex2 - padding, 5 } );
	else
		ImGui::SetCursorPos( { windowWidth - padding - sizex2 - padding, 5 } );
	ImGui::TextColored( ImVec4( 0.f, 0.f, 0.f, 1.f ), strtitle.c_str( ) );
	if ( albumArtTexture )
		ImGui::SetCursorPos( { windowWidth - imageWidth - padding - sizex2 - padding + 1, 4 } );
	else
		ImGui::SetCursorPos( { windowWidth - padding - sizex2 - padding + 1, 4 } );
	ImGui::TextColored( ImVec4( 1.f, 1.f, 1.f, 1.f ), strtitle.c_str( ) );
	if ( albumArtTexture ) {
		ImGui::PushItemWidth( 158 );
		ImGui::SetCursorPos( { sz.x - imageWidth - padding - 126, 40 } );
		ImGui::ProgressBar( smoothProgress, ImVec2( 0.0f, 2.0f ) );
		ImGui::PopItemWidth( );
	}
	ImGui::Spacing( );
	ImGui::SetWindowSize( { 400, 100 } );
	sz = ImGui::GetWindowSize( );
	ImGui::PopFont( );
	ImGui::End( );
}
static c_color get_shared_rgb_color( float dt )
{
	static float rgb_phase = 0.0f;
	bool wm_rgb       = GET_VARIABLE( g_variables.m_watermark_rgb_mode, bool );
	float& wm_speed   = GET_VARIABLE( g_variables.m_watermark_rgb_speed, float );
	if ( !wm_rgb ) {
		return GET_VARIABLE( g_variables.m_accent, c_color );
	}
	float speed = wm_speed;
	if ( speed <= 0.0f )
		speed = 0.5f;
	const float global_slow_factor = 0.2f;
	rgb_phase += dt * speed * global_slow_factor;
	if ( rgb_phase > 1.0f )
		rgb_phase -= floorf( rgb_phase );
	float h = rgb_phase * 6.0f;
	int i   = (int)h;
	float f = h - (float)i;
	float q = 1.0f - f;
	const float saturation = 0.6f;
	const float value = 0.9f;
	float r = 0.0f, g = 0.0f, b = 0.0f;
	switch ( i % 6 ) {
	case 0: r = value; g = value * (1.0f - saturation * (1.0f - f)); b = value * (1.0f - saturation); break;
	case 1: r = value * (1.0f - saturation * q); g = value; b = value * (1.0f - saturation); break;
	case 2: r = value * (1.0f - saturation); g = value; b = value * (1.0f - saturation * (1.0f - f)); break;
	case 3: r = value * (1.0f - saturation); g = value * (1.0f - saturation * q); b = value; break;
	case 4: r = value * (1.0f - saturation * (1.0f - f)); g = value * (1.0f - saturation); b = value; break;
	case 5: r = value; g = value * (1.0f - saturation); b = value * (1.0f - saturation * q); break;
	}
	return c_color(
		static_cast<int>( r * 255.0f ),
		static_cast<int>( g * 255.0f ),
		static_cast<int>( b * 255.0f ),
		255
	);
}
void n_misc::impl_t::render_watermark( )
{
	static std::string typing_text = "";
	static float typing_timer = 0.0f;
	static bool typing_deleting = false;
	static size_t typing_index = 0;
	ImDrawList* draw_list = ImGui::GetForegroundDrawList( );
	if ( !draw_list )
		return;
	if ( g_ctx.m_width <= 0 || g_ctx.m_height <= 0 )
		return;
	const int dpi_percent = std::clamp( GET_VARIABLE( g_variables.m_menu_dpi_scale, int ), 75, 200 );
	const float dpi_scale = static_cast< float >( dpi_percent ) / 100.0f;
	float watermark_width = 95.0f * dpi_scale;
	float watermark_height = 28.0f * dpi_scale;
	float padding_horizontal = 14.0f * dpi_scale;
	float padding_vertical = 4.0f * dpi_scale;
	const float corner_radius = 4.0f * dpi_scale;
	const float min_watermark_width = 95.0f * dpi_scale;
	bool custom_text_enabled = GET_VARIABLE( g_variables.m_watermark_custom_text, bool );
	bool mikudere_type_enabled = GET_VARIABLE( g_variables.m_watermark_mikudere_type, bool );
	int show_flags = GET_VARIABLE( g_variables.m_watermark_show_flags, int );
	bool show_user = (show_flags & 1) != 0;
	bool show_time = (show_flags & 2) != 0;
	bool show_fps = (show_flags & 4) != 0;
	bool show_pts = (show_flags & 8) != 0;
	bool has_additional_info = show_user || show_time || show_fps || show_pts;
	if ( mikudere_type_enabled ) {
		watermark_width = 95.0f * dpi_scale;
		watermark_height = 28.0f * dpi_scale;
	} else if ( custom_text_enabled || has_additional_info ) {
		ImFont* selected_font = fonts_for_gui::watermark_font;
		if ( GET_VARIABLE( g_variables.m_watermark_bold_font, bool ) && fonts_for_gui::bolder ) {
			selected_font = fonts_for_gui::bolder;
		}
		if ( selected_font ) {
			ImGui::PushFont( selected_font );
			std::string full_text = "";
			std::string main_text = "kamibebra";
			if ( GET_VARIABLE( g_variables.m_watermark_custom_text, bool ) ) {
				main_text = GET_VARIABLE( g_variables.m_watermark_custom_text_value, std::string );
			}
			bool has_additional_elements = show_user || show_time || show_fps || show_pts;
			bool typing_enabled = GET_VARIABLE( g_variables.m_watermark_typing, bool );
			static std::string last_main_text = "";
			if (main_text != last_main_text) {
				typing_index = 0;
				typing_deleting = false;
				typing_timer = 0.0f;
				last_main_text = main_text;
			}
			if ( typing_enabled ) {
				int typing_speed_chars = GET_VARIABLE( g_variables.m_watermark_typing_speed, int );
				int deleting_speed_chars = GET_VARIABLE( g_variables.m_watermark_deleting_speed, int );
				std::string text_to_animate = main_text;
				const float TYPING_SPEED = 1.0f / static_cast<float>(typing_speed_chars);
				const float DELETING_SPEED = 1.0f / static_cast<float>(deleting_speed_chars);
				static float pause_timer = 0.0f;
				static bool in_pause = false;
				const float PAUSE_AFTER_TYPING = GET_VARIABLE( g_variables.m_watermark_typing_inactive, float );
				const float PAUSE_AFTER_DELETING = GET_VARIABLE( g_variables.m_watermark_deleting_inactive, float );
				static bool pause_after_typing = false;
				if (in_pause) {
					pause_timer += ImGui::GetIO().DeltaTime;
					float required_pause = pause_after_typing ? PAUSE_AFTER_TYPING : PAUSE_AFTER_DELETING;
					if (pause_timer >= required_pause) {
						in_pause = false;
						pause_timer = 0.0f;
						if (pause_after_typing) {
							typing_deleting = true;
						} else {
							typing_deleting = false;
						}
					}
				} else {
					typing_timer += ImGui::GetIO().DeltaTime;
					float current_speed = typing_deleting ? DELETING_SPEED : TYPING_SPEED;
					if (typing_timer >= current_speed) {
						typing_timer = 0.0f;
						if (!typing_deleting) {
							if (typing_index < text_to_animate.length()) {
								typing_index++;
							} else {
								in_pause = true;
								pause_timer = 0.0f;
								pause_after_typing = true;
							}
						} else {
							if (typing_index > 0) {
								typing_index--;
							} else {
								in_pause = true;
								pause_timer = 0.0f;
								pause_after_typing = false;
							}
						}
					}
				}
				if (typing_index > 0) {
					typing_text = text_to_animate.substr(0, typing_index);
				} else {
					typing_text = "";
				}
				full_text = typing_text;
			} else {
				full_text = main_text;
				typing_index = 0;
				typing_deleting = false;
			}
			if ( show_user ) {
				std::string user_text = "dev";
				if ( g_interfaces.m_engine_client && g_interfaces.m_engine_client->is_connected( ) ) {
					player_info_t player_info;
					if ( g_interfaces.m_engine_client->get_player_info( g_interfaces.m_engine_client->get_local_player( ), &player_info ) ) {
						user_text = std::string( player_info.m_name );
					}
				}
				full_text += " | " + user_text;
			}
			if ( show_time ) {
				time_t now = time( nullptr );
				tm local_time;
				localtime_s( &local_time, &now );
				char time_buffer[6];
				snprintf( time_buffer, sizeof( time_buffer ), "%02d:%02d", local_time.tm_hour, local_time.tm_min );
				full_text += " | " + std::string( time_buffer );
			}
			if ( show_fps ) {
				static int cached_fps = 0;
				static float fps_update_timer = 0.0f;
				fps_update_timer += ImGui::GetIO( ).DeltaTime;
				if ( fps_update_timer >= 0.5f ) {
					cached_fps = static_cast< int >( 1.0f / ImGui::GetIO( ).DeltaTime );
					fps_update_timer = 0.0f;
				}
				full_text += " | " + std::to_string( cached_fps ) + " fps";
			}
			if ( show_pts ) {
				full_text += " | " + std::to_string( m_points_system.m_current_points ) + " pts";
			}
			const float selected_font_size = selected_font->FontSize * dpi_scale;
			ImVec2 text_size = selected_font->CalcTextSizeA( selected_font_size, FLT_MAX, 0.0f, full_text.c_str( ) );
			ImGui::PopFont( );
			float target_width = text_size.x + padding_horizontal * 2.0f;
			if ( target_width < min_watermark_width ) {
				target_width = min_watermark_width;
			}
			float target_height = 28.0f * dpi_scale;
			static float current_width = target_width;
			static float current_height = target_height;
			float lerp_speed = 12.0f * ImGui::GetIO().DeltaTime;
			current_width += (target_width - current_width) * lerp_speed;
			current_height += (target_height - current_height) * lerp_speed;
			watermark_width = current_width;
			watermark_height = current_height;
		}
	}
	if ( mikudere_type_enabled ) {
		watermark_width = 95.0f * dpi_scale;
		watermark_height = 28.0f * dpi_scale;
	}
	watermark_height = 28.0f * dpi_scale;
	float distance_pts_to_image = 5.0f * dpi_scale;
	bool pts_display_enabled = GET_VARIABLE( g_variables.m_pts_display, bool );
	bool show_pts_picture = GET_VARIABLE( g_variables.m_pts_show_picture, bool );
		float distance_watermark_to_pts = 5.0f * dpi_scale;
	const float pos_x = g_ctx.m_width - watermark_width - 8.0f * dpi_scale;
	const float pos_y = 8.0f * dpi_scale;
	float current_time           = ImGui::GetTime( );
	float time_since_last_detect = current_time - m_points_system.m_last_detect_time;
	static float current_pts_x     = 0.0f;
	static float current_pts_alpha = 0.0f;
	static float target_pts_x      = 0.0f;
	static float target_pts_alpha  = 0.0f;
	static bool first_run          = true;
	static bool prev_show_pts_picture = show_pts_picture;
		if ( first_run ) {
			current_pts_x = g_ctx.m_width - 90.0f * dpi_scale;
			current_pts_alpha = 0.0f;
			target_pts_x  = g_ctx.m_width - 90.0f * dpi_scale;
			target_pts_alpha = 0.0f;
		first_run     = false;
		prev_show_pts_picture = show_pts_picture;
	}
	if ( prev_show_pts_picture != show_pts_picture ) {
		std::string pts_text_recalc = std::to_string( m_points_system.m_current_points ) + " pts";
		ImVec2 pts_text_size_recalc = ImGui::CalcTextSize( pts_text_recalc.c_str( ) );
		float new_distance = 5.0f * dpi_scale;
		target_pts_x = pos_x - new_distance - pts_text_size_recalc.x;
		prev_show_pts_picture = show_pts_picture;
	}
	float lerp_speed = 4.0f * ImGui::GetIO( ).DeltaTime;
	if ( GET_VARIABLE( g_variables.m_pts_appear_animation_speed_enable, bool ) ) {
		float appear_speed = GET_VARIABLE( g_variables.m_pts_appear_animation_speed, float );
		if ( appear_speed < 0.5f ) appear_speed = 0.5f;
		if ( appear_speed > 20.0f ) appear_speed = 20.0f;
		lerp_speed = appear_speed * ImGui::GetIO( ).DeltaTime;
	}
	if ( !pts_display_enabled ) {
		target_pts_x     = g_ctx.m_width - 90.0f * dpi_scale;
		target_pts_alpha = 0.0f;
	} else if ( m_points_system.m_current_points <= 0 ) {
		target_pts_x     = g_ctx.m_width - 90.0f * dpi_scale;
		target_pts_alpha = 0.0f;
	} else {
		if ( time_since_last_detect > 3.0f ) {
			target_pts_x     = g_ctx.m_width - 90.0f * dpi_scale;
			target_pts_alpha = 0.0f;
		} else {
			std::string pts_text = std::to_string( m_points_system.m_current_points ) + " pts";
			ImVec2 pts_text_size = ImGui::CalcTextSize( pts_text.c_str( ) );
			float actual_distance = 5.0f * dpi_scale;
			target_pts_x         = pos_x - actual_distance - pts_text_size.x;
			target_pts_alpha     = 1.0f;
		}
	}
	float lerp_factor = lerp_speed;
	float position_diff = target_pts_x - current_pts_x;
	float alpha_diff = target_pts_alpha - current_pts_alpha;
	current_pts_x = current_pts_x + position_diff * lerp_factor * 0.8f;
	current_pts_alpha = current_pts_alpha + alpha_diff * lerp_factor;
	bool watermark_outline_enabled = GET_VARIABLE( g_variables.m_watermark_outline, bool );
	bool outline_always_on = GET_VARIABLE( g_variables.m_watermark_outline_always, bool );
	bool outline_sync_with_pts = GET_VARIABLE( g_variables.m_watermark_outline_sync_with_pts, bool );
	float target_outline_alpha = 0.0f;
	if ( !watermark_outline_enabled ) {
		target_outline_alpha = 0.0f;
	} else if ( outline_always_on ) {
		target_outline_alpha = 1.0f;
	} else if ( outline_sync_with_pts ) {
		bool pts_value_active = m_points_system.m_current_points > 0;
		bool external_pts_active = m_outline_sync.m_pts_active || ( pts_value_active && time_since_last_detect < 3.5f );
		int show_flags = GET_VARIABLE( g_variables.m_watermark_show_flags, int );
		bool show_pts = (show_flags & 8) != 0;
		bool internal_pts_active = false;
		if ( show_pts && pts_value_active ) {
			internal_pts_active = true;
		}
		if ( external_pts_active || internal_pts_active ) {
			if ( pts_value_active ) {
				target_outline_alpha = 1.0f;
			} else if ( external_pts_active ) {
				target_outline_alpha = m_outline_sync.m_pts_alpha_with_animation;
			} else {
				target_outline_alpha = 1.0f;
			}
		} else {
			target_outline_alpha = 0.0f;
		}
	} else {
		target_outline_alpha = 0.0f;
	}
	m_outline_sync.m_outline_alpha = target_outline_alpha;
	if ( m_outline_sync.m_outline_alpha < 0.15f ) {
		m_outline_sync.m_outline_alpha = 0.0f;
	}
	if ( m_outline_sync.m_outline_alpha < 0.0f ) {
		m_outline_sync.m_outline_alpha = 0.0f;
	}
	if ( m_outline_sync.m_outline_alpha > 1.0f ) {
		m_outline_sync.m_outline_alpha = 1.0f;
	}
	float outline_alpha = m_outline_sync.m_outline_alpha;
	float watermark_rounding = corner_radius;
	if ( GET_VARIABLE( g_variables.m_watermark_rounding, bool ) ) {
		watermark_rounding = GET_VARIABLE( g_variables.m_watermark_rounding_value, float ) * dpi_scale;
	}
	draw_list->AddRectFilled( ImVec2( pos_x, pos_y ), ImVec2( pos_x + watermark_width, pos_y + watermark_height ), IM_COL32( 16, 16, 16, 255 ),
	                          watermark_rounding );
	if ( GET_VARIABLE( g_variables.m_watermark_outline, bool ) && outline_alpha > 0.001f ) {
		c_color outline_color = GET_VARIABLE( g_variables.m_watermark_rgb_mode, bool )
			? get_shared_rgb_color( ImGui::GetIO( ).DeltaTime )
			: GET_VARIABLE( g_variables.m_watermark_outline_color, c_color );
		float outline_thickness = 1.0f * dpi_scale;
		int final_alpha = static_cast< int >( static_cast< float >( outline_color[ 3 ] ) * outline_alpha );
		ImU32 synced_outline_color = IM_COL32( outline_color[ 0 ], outline_color[ 1 ], outline_color[ 2 ], final_alpha );
		draw_list->AddRect( ImVec2( pos_x, pos_y ), 
		                    ImVec2( pos_x + watermark_width, pos_y + watermark_height ), 
		                    synced_outline_color,
		                    watermark_rounding, 
		                    ImDrawFlags_RoundCornersAll, 
		                    outline_thickness );
	}
	if ( fonts_for_gui::watermark_font ) {
		ImFont* selected_font = fonts_for_gui::watermark_font;
		if ( GET_VARIABLE( g_variables.m_watermark_bold_font, bool ) && fonts_for_gui::bolder ) {
			selected_font = fonts_for_gui::bolder;
		}
		ImGui::PushFont( selected_font );
		std::string watermark_full_text = "";
		std::string main_text = "kamibebra";
		if ( GET_VARIABLE( g_variables.m_watermark_custom_text, bool ) ) {
			main_text = GET_VARIABLE( g_variables.m_watermark_custom_text_value, std::string );
		}
		bool typing_enabled = GET_VARIABLE( g_variables.m_watermark_typing, bool );
		std::string display_main_text = typing_enabled ? typing_text : main_text;
		watermark_full_text = display_main_text;
		int show_flags = GET_VARIABLE( g_variables.m_watermark_show_flags, int );
		bool show_user = (show_flags & 1) != 0;
		bool show_time = (show_flags & 2) != 0;
		bool show_fps = (show_flags & 4) != 0;
		bool show_pts = (show_flags & 8) != 0;
		std::string user_text = "";
		if ( show_user ) {
			user_text = "dev";
		}
		std::string time_text = "";
		if ( show_time ) {
			time_t now = time( nullptr );
			tm local_time;
			localtime_s( &local_time, &now );
			char time_buffer[6];
			snprintf( time_buffer, sizeof( time_buffer ), "%02d:%02d", local_time.tm_hour, local_time.tm_min );
			time_text = std::string( time_buffer );
		}
		std::string fps_text = "";
		if ( show_fps ) {
			static int cached_fps = 0;
			static float fps_update_timer = 0.0f;
			fps_update_timer += ImGui::GetIO( ).DeltaTime;
			if ( fps_update_timer >= 0.5f ) {
				cached_fps = static_cast< int >( 1.0f / ImGui::GetIO( ).DeltaTime );
				fps_update_timer = 0.0f;
			}
			fps_text = std::to_string( cached_fps );
		}
		std::string pts_text = "";
		static int displayed_pts = 0;
		static int target_pts = 0;
		static float pts_animation_timer = 0.0f;
		static bool pts_appearing = false;
		static bool pts_disappearing = false;
		static float pts_scale = 0.0f;
		if ( show_pts ) {
			int current_pts = m_points_system.m_current_points;
			if ( current_pts != target_pts ) {
				if ( current_pts > target_pts ) {
					pts_appearing = true;
					pts_disappearing = false;
					pts_scale = 0.0f;
					pts_animation_timer = 0.0f;
				} else if ( current_pts < target_pts ) {
					pts_disappearing = true;
					pts_appearing = false;
					pts_animation_timer = 0.0f;
				}
				target_pts = current_pts;
			}
			if ( pts_appearing ) {
				pts_animation_timer += ImGui::GetIO( ).DeltaTime;
				float appear_duration = 0.3f;
				float progress = ImClamp( pts_animation_timer / appear_duration, 0.0f, 1.0f );
				float c1 = 1.70158f;
				float c3 = c1 + 1.0f;
				pts_scale = 1.0f + c3 * powf( progress - 1.0f, 3.0f ) + c1 * powf( progress - 1.0f, 2.0f );
				displayed_pts = target_pts;
				if ( progress >= 1.0f ) {
					pts_appearing = false;
					pts_scale = 1.0f;
				}
			}
			if ( pts_disappearing ) {
				pts_animation_timer += ImGui::GetIO( ).DeltaTime;
				float countdown_speed = 0.002f;
				if ( pts_animation_timer >= countdown_speed ) {
					pts_animation_timer = 0.0f;
					if ( displayed_pts > target_pts ) {
						displayed_pts--;
					} else {
						pts_disappearing = false;
						pts_scale = 1.0f;
					}
				}
			}
			if ( !pts_appearing && !pts_disappearing ) {
				displayed_pts = current_pts;
				pts_scale = 1.0f;
			}
			pts_text = std::to_string( displayed_pts );
		}
		bool has_additional = !user_text.empty() || !time_text.empty() || !fps_text.empty() || !pts_text.empty();
		std::string full_display_text = display_main_text;
		if (!user_text.empty()) full_display_text += " | " + user_text;
		if (!time_text.empty()) full_display_text += " | " + time_text;
		if (!fps_text.empty()) full_display_text += " | " + fps_text + " fps";
		if (!pts_text.empty()) full_display_text += " | " + pts_text + " pts";
		const float watermark_font_size = selected_font->FontSize * dpi_scale;
		float text_x, text_y;
		ImVec2 full_text_size = selected_font->CalcTextSizeA( watermark_font_size, FLT_MAX, 0.0f, full_display_text.c_str( ) );
		text_x = pos_x + ( watermark_width - full_text_size.x ) * 0.5f;
		text_y = pos_y + ( watermark_height - full_text_size.y ) * 0.5f - 1.0f * dpi_scale;
		ImU32 white_color;
		if ( GET_VARIABLE( g_variables.m_watermark_text_color_like_stroke, bool ) ) {
			c_color outline_color_var = GET_VARIABLE( g_variables.m_watermark_outline_color, c_color );
			white_color = IM_COL32( outline_color_var[ 0 ], outline_color_var[ 1 ], outline_color_var[ 2 ], 255 );
		} else {
			white_color = IM_COL32( 255, 255, 255, 255 );
		}
		ImVec2 display_text_size = selected_font->CalcTextSizeA( watermark_font_size, FLT_MAX, 0.0f, display_main_text.c_str( ) );
		if ( !display_main_text.empty() ) {
			draw_list->AddText( selected_font, watermark_font_size, ImVec2( text_x, text_y ),
			                    white_color, display_main_text.c_str( ) );
		}
		float current_x = text_x + display_text_size.x;
		ImU32 info_color = IM_COL32( 146, 144, 145, 255 );
		if ( !user_text.empty( ) ) {
			std::string separator = " | ";
			ImVec2 separator_size = selected_font->CalcTextSizeA( watermark_font_size, FLT_MAX, 0.0f, separator.c_str( ) );
			draw_list->AddText( selected_font, watermark_font_size, ImVec2( current_x, text_y ),
			                    info_color, separator.c_str( ) );
			current_x += separator_size.x;
			ImVec2 user_size = selected_font->CalcTextSizeA( watermark_font_size, FLT_MAX, 0.0f, user_text.c_str( ) );
			draw_list->AddText( selected_font, watermark_font_size, ImVec2( current_x, text_y ),
			                    info_color, user_text.c_str( ) );
			current_x += user_size.x;
		}
		if ( !time_text.empty( ) ) {
			std::string separator = " | ";
			ImVec2 separator_size = selected_font->CalcTextSizeA( watermark_font_size, FLT_MAX, 0.0f, separator.c_str( ) );
			draw_list->AddText( selected_font, watermark_font_size, ImVec2( current_x, text_y ),
			                    info_color, separator.c_str( ) );
			current_x += separator_size.x;
			ImVec2 time_size = selected_font->CalcTextSizeA( watermark_font_size, FLT_MAX, 0.0f, time_text.c_str( ) );
			draw_list->AddText( selected_font, watermark_font_size, ImVec2( current_x, text_y ),
			                    info_color, time_text.c_str( ) );
			current_x += time_size.x;
		}
		if ( !fps_text.empty( ) ) {
			std::string separator = " | ";
			ImVec2 separator_size = selected_font->CalcTextSizeA( watermark_font_size, FLT_MAX, 0.0f, separator.c_str( ) );
			draw_list->AddText( selected_font, watermark_font_size, ImVec2( current_x, text_y ),
			                    info_color, separator.c_str( ) );
			current_x += separator_size.x;
			ImVec2 fps_num_size = selected_font->CalcTextSizeA( watermark_font_size, FLT_MAX, 0.0f, fps_text.c_str( ) );
			draw_list->AddText( selected_font, watermark_font_size, ImVec2( current_x, text_y ),
			                    white_color, fps_text.c_str( ) );
			current_x += fps_num_size.x;
			std::string fps_label = " fps";
			ImVec2 fps_label_size = selected_font->CalcTextSizeA( watermark_font_size, FLT_MAX, 0.0f, fps_label.c_str( ) );
			draw_list->AddText( selected_font, watermark_font_size, ImVec2( current_x, text_y ),
			                    info_color, fps_label.c_str( ) );
			current_x += fps_label_size.x;
		}
		if ( !pts_text.empty( ) ) {
			std::string separator = " | ";
			ImVec2 separator_size = selected_font->CalcTextSizeA( watermark_font_size, FLT_MAX, 0.0f, separator.c_str( ) );
			draw_list->AddText( selected_font, watermark_font_size, ImVec2( current_x, text_y ),
			                    info_color, separator.c_str( ) );
			current_x += separator_size.x;
			float pts_display_font_size = watermark_font_size * pts_scale;
			ImVec2 pts_num_size = selected_font->CalcTextSizeA( pts_display_font_size, FLT_MAX, 0.0f, pts_text.c_str( ) );
			float pts_y_centered = text_y + ( watermark_font_size - pts_display_font_size ) * 0.5f;
			draw_list->AddText( selected_font, pts_display_font_size, ImVec2( current_x, pts_y_centered ),
			                    white_color, pts_text.c_str( ) );
			current_x += pts_num_size.x;
			float pts_label_font_size = watermark_font_size * 0.85f * pts_scale;
			std::string pts_label = " pts";
			ImVec2 pts_label_size = selected_font->CalcTextSizeA( pts_label_font_size, FLT_MAX, 0.0f, pts_label.c_str( ) );
			float pts_label_y_centered = pts_y_centered + ( pts_num_size.y - pts_label_size.y ) * 0.5f;
			draw_list->AddText( selected_font, pts_label_font_size, ImVec2( current_x, pts_label_y_centered ),
			                    white_color, pts_label.c_str( ) );
			current_x += pts_label_size.x;
		}
		ImGui::PopFont( );
	}
	if ( current_pts_alpha <= 0.001f && m_points_system.m_current_points > 0 && time_since_last_detect > 3.5f ) {
		m_points_system.m_current_points   = 0;
		m_points_system.m_previous_points  = 0;
		m_points_system.m_last_detect_time = 0;
		reset_detect_states( );
	}
	static bool cleanup_done = false;
	if ( !cleanup_done && !g_interfaces.m_engine_client->is_in_game( ) ) {
		cleanup_done = true;
	} else if ( cleanup_done && g_interfaces.m_engine_client->is_in_game( ) ) {
		cleanup_done = false;
	}
}
void n_misc::impl_t::on_end_scene( )
{
	static std::string last_map_end = "";
	static bool was_connected_end   = false;
	std::string current_map_end = "";
	bool is_connected_end = false;
	if ( g_interfaces.m_engine_client ) {
		current_map_end = g_interfaces.m_engine_client->get_level_name( );
		is_connected_end = g_interfaces.m_engine_client->is_connected( );
		if ( !g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game( ) || last_map_end != current_map_end ||
		     ( !was_connected_end && is_connected_end ) ) {
			m_points_system.m_current_points = 0;
			m_points_system.m_previous_points = 0;
			reset_detect_states( );
			last_map_end = current_map_end;
		}
		was_connected_end = is_connected_end;
	}
	if ( GET_VARIABLE( g_variables.trackdispay, bool ) )
		RenderMediaPlayer( );
	if ( GET_VARIABLE( g_variables.m_watermark, bool ) ) {
		this->render_watermark( );
	}
	this->update_clantag( );
	if ( !g_interfaces.m_engine_client || !g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game( ) || g_ctx.m_local->get_observer_mode( ) == 1  ) {
		if ( g_menu.m_opened && GET_VARIABLE( g_variables.m_spectators_list, bool ) ) {
			this->draw_spectator_list( );
		}
		return;
	}
	if ( g_ctx.m_local->get_team( ) <= 1 && !g_menu.m_opened )
		return;
	{
		static bool noclip_key_was_down = false;
		bool noclip_key_down = g_input.check_input( &GET_VARIABLE( g_variables.m_practice_noclip_key, key_bind_t ) );
		if ( noclip_key_down && !noclip_key_was_down && g_ctx.m_local && g_ctx.m_local->is_alive( ) ) {
			auto sv_cheats = g_convars[ HASH_BT( "sv_cheats" ) ];
			if ( sv_cheats ) {
				sv_cheats->set_value( 1 );
			}
			g_interfaces.m_engine_client->client_cmd_unrestricted( "noclip" );
		}
		noclip_key_was_down = noclip_key_down;
	}
	[ & ]( const bool can_draw_practice_window ) {
		if ( !can_draw_practice_window )
			return;
		if ( !g_ctx.m_local->is_alive( ) )
			return;
		constexpr float window_width     = 170.0f;
		const float window_rounding  = GET_VARIABLE( g_variables.m_practice_window_rounding_enable, bool ) 
			? GET_VARIABLE( g_variables.m_practice_window_rounding, float ) : 7.0f;
		constexpr float window_padding   = 0.0f;
		constexpr float title_top_margin = 2.0f;
		constexpr float first_bind_top   = 22.0f;
		constexpr float bind_left_margin = 10.0f;
		constexpr float key_box_size     = 20.0f;
		constexpr float key_box_rounding = 5.0f;
		constexpr float bind_row_height  = 24.0f;
		constexpr float bind_row_spacing = 0.5f;
		constexpr float bottom_margin    = 2.0f;
		constexpr ImU32 bg_color         = IM_COL32( 13, 13, 13, 255 );
		constexpr ImU32 key_box_border   = IM_COL32( 22, 22, 22, 255 );
		constexpr ImU32 key_text_color   = IM_COL32( 171, 171, 171, 255 );
		constexpr ImU32 label_color      = IM_COL32( 171, 171, 171, 255 );
		constexpr ImU32 title_color      = IM_COL32( 255, 255, 255, 255 );
		float vertical_offset = -25.0f;
		float window_height = first_bind_top + 3 * bind_row_height + 2 * bind_row_spacing + bottom_margin;
		ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, window_rounding );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( window_padding, window_padding ) );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
		ImGui::PushStyleColor( ImGuiCol_WindowBg, bg_color );
		float saved_pos_x = GET_VARIABLE( g_variables.m_practice_window_pos_x, float );
		float saved_pos_y = GET_VARIABLE( g_variables.m_practice_window_pos_y, float );
		if ( saved_pos_x < 0 )
			saved_pos_x = 0;
		if ( saved_pos_y < 0 )
			saved_pos_y = 0;
		if ( saved_pos_x + window_width > g_ctx.m_width )
			saved_pos_x = g_ctx.m_width - window_width;
		if ( saved_pos_y + window_height > g_ctx.m_height )
			saved_pos_y = g_ctx.m_height - window_height;
		ImGui::SetNextWindowPos( ImVec2( saved_pos_x, saved_pos_y ), ImGuiCond_FirstUseEver );
		ImGui::SetNextWindowSize( ImVec2( window_width, window_height ), ImGuiCond_Always );
		int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
		bool window_shadow_enabled = GET_VARIABLE( g_variables.m_practice_window_shadow, bool );
		if ( window_shadow_enabled ) {
			ImDrawList* bg_draw_list = ImGui::GetBackgroundDrawList();
			if ( bg_draw_list ) {
				const float shadow_offset = 3.0f;
				ImU32 shadow_color = IM_COL32( 0, 0, 0, 128 );
				bg_draw_list->AddRectFilled(
					ImVec2( saved_pos_x + shadow_offset, saved_pos_y + shadow_offset ),
					ImVec2( saved_pos_x + window_width + shadow_offset, saved_pos_y + window_height + shadow_offset ),
					shadow_color,
					window_rounding,
					ImDrawFlags_RoundCornersAll
				);
			}
		}
		ImGui::Begin( "Checkpoints", nullptr, flags );
		{
			bool bold_font_enabled = GET_VARIABLE( g_variables.m_practice_window_bold_font, bool );
			bool inner_stroke_enabled = GET_VARIABLE( g_variables.m_practice_window_inner_stroke, bool );
			ImDrawList* draw_list = ImGui::GetWindowDrawList( );
			ImVec2 win_pos        = ImGui::GetWindowPos( );
			if ( inner_stroke_enabled ) {
				ImVec2 window_size = ImGui::GetWindowSize();
				const float inner_offset = 2.0f;
				ImU32 inner_stroke_color = IM_COL32( 45, 43, 44, 255 );
				if ( draw_list ) {
					draw_list->AddRect( 
						ImVec2( win_pos.x + inner_offset, win_pos.y + inner_offset ), 
						ImVec2( win_pos.x + window_size.x - inner_offset, win_pos.y + window_size.y - inner_offset ), 
						inner_stroke_color,
						window_rounding - inner_offset, 
						ImDrawFlags_RoundCornersAll, 
						1.5f 
					);
				}
			}
			ImFont* title_font = fonts_for_gui::tasorbiter_addwindow;
			ImFont* item_font = fonts_for_gui::tasorbiter_addwindow;
			bool practice_shadow_enabled = GET_VARIABLE( g_variables.m_practice_window_shadow, bool );
			ImGui::PushFont( title_font );
			std::string title_str;
			if ( GET_VARIABLE( g_variables.m_practice_window_custom_title, bool ) ) {
				title_str = GET_VARIABLE( g_variables.m_practice_window_custom_title_text, std::string );
				if ( title_str.empty( ) )
					title_str = bold_font_enabled ? "checkpoints" : "Checkpoints";
			} else {
				title_str = bold_font_enabled ? "checkpoints" : "Checkpoints";
			}
			ImVec2 title_size = ImGui::CalcTextSize( title_str.c_str( ) );
			ImGui::SetCursorPosY( title_top_margin );
			ImGui::SetCursorPosX( ( window_width - title_size.x ) * 0.5f );
			ImVec2 title_pos = ImGui::GetCursorScreenPos();
			if ( practice_shadow_enabled && draw_list && title_font ) {
				float shadow_offset_x = 1.0f;
				float shadow_offset_y = 1.0f;
				ImU32 shadow_col = IM_COL32( 0, 0, 0, 128 );
				draw_list->AddText( title_font, title_font->FontSize, 
					ImVec2( title_pos.x + shadow_offset_x, title_pos.y + shadow_offset_y ), shadow_col, title_str.c_str( ) );
			}
			ImGui::TextColored( ImGui::ColorConvertU32ToFloat4( title_color ), title_str.c_str( ) );
			ImGui::PopFont( );
			ImGui::SetCursorPos( ImVec2( 0, 0 ) );
			ImGui::InvisibleButton( "##checkpoints_drag", ImVec2( window_width, window_height ) );
			if ( ImGui::IsItemActive( ) && ImGui::IsMouseDragging( 0 ) ) {
				ImVec2 delta   = ImGui::GetMouseDragDelta( 0 );
				ImVec2 new_pos = ImVec2( saved_pos_x + delta.x, saved_pos_y + delta.y );
				if ( new_pos.x >= 0 && new_pos.y >= 0 && new_pos.x + window_width <= g_ctx.m_width && new_pos.y + window_height <= g_ctx.m_height ) {
					ImGui::SetWindowPos( "Checkpoints", new_pos );
					ImGui::ResetMouseDragDelta( 0 );
				}
			}
			if ( ImGui::IsItemHovered( ) ) {
				ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
			}
			struct bind_row_t {
				const char* label;
				const char* key;
			};
			bind_row_t rows[ 3 ] = { { "Save", FILTERED_KEY_NAMES[ GET_VARIABLE( g_variables.m_practice_cp_key, key_bind_t ).m_key ] },
				                     { "Teleport", FILTERED_KEY_NAMES[ GET_VARIABLE( g_variables.m_practice_tp_key, key_bind_t ).m_key ] },
				                     { "Noclip", FILTERED_KEY_NAMES[ GET_VARIABLE( g_variables.m_practice_noclip_key, key_bind_t ).m_key ] } };
			float y = first_bind_top;
			for ( int i = 0; i < 3; ++i ) {
				ImGui::SetCursorPosY( y );
				ImGui::SetCursorPosX( bind_left_margin );
				ImVec2 label_text_pos = ImGui::GetCursorScreenPos();
				ImGui::PushFont( item_font );
				if ( practice_shadow_enabled && draw_list && item_font ) {
					float shadow_offset_x = 1.0f;
					float shadow_offset_y = 1.0f;
					ImU32 shadow_col = IM_COL32( 0, 0, 0, 128 );
					draw_list->AddText( item_font, item_font->FontSize, 
						ImVec2( label_text_pos.x + shadow_offset_x, label_text_pos.y + shadow_offset_y ), shadow_col, rows[ i ].label );
				}
				ImGui::TextColored( ImGui::ColorConvertU32ToFloat4( label_color ), rows[ i ].label );
				ImGui::PopFont( );
				ImVec2 label_pos   = ImGui::GetCursorScreenPos( );
				ImVec2 key_box_pos = ImVec2( win_pos.x + window_width - bind_left_margin - key_box_size, label_pos.y + vertical_offset );
				draw_list->AddRect( key_box_pos, ImVec2( key_box_pos.x + key_box_size, key_box_pos.y + key_box_size ), key_box_border,
				                    3.0f,
				                    ImDrawFlags_RoundCornersAll,
				                    0.5f );
				std::string key_text = rows[ i ].key;
				std::transform( key_text.begin( ), key_text.end( ), key_text.begin( ), ::toupper );
				ImVec2 key_text_size = item_font->CalcTextSizeA( item_font->FontSize, FLT_MAX, 0.0f, key_text.c_str( ) );
				ImVec2 key_text_pos  = ImVec2( key_box_pos.x + ( key_box_size - key_text_size.x ) * 0.5f,
				                               key_box_pos.y + ( key_box_size - key_text_size.y ) * 0.2f + 2.5f
				 );
				if ( practice_shadow_enabled && draw_list && item_font ) {
					float shadow_offset_x = 1.0f;
					float shadow_offset_y = 1.0f;
					ImU32 shadow_col = IM_COL32( 0, 0, 0, 128 );
					draw_list->AddText( item_font, item_font->FontSize, 
						ImVec2( key_text_pos.x + shadow_offset_x, key_text_pos.y + shadow_offset_y ), shadow_col, key_text.c_str() );
				}
				draw_list->AddText( item_font, item_font->FontSize, key_text_pos, key_text_color, key_text.c_str( ) );
				y += bind_row_height + bind_row_spacing;
				if ( i == 2 ) {
					y += 2.0f;
				}
			}
		}
		ImVec2 current_pos = ImGui::GetWindowPos( );
		if ( current_pos.x != saved_pos_x || current_pos.y != saved_pos_y ) {
			GET_VARIABLE( g_variables.m_practice_window_pos_x, float ) = current_pos.x;
			GET_VARIABLE( g_variables.m_practice_window_pos_y, float ) = current_pos.y;
			static std::string default_config = "default";
			g_config.save( default_config );
		}
		ImGui::End( );
		ImGui::PopStyleColor( 1 );
		ImGui::PopStyleVar( 3 );
	}( GET_VARIABLE( g_variables.m_practice_window, bool ) );
	if ( GET_VARIABLE( g_variables.m_spectators_list, bool ) ) {
		this->draw_spectator_list( );
	}
}
void n_misc::impl_t::on_fire_event( ) { }
void n_misc::impl_t::practice_window_think( )
{
	if ( !GET_VARIABLE( g_variables.m_practice_window, bool ) )
		return;
	if ( !g_interfaces.m_engine_client || !g_ctx.m_local )
		return;
	if ( !g_convars[ HASH_BT( "sv_cheats" ) ]->get_bool( ) || g_interfaces.m_engine_client->is_console_visible( ) )
		return;
	const auto cp_key = GET_VARIABLE( g_variables.m_practice_cp_key, key_bind_t ).m_key;
	const auto tp_key = GET_VARIABLE( g_variables.m_practice_tp_key, key_bind_t ).m_key;
	if ( g_input.is_key_released( cp_key ) ) {
		if ( !( g_ctx.m_local->get_flags( ) & fl_onground ) ) {
			return;
		}
		c_angle saved_angles = { };
		g_interfaces.m_engine_client->get_view_angles( saved_angles );
		g_misc.practice.saved_angles   = saved_angles;
		g_misc.practice.saved_position = g_ctx.m_local->get_abs_origin( );
	} else if ( g_input.is_key_released( tp_key ) ) {
		if ( g_misc.practice.saved_angles.is_zero( ) || g_misc.practice.saved_position.is_zero( ) )
			return;
		g_interfaces.m_engine_client->client_cmd_unrestricted(
			std::string( "setpos " )
				.append( std::vformat( "{} {} {}", std::make_format_args( g_misc.practice.saved_position.m_x, g_misc.practice.saved_position.m_y,
		                                                                  g_misc.practice.saved_position.m_z ) ) )
				.append( ";setang " )
				.append( std::vformat( "{} {} {}", std::make_format_args( g_misc.practice.saved_angles.m_x, g_misc.practice.saved_angles.m_y,
		                                                                  g_misc.practice.saved_angles.m_z ) ) )
				.c_str( ) );
	}
}
void n_misc::impl_t::disable_post_processing( )
{
	g_convars[ HASH_BT( "mat_postprocess_enable" ) ]->set_value( !( GET_VARIABLE( g_variables.m_disable_post_processing, bool ) ) );
}
void n_misc::impl_t::remove_panorama_blur( )
{
	g_convars[ HASH_BT( "@panorama_disable_blur" ) ]->set_value( GET_VARIABLE( g_variables.m_remove_panorama_blur, bool ) );
}
void n_misc::impl_t::draw_spectating_local( )
{
	if ( !g_interfaces.m_engine_client || !g_interfaces.m_client_entity_list )
		return;
	int m_y = 5;
	std::vector< spectator_data_t > spectator_data{ };
	if ( !g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game( ) || !GET_VARIABLE( g_variables.m_spectators_list, bool ) ||
	     GET_VARIABLE( g_variables.m_spectators_list_type, int ) != 1  ) {
		if ( !spectator_data.empty( ) ) {
			g_ctx.m_last_spectators_y = 5;
			spectator_data.clear( );
		}
		return;
	}
	constexpr auto get_player_spec_type = [ & ]( int obs_mode ) -> std::string {
		switch ( obs_mode ) {
		case e_obs_mode::obs_mode_deathcam:
			return "deathcam";
		case e_obs_mode::obs_mode_freezecam:
			return "freezecam";
		case e_obs_mode::obs_mode_in_eye:
			return "first person";
		case e_obs_mode::obs_mode_chase:
			return "3rd person";
		case e_obs_mode::obs_mode_roaming:
			return "roaming";
		case e_obs_mode::obs_mode_fixed:
			return "fixed";
		default:
			return "first person";
		}
	};
	g_entity_cache.enumerate( e_enumeration_type::type_players, [ & ]( c_base_entity* entity ) {
		if ( !entity || entity->is_alive( ) || entity->is_dormant( ) )
			return;
		const auto entity_index = entity->get_index( );
		const auto entity_team  = entity->get_team( );
		const auto spectated_player = reinterpret_cast< c_base_entity* >(
			g_interfaces.m_client_entity_list->get_client_entity_from_handle( entity->get_observer_target_handle( ) ) );
		if ( !spectated_player || spectated_player != g_ctx.m_local )
			return;
		player_info_t spectating_info{ };
		g_interfaces.m_engine_client->get_player_info( entity_index, &spectating_info );
		std::string spectator_name = g_menu.get_server_player_name( entity_index, spectating_info.m_name );
		if ( spectator_name.length( ) > 12 ) {
			spectator_name = spectator_name.substr( 0, 12 ) + "...";
		}
		spectator_data.push_back( { std::format( "{} | {}", spectator_name,
		                                         get_player_spec_type( entity->get_observer_mode( ) ) ),
		                            spectating_info.m_fake_player ? entity_team == e_team_id::team_tt   ? g_render.m_terrorist_avatar
		                                                            : entity_team == e_team_id::team_ct ? g_render.m_counter_terrorist_avatar
		                                                                                                : nullptr
		                                                          : g_avatar_cache[ entity_index ],
		                            GET_VARIABLE( g_variables.m_spectators_list_text_color_one, c_color ) } );
	} );
	if ( spectator_data.empty( ) ) {
		g_ctx.m_last_spectators_y = 5;
		return;
	}
	const auto draw_avatar            = GET_VARIABLE( g_variables.m_spectators_avatar, bool );
	constexpr static auto avatar_size = 14.f;
	for ( const auto& data : spectator_data ) {
		if ( draw_avatar )
			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_texture,
				std::make_any< texture_draw_object_t >( c_vector_2d( 10, m_y ), c_vector_2d( avatar_size, avatar_size ),
			                                            IM_COL32( 255, 255, 255, 255 ), data.m_avatar, 0.f, ImDrawFlags_::ImDrawFlags_None ) );
		g_render.m_draw_data.emplace_back(
			e_draw_type::draw_type_text,
			std::make_any< text_draw_object_t >( g_render.m_fonts[ e_font_names::font_name_tahoma_bd_12 ], c_vector_2d( draw_avatar ? 27 : 10, m_y ),
		                                         data.m_text.c_str( ), data.m_color.get_u32( ),
		                                         IM_COL32( 0, 0, 0, data.m_color.base< e_color_type::color_type_a >( ) ),
		                                         e_text_flags::text_flag_dropshadow ) );
		m_y += 15;
		g_ctx.m_last_spectators_y = m_y;
	}
}
void n_misc::impl_t::draw_spectator_list( )
{
	if ( !g_interfaces.m_engine_client )
		return;
	if ( !g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game( ) || !GET_VARIABLE( g_variables.m_spectators_list, bool ) )
		return;
	if ( g_ctx.m_width <= 0 || g_ctx.m_height <= 0 )
		return;
	const int design = GET_VARIABLE( g_variables.m_spectators_design, int );
	const bool show_all = GET_VARIABLE( g_variables.m_spectators_show_all, bool );
	const bool local_shadow = GET_VARIABLE( g_variables.m_spectators_local_shadow, bool );
	const c_color shadow_color = GET_VARIABLE( g_variables.m_spectators_shadow_color, c_color );
		const bool always_show = GET_VARIABLE( g_variables.m_spectators_always_show, bool );
		const bool window_shadow =
			GET_VARIABLE( g_variables.m_spectators_list_shadow, bool ) ||
			GET_VARIABLE( g_variables.m_spectators_window_shadow, bool );
	const c_color other_color = GET_VARIABLE( g_variables.m_spectators_other_color, c_color );
	const c_color local_color = GET_VARIABLE( g_variables.m_spectators_local_color, c_color );
	const bool other_shadow = GET_VARIABLE( g_variables.m_spectators_other_shadow, bool );
	const c_color other_shadow_color = GET_VARIABLE( g_variables.m_spectators_other_shadow_color, c_color );
	if ( design == 0 ) {
		if ( !fonts_for_gui::tasorbiter_addwindow )
			return;
		const float window_rounding = 5.0f;
		constexpr float window_padding = 0.0f;
		constexpr float title_top_margin = 4.0f;
		constexpr float title_to_list_spacing = 8.0f;
		constexpr float item_left_margin = 10.0f;
		constexpr float item_height = 17.0f;
		constexpr float item_spacing = 1.0f;
		constexpr float bottom_padding = 5.0f;
		constexpr ImU32 bg_color = IM_COL32( 13, 13, 13, 255 );
		constexpr ImU32 text_color = IM_COL32( 255, 255, 255, 255 );
		struct spectator_entry_t {
			std::string observer_name;
			std::string target_name;
			bool is_local_target;
		};
		struct spectator_animation_t {
			std::string key;
			spectator_entry_t entry;
			float progress;
			bool seen_this_frame;
			bool is_removing;
		};
		static std::vector< spectator_animation_t > spectator_animations;
		const bool animate_new_entries = GET_VARIABLE( g_variables.m_spectators_animate_new_entries, bool );
		const float delta_time = ImGui::GetIO( ).DeltaTime;
		constexpr float appearance_speed = 4.0f;
		std::vector< spectator_entry_t > spectators;
		g_entity_cache.enumerate( e_enumeration_type::type_players, [ & ]( c_base_entity* entity ) {
			if ( !entity || entity->is_alive( ) || entity->is_dormant( ) )
				return;
			const auto entity_index = entity->get_index( );
			if ( entity_index <= 0 || entity_index > 64 )
				return;
			const auto spectated_player = reinterpret_cast< c_base_entity* >(
				g_interfaces.m_client_entity_list->get_client_entity_from_handle( entity->get_observer_target_handle( ) ) );
			if ( !spectated_player || !spectated_player->is_alive( ) )
				return;
			if ( !show_all && spectated_player != g_ctx.m_local )
				return;
			player_info_t spectating_info{ };
			if ( !g_interfaces.m_engine_client->get_player_info( entity_index, &spectating_info ) )
				return;
			player_info_t target_info{ };
			if ( !g_interfaces.m_engine_client->get_player_info( spectated_player->get_index( ), &target_info ) )
				return;
			spectator_entry_t entry;
			entry.observer_name = g_menu.get_server_player_name( entity_index, spectating_info.m_name );
			entry.target_name = g_menu.get_server_player_name( spectated_player->get_index( ), target_info.m_name );
			entry.is_local_target = ( spectated_player == g_ctx.m_local );
			spectators.push_back( entry );
		} );
		bool dynamic_list = GET_VARIABLE( g_variables.m_spectators_dynamic_list, bool );
		if ( !dynamic_list ) {
			for ( auto& spec : spectators ) {
				if ( spec.target_name.length( ) > 9 )
					spec.target_name = spec.target_name.substr( 0, 9 ) + "...";
			}
		}
		auto make_spectator_key = []( const spectator_entry_t& entry ) {
			return entry.observer_name + "\n" + entry.target_name;
		};
		for ( auto& animation : spectator_animations ) {
			animation.seen_this_frame = false;
		}
		for ( const auto& spec : spectators ) {
			const std::string key = make_spectator_key( spec );
			auto animation_it = std::find_if( spectator_animations.begin( ), spectator_animations.end( ), [ & ]( const spectator_animation_t& animation ) {
				return animation.key == key;
			} );
			if ( animation_it == spectator_animations.end( ) ) {
				spectator_animations.push_back( { key, spec, animate_new_entries ? 0.0f : 1.0f, true, false } );
				continue;
			}
			animation_it->entry = spec;
			animation_it->seen_this_frame = true;
			animation_it->is_removing = false;
			if ( !animate_new_entries ) {
				animation_it->progress = 1.0f;
			}
		}
		for ( auto& animation : spectator_animations ) {
			if ( !animation.seen_this_frame ) {
				animation.is_removing = true;
			}
		}
		if ( animate_new_entries ) {
			for ( auto& animation : spectator_animations ) {
				if ( animation.is_removing ) {
					animation.progress = std::clamp( animation.progress - delta_time * appearance_speed, 0.0f, 1.0f );
				} else {
					animation.progress = std::clamp( animation.progress + delta_time * appearance_speed, 0.0f, 1.0f );
				}
			}
		}
		spectator_animations.erase( std::remove_if( spectator_animations.begin( ), spectator_animations.end( ),
			[ & ]( const spectator_animation_t& animation ) {
				if ( animate_new_entries ) {
					return animation.is_removing && animation.progress <= 0.0f;
				}
				return !animation.seen_this_frame;
			} ), spectator_animations.end( ) );
		const bool has_spectators = !spectators.empty( );
		const bool has_rendered_entries = animate_new_entries ? !spectator_animations.empty( ) : has_spectators;
		if ( !has_rendered_entries && !always_show )
			return;
		float max_text_width = 0.0f;
		const float min_window_width = 200.0f;
		ImGui::PushFont( fonts_for_gui::tasorbiter_addwindow );
		ImVec2 title_size = ImGui::CalcTextSize( "Spectators" );
		max_text_width = title_size.x;
		if ( animate_new_entries ) {
			for ( const auto& animation : spectator_animations ) {
				std::string full_text = animation.entry.observer_name + "   >   " + animation.entry.target_name;
				ImVec2 text_size = ImGui::CalcTextSize( full_text.c_str( ) );
				if ( text_size.x > max_text_width ) {
					max_text_width = text_size.x;
				}
			}
		} else {
			for ( const auto& spec : spectators ) {
				std::string full_text = spec.observer_name + "   >   " + spec.target_name;
				ImVec2 text_size = ImGui::CalcTextSize( full_text.c_str( ) );
				if ( text_size.x > max_text_width ) {
					max_text_width = text_size.x;
				}
			}
		}
		ImGui::PopFont( );
		float dynamic_window_width = max_text_width + item_left_margin * 2.0f + 10.0f;
		if ( dynamic_window_width < min_window_width ) {
			dynamic_window_width = min_window_width;
		}
		const float window_width = dynamic_window_width;
		const float font_size = 12.0f;
		size_t items_count = has_rendered_entries ? ( animate_new_entries ? spectator_animations.size( ) : spectators.size( ) ) : 1;
		float list_height = items_count * item_height + ( items_count > 1 ? ( items_count - 1 ) * item_spacing : 0 );
		if ( has_rendered_entries && animate_new_entries ) {
			list_height = 0.0f;
			for ( size_t i = 0; i < spectator_animations.size( ); i++ ) {
				const float progress = spectator_animations[ i ].progress;
				list_height += item_height * progress;
				if ( i + 1 < spectator_animations.size( ) ) {
					list_height += item_spacing;
				}
			}
		}
		float window_height = title_top_margin + font_size + title_to_list_spacing + list_height + bottom_padding;
		ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, window_rounding );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( window_padding, window_padding ) );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
		ImGui::PushStyleColor( ImGuiCol_WindowBg, bg_color );
		float saved_pos_x = GET_VARIABLE( g_variables.m_spectators_list_pos_x, float );
		float saved_pos_y = GET_VARIABLE( g_variables.m_spectators_list_pos_y, float );
		if ( saved_pos_x < 0 ) saved_pos_x = 0;
		if ( saved_pos_y < 0 ) saved_pos_y = 0;
		if ( saved_pos_x + window_width > g_ctx.m_width ) saved_pos_x = static_cast< float >( g_ctx.m_width ) - window_width;
		if ( saved_pos_y + window_height > g_ctx.m_height ) saved_pos_y = static_cast< float >( g_ctx.m_height ) - window_height;
		ImGui::SetNextWindowPos( ImVec2( saved_pos_x, saved_pos_y ), ImGuiCond_FirstUseEver );
		ImGui::SetNextWindowSize( ImVec2( window_width, window_height ), ImGuiCond_Always );
		int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
			if ( window_shadow ) {
			ImDrawList* bg_draw_list = ImGui::GetBackgroundDrawList( );
			if ( bg_draw_list ) {
				const float shadow_offset = 3.0f;
				ImU32 shadow_col = IM_COL32( 0, 0, 0, 128 );
				bg_draw_list->AddRectFilled(
					ImVec2( saved_pos_x + shadow_offset, saved_pos_y + shadow_offset ),
					ImVec2( saved_pos_x + window_width + shadow_offset, saved_pos_y + window_height + shadow_offset ),
					shadow_col,
					window_rounding,
					ImDrawFlags_RoundCornersAll
				);
			}
		}
		ImGui::Begin( "Spectators", nullptr, flags );
		{
			bool bold_font_enabled = GET_VARIABLE( g_variables.m_spectators_bold_font, bool );
			ImVec2 window_pos = ImGui::GetWindowPos( );
			ImVec2 window_size = ImGui::GetWindowSize( );
			ImDrawList* draw_list = ImGui::GetWindowDrawList( );
			ImFont* title_font = fonts_for_gui::tasorbiter_addwindow;
			if ( bold_font_enabled && fonts_for_gui::bolder )
				title_font = fonts_for_gui::bolder;
			if ( title_font )
				ImGui::PushFont( title_font );
			std::string title_str;
			if ( GET_VARIABLE( g_variables.m_spectators_custom_title, bool ) ) {
				title_str = GET_VARIABLE( g_variables.m_spectators_custom_title_text, std::string );
				if ( title_str.empty( ) )
					title_str = bold_font_enabled ? "spectators" : "Spectators";
			} else {
				title_str = bold_font_enabled ? "spectators" : "Spectators";
			}
			ImVec2 title_size = ImGui::CalcTextSize( title_str.c_str( ) );
			ImGui::SetCursorPosY( title_top_margin );
			ImGui::SetCursorPosX( ( window_width - title_size.x ) * 0.5f );
			ImGui::TextColored( ImGui::ColorConvertU32ToFloat4( text_color ), title_str.c_str( ) );
			if ( title_font )
				ImGui::PopFont( );
			ImGui::SetCursorPos( ImVec2( 0, 0 ) );
			ImGui::InvisibleButton( "##spectators_drag", ImVec2( window_width, window_height ) );
			if ( ImGui::IsItemActive( ) && ImGui::IsMouseDragging( 0 ) ) {
				ImVec2 delta = ImGui::GetMouseDragDelta( 0 );
				ImVec2 new_pos = ImVec2( saved_pos_x + delta.x, saved_pos_y + delta.y );
				if ( new_pos.x >= 0 && new_pos.y >= 0 && new_pos.x + window_width <= g_ctx.m_width && new_pos.y + window_height <= g_ctx.m_height ) {
					ImGui::SetWindowPos( "Spectators", new_pos );
					ImGui::ResetMouseDragDelta( 0 );
				}
			}
			if ( ImGui::IsItemHovered( ) )
				ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
			ImFont* item_font = fonts_for_gui::tasorbiter_addwindow;
			if ( bold_font_enabled && fonts_for_gui::bolder )
				item_font = fonts_for_gui::bolder;
			float y = title_top_margin + font_size + title_to_list_spacing;
			if ( !has_spectators && always_show ) {
				ImGui::SetCursorPosY( y );
				ImGui::SetCursorPosX( item_left_margin );
				if ( item_font )
					ImGui::PushFont( item_font );
				const char* no_spectators_text = bold_font_enabled ? "no spectators..." : "No spectators...";
				ImU32 no_spectators_color = IM_COL32( 100, 100, 100, 255 );
				ImGui::TextColored( ImGui::ColorConvertU32ToFloat4( no_spectators_color ), no_spectators_text );
				if ( item_font )
					ImGui::PopFont( );
			} else {
				float current_y = y;
				const size_t render_count = animate_new_entries ? spectator_animations.size( ) : spectators.size( );
				for ( size_t i = 0; i < render_count; i++ ) {
					const spectator_entry_t& render_entry = animate_new_entries ? spectator_animations[ i ].entry : spectators[ i ];
					const float progress = animate_new_entries ? spectator_animations[ i ].progress : 1.0f;
					const float row_height = item_height * progress;
					if ( progress <= 0.0f ) {
						current_y += row_height + item_spacing;
						continue;
					}
					std::string display_text;
					if ( show_all ) {
						display_text = render_entry.observer_name + "   >   " + render_entry.target_name;
					} else {
						display_text = render_entry.observer_name;
					}
					ImU32 text_color_final = render_entry.is_local_target ? 
						IM_COL32( local_color[ 0 ], local_color[ 1 ], local_color[ 2 ], local_color[ 3 ] ) :
						IM_COL32( other_color[ 0 ], other_color[ 1 ], other_color[ 2 ], other_color[ 3 ] );
					bool need_shadow = render_entry.is_local_target ? local_shadow : other_shadow;
					ImU32 shadow_col = render_entry.is_local_target ? 
						IM_COL32( shadow_color[ 0 ], shadow_color[ 1 ], shadow_color[ 2 ], shadow_color[ 3 ] ) :
						IM_COL32( other_shadow_color[ 0 ], other_shadow_color[ 1 ], other_shadow_color[ 2 ], other_shadow_color[ 3 ] );
					ImVec2 text_pos( window_pos.x + item_left_margin, window_pos.y + current_y );
					if ( draw_list && item_font ) {
						const float native_size = item_font->FontSize;
						draw_list->PushClipRect(
							ImVec2( window_pos.x + item_left_margin, window_pos.y + current_y ),
							ImVec2( window_pos.x + window_width - item_left_margin, window_pos.y + current_y + row_height ),
							true
						);
						if ( need_shadow ) {
							const float shadow_offset = 1.0f;
							draw_list->AddText( item_font, native_size,
								ImVec2( text_pos.x + shadow_offset, text_pos.y + shadow_offset ),
								shadow_col, display_text.c_str( ) );
						}
						draw_list->AddText( item_font, native_size, text_pos, text_color_final, display_text.c_str( ) );
						draw_list->PopClipRect( );
					} else {
						ImGui::SetCursorPosY( current_y );
						ImGui::SetCursorPosX( item_left_margin );
						if ( item_font )
							ImGui::PushFont( item_font );
						ImGui::TextColored( ImGui::ColorConvertU32ToFloat4( text_color_final ), display_text.c_str( ) );
						if ( item_font )
							ImGui::PopFont( );
					}
					current_y += row_height + item_spacing;
				}
			}
		}
		ImVec2 current_pos = ImGui::GetWindowPos( );
		ImVec2 current_size = ImGui::GetWindowSize( );
		if ( current_pos.x != saved_pos_x || current_pos.y != saved_pos_y ) {
			GET_VARIABLE( g_variables.m_spectators_list_pos_x, float ) = current_pos.x;
			GET_VARIABLE( g_variables.m_spectators_list_pos_y, float ) = current_pos.y;
		}
		ImGui::End( );
		ImGui::PopStyleColor( 1 );
		ImGui::PopStyleVar( 3 );
		if ( GET_VARIABLE( g_variables.m_spectators_gif_enabled, bool ) ) {
			ImDrawList* gif_draw = ImGui::GetBackgroundDrawList( );
			if ( gif_draw ) {
				float gif_size = GET_VARIABLE( g_variables.m_spectators_gif_width, float );
				float gif_w = gif_size;
				float gif_h = gif_size;
				float gif_off_y = 0.0f;
				float gif_x = current_pos.x + ( current_size.x - gif_w ) * 0.5f;
				float gif_y = current_pos.y - gif_h - gif_off_y;
				SpectatorGifRenderer& current_gif = GetCurrentGifRenderer();
				if (current_gif.IsInitialized()) {
					extern IDirect3DDevice9* ImGui_ImplDX9_GetDevice();
					IDirect3DDevice9* device = ImGui_ImplDX9_GetDevice();
					if (device) {
						IDirect3DTexture9* current_frame = current_gif.GetCurrentFrameTexture(device);
						if (current_frame) {
							gif_draw->AddImage( (ImTextureID)current_frame,
								ImVec2( gif_x, gif_y ), ImVec2( gif_x + gif_w, gif_y + gif_h ),
								ImVec2( 0, 0 ), ImVec2( 1, 1 ), IM_COL32( 255, 255, 255, 255 ) );
						} else {
							gif_draw->AddRectFilled( ImVec2( gif_x, gif_y ),
								ImVec2( gif_x + gif_w, gif_y + gif_h ),
								IM_COL32( 80, 80, 80, 120 ), 4.0f );
						}
					} else {
						gif_draw->AddRectFilled( ImVec2( gif_x, gif_y ),
							ImVec2( gif_x + gif_w, gif_y + gif_h ),
							IM_COL32( 80, 80, 80, 120 ), 4.0f );
					}
				} else {
					gif_draw->AddRectFilled( ImVec2( gif_x, gif_y ),
						ImVec2( gif_x + gif_w, gif_y + gif_h ),
						IM_COL32( 80, 80, 80, 120 ), 4.0f );
				}
			}
		}
	}
}
IDirect3DTexture9* n_misc::impl_t::create_texture_from_url( const std::string& url )
{
	HINTERNET hInternet = InternetOpenA( "Mozilla/5.0 (Windows NT 10.0; Win64; x64)", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( !hInternet ) {
		return nullptr;
	}
	DWORD timeout = 30000;
	InternetSetOptionA( hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout) );
	InternetSetOptionA( hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout) );
	InternetSetOptionA( hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout) );
	DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | 
	              INTERNET_FLAG_NO_UI | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_COOKIES;
	if ( url.find( "https://" ) != std::string::npos ) {
		flags |= INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
	}
	HINTERNET hUrl = InternetOpenUrlA( hInternet, url.c_str( ), NULL, 0, flags, 0 );
	if ( !hUrl ) {
		InternetCloseHandle( hInternet );
		return nullptr;
	}
	std::vector< unsigned char > buffer;
	char tempBuffer[ 4096 ];
	DWORD bytesRead;
	while ( InternetReadFile( hUrl, tempBuffer, sizeof( tempBuffer ), &bytesRead ) && bytesRead > 0 ) {
		buffer.insert( buffer.end( ), tempBuffer, tempBuffer + bytesRead );
	}
	InternetCloseHandle( hUrl );
	InternetCloseHandle( hInternet );
	if ( buffer.empty( ) ) {
		return nullptr;
	}
	return create_texture_from_bytes( buffer.data( ), buffer.size( ) );
}
IDirect3DTexture9* n_misc::impl_t::create_texture_from_file( const std::string& filepath )
{
	if ( filepath.empty( ) )
		return nullptr;
	IDirect3DTexture9* texture = nullptr;
	HRESULT hr = D3DXCreateTextureFromFileA( g_interfaces.m_direct_device, filepath.c_str( ), &texture );
	if ( FAILED( hr ) ) {
		return nullptr;
	}
	return texture;
}
IDirect3DTexture9* n_misc::impl_t::create_texture_from_bytes( const unsigned char* data, size_t data_size )
{
	if ( !data || data_size == 0 ) {
		return nullptr;
	}
	if ( !g_interfaces.m_direct_device ) {
		return nullptr;
	}
	IDirect3DTexture9* texture = nullptr;
	HRESULT hr = D3DXCreateTextureFromFileInMemory( g_interfaces.m_direct_device, data, data_size, &texture );
	if ( FAILED( hr ) ) {
		return nullptr;
	}
	return texture;
}
static int skoka_pts_nado = 0;
void n_misc::impl_t::stepan_dolnaeb_render( )
{
	bool pts_display_enabled = GET_VARIABLE( g_variables.m_pts_display, bool );
	bool show_pts_picture = GET_VARIABLE( g_variables.m_pts_show_picture, bool );
	if ( !pts_display_enabled && !show_pts_picture )
		return;
	{
		static bool logged_texture_status = false;
		static bool logged_render_call = false;
		if ( !logged_texture_status ) {
			if ( g_misc.m_pts_images.m_image1_texture && g_misc.m_pts_images.m_image2_texture && g_misc.m_pts_images.m_image3_texture && g_misc.m_pts_images.m_image4_texture ) {
				g_console.print( "[PTS] All 4 textures loaded successfully" );
			} else {
				char msg[256];
				sprintf_s( msg, "[PTS] Texture status - Image1: %s, Image2: %s, Image3: %s, Image4: %s", 
					g_misc.m_pts_images.m_image1_texture ? "OK" : "NULL",
					g_misc.m_pts_images.m_image2_texture ? "OK" : "NULL",
					g_misc.m_pts_images.m_image3_texture ? "OK" : "NULL",
					g_misc.m_pts_images.m_image4_texture ? "OK" : "NULL" );
				g_console.print< n_console::log_level::WARNING >( msg );
			}
			logged_texture_status = true;
		}
		if ( !logged_render_call ) {
			g_console.print( "[PTS] stepan_dolnaeb_render() is being called" );
			logged_render_call = true;
		}
		bool textures_loaded = ( g_misc.m_pts_images.m_image1_texture || g_misc.m_pts_images.m_image2_texture || g_misc.m_pts_images.m_image3_texture || g_misc.m_pts_images.m_image4_texture );
		static int pts     = 0;
		static int old_pts = 0;
		int new_pts = m_points_system.m_current_points;
		if ( new_pts == 0 && pts > 0 ) {
			old_pts = 0;
		}
		pts = new_pts;
		static float y_pos_text_move  = 0.0f;
		static float y_pos_image_move = 0.0f;
		static double total_time   = 0.0;
		static float display_timer = 0.0f;
		static bool fading         = false;
		static float fade_timer    = 0.0f;
		static float anim_timer = 0.0f;
		static bool anim_active = false;
		static float shadow_pos_x = 0.0f;
		static float shadow_pos_y = 0.0f;
		static float shadow_pos_x_small = 0.0f;
		static float shadow_pos_y_small = 0.0f;
		static bool prev_return_down = false;
		static int add_count         = 0;
		static float current_pts_x = 0.0f;
		static float current_pts_alpha = 0.0f;
		static bool pts_has_been_visible = false;
		static float bob_phase = 0.0f;
		static float trail_offset_y = 0.0f;
		static bool first_reveal_active = false;
		static float first_reveal_timer = 0.0f;
		float current_time = ImGui::GetTime();
		float time_since_last_detect = current_time - m_points_system.m_last_detect_time;
		const int dpi_percent = std::clamp( GET_VARIABLE( g_variables.m_menu_dpi_scale, int ), 75, 200 );
		const float dpi_scale = static_cast< float >( dpi_percent ) / 100.0f;
		static float INACTIVITY_TIME     = 3.0f;
		static float FADE_DURATION       = 0.35f;
		static float QUICK_ANIM_DURATION = 0.25f;
		static float FIRST_REVEAL_DURATION = 0.22f;
		const float RIGHT_PX            = 50.0f * dpi_scale;
		float IMAGE_W = 32.0f, IMAGE_H = 32.0f;
		IMAGE_W = 40.0f * dpi_scale;
		IMAGE_H = 24.0f * dpi_scale;
		const float BEBRA_W = 24.0f * dpi_scale, BEBRA_H = 24.0f * dpi_scale;
		const float RECT_W = 95.0f * dpi_scale, RECT_H = 28.0f * dpi_scale;
		const float FONT_SIZE     = 18.0f * dpi_scale;
		const float GLOBAL_TEXT_Y = 22.f * dpi_scale;
		const float IMAGE_DISTANCE      = -8.0f * dpi_scale;
		const ImVec2 text_extra_offset  = ImVec2( 0.0f, -0.0f );
		const ImVec2 image_extra_offset = ImVec2( 0.0f, -6.0f * dpi_scale );
		static float ALPHA_OUT_TIME = 0.08f;
		static float ALPHA_IN_TIME  = 0.08f;
		static float INERTIA_STRENGTH = 200.0f;
		static float INERTIA_DAMPING  = 40.0f;
		static float IMPULSE_STRENGTH = 6.0f;
		static float offset_x_curr = 0.0f;
		static float offset_x_vel  = 0.0f;
		static float offset_target = 0.0f;
		static int cent_state     = 0;
		static float cent_timer   = 0.0f;
		static float cent_from    = 0.0f;
		static float cent_to      = 0.0f;
		static float cent_current = 0.0f;
		static float CENT_IN_DUR  = 0.06f;
		static float CENT_OUT_DUR = 0.12f;
		static float CENT_Y_ADJUST = 6.0f;
		static float pts_float_offset = 0.0f;
		static float pts_float_time = 0.0f;
		static float pts_velocity = 0.0f;
		static bool pts_was_reset = false;
		const float PI = 3.14159265359f;
		ImGuiIO& io = ImGui::GetIO( );
		float dt    = io.DeltaTime > 0.0f ? io.DeltaTime : ( 1.0f / 60.0f );
		total_time += dt;
		float watermark_width = 0.0f;
		if ( GET_VARIABLE( g_variables.m_watermark, bool ) ) {
			std::string wm_text = "kamibebra";
			if ( GET_VARIABLE( g_variables.m_watermark_custom_text, bool ) )
				wm_text = GET_VARIABLE( g_variables.m_watermark_custom_text_value, std::string );
			int show_flags_wm = GET_VARIABLE( g_variables.m_watermark_show_flags, int );
			std::string full_wm = wm_text;
			
			ImVec2 wm_size = ImGui::CalcTextSize( full_wm.c_str() );
			wm_size.x *= dpi_scale;
			wm_size.y *= dpi_scale;
			watermark_width = wm_size.x + 28.0f * dpi_scale;
			if ( watermark_width < 95.0f * dpi_scale ) watermark_width = 95.0f * dpi_scale;
		}
		ImFont* pts_font = fonts_for_gui::fonteditor_pts_font;
		float pts_font_size = GET_VARIABLE( g_variables.m_pts_thin_font_size, float );
		if ( pts_font_size < 8.0f ) pts_font_size = 8.0f;
		if ( pts_font_size > 64.0f ) pts_font_size = 64.0f;
		pts_font_size *= dpi_scale;
		std::string temp_text       = std::to_string( pts ) + " pts";
			ImVec2 temp_text_size =
			pts_font->CalcTextSizeA( pts_font_size, FLT_MAX, 0.0f, temp_text.c_str( ) );
		bool show_pts_picture_render = GET_VARIABLE( g_variables.m_pts_show_picture, bool );
		const float distance_watermark_to_pts_render = -8.0f * dpi_scale;
		const float watermark_pos_x = g_ctx.m_width - watermark_width - 8.0f * dpi_scale;
		float total_width = 0.0f;
		if ( pts_display_enabled && show_pts_picture_render ) {
			total_width = temp_text_size.x + IMAGE_DISTANCE + IMAGE_W;
		} else if ( pts_display_enabled ) {
			total_width = temp_text_size.x;
		} else if ( show_pts_picture_render ) {
			total_width = IMAGE_W;
		}
		float pts_base_x = watermark_pos_x - distance_watermark_to_pts_render - total_width;
		if ( pts_base_x < 0.0f || pts_base_x > g_ctx.m_width ) {
			pts_base_x = 100.0f;
		}
		static bool pts_first_init = true;
		if ( pts_first_init ) {
			current_pts_x = pts_base_x;
			pts_first_init = false;
		}
		static float pts_y_current = 8.0f;
		static bool pts_y_ever_shown = false;
		const float PTS_Y_REST = 25.0f;
		const float PTS_Y_WATERMARK = 8.0f;
		if ( !pts_y_ever_shown && anim_active ) {
			pts_y_current = PTS_Y_WATERMARK;
			pts_y_ever_shown = true;
		}
		if ( !pts_y_ever_shown && first_reveal_active ) {
			pts_y_current = PTS_Y_WATERMARK;
			pts_y_ever_shown = true;
		}
		if ( pts_y_ever_shown ) {
			float y_lerp_speed = 8.5f * dt;
			pts_y_current += ( PTS_Y_REST - pts_y_current ) * y_lerp_speed;
			if ( fabsf( pts_y_current - PTS_Y_REST ) < 0.1f )
				pts_y_current = PTS_Y_REST;
		}
		if ( fading && fade_timer >= FADE_DURATION ) {
			pts_y_ever_shown = false;
			pts_y_current = PTS_Y_WATERMARK;
		}
			ImVec2 pos_text  = ImVec2( pts_base_x, pts_y_current );
		ImDrawList* draw = ImGui::GetBackgroundDrawList( );
		ImVec2 base_text_pos  = ImVec2( pos_text.x, pos_text.y );
		ImVec2 rect_top_left =
			ImVec2( base_text_pos.x + temp_text_size.x + IMAGE_DISTANCE + IMAGE_W + 8.0f, base_text_pos.y - RECT_H / 2.0f + text_extra_offset.y );
		ImVec2 rect_bottom_right = ImVec2( rect_top_left.x + RECT_W, rect_top_left.y + RECT_H );
		float rect_rounding      = 8.0f;
		ImU32 rect_color         = IM_COL32( 0, 0, 0, 255 );
		bool first_pts_appearance = ( pts > 0 && old_pts <= 0 && !pts_has_been_visible );
		if ( pts > old_pts && m_points_system.m_current_points > 0 ) {
			display_timer = INACTIVITY_TIME;
			add_count++;
			if ( fading ) {
				fading     = false;
				fade_timer = 0.0f;
			}
			if ( first_pts_appearance ) {
				first_reveal_active = true;
				first_reveal_timer = 0.0f;
				anim_timer = QUICK_ANIM_DURATION;
				anim_active = false;
				offset_x_curr = 0.0f;
				offset_x_vel = 0.0f;
				pts_y_current = PTS_Y_WATERMARK;
				pts_y_ever_shown = true;
			} else {
				first_reveal_active = false;
				offset_x_vel += RIGHT_PX * IMPULSE_STRENGTH;
				anim_timer  = 0.0f;
				anim_active = true;
			}
			old_pts     = pts;
		}
		if ( display_timer <= 0.0f && pts > 0 && !fading ) {
			fading     = true;
			fade_timer = 0.0f;
		}
		float pts_speed_multiplier = 1.0f;
		if ( GET_VARIABLE( g_variables.m_pts_speed_enable, bool ) ) {
			pts_speed_multiplier = GET_VARIABLE( g_variables.m_pts_speed, float );
			if ( pts_speed_multiplier < 0.1f ) pts_speed_multiplier = 0.1f;
			if ( pts_speed_multiplier > 10.0f ) pts_speed_multiplier = 10.0f;
		}
		if ( anim_active ) {
			anim_timer += dt;
			if ( anim_timer >= QUICK_ANIM_DURATION ) {
				anim_timer  = QUICK_ANIM_DURATION;
				anim_active = false;
			}
		}
		if ( first_reveal_active ) {
			first_reveal_timer += dt;
			if ( first_reveal_timer >= FIRST_REVEAL_DURATION ) {
				first_reveal_timer = FIRST_REVEAL_DURATION;
				first_reveal_active = false;
			}
		}
		if ( show_pts_picture && !pts_display_enabled && !anim_active ) {
			anim_timer += dt;
			if ( anim_timer > 10.0f ) anim_timer = fmodf( anim_timer, 10.0f );
		}
		if ( fading ) {
			fade_timer += dt;
			if ( fade_timer > FADE_DURATION )
				fade_timer = FADE_DURATION;
		}
		if ( display_timer > 0.0f ) {
			display_timer -= dt;
			if ( display_timer < 0.0f )
				display_timer = 0.0f;
		}
		auto smoothLerp = [](float a, float b, float t) -> float { return a + (b - a) * t; };
		auto easeInOutSine = [PI](float x) -> float { return -(cosf(PI * x) - 1.0f) / 2.0f; };

		static float global_animation_time = 0.0f;
		global_animation_time += dt;
		if ( global_animation_time > 1000.0f ) global_animation_time = 0.0f; // Сброс чтобы не переполнялось

		float time_speed = pts_speed_multiplier;
		if ( time_speed < 0.1f ) time_speed = 1.0f;
		
		pts_float_time += dt * time_speed;
		const float TWO_PI = 2.0f * PI;
		if ( pts_float_time >= TWO_PI ) {
			pts_float_time -= TWO_PI;
		}
		bob_phase += dt * time_speed * 3.6f;
		if ( bob_phase >= TWO_PI ) {
			bob_phase -= TWO_PI;
		}

		const float BOB_AMPLITUDE = 3.75f;
		const float IMAGE_AMPLITUDE = 2.65f;
		float bob_wave = sinf( bob_phase );
		float image_sine = sinf( bob_phase - 0.55f );
		y_pos_image_move = image_sine * IMAGE_AMPLITUDE;
		
		bool wave_effect_enabled = false;

		const float PHASE_SHIFT = PI;

		float text_sine = sinf( bob_phase - PHASE_SHIFT );
		y_pos_text_move = text_sine * BOB_AMPLITUDE;

		if ( !wave_effect_enabled ) {
			y_pos_text_move = bob_wave * BOB_AMPLITUDE;
		}
		
		float backtrack_pts_multiplier = 0.0f;
		backtrack_pts_multiplier = 0.0f;

		if ( backtrack_pts_multiplier > 0.0f && !wave_effect_enabled ) {
			static float backtrack_text_offset = 0.0f;
			float lerp_speed = 8.0f / backtrack_pts_multiplier;
			backtrack_text_offset += (y_pos_image_move - backtrack_text_offset) * ImClamp(dt * lerp_speed, 0.0f, 1.0f);
			y_pos_text_move = backtrack_text_offset;
		}
		float alpha   = 1.0f;
		offset_target = 0.0f;
		auto easeOutQuad = []( float x ) -> float { return 1.0f - ( 1.0f - x ) * ( 1.0f - x ); };
		auto easeInQuad  = []( float x ) -> float { return x * x; };
		auto easeOutCubic = []( float x ) -> float { return 1.0f - powf( 1.0f - x, 3.0f ); };
		auto easeInCubic  = []( float x ) -> float { return x * x * x; };
		
		// Расстояние от ватермарка до конечной позиции pts
		const float SLIDE_DISTANCE = watermark_width + distance_watermark_to_pts_render + total_width + 20.0f * dpi_scale;
		
		if ( fading ) {
			// При затухании: элементы заезжают обратно вправо (в ватермарк) с плавным уменьшением альфы
			float fade_progress = ImClamp( fade_timer / FADE_DURATION, 0.0f, 1.0f );
			offset_target = SLIDE_DISTANCE * easeInCubic( fade_progress );
			alpha = 1.0f - easeInQuad( fade_progress );
		} else if ( first_reveal_active ) {
			// Первое появление: плавно выезжают из ватермарка (справа налево) с увеличением альфы
			float reveal_t = ImClamp( first_reveal_timer / FIRST_REVEAL_DURATION, 0.0f, 1.0f );
			float reveal_ease = easeOutCubic( reveal_t );
			offset_target = SLIDE_DISTANCE * ( 1.0f - reveal_ease );
			alpha = reveal_ease;
		} else if ( anim_active ) {
			float t = anim_timer / QUICK_ANIM_DURATION;
			t       = ImClamp( t, 0.0f, 1.0f );
			if ( t < 0.5f ) {
				// Первая половина: заезжают обратно вправо (в ватермарк) с уменьшением альфы
				float half_progress = t * 2.0f;
				offset_target = SLIDE_DISTANCE * easeInCubic( half_progress );
				alpha = 1.0f - easeInQuad( half_progress );
			} else {
				// Вторая половина: выезжают из ватермарка с увеличением альфы
				float half_progress = ( t - 0.5f ) * 2.0f;
				offset_target = SLIDE_DISTANCE * ( 1.0f - easeOutCubic( half_progress ) );
				alpha = easeOutQuad( half_progress );
			}
		} else {
			offset_target = 0.0f;
			alpha         = 1.0f;
		}
		std::string pts_text_tmp = std::to_string( pts ) + " pts";
		ImVec2 text_size_tmp = pts_font->CalcTextSizeA( pts_font_size, FLT_MAX, 0.0f, pts_text_tmp.c_str( ) );
		float rect_center_y = rect_top_left.y + RECT_H * 0.5f;
		float base_text_y = pos_text.y + text_size_tmp.y / 2.5f + text_extra_offset.y + y_pos_text_move;
		if ( ( anim_active || fading ) && cent_state == 0 ) {
			cent_state = 1;
			cent_timer = 0.0f;
			cent_from  = cent_current;
			cent_to    = ( rect_center_y + CENT_Y_ADJUST ) - base_text_y;
		}
		if ( !( anim_active || fading ) && cent_state == 1 ) {
			cent_state = 2;
			cent_timer = 0.0f;
			cent_from  = cent_current;
			cent_to    = 0.0f;
		}
		if ( cent_state != 0 ) {
			cent_timer += dt;
			float prog   = ( cent_state == 1 ) ? ImClamp( cent_timer / CENT_IN_DUR, 0.0f, 1.0f ) : ImClamp( cent_timer / CENT_OUT_DUR, 0.0f, 1.0f );
			float ease   = easeOutQuad( prog );
			cent_current = cent_from + ( cent_to - cent_from ) * ease;
			if ( ( cent_state == 1 && prog >= 1.0f ) || ( cent_state == 2 && prog >= 1.0f ) ) {
				if ( cent_state == 2 ) {
					cent_state   = 0;
					cent_current = cent_to;
				}
			}
		}
		float k     = INERTIA_STRENGTH;
		float c     = INERTIA_DAMPING;
		float accel = k * ( offset_target - offset_x_curr ) - c * offset_x_vel;
		offset_x_vel += accel * dt;
		offset_x_curr += offset_x_vel * dt;
		if ( fabsf( offset_x_curr ) < 0.0001f ) {
			offset_x_curr = 0.0f;
			offset_x_vel  = 0.0f;
		}
		static float reset_delay_timer = 0.0f;
		if ( fading && fade_timer >= FADE_DURATION && alpha <= 0.001f ) {
			reset_delay_timer += dt;
			if ( reset_delay_timer >= 0.2f ) {
				pts           = 0;
				old_pts       = 0;
				add_count     = 0;
				fading        = false;
				fade_timer    = 0.0f;
				anim_active   = false;
				anim_timer    = 0.0f;
				display_timer = 0.0f;
				alpha         = 1.0f;
				offset_x_curr = 0.0f;
				offset_x_vel  = 0.0f;
				reset_delay_timer = 0.0f;
				pts_was_reset = true;
				pts_float_offset = 0.0f;
				pts_float_time = 0.0f;
				pts_velocity = 0.0f;
				bob_phase = 0.0f;
				trail_offset_y = 0.0f;
				pts_has_been_visible = false;
				first_reveal_active = false;
				first_reveal_timer = 0.0f;
				y_pos_text_move = 0.0f;
				y_pos_image_move = 0.0f;
				shadow_pos_x = 0.0f;
				shadow_pos_y = 0.0f;
				shadow_pos_x_small = 0.0f;
				shadow_pos_y_small = 0.0f;
				cent_state = 0;
				cent_timer = 0.0f;
				cent_current = 0.0f;
				cent_from = 0.0f;
				cent_to = 0.0f;
				m_points_system.m_current_points = 0;
				m_points_system.m_previous_points = 0;
				reset_detect_states( );
				m_outline_sync.m_pts_active = false;
				m_outline_sync.m_outline_alpha = 0.0f;
				m_outline_sync.m_pts_alpha_with_animation = 0.0f;
			}
		} else {
			reset_delay_timer = 0.0f;
			if ( pts > 0 && m_points_system.m_current_points > 0 ) {
				pts_was_reset = false;
				pts_has_been_visible = true;
			}
		}
		bool should_render = ( pts != 0 || anim_active || fading );
		if ( pts_was_reset && m_points_system.m_current_points <= 0 ) {
			should_render = false;
			pts = 0;
			old_pts = 0;
			add_count = 0;
			fading = false;
			fade_timer = 0.0f;
			anim_active = false;
			anim_timer = 0.0f;
			first_reveal_active = false;
			first_reveal_timer = 0.0f;
			display_timer = 0.0f;
			alpha = 0.0f;
			offset_x_curr = 0.0f;
			offset_x_vel = 0.0f;
		}
		if ( !should_render ) {
			const float outline_lerp_speed = 8.0f;
			float outline_lerp_factor = outline_lerp_speed * dt;
			m_outline_sync.m_outline_alpha += ( 0.0f - m_outline_sync.m_outline_alpha ) * outline_lerp_factor;
			if ( m_outline_sync.m_outline_alpha < 0.001f ) {
				m_outline_sync.m_outline_alpha = 0.0f;
				m_outline_sync.m_pts_active = false;
				m_outline_sync.m_pts_alpha_with_animation = 0.0f;
			}
		}
		if ( should_render ) {
			static bool logged_drawing = false;
			if ( !logged_drawing ) {
				char msg[256];
				sprintf_s( msg, "[PTS] Starting to draw - pts: %d, anim_active: %d, fading: %d", pts, anim_active, fading );
				g_console.print( msg );
				logged_drawing = true;
			}
			std::string pts_label = "pts";
			pts_label = "pts";
			std::string pts_text = std::to_string( pts ) + " " + pts_label;
			ImVec2 text_size     = pts_font->CalcTextSizeA( pts_font_size, FLT_MAX, 0.0f, pts_text.c_str( ) );
			float pts_x_offset = 0.0f;
			if ( GET_VARIABLE( g_variables.m_pts_x_coordinates, bool ) ) {
				pts_x_offset = GET_VARIABLE( g_variables.m_pts_x_offset, float );
			}
			ImVec2 text_draw_pos = ImVec2( pos_text.x, pos_text.y + text_size.y / 2.5f );
			text_draw_pos.x += offset_x_curr + text_extra_offset.x + pts_x_offset;
			text_draw_pos.y += y_pos_text_move + text_extra_offset.y + cent_current;
			current_pts_x = text_draw_pos.x;
			float target_outline_alpha = alpha;
			m_outline_sync.m_pts_alpha_with_animation = target_outline_alpha;
			m_outline_sync.m_outline_alpha = target_outline_alpha;
			m_outline_sync.m_pts_active = ( pts > 0 || anim_active || fading );
			int alpha_byte   = ( int )ImClamp( alpha * 255.0f, 0.0f, 255.0f );
			int shadow_alpha = ( int )ImClamp( alpha * 180.0f, 0.0f, 255.0f );
			if ( shadow_alpha > alpha_byte )
				shadow_alpha = alpha_byte;
			int trail_alpha_far = ( int )ImClamp( alpha * 90.0f, 0.0f, 255.0f );
			int trail_alpha_mid = ( int )ImClamp( alpha * 135.0f, 0.0f, 255.0f );
			c_color shadow_base = GET_VARIABLE( g_variables.m_accent, c_color );
			ImU32 purple = IM_COL32(
				shadow_base[ 0 ],
				shadow_base[ 1 ],
				shadow_base[ 2 ],
				shadow_alpha
			);
			text_draw_pos.y -= GLOBAL_TEXT_Y;
			bool backtrack_shadow_enabled = false;
			int backtrack_strength = 1;
			if ( pts_display_enabled ) {
				backtrack_shadow_enabled = false;
				backtrack_strength = 1;
			}
			const float dt = ImGui::GetIO( ).DeltaTime;
			bool pts_fully_visible = ( alpha >= 0.99f );
			bool is_bobbing = !anim_active && !fading && pts_fully_visible && pts > 0;
			static bool shadow_initialized = false;
			if ( anim_active && !shadow_initialized ) {
				shadow_pos_x = text_draw_pos.x + 1.0f;
				shadow_pos_y = text_draw_pos.y + 1.0f;
				shadow_pos_x_small = text_draw_pos.x + 0.5f;
				shadow_pos_y_small = text_draw_pos.y + 0.5f;
				shadow_initialized = true;
			}
			if ( !anim_active && !is_bobbing && alpha < 0.1f ) {
				shadow_initialized = false;
			}
			if ( pts_display_enabled ) {
				float backtrack_multiplier = backtrack_shadow_enabled ? powf( 1.5f, static_cast< float >( backtrack_strength ) ) : 1.0f;
				float trail_responsiveness = backtrack_shadow_enabled ? ( 8.0f / backtrack_multiplier ) : 5.0f;
				if ( trail_responsiveness < 1.0f )
					trail_responsiveness = 1.0f;
				float trail_lerp = ImClamp( dt * trail_responsiveness, 0.0f, 1.0f );
				float trail_target = is_bobbing ? ( -y_pos_text_move * ( backtrack_shadow_enabled ? 0.9f * backtrack_multiplier : 1.15f ) ) : 0.0f;
				trail_offset_y += ( trail_target - trail_offset_y ) * trail_lerp;
				float anchor_offset_y = 1.0f;
				shadow_pos_x = text_draw_pos.x + 1.0f;
				shadow_pos_x_small = text_draw_pos.x + 0.5f;
				shadow_pos_y = text_draw_pos.y + anchor_offset_y + trail_offset_y;
				shadow_pos_y_small = text_draw_pos.y + 0.5f + trail_offset_y * 0.55f;

				ImU32 trail_far = IM_COL32( shadow_base[ 0 ], shadow_base[ 1 ], shadow_base[ 2 ], trail_alpha_far );
				ImU32 trail_mid = IM_COL32( shadow_base[ 0 ], shadow_base[ 1 ], shadow_base[ 2 ], trail_alpha_mid );
				draw->AddText( pts_font, pts_font_size,
				               ImVec2( text_draw_pos.x + 1.8f, text_draw_pos.y + 1.8f + trail_offset_y * 1.35f ), trail_far, pts_text.c_str( ) );
				draw->AddText( pts_font, pts_font_size,
				               ImVec2( text_draw_pos.x + 1.25f, text_draw_pos.y + 1.25f + trail_offset_y ), trail_mid, pts_text.c_str( ) );
				draw->AddText( pts_font, pts_font_size,
				               ImVec2( shadow_pos_x, shadow_pos_y ), purple, pts_text.c_str( ) );
				draw->AddText( pts_font, pts_font_size,
				               ImVec2( shadow_pos_x_small, shadow_pos_y_small ), purple, pts_text.c_str( ) );
			}
			if ( pts_display_enabled ) {
				ImU32 pts_color = IM_COL32( 255, 255, 255, alpha_byte );
				draw->AddText( pts_font, pts_font_size, text_draw_pos, pts_color, pts_text.c_str( ) );
				text_draw_pos.y += GLOBAL_TEXT_Y;
			}
			static float current_picture_x = 0.0f;
			static float current_picture_alpha = 0.0f;
			static float target_picture_x = 0.0f;
			static float target_picture_alpha = 0.0f;
			static bool picture_first_run = true;
			if ( !pts_display_enabled ) {
				const float pos_x = g_ctx.m_width - watermark_width - 8.0f;
				const float target_x = pos_x - 5.0f - IMAGE_W;
				if ( picture_first_run ) {
					current_picture_x = g_ctx.m_width - 90.0f;
					target_picture_x = target_x;
					current_picture_alpha = 0.0f;
					target_picture_alpha = 0.0f;
					picture_first_run = false;
				}
				if ( m_points_system.m_current_points <= 0 ) {
					target_picture_x = g_ctx.m_width - 90.0f;
					target_picture_alpha = 0.0f;
				} else {
					if ( time_since_last_detect > 4.0f ) {
						target_picture_x = g_ctx.m_width - 90.0f;
						target_picture_alpha = 0.0f;
					} else {
						target_picture_x = target_x;
						target_picture_alpha = 1.0f;
					}
				}
				float lerp_speed = 4.0f * dt;
				static float picture_appearance_boost_time_left = 0.0f;
				if ( target_picture_alpha > current_picture_alpha + 0.05f && current_picture_alpha < 0.1f && picture_appearance_boost_time_left <= 0.0f ) {
					picture_appearance_boost_time_left = 0.35f;
				}
				if ( picture_appearance_boost_time_left > 0.0f ) {
					lerp_speed *= 1.8f;
					picture_appearance_boost_time_left -= dt;
				}
				float lerp_factor = lerp_speed;
				float x_diff = target_picture_x - current_picture_x;
				float alpha_diff = target_picture_alpha - current_picture_alpha;
				current_picture_x = current_picture_x + x_diff * lerp_factor * 0.8f;
				current_picture_alpha = current_picture_alpha + alpha_diff * lerp_factor;
			}
			if ( show_pts_picture ) {
				ImVec2 image_top_left;
				ImVec2 image_bottom_right;
				float picture_x_offset = 0.0f;
				if ( GET_VARIABLE( g_variables.m_pts_picture_x_coordinates, bool ) ) {
					picture_x_offset = GET_VARIABLE( g_variables.m_pts_picture_x_offset, float );
				}
				if ( pts_display_enabled ) {
					image_top_left.x = current_pts_x + text_size.x + IMAGE_DISTANCE + image_extra_offset.x + picture_x_offset;
					image_top_left.y = text_draw_pos.y - ( IMAGE_H / 2.0f ) + image_extra_offset.y + y_pos_image_move + cent_current;
					image_bottom_right = ImVec2( image_top_left.x + IMAGE_W, image_top_left.y + IMAGE_H );
				} else {
					image_top_left.x = current_picture_x + picture_x_offset;
					image_top_left.y = 8.0f + ( 28.0f - IMAGE_H ) / 2.0f + y_pos_image_move + cent_current;
					image_bottom_right = ImVec2( image_top_left.x + IMAGE_W, image_top_left.y + IMAGE_H );
				}
				LPDIRECT3DTEXTURE9 useTex = nullptr;
				bool flip_v               = false;
				bool pic_bebra = GET_VARIABLE( g_variables.m_pts_picture_bebra, bool );
				bool pic_bebra_cool = GET_VARIABLE( g_variables.m_pts_picture_bebra_cool, bool );
				bool always_reversed = false;
				bool always_default = GET_VARIABLE( g_variables.m_pts_picture_always_default, bool );
				struct { bool enabled; LPDIRECT3DTEXTURE9 texture; } pic_entries[] = {
					{ pic_bebra,      g_misc.m_pts_images.m_image1_texture },
					{ pic_bebra_cool, g_misc.m_pts_images.m_image2_texture },
					{ true,           g_misc.m_pts_images.m_image3_texture },
				};
				std::vector< LPDIRECT3DTEXTURE9 > available_textures;
				for ( auto& entry : pic_entries ) {
					if ( entry.enabled && entry.texture )
						available_textures.push_back( entry.texture );
				}
				if ( available_textures.empty( ) ) {
					if ( g_misc.m_pts_images.m_image1_texture ) available_textures.push_back( g_misc.m_pts_images.m_image1_texture );
					if ( g_misc.m_pts_images.m_image2_texture ) available_textures.push_back( g_misc.m_pts_images.m_image2_texture );
					if ( g_misc.m_pts_images.m_image3_texture ) available_textures.push_back( g_misc.m_pts_images.m_image3_texture );
				}
				if ( !available_textures.empty( ) ) {
					int texture_switch_every = 2;
					int texture_index = ( add_count > 0 ) ? ( ( add_count - 1 ) / texture_switch_every ) % (int)available_textures.size( ) : 0;
					useTex = available_textures[ texture_index ];
					int flip_cycle = ( add_count > 0 ) ? ( ( add_count - 1 ) % ( texture_switch_every * 2 ) ) : 0;
					flip_v = ( flip_cycle >= texture_switch_every );
				}
				if ( always_default ) {
					flip_v = false;
				}
				else if ( always_reversed ) {
					flip_v = false;
				}
				ImVec2 uv0( 0.0f, 0.0f ), uv1( 1.0f, 1.0f );
				if ( flip_v ) {
					uv0 = ImVec2( 0.0f, 1.0f );
					uv1 = ImVec2( 1.0f, 0.0f );
				}
				int image_alpha = 255;
				if ( pts_display_enabled ) {
					image_alpha = ( int )ImClamp( alpha * 255.0f, 0.0f, 255.0f );
				} else {
					image_alpha = ( int )ImClamp( current_picture_alpha * 255.0f, 0.0f, 255.0f );
				}
				if ( useTex ) {
					draw->AddImage( useTex, image_top_left, image_bottom_right, uv0, uv1, IM_COL32( 255, 255, 255, image_alpha ) );
				} else {
					ImU32 placeholder_color = IM_COL32( 170, 0, 255, image_alpha );
					draw->AddRectFilled( image_top_left, image_bottom_right, placeholder_color, 4.0f );
					ImVec2 question_pos = ImVec2( 
						image_top_left.x + ( IMAGE_W - ImGui::CalcTextSize( "?" ).x ) * 0.5f,
						image_top_left.y + ( IMAGE_H - ImGui::CalcTextSize( "?" ).y ) * 0.5f
					);
					draw->AddText( question_pos, IM_COL32( 255, 255, 255, image_alpha ), "?" );
				}
			}
		}
	}
}
void n_misc::impl_t::stepan_dolbaeb_ne_smog_pts_realize( )
{
	if ( !g_ctx.m_local || !g_interfaces.m_engine_client || !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	if ( !g_ctx.m_cmd )
		return;
	const bool any_action = ( g_ctx.m_cmd->m_buttons & ( in_jump | in_duck ) ) ||
		g_input.check_input( &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) ) ||
		g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) ||
		g_input.check_input( &GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ) ) ||
		g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ) );
	if ( !any_action )
		return;
	static bool debug_logging_enabled = true;
	static DWORD timer = 0;
	DWORD current_time = (DWORD)GetTickCount64( );
	if ( timer > current_time )
		return;
	static int surf_timer = 0;
	c_vector local_velocity = g_ctx.m_local->get_velocity( );
	c_vector backup_velocity = g_prediction.backup_data.m_velocity;
	int local_flags = g_ctx.m_local->get_flags( );
	int backup_flags = g_prediction.backup_data.m_flags;
	int move_type = g_ctx.m_local->get_move_type( );
	float surf_z_velocity = -6.25f;
	float surf_tolerance = 0.1f;
	const bool surf_action = g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ) ) ||
		g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) );
	if ( surf_action && fabsf( local_velocity.m_z - surf_z_velocity ) < surf_tolerance && 
	     fabsf( backup_velocity.m_z - surf_z_velocity ) < surf_tolerance )
	{
		skoka_pts_nado = 25;
		timer = current_time + 350;
		if ( surf_timer == 0 ) {
			skoka_pts_nado = 100;
			if ( debug_logging_enabled ) {
				g_console.print( "[PTS] Detected SURF (first) - 100 pts" );
			}
		} else if ( debug_logging_enabled ) {
			g_console.print( "[PTS] Detected SURF - 25 pts" );
		}
		surf_timer++;
		return;
	}
	surf_timer = 0;
	if ( g_input.check_input( &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) ) )
	{
		if ( backup_velocity.m_z < -6.0f && 
		     local_velocity.m_z > backup_velocity.m_z &&
		     local_velocity.m_z < -6.0f )
		{
			skoka_pts_nado = 125;
			timer = current_time + 200;
			if ( debug_logging_enabled ) {
				g_console.print( "[PTS] Detected EDGE BUG - 125 pts" );
			}
			return;
		}
	}
	if ( GET_VARIABLE( g_variables.m_jump_bug, bool ) && 
	     g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) &&
	     !g_input.check_input( &GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ) ) &&
	     !g_movement.m_edgebug_data.m_will_edgebug &&
	     move_type != move_type_ladder &&
	     move_type != move_type_noclip && 
	     move_type != move_type_observer ) {
		bool velocity_z_increasing = local_velocity.m_z > backup_velocity.m_z;
		bool was_not_on_ground = !( backup_flags & 1 );
		bool not_on_ground_now = !( local_flags & 1 );
		if ( velocity_z_increasing && was_not_on_ground && not_on_ground_now ) {
			skoka_pts_nado = 50;
			timer = current_time + 200;
			if ( debug_logging_enabled ) {
				g_console.print( "[PTS] Detected JUMP BUG - 50 pts" );
			}
			return;
		}
	}
	if ( g_input.check_input( &GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ) ) ) {
		if ( backup_velocity.length_2d( ) < 100.0f && local_velocity.length_2d( ) > 200.0f ) {
			skoka_pts_nado = 175;
			timer = current_time + 500;
			if ( debug_logging_enabled ) {
				g_console.print( "[PTS] Detected FIREMAN - 175 pts" );
			}
			return;
		}
	}
	if ( ( g_ctx.m_cmd->m_buttons & in_jump ) && backup_velocity.m_z > 50.0f &&
	     local_velocity.m_z < backup_velocity.m_z * 0.3f && move_type != move_type_ladder && !( local_flags & 1 ) )
	{
		skoka_pts_nado = 50;
		timer = current_time + 500;
		if ( debug_logging_enabled ) {
			g_console.print( "[PTS] Detected HEAD CEILING - 50 pts" );
		}
		return;
	}
}
void n_misc::impl_t::update_clantag( )
{
	using SetClanTag_t = void(__fastcall*)( const char*, const char* );
	static SetClanTag_t set_clan_tag = nullptr;
	static bool initialization_attempted = false;
	if ( !initialization_attempted ) {
		initialization_attempted = true;
		auto engine_module = GetModuleHandleA( "engine.dll" );
		if ( engine_module ) {
			auto pattern = reinterpret_cast<std::uint8_t*>( engine_module );
			for ( size_t i = 0; i < 0x500000; i++ ) {
				if ( pattern[i] == 0x53 && pattern[i+1] == 0x56 && pattern[i+2] == 0x57 && 
				     pattern[i+3] == 0x8B && pattern[i+4] == 0xDA && pattern[i+5] == 0x8B && 
				     pattern[i+6] == 0xF9 && pattern[i+7] == 0xFF && pattern[i+8] == 0x15 ) {
					set_clan_tag = reinterpret_cast<SetClanTag_t>( &pattern[i] );
					break;
				}
			}
		}
	}
	if ( !set_clan_tag ) {
		return;
	}
	if ( !GET_VARIABLE( g_variables.m_clantag, bool ) ) {
		static bool cleared_off = false;
		if ( !cleared_off ) {
			set_clan_tag( "", "" );
			cleared_off = true;
		}
		return;
	}
	static bool cleared_off = false;
	cleared_off = false;
	if ( !g_interfaces.m_engine_client || !g_interfaces.m_engine_client->is_connected( ) )
		return;
	std::string base_text = "bebra";
	if ( GET_VARIABLE( g_variables.m_clantag_custom_text, bool ) ) {
		base_text = GET_VARIABLE( g_variables.m_clantag_custom_text_value, std::string );
		if ( base_text.empty( ) ) {
			base_text = "bebra";
		}
	}
	int animation_type = GET_VARIABLE( g_variables.m_clantag_animation_type, int );
	if ( animation_type == 0 ) {
		static const char* emojis[] = { ":c", "x_x", ">:3", "G_G", ">_<", "c:", ">.<", "B-)" };
		static const int emoji_count = 8;
		static int current_emoji_index = 0;
		static int animation_stage = 0;
		static float last_update_time = 0.0f;
		static bool cursor_visible = true;
		static float cursor_blink_time = 0.0f;
		static std::string last_base_text = base_text;
		if ( base_text != last_base_text ) {
			animation_stage = 0;
			last_update_time = 0.0f;
			last_base_text = base_text;
		}
		float current_time = ImGui::GetTime();
		if ( current_time - cursor_blink_time >= 0.5f ) {
			cursor_visible = !cursor_visible;
			cursor_blink_time = current_time;
		}
		const char* current_emoji = emojis[current_emoji_index];
		int emoji_length = strlen(current_emoji);
		int emoji_end_stage = emoji_length - 1;
		int pause_stage = emoji_end_stage + 1;
		int delete_start_stage = pause_stage + 1;
		int delete_end_stage = delete_start_stage + emoji_length - 1;
		int total_stages = delete_end_stage + 1;
		float delay = 1.0f;
		if ( animation_stage == pause_stage ) {
			delay = 5.0f;
		}
		if ( current_time - last_update_time >= delay ) {
			last_update_time = current_time;
			animation_stage++;
			if ( animation_stage >= total_stages ) {
				animation_stage = 0;
				current_emoji_index = ( current_emoji_index + 1 ) % emoji_count;
			}
		}
		std::string clantag = base_text + " ";
		if ( animation_stage <= emoji_end_stage ) {
			int emoji_chars = animation_stage + 1;
			clantag += std::string(current_emoji).substr(0, emoji_chars);
		} else if ( animation_stage == pause_stage ) {
			clantag += current_emoji;
		} else if ( animation_stage > pause_stage && animation_stage <= delete_end_stage ) {
			int chars_to_delete = animation_stage - pause_stage;
			int remaining_emoji_chars = emoji_length - chars_to_delete;
			if ( remaining_emoji_chars > 0 ) {
				clantag += std::string(current_emoji).substr(0, remaining_emoji_chars);
			}
		}
		if ( cursor_visible && animation_stage != pause_stage ) {
			clantag += "|";
		} else if ( animation_stage == pause_stage ) {
			if ( cursor_visible ) {
				clantag += " |";
			} else {
				clantag += " ";
			}
		}
		static std::string last_clantag = "";
		if ( clantag != last_clantag ) {
			if ( clantag.length() <= 32 ) {
				set_clan_tag( clantag.c_str(), clantag.c_str() );
				last_clantag = clantag;
			}
		}
	}
	else if ( animation_type == 1 ) {
		static int animation_stage = 0;
		static float last_update_time = 0.0f;
		static std::string last_base_text = base_text;
		if ( base_text != last_base_text ) {
			animation_stage = 0;
			last_update_time = 0.0f;
			last_base_text = base_text;
		}
		float current_time = ImGui::GetTime();
		int text_length = base_text.length();
		int typing_end_stage = text_length - 1;
		int pause_stage = typing_end_stage + 1;
		int delete_start_stage = pause_stage + 1;
		int delete_end_stage = delete_start_stage + text_length - 1;
		int empty_stage = delete_end_stage + 1;
		int total_stages = empty_stage + 1;
		float delay = 1.0f;
		if ( animation_stage == pause_stage ) {
			delay = 2.0f;
		} else if ( animation_stage == empty_stage ) {
			delay = 1.0f;
		}
		if ( current_time - last_update_time >= delay ) {
			last_update_time = current_time;
			animation_stage++;
			if ( animation_stage >= total_stages ) {
				animation_stage = 0;
			}
		}
		std::string clantag = "\xE2\x99\xAC ";
		if ( animation_stage <= typing_end_stage ) {
			clantag += base_text.substr( 0, animation_stage + 1 );
		} else if ( animation_stage == pause_stage ) {
			clantag += base_text;
		} else if ( animation_stage > pause_stage && animation_stage <= delete_end_stage ) {
			int chars_deleted = animation_stage - pause_stage;
			int remaining_chars = text_length - chars_deleted;
			if ( remaining_chars > 0 ) {
				clantag += base_text.substr( chars_deleted );
			}
		}
		static std::string last_clantag = "";
		if ( clantag != last_clantag ) {
			if ( clantag.length() <= 32 ) {
				set_clan_tag( clantag.c_str(), clantag.c_str() );
				last_clantag = clantag;
			}
		}
	}
}
void n_misc::impl_t::mouse_fix()
{
	if (!GET_VARIABLE(g_variables.m_mouse_fix, bool))
		return;
	if (!g_ctx.m_local || !g_ctx.m_local->is_alive())
		return;
	static c_angle last_view_angles;
	c_angle current_angles;
	g_interfaces.m_engine_client->get_view_angles(current_angles);
	last_view_angles = current_angles;
}
void n_misc::impl_t::camera_fix()
{
	return;
}
