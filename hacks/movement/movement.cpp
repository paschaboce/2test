#include "movement.h"
#include "grief.h"
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <mmsystem.h>
#include <string>
#pragma comment( lib, "winmm.lib" )
#include "../../dependencies/imgui/imgui.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../../globals/interfaces/interfaces.h"
#include "../../hooks/recv.h"
#include "../../instalisation fonts/fonts2.h"
#include "../chat_hud.h"
#include "../misc/misc.h"
#include "../movement_recorder/movement_recorder.h"
#include "../prediction/prediction.h"
#include "edgebug.h"
#include "source_move.h"

void CorrectMovement( c_user_cmd* cmd, c_angle wish_angle, c_angle old_angles );
void start_movement_fix( c_user_cmd* cmd );
void end_movement_fix( c_user_cmd* cmd );

bool AlertJB                      = false;
bool alert_mini_crouch_hop        = false;
bool bounce_alert_mini_crouch_hop = false;
bool AlertJB2                     = false;
int b_buttons                     = 0;
c_vector round_pos( const c_vector& point );
static void lester_kill_poll_audio_wallclock( );
namespace
{
	constexpr float headbounce_velocity_z = -6.25f;

	C_RecvHook* g_air_stuck_velocity_proxy       = nullptr;
	RecvVarProxyFn g_air_stuck_velocity_original = nullptr;

	inline float get_half_gravity_per_tick( )
	{
		const float sv_gravity = g_convars[ HASH_BT( "sv_gravity" ) ] ? g_convars[ HASH_BT( "sv_gravity" ) ]->get_float( ) : 800.f;
		return -( sv_gravity * g_interfaces.m_global_vars_base->m_interval_per_tick * 0.5f );
	}

	inline bool is_pixelsurf_velocity( float z, float tolerance = 0.01f )
	{
		return std::abs( z - get_half_gravity_per_tick( ) ) < tolerance;
	}

	inline bool is_headbounce_velocity( float z, float tolerance = 0.25f )
	{
		return std::abs( z - headbounce_velocity_z ) < tolerance;
	}

	inline bool is_texturebug_followup_velocity( float z )
	{
		return is_pixelsurf_velocity( z ) || is_headbounce_velocity( z );
	}

	

} // namespace
class WindowDetect
{
public:
	enum WindowState {
		FADE_IN,
		VISIBLE,
		FADE_OUT
	};
	struct AnimatedWindow {
		int type;
		std::string info;
		float elapsed;
		WindowState state;
		float fadeInDuration;
		float visibleDuration;
		float fadeOutDuration;
		float totalDuration;
		float alpha;
		ImVec2 targetPosition;
		ImVec2 currentPosition;
		float accentProgress;
		float accentWidth;
		bool startFadeOut;
	};
	static constexpr float NORMAL_FADEIN_DURATION  = 0.25f;
	static constexpr float QUICK_FADEIN_DURATION   = 0.15f;
	static constexpr float VISIBLE_DURATION        = 1.8f;
	static constexpr float NORMAL_FADEOUT_DURATION = 0.2f;
	static constexpr float QUICK_FADEOUT_DURATION  = 0.12f;
	static constexpr float WINDOW_SPACING          = 5.0f / 1080.0f;
	void AddWindow( int type, const std::string& info )
	{
		if ( info.empty( ) || info.length( ) > 200 ) {
			return;
		}
		static std::deque< std::pair< std::string, float > > recent_messages;
		float current_time = ImGui::GetTime( );
		while ( !recent_messages.empty( ) && current_time - recent_messages.front( ).second > 2.0f ) {
			recent_messages.pop_front( );
		}
		for ( const auto& recent : recent_messages ) {
			if ( recent.first == info ) {
				return;
			}
		}
		recent_messages.push_back( { info, current_time } );
		const float baseYPosition         = 0.8f;
		constexpr int MAX_VISIBLE_WINDOWS = 5;
		auto* font                        = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ];
		ImVec2 infoTextSize               = font->CalcTextSizeA( font->FontSize, FLT_MAX, NULL, info.c_str( ) );
		float windowHeight                = 30.0f / 1080.0f;
		AnimatedWindow newWin;
		newWin.type            = type;
		newWin.info            = info;
		newWin.elapsed         = 0.f;
		newWin.state           = FADE_IN;
		newWin.fadeInDuration  = NORMAL_FADEIN_DURATION;
		newWin.visibleDuration = VISIBLE_DURATION;
		newWin.fadeOutDuration = NORMAL_FADEOUT_DURATION;
		newWin.totalDuration   = NORMAL_FADEIN_DURATION + VISIBLE_DURATION + NORMAL_FADEOUT_DURATION;
		newWin.alpha           = 0.f;
		newWin.accentProgress  = 0.f;
		newWin.accentWidth     = 0.f;
		newWin.startFadeOut    = false;
		int visibleCount       = 0;
		for ( auto& win : windows ) {
			if ( win.state != FADE_OUT ) {
				visibleCount++;
			}
		}
		for ( auto& win : windows ) {
			if ( win.state != FADE_OUT ) {
				win.targetPosition.y -= ( windowHeight + WINDOW_SPACING );
				win.currentPosition.y -= ( windowHeight + WINDOW_SPACING );
				if ( visibleCount >= MAX_VISIBLE_WINDOWS &&
				     win.targetPosition.y < baseYPosition - ( MAX_VISIBLE_WINDOWS - 1 ) * ( windowHeight + WINDOW_SPACING ) ) {
					win.state           = FADE_OUT;
					win.elapsed         = 0.f;
					win.fadeOutDuration = NORMAL_FADEOUT_DURATION;
				}
			}
		}
		newWin.targetPosition  = ImVec2( 0.5f, baseYPosition );
		newWin.currentPosition = ImVec2( 0.5f, baseYPosition );
		if ( newWin.targetPosition.x < 0.0f || newWin.targetPosition.x > 1.0f || newWin.targetPosition.y < 0.0f || newWin.targetPosition.y > 1.0f ) {
			newWin.targetPosition.x = std::clamp( newWin.targetPosition.x, 0.01f, 0.99f );
			newWin.targetPosition.y = std::clamp( newWin.targetPosition.y, 0.01f, 0.99f );
			newWin.currentPosition  = newWin.targetPosition;
		}
		windows.push_back( newWin );
	}
	float Lerp( float a, float b, float t )
	{
		return a + t * ( b - a );
	}
	float EaseInOutCubic( float t )
	{
		return t < 0.5f ? 4.0f * t * t * t : 1.0f - pow( -2.0f * t + 2.0f, 3.0f ) / 2.0f;
	}
	void Update( float deltaTime )
	{
		CleanupOldWindows( );
		std::sort( windows.begin( ), windows.end( ),
		           []( const AnimatedWindow& a, const AnimatedWindow& b ) { return a.targetPosition.y > b.targetPosition.y; } );
		for ( auto it = windows.begin( ); it != windows.end( ); ) {
			it->elapsed += deltaTime;
			float totalElapsed = 0.0f;
			if ( it->state == FADE_IN ) {
				totalElapsed = it->elapsed + 0.2f;
			} else if ( it->state == VISIBLE ) {
				totalElapsed = it->fadeInDuration + it->elapsed + 0.2f;
			} else if ( it->state == FADE_OUT ) {
				totalElapsed = it->fadeInDuration + it->visibleDuration + it->elapsed + 0.2f;
			}
			it->accentProgress = totalElapsed / ( it->totalDuration + 0.4f );
			if ( it->accentProgress > 1.0f )
				it->accentProgress = 1.0f;
			switch ( it->state ) {
			case FADE_IN:
				it->alpha = it->elapsed / it->fadeInDuration;
				if ( it->alpha > 1.0f )
					it->alpha = 1.0f;
				if ( it->elapsed >= it->fadeInDuration ) {
					it->state   = VISIBLE;
					it->elapsed = 0.f;
					it->alpha   = 1.f;
				}
				break;
			case VISIBLE:
				it->alpha = 1.f;
				if ( it->elapsed >= it->visibleDuration && !it->startFadeOut ) {
					it->startFadeOut = true;
					it->state        = FADE_OUT;
					it->elapsed      = 0.f;
				}
				break;
			case FADE_OUT:
				it->alpha = 1.0f - ( it->elapsed / it->fadeOutDuration );
				if ( it->alpha < 0.0f )
					it->alpha = 0.0f;
				if ( it->elapsed >= it->fadeOutDuration ) {
					it = windows.erase( it );
					continue;
				}
				break;
			}
			++it;
		}
	}
	void Draw( ImDrawList* draw, const ImVec2& displaySize )
	{
		for ( const auto& win : windows ) {
			if ( win.state == FADE_OUT && win.alpha <= 0.0f ) {
				continue;
			}
			auto* font          = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ];
			ImVec2 infoTextSize = font->CalcTextSizeA( font->FontSize, FLT_MAX, NULL, win.info.c_str( ) );
			float widthPadding  = 18.0f;
			float fixedHeight   = 30.0f;
			ImVec2 size         = ImVec2( infoTextSize.x + widthPadding, fixedHeight );
			ImVec2 pos      = ImVec2( displaySize.x * win.currentPosition.x - size.x * 0.5f, displaySize.y * win.currentPosition.y - size.y * 0.5f );
			int a           = static_cast< int >( win.alpha * 255 );
			ImU32 bgColor   = IM_COL32( 14, 14, 14, a );
			ImU32 textColor = IM_COL32( 240, 240, 240, a );
			const float cornerRadius = 4.0f;
			draw->AddRectFilled( pos, ImVec2( pos.x + size.x, pos.y + size.y ), bgColor, cornerRadius );
			c_color accent_from_menu = GET_VARIABLE( g_variables.m_accent, c_color );
			ImVec4 accentColor = ImVec4( accent_from_menu[ 0 ] / 255.0f, accent_from_menu[ 1 ] / 255.0f, accent_from_menu[ 2 ] / 255.0f, win.alpha );
			const float accentHeight = 1.0f;
			float maxWidth           = size.x - 2.0f;
			float margin             = 1.0f;
			float currentWidth       = maxWidth * ( 1.0f - win.accentProgress );
			if ( currentWidth < 0 )
				currentWidth = 0;
			float startX = pos.x + margin;
			if ( currentWidth > 0 ) {
				float centerX   = pos.x + size.x * 0.5f;
				float halfWidth = currentWidth * 0.5f;
				float lineY     = pos.y;
				draw->AddRectFilled( ImVec2( centerX - halfWidth, lineY ), ImVec2( centerX + halfWidth, lineY + accentHeight ),
				                     ImGui::GetColorU32( accentColor ) );
			}
			float leftPadding = 9.0f;
			ImVec2 textPos    = ImVec2( pos.x + leftPadding + ( size.x - widthPadding - infoTextSize.x ) * 0.5f,
			                            pos.y + ( size.y - infoTextSize.y ) * 0.5f - 2.5f + 2.0f );
			if ( win.info.find( "(" ) != std::string::npos && win.info.find( ")" ) != std::string::npos ) {
				ImFont* doctrine_font = fonts_for_gui::doctrine_addwindow;
				if ( doctrine_font == nullptr ) {
					doctrine_font = font;
				}
				std::string text = win.info;
				float current_x  = textPos.x;
				float shadow_x   = textPos.x + 1;
				size_t pos       = 0;
				while ( pos < text.length( ) ) {
					if ( text[ pos ] == '(' || text[ pos ] == ')' ) {
						std::string bracket( 1, text[ pos ] );
						draw->AddText( doctrine_font, doctrine_font->FontSize, ImVec2( shadow_x, textPos.y + 1 ), IM_COL32( 0, 0, 0, a / 4 ),
						               bracket.c_str( ) );
						draw->AddText( doctrine_font, doctrine_font->FontSize, ImVec2( current_x, textPos.y ), textColor, bracket.c_str( ) );
						ImVec2 bracket_size = doctrine_font->CalcTextSizeA( doctrine_font->FontSize, FLT_MAX, NULL, bracket.c_str( ) );
						current_x += bracket_size.x + 0.3f;
						shadow_x += bracket_size.x + 0.3f;
					} else {
						std::string char_str( 1, text[ pos ] );
						draw->AddText( font, font->FontSize, ImVec2( shadow_x, textPos.y + 1 ), IM_COL32( 0, 0, 0, a / 4 ), char_str.c_str( ) );
						draw->AddText( font, font->FontSize, ImVec2( current_x, textPos.y ), textColor, char_str.c_str( ) );
						ImVec2 char_size = font->CalcTextSizeA( font->FontSize, FLT_MAX, NULL, char_str.c_str( ) );
						current_x += char_size.x;
						shadow_x += char_size.x;
					}
					pos++;
				}
			} else {
				draw->AddText( font, font->FontSize, ImVec2( textPos.x + 1, textPos.y + 1 ), IM_COL32( 0, 0, 0, a / 4 ), win.info.c_str( ) );
				draw->AddText( font, font->FontSize, textPos, textColor, win.info.c_str( ) );
			}
		}
	}

private:
	std::vector< AnimatedWindow > windows;
	void CleanupOldWindows( )
	{
		constexpr int MAX_WINDOWS = 10;
		if ( windows.size( ) > MAX_WINDOWS ) {
			int windowsToRemove = windows.size( ) - MAX_WINDOWS;
			std::sort( windows.begin( ), windows.end( ),
			           []( const AnimatedWindow& a, const AnimatedWindow& b ) { return a.targetPosition.y < b.targetPosition.y; } );
			for ( int i = 0; i < windowsToRemove && i < windows.size( ); i++ ) {
				if ( windows[ i ].state != FADE_OUT ) {
					windows[ i ].state           = FADE_OUT;
					windows[ i ].elapsed         = 0.f;
					windows[ i ].fadeOutDuration = QUICK_FADEOUT_DURATION;
				}
			}
		}
	}
};
WindowDetect myWindowDetect;
void movement_add_window( int type, const std::string& info )
{
	myWindowDetect.AddWindow( type, info );
}
int ignoretime = 0;
void n_movement::impl_t::on_create_move_pre( )
{
	if ( GET_VARIABLE( g_variables.m_no_crouch_cooldown, bool ) )
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_bullrush;

	if ( GET_VARIABLE( g_variables.m_long_jump, bool ) &&
	     GET_VARIABLE( g_variables.m_long_jump_block_ws, bool ) &&
	     g_input.check_input( &GET_VARIABLE( g_variables.m_long_jump_key, key_bind_t ) ) &&
	     !( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) ) {
		g_ctx.m_cmd->m_forward_move = 0.0f;
		g_ctx.m_cmd->m_buttons &= ~( e_command_buttons::in_forward | e_command_buttons::in_back );
	}

	this->pixel_finder( g_ctx.m_cmd );
	this->ladder_freelook_climb( g_ctx.m_cmd );
	if ( g_ctx.m_cmd && m_ladder_freelook_climb_data.m_phase != ladder_freelook_climb_data_t::phase_idle ) {
		g_ctx.m_cmd->m_view_point = c_angle( 0.f, m_ladder_freelook_climb_data.m_server_yaw, 0.f );
		return;
	}
	
	this->auto_one_hop( g_ctx.m_cmd );
	if ( m_auto_one_hop_data.m_phase != auto_one_hop_data_t::phase_idle )
		return;

	const auto move_type = g_ctx.m_local->get_move_type( );
	if ( move_type == e_move_types::move_type_ladder || move_type == e_move_types::move_type_noclip || move_type == e_move_types::move_type_fly ||
	     move_type == e_move_types::move_type_observer ) {
		if ( move_type == e_move_types::move_type_ladder )
			this->ladder_glide( g_ctx.m_cmd );
		return;
	}

	if ( GET_VARIABLE( g_variables.m_bunny_hop, bool ) &&
	     !( GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) ) )
		this->bunny_hop( );

	if ( g_ctx.m_local ) {
		if ( g_ctx.m_local->get_health( ) != 0 && g_ctx.m_local->is_alive( ) ) {
			b_buttons                 = g_ctx.m_cmd->m_buttons;
			const int predicted_frame = g_interfaces.m_prediction->m_commands_predicted - 1;
			g_prediction.restore_entity_to_predicted_frame( predicted_frame );
			this->pixelsurf_assist( g_ctx.m_cmd );
			this->pixelsurf_assist_ground_help( g_ctx.m_cmd );
			this->autobounce_assist( g_ctx.m_cmd );

			static bool off_nahui = false;
			if ( g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) ) ) {
				if ( alert_mini_crouch_hop ) {
					if ( off_nahui ) {
						if ( is_pixelsurf_velocity( g_prediction.backup_data.m_velocity.m_z ) ) {
							off_nahui             = false;
							alert_mini_crouch_hop = false;
						}
						if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
							off_nahui             = false;
							alert_mini_crouch_hop = false;
						}
					}
					if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
						g_ctx.m_cmd->m_buttons &= ~e_command_buttons::in_duck;
						g_ctx.m_cmd->m_buttons |= in_jump;
						off_nahui = true;
					} else {
						g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
					}
				}
			} else {
				alert_mini_crouch_hop = false;
				off_nahui             = false;
			}

			static bool off_nahui2 = false;
			if ( g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) ) ) {
				if ( bounce_alert_mini_crouch_hop ) {
					if ( off_nahui2 ) {
						if ( is_pixelsurf_velocity( g_prediction.backup_data.m_velocity.m_z ) ) {
							off_nahui2                   = false;
							bounce_alert_mini_crouch_hop = false;
						}
						if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
							off_nahui2                   = false;
							bounce_alert_mini_crouch_hop = false;
						}
					}
					if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
						g_ctx.m_cmd->m_buttons &= ~e_command_buttons::in_duck;
						g_ctx.m_cmd->m_buttons |= in_jump;
						off_nahui2 = true;
					} else {
						g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
					}
				}
			} else {
				bounce_alert_mini_crouch_hop = false;
				off_nahui2                   = false;
			}

			if ( AlertJB || AlertJB2 )
				g_ctx.m_cmd->m_buttons |= in_jump;
			if ( m_pixelsurf_data.should_pixel_surf )
				g_ctx.m_cmd->m_buttons |= in_duck;
			this->apply_nulls( );
			if ( GET_VARIABLE( g_variables.m_auto_align, bool ) && !( g_prediction.backup_data.m_flags & 1 ) )
				this->auto_align( g_ctx.m_cmd );
			g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );

			this->extended_texture_bug( g_ctx.m_cmd );
			this->air_stuck( g_ctx.m_cmd );
			this->test( g_ctx.m_cmd );
			this->wall_stuck( g_ctx.m_cmd );
			this->antoha_bug( g_ctx.m_cmd );
			this->wall_climb( g_ctx.m_cmd );
			g_grief.blockbot( g_ctx.m_cmd );
			g_grief.door_spam( g_ctx.m_cmd );

			this->pixel_calc( g_ctx.m_cmd );
			this->fast_ladder( g_ctx.m_cmd );
			this->ladder_glide( g_ctx.m_cmd );
			this->ladder_bug( g_ctx.m_cmd );
			this->AutoStrafe( g_ctx.m_cmd ); 
			this->auto_crouch( g_ctx.m_cmd );
			this->detect_head_ceiling_hit( );
			m_pixelsurf_assist_t.set_point = false;
			if ( g_ctx.m_local->get_flags( ) & 1 )
				m_pixelsurf_data.should_pixel_surf = false;
		}
	}
}
void n_movement::impl_t::apply_nulls( )
{
	if ( !g_ctx.m_local || !g_ctx.m_cmd )
		return;
	if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
		return;
	const bool should_nulls = GET_VARIABLE( g_variables.m_nulls_assist, bool );
	if ( !should_nulls )
		return;
	if ( g_ctx.m_cmd->m_buttons & ( in_forward | in_back | in_moveleft | in_moveright ) ) {
		g_ctx.m_cmd->m_buttons &= ~in_moveleft;
		g_ctx.m_cmd->m_buttons &= ~in_moveright;
		g_ctx.m_cmd->m_buttons &= ~in_forward;
		g_ctx.m_cmd->m_buttons &= ~in_back;
	}
}
void n_movement::impl_t::bunny_hop( )
{
	if ( !( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) )
		g_ctx.m_cmd->m_buttons &= ~e_command_buttons::in_jump;
}
void n_movement::impl_t::extended_texture_bug( c_user_cmd* cmd )
{
	static bool active = false;
	static float start_z = 0.0f;

	const auto release_jetpack = [ & ]( ) {
		if ( active ) {
			g_interfaces.m_engine_client->execute_client_cmd( "-jetpack" );
			g_movement_recorder.notify_jetpack_state( false );
			active = false;
		}
	};

	if ( !GET_VARIABLE( g_variables.m_extended_texture_bug, bool ) || !cmd || !g_ctx.m_local || !g_ctx.m_local->is_alive( ) ) {
		release_jetpack( );
		return;
	}

	if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_extended_texture_bug_key, key_bind_t ) ) ) {
		release_jetpack( );
		return;
	}

	const c_vector origin = g_ctx.m_local->get_origin( );
	const float velocity_z = g_ctx.m_local->get_velocity( ).m_z;

	if ( velocity_z > 0.0f ) {
		release_jetpack( );
		return;
	}

	if ( active ) {
		if ( std::fabsf( origin.m_z - start_z ) >= 10.0f )
			release_jetpack( );
		return;
	}

	if ( std::fabsf( velocity_z - -6.25f ) <= 0.001f ) {
		start_z = origin.m_z;
		active  = true;
		g_interfaces.m_engine_client->execute_client_cmd( "+jetpack" );
		g_movement_recorder.notify_jetpack_state( true );
	}
}
bool can = false;
float LerpFloat( float a, float b, float t )
{
	return a + ( b - a ) * t;
}
std::vector< points_check_t > m_bounce_points_check{ };
std::vector< points_check_t > m_points_check{ };
bool point_menu_is_opened( )
{
	for ( size_t i = 0; i < m_bounce_points_check.size( ); ++i ) {
		points_check_t& point = m_bounce_points_check[ i ];
		if ( point.open_settings )
			return true;
	}
	for ( size_t i = 0; i < m_points_check.size( ); ++i ) {
		points_check_t& point = m_points_check[ i ];
		if ( point.open_settings )
			return true;
	}
	return false;
}
ImColor LerpColor( const ImColor& a, const ImColor& b, float t )
{
	t      = std::clamp( t, 0.0f, 1.0f );
	int r  = static_cast< int >( a.Value.x + ( b.Value.x - a.Value.x ) * t );
	int g  = static_cast< int >( a.Value.y + ( b.Value.y - a.Value.y ) * t );
	int b_ = static_cast< int >( a.Value.z + ( b.Value.z - a.Value.z ) * t );
	int a_ = static_cast< int >( a.Value.w + ( b.Value.w - a.Value.w ) * t );
	return ImColor( r, g, b_, a_ );
}
bool InCrosshair( float x, float y, float radius )
{
	float centerX = g_ctx.m_width / 2;
	float centerY = g_ctx.m_height / 2;
	float dx      = centerX - x;
	float dy      = centerY - y;
	return ( dx * dx + dy * dy ) <= ( radius * radius );
}
bool detect_opened_points  = false;
int gloabal_settings_timer = 0;
#include "../../instalisation fonts/fonts2.h"
#include <conio.h>
extern void checkbox(
	const char* name, bool* variable, bool disabled = false, bool popup = false, const ImVec2& popup_size = ImVec2( ),
	const std::function< void( ) >& popup_function = [] { } );
bool isEnterPressed( )
{
	static bool keyWasDown = false;
	if ( GetAsyncKeyState( VK_RETURN ) & 0x8000 ) {
		keyWasDown = true;
	} else if ( keyWasDown ) {
		keyWasDown = false;
		return true;
	}
	return false;
}
void RenderPoints( std::vector< points_check_t >& points, const c_vector& playerPos, const std::string& currentMap )
{
	float dt                       = ImGui::GetIO( ).DeltaTime;
	const float scaleSpeed         = 10.0f;
	static bool isBackspacePressed = false;
	if ( GetAsyncKeyState( VK_BACK ) & 0x8000 ) {
		isBackspacePressed = true;
	} else {
		isBackspacePressed = false;
	}
	auto accentclr = ImGui::GetStyle( ).Colors[ ImGuiCol_::ImGuiCol_Accent ];
	if ( GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ) ) {
		for ( size_t i = 0; i < points.size( ); ++i ) {
			points_check_t& point = points[ i ];
			if ( point.map != currentMap )
				continue;
			float assist_height   = GET_VARIABLE( g_variables.m_pixel_surf_assist_render_height, float );
			c_vector adjusted_pos = point.pos;
			adjusted_pos.m_z += assist_height;
			c_vector_2d screenPos;
			if ( !g_render.world_to_screen( adjusted_pos, screenPos ) )
				continue;
			float distance = playerPos.dist_to( adjusted_pos );
			if ( distance > 1100.f )
				continue;
			float baseRadius = 10.0f;
			if ( distance > 275.0f ) {
				if ( distance <= 650.0f ) {
					float t    = std::clamp( ( distance - 275.0f ) / 375.0f, 0.0f, 1.0f );
					t          = t * t * ( 3.0f - 2.0f * t );
					baseRadius = LerpFloat( 10.0f, 2.0f, t );
				} else {
					baseRadius = 2.0f;
				}
			}
			baseRadius             = std::max( baseRadius, 3.0f );
			float outlineThickness = 0.5f;
			if ( distance > 200.0f ) {
				float t          = std::clamp( ( distance - 200.0f ) / 200.0f, 0.0f, 1.0f );
				outlineThickness = LerpFloat( 0.5f, 1.0f, t );
			}
			float alphaMultiplier = 1.0f;
			if ( distance > 1400.0f ) {
				alphaMultiplier = std::clamp( ( 1500.0f - distance ) / 100.0f, 0.0f, 1.0f );
			}
			unsigned int outlineColor;
			bool inRadius = distance <= point.radius;
			if ( point.is_free_point ) {
				int outlineA = static_cast< int >( 255.0f * alphaMultiplier );
				outlineColor = c_color( 250, 120, 120, outlineA ).get_u32( );
			} else if ( !point.active ) {
				int outlineA = static_cast< int >( 255.0f * alphaMultiplier );
				outlineColor = c_color( 150, 150, 150, outlineA ).get_u32( );
			} else if ( inRadius ) {
				int outlineR = static_cast< int >( accentclr.x * 255.f );
				int outlineG = static_cast< int >( accentclr.y * 255.f );
				int outlineB = static_cast< int >( accentclr.z * 255.f );
				int outlineA = static_cast< int >( 255.0f * alphaMultiplier );
				outlineColor = c_color( outlineR, outlineG, outlineB, outlineA ).get_u32( );
			} else {
				int outlineA = static_cast< int >( 255.0f * alphaMultiplier );
				outlineColor = c_color( 255, 255, 255, outlineA ).get_u32( );
			}
			int fillAlpha;
			unsigned int fillColor;
			if ( point.is_free_point ) {
				fillAlpha = static_cast< int >( 255.0f * alphaMultiplier );
				fillColor = c_color( 250, 120, 120, fillAlpha ).get_u32( );
			} else {
				fillAlpha = static_cast< int >( 100.0f * alphaMultiplier );
				fillColor = c_color( 0, 0, 0, fillAlpha ).get_u32( );
			}
			bool isHovered       = InCrosshair( screenPos.m_x, screenPos.m_y, baseRadius );
			float proximityScale = 1.0f;
			if ( inRadius && point.radius > 0.0f ) {
				float proximity = std::clamp( 1.0f - ( distance / point.radius ), 0.0f, 1.0f );
				proximity       = proximity * proximity * ( 3.0f - 2.0f * proximity );
				proximityScale  = LerpFloat( 1.15f, 1.85f, proximity );
			}
			float hoverScale   = isHovered ? 1.5f : 1.0f;
			float targetScale  = std::max( proximityScale, hoverScale );
			point.currentScale    = LerpFloat( point.currentScale, targetScale, dt * scaleSpeed );
			float effectiveRadius = baseRadius * point.currentScale;
			if ( isHovered ) {
				if ( isEnterPressed( ) ) {
					gloabal_settings_timer = GetTickCount64( ) + 500;
					point.open_settings    = true;
					g_menu.m_opened        = true;
				}
			} else {
				point.open_settings = false;
			}
			filled_circle_draw_object_t filledCircle{ screenPos, effectiveRadius, fillColor, 32 };
			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_filled_circle, std::make_any< filled_circle_draw_object_t >( filledCircle ) );
			unsigned int grayColor = c_color( 128, 128, 128, 255 ).get_u32( );
			const int segments     = 32;
			const float angleStep  = 2.0f * 3.14159f / segments;
			for ( int j = 0; j < segments; ++j ) {
				float angle1       = j * angleStep;
				float angle2       = ( j + 1 ) * angleStep;
				bool isGraySegment = ( angle1 >= 3.93f && angle1 <= 5.50f ) || ( angle2 >= 3.93f && angle2 <= 5.50f );
				c_vector_2d pos1, pos2;
				pos1.m_x                  = screenPos.m_x + cos( angle1 ) * effectiveRadius;
				pos1.m_y                  = screenPos.m_y + sin( angle1 ) * effectiveRadius;
				pos2.m_x                  = screenPos.m_x + cos( angle2 ) * effectiveRadius;
				pos2.m_y                  = screenPos.m_y + sin( angle2 ) * effectiveRadius;
				unsigned int segmentColor = isGraySegment ? grayColor : outlineColor;
				line_draw_object_t outlineLine{ pos1, pos2, segmentColor, outlineThickness };
				g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( outlineLine ) );
			}
			if ( isHovered && point.show_3d_radius ) {
				float display_radius      = point.radius;
				const int circle_segments = 64;
				const float angle_step    = 2.0f * 3.14159f / circle_segments;
				for ( int j = 0; j < circle_segments; ++j ) {
					float angle1  = j * angle_step;
					float angle2  = ( j + 1 ) * angle_step;
					c_vector pos1 = point.pos + c_vector( cos( angle1 ) * display_radius, sin( angle1 ) * display_radius, 0.f );
					c_vector pos2 = point.pos + c_vector( cos( angle2 ) * display_radius, sin( angle2 ) * display_radius, 0.f );
					c_vector_2d screen_pos1, screen_pos2;
					if ( g_render.world_to_screen( pos1, screen_pos1 ) && g_render.world_to_screen( pos2, screen_pos2 ) ) {
						unsigned int white_color = c_color( 255, 255, 255, 200 ).get_u32( );
						line_draw_object_t radius_line{ screen_pos1, screen_pos2, white_color, 1.0f };
						g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( radius_line ) );
					}
				}
				if ( point.use_custom_first_jump_radius ) {
					float first_jump_radius = point.first_jump_radius;
					for ( int j = 0; j < circle_segments; ++j ) {
						float angle1  = j * angle_step;
						float angle2  = ( j + 1 ) * angle_step;
						c_vector pos1 = point.pos + c_vector( cos( angle1 ) * first_jump_radius, sin( angle1 ) * first_jump_radius, 0.f );
						c_vector pos2 = point.pos + c_vector( cos( angle2 ) * first_jump_radius, sin( angle2 ) * first_jump_radius, 0.f );
						c_vector_2d screen_pos1, screen_pos2;
						if ( g_render.world_to_screen( pos1, screen_pos1 ) && g_render.world_to_screen( pos2, screen_pos2 ) ) {
							unsigned int green_color = c_color( 0, 255, 0, 200 ).get_u32( );
							line_draw_object_t radius_line{ screen_pos1, screen_pos2, green_color, 1.0f };
							g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( radius_line ) );
						}
					}
				}
				if ( point.enable_restricted_binds && point.selected_restricted_bind != 0 ) {
					float restricted_radius = point.restricted_binds_radius;
					for ( int j = 0; j < circle_segments; ++j ) {
						float angle1  = j * angle_step;
						float angle2  = ( j + 1 ) * angle_step;
						c_vector pos1 = point.pos + c_vector( cos( angle1 ) * restricted_radius, sin( angle1 ) * restricted_radius, 0.f );
						c_vector pos2 = point.pos + c_vector( cos( angle2 ) * restricted_radius, sin( angle2 ) * restricted_radius, 0.f );
						c_vector_2d screen_pos1, screen_pos2;
						if ( g_render.world_to_screen( pos1, screen_pos1 ) && g_render.world_to_screen( pos2, screen_pos2 ) ) {
							unsigned int red_color = c_color( 255, 0, 0, 200 ).get_u32( );
							line_draw_object_t radius_line{ screen_pos1, screen_pos2, red_color, 1.0f };
							g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( radius_line ) );
						}
					}
				}
			}
			if ( isHovered && isBackspacePressed ) {
				points.erase( points.begin( ) + i );
				break;
			}
		}
	}
	if ( GET_VARIABLE( g_variables.m_bouncee_assist, bool ) ) {
		for ( size_t i = 0; i < m_bounce_points_check.size( ); ++i ) {
			points_check_t& point = m_bounce_points_check[ i ];
			if ( point.map != currentMap )
				continue;
			float assist_height   = GET_VARIABLE( g_variables.m_pixel_surf_assist_render_height, float );
			c_vector adjusted_pos = point.pos;
			adjusted_pos.m_z += assist_height;
			c_vector_2d screenPos;
			if ( !g_render.world_to_screen( adjusted_pos, screenPos ) )
				continue;
			float distance = playerPos.dist_to( adjusted_pos );
			if ( distance > 1100.f )
				continue;
			float baseRadius = 10.0f;
			if ( distance > 275.0f ) {
				if ( distance <= 650.0f ) {
					float t    = std::clamp( ( distance - 275.0f ) / 375.0f, 0.0f, 1.0f );
					t          = t * t * ( 3.0f - 2.0f * t );
					baseRadius = LerpFloat( 10.0f, 2.0f, t );
				} else {
					baseRadius = 2.0f;
				}
			}
			baseRadius             = std::max( baseRadius, 3.0f );
			float outlineThickness = 0.5f;
			if ( distance > 200.0f ) {
				float t          = std::clamp( ( distance - 200.0f ) / 200.0f, 0.0f, 1.0f );
				outlineThickness = LerpFloat( 0.5f, 1.0f, t );
			}
			float alphaMultiplier = 1.0f;
			if ( distance > 1400.0f ) {
				alphaMultiplier = std::clamp( ( 1500.0f - distance ) / 100.0f, 0.0f, 1.0f );
			}
			unsigned int outlineColor;
			bool inRadius = distance <= point.radius;
			if ( point.is_free_point ) {
				int outlineA = static_cast< int >( 255.0f * alphaMultiplier );
				outlineColor = c_color( 250, 120, 120, outlineA ).get_u32( );
			} else if ( !point.active ) {
				int outlineA = static_cast< int >( 255.0f * alphaMultiplier );
				outlineColor = c_color( 150, 150, 150, outlineA ).get_u32( );
			} else if ( inRadius ) {
				int outlineR = static_cast< int >( accentclr.x * 255.f );
				int outlineG = static_cast< int >( accentclr.y * 255.f );
				int outlineB = static_cast< int >( accentclr.z * 255.f );
				int outlineA = static_cast< int >( 255.0f * alphaMultiplier );
				outlineColor = c_color( outlineR, outlineG, outlineB, outlineA ).get_u32( );
			} else {
				int outlineA = static_cast< int >( 255.0f * alphaMultiplier );
				outlineColor = c_color( 255, 255, 255, outlineA ).get_u32( );
			}
			int fillAlpha;
			unsigned int fillColor;
			if ( point.is_free_point ) {
				fillAlpha = static_cast< int >( 255.0f * alphaMultiplier );
				fillColor = c_color( 250, 120, 120, fillAlpha ).get_u32( );
			} else {
				fillAlpha = static_cast< int >( 100.0f * alphaMultiplier );
				fillColor = c_color( 0, 0, 0, fillAlpha ).get_u32( );
			}
			bool isHovered = InCrosshair( screenPos.m_x, screenPos.m_y, baseRadius );
			if ( isHovered ) {
				if ( isEnterPressed( ) ) {
					gloabal_settings_timer = GetTickCount64( ) + 500;
					point.open_settings    = true;
					g_menu.m_opened        = true;
				}
			} else {
				point.open_settings = false;
			}
			float targetScale     = isHovered ? 1.5f : 1.0f;
			point.currentScale    = LerpFloat( point.currentScale, targetScale, dt * scaleSpeed );
			float effectiveRadius = baseRadius * point.currentScale;
			filled_circle_draw_object_t filledCircle{ screenPos, effectiveRadius, fillColor, 32 };
			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_filled_circle, std::make_any< filled_circle_draw_object_t >( filledCircle ) );
			unsigned int grayColor = c_color( 128, 128, 128, 255 ).get_u32( );
			const int segments     = 32;
			const float angleStep  = 2.0f * 3.14159f / segments;
			for ( int j = 0; j < segments; ++j ) {
				float angle1       = j * angleStep;
				float angle2       = ( j + 1 ) * angleStep;
				bool isGraySegment = ( angle1 >= 0.79f && angle1 <= 2.36f ) || ( angle2 >= 0.79f && angle2 <= 2.36f );
				c_vector_2d pos1, pos2;
				pos1.m_x                  = screenPos.m_x + cos( angle1 ) * effectiveRadius;
				pos1.m_y                  = screenPos.m_y + sin( angle1 ) * effectiveRadius;
				pos2.m_x                  = screenPos.m_x + cos( angle2 ) * effectiveRadius;
				pos2.m_y                  = screenPos.m_y + sin( angle2 ) * effectiveRadius;
				unsigned int segmentColor = isGraySegment ? grayColor : outlineColor;
				line_draw_object_t outlineLine{ pos1, pos2, segmentColor, outlineThickness };
				g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( outlineLine ) );
			}
			if ( isHovered && point.show_3d_radius ) {
				float display_radius      = point.radius;
				const int circle_segments = 64;
				const float angle_step    = 2.0f * 3.14159f / circle_segments;
				for ( int j = 0; j < circle_segments; ++j ) {
					float angle1  = j * angle_step;
					float angle2  = ( j + 1 ) * angle_step;
					c_vector pos1 = point.pos + c_vector( cos( angle1 ) * display_radius, sin( angle1 ) * display_radius, 0.f );
					c_vector pos2 = point.pos + c_vector( cos( angle2 ) * display_radius, sin( angle2 ) * display_radius, 0.f );
					c_vector_2d screen_pos1, screen_pos2;
					if ( g_render.world_to_screen( pos1, screen_pos1 ) && g_render.world_to_screen( pos2, screen_pos2 ) ) {
						unsigned int white_color = c_color( 255, 255, 255, 200 ).get_u32( );
						line_draw_object_t radius_line{ screen_pos1, screen_pos2, white_color, 1.0f };
						g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( radius_line ) );
					}
				}
				if ( point.use_custom_first_jump_radius ) {
					float first_jump_radius = point.first_jump_radius;
					for ( int j = 0; j < circle_segments; ++j ) {
						float angle1  = j * angle_step;
						float angle2  = ( j + 1 ) * angle_step;
						c_vector pos1 = point.pos + c_vector( cos( angle1 ) * first_jump_radius, sin( angle1 ) * first_jump_radius, 0.f );
						c_vector pos2 = point.pos + c_vector( cos( angle2 ) * first_jump_radius, sin( angle2 ) * first_jump_radius, 0.f );
						c_vector_2d screen_pos1, screen_pos2;
						if ( g_render.world_to_screen( pos1, screen_pos1 ) && g_render.world_to_screen( pos2, screen_pos2 ) ) {
							unsigned int green_color = c_color( 0, 255, 0, 200 ).get_u32( );
							line_draw_object_t radius_line{ screen_pos1, screen_pos2, green_color, 1.0f };
							g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( radius_line ) );
						}
					}
				}
				if ( point.enable_restricted_binds && point.selected_restricted_bind != 0 ) {
					float restricted_radius = point.restricted_binds_radius;
					for ( int j = 0; j < circle_segments; ++j ) {
						float angle1  = j * angle_step;
						float angle2  = ( j + 1 ) * angle_step;
						c_vector pos1 = point.pos + c_vector( cos( angle1 ) * restricted_radius, sin( angle1 ) * restricted_radius, 0.f );
						c_vector pos2 = point.pos + c_vector( cos( angle2 ) * restricted_radius, sin( angle2 ) * restricted_radius, 0.f );
						c_vector_2d screen_pos1, screen_pos2;
						if ( g_render.world_to_screen( pos1, screen_pos1 ) && g_render.world_to_screen( pos2, screen_pos2 ) ) {
							unsigned int red_color = c_color( 255, 0, 0, 200 ).get_u32( );
							line_draw_object_t radius_line{ screen_pos1, screen_pos2, red_color, 1.0f };
							g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( radius_line ) );
						}
					}
				}
			}
			if ( isHovered && isBackspacePressed ) {
				m_bounce_points_check.erase( m_bounce_points_check.begin( ) + i );
				break;
			}
		}
	}
}
void n_movement::impl_t::on_paint_traverse( )
{
	can = true;
	this->pixel_finder( nullptr );
	this->pixelsurf_assist_render( );
	this->pixel_calc_render( );

	if ( GET_VARIABLE( g_variables.m_pixel_surf_line_render, bool ) && !m_pixelsurf_data.m_trajectory_segments.empty( ) ) {
		const bool show_only_active    = GET_VARIABLE( g_variables.m_pixel_surf_line_show_only_active, bool );
		const bool is_pixelsurf_active = m_pixelsurf_data.m_predicted_succesful || m_pixelsurf_data.m_in_pixel_surf;

		// FIX: show_only_active больше НЕ скрывает сохранённые сегменты —
		// они должны оставаться видимыми после слезания с сюрфа (это карта
		// solution'ов, а не индикатор активности). Флаг теперь влияет только
		// на live-рендер full_trajectory ниже.
		{
			const c_color line_color      = GET_VARIABLE( g_variables.m_pixel_surf_line_color, c_color );
			const float thickness         = GET_VARIABLE( g_variables.m_pixel_surf_line_thickness, float );
			const std::string current_map = g_interfaces.m_engine_client->get_level_name_short( );

			if ( m_pixelsurf_data.m_delete_mode && ImGui::IsMouseClicked( 0 ) && !ImGui::GetIO( ).WantCaptureMouse ) {
				ImVec2 mouse_pos    = ImGui::GetMousePos( );
				int closest_segment = -1;
				float min_dist      = 15.0f;

				for ( size_t seg_idx = 0; seg_idx < m_pixelsurf_data.m_trajectory_segments.size( ); ++seg_idx ) {
					const auto& segment = m_pixelsurf_data.m_trajectory_segments[ seg_idx ];
					if ( segment.map_name != current_map || segment.points.size( ) < 2 )
						continue;

					for ( size_t i = 1; i < segment.points.size( ); ++i ) {
						c_vector_2d screen_start, screen_end;
						if ( g_render.world_to_screen( segment.points[ i - 1 ], screen_start ) &&
						     g_render.world_to_screen( segment.points[ i ], screen_end ) ) {
							float dist = point_to_line_distance( c_vector_2d( mouse_pos.x, mouse_pos.y ), screen_start, screen_end );
							if ( dist < min_dist ) {
								min_dist        = dist;
								closest_segment = seg_idx;
							}
						}
					}
				}

				if ( closest_segment >= 0 ) {
					m_pixelsurf_data.m_trajectory_segments.erase( m_pixelsurf_data.m_trajectory_segments.begin( ) + closest_segment );
					m_pixelsurf_data.m_trajectory_points.clear( );
					for ( const auto& seg : m_pixelsurf_data.m_trajectory_segments ) {
						for ( const auto& pt : seg.points ) {
							m_pixelsurf_data.m_trajectory_points.push_back( pt );
						}
					}
				}
			}

			for ( const auto& segment : m_pixelsurf_data.m_trajectory_segments ) {
				if ( segment.map_name != current_map || segment.points.size( ) < 2 )
					continue;

				for ( size_t i = 1; i < segment.points.size( ); ++i ) {
					c_vector_2d screen_start, screen_end;
					if ( g_render.world_to_screen( segment.points[ i - 1 ], screen_start ) &&
					     g_render.world_to_screen( segment.points[ i ], screen_end ) ) {
						g_render.m_draw_data.emplace_back(
							e_draw_type::draw_type_line,
							std::make_any< line_draw_object_t >( screen_start, screen_end, line_color.get_u32( ), thickness ) );
					}
				}
			}

			if ( GET_VARIABLE( g_variables.m_pixel_surf_line_full_trajectory, bool ) && is_pixelsurf_active && g_ctx.m_local &&
			     g_ctx.m_local->is_alive( ) ) {
				// FIX: pixelsurf solution живёт у фитов игрока, а не у груди.
				// Прежняя версия трассировала из mid-body по направлению velocity,
				// но во время surf velocity направлен ПАРАЛЛЕЛЬНО стене (вдоль
				// слайда), а не В стену — поэтому луч улетал и попадал в чужую
				// стенку, а линия рисовалась на уровне головы.
				// Решение: anchor на feet-уровне + 360° сканирование вокруг
				// игрока, берём ближайшую стену как surf-стену.
				c_vector feet_pos = g_ctx.m_local->get_origin( );
				feet_pos.m_z += 4.0f; // лёгкий лифт чтобы не стартовать в полу

				c_vector horiz_dir = g_ctx.m_local->get_velocity( );
				horiz_dir.m_z      = 0.f;

				if ( horiz_dir.length_2d( ) > 0.1f ) {
					horiz_dir.normalize_in_place( );

					// 360° скан на коротком радиусе — surf-стена сбоку, не спереди.
					trace_t     initial{ };
					float       closest_dist = FLT_MAX;
					bool        found_wall   = false;
					constexpr int   k_scan_dirs   = 16;
					constexpr float k_scan_radius = 32.0f;
					constexpr float k_two_pi      = 6.28318530717958647692f;

					for ( int i = 0; i < k_scan_dirs; ++i ) {
						const float ang = static_cast< float >( i ) * ( k_two_pi / k_scan_dirs );
						c_vector dir( cosf( ang ), sinf( ang ), 0.f );

						trace_t        t{ };
						ray_t          ray( feet_pos, feet_pos + dir * k_scan_radius );
						c_trace_filter flt( g_ctx.m_local );
						g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &t );

						if ( t.m_fraction < 1.0f && !t.m_start_solid ) {
							const float d = t.m_fraction * k_scan_radius;
							if ( d < closest_dist ) {
								closest_dist = d;
								initial      = t;
								found_wall   = true;
							}
						}
					}

					if ( found_wall ) {
						const c_vector wall_normal = initial.m_plane.m_normal;

						// Tangent = horizontal movement direction projected onto the wall plane.
						c_vector tangent = horiz_dir - wall_normal * horiz_dir.dot_product( wall_normal );

						// Fallback: player pressing perpendicularly into wall → tangent ~0.
						// Use horizontal tangent along the wall face: cross(up, normal).
						if ( tangent.length( ) < 0.01f ) {
							tangent = c_vector( -wall_normal.m_y, wall_normal.m_x, 0.f );
							if ( tangent.dot_product( horiz_dir ) < 0.f )
								tangent = tangent * -1.0f;
						}

						if ( tangent.length( ) > 0.01f ) {
							tangent.normalize_in_place( );

							std::vector< c_vector > scan_points;
							scan_points.reserve( 48 );
							scan_points.push_back( initial.m_end );

							constexpr float k_step          = 16.0f;  // distance between probes
							constexpr int   k_max_steps     = 48;     // ~768 units of solution
							constexpr float k_probe_depth   = 48.0f;
							constexpr float k_normal_cohere = 0.8f;   // reject if wall turns too sharply

							c_vector probe_ref = initial.m_end;
							for ( int i = 0; i < k_max_steps; ++i ) {
								// step along the tangent on the wall surface
								probe_ref = probe_ref + tangent * k_step;

								// trace from "in front of" the wall back into it — finds the
								// continuation of the same face at this forward position
								c_vector probe_start = probe_ref + wall_normal * k_probe_depth;
								c_vector probe_end   = probe_ref - wall_normal * k_probe_depth;

								trace_t t{ };
								ray_t ray( probe_start, probe_end );
								c_trace_filter flt2( g_ctx.m_local );
								g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt2, &t );

								if ( t.m_fraction >= 1.0f || t.m_start_solid )
									break; // wall ended

								if ( t.m_plane.m_normal.dot_product( wall_normal ) < k_normal_cohere )
									break; // wall curved away / corner

								scan_points.push_back( t.m_end );

								// Re-anchor probe_ref to the actual contact so subsequent
								// tangent steps slide along the true surface (handles gently
								// curved / ramped walls).
								probe_ref = t.m_end + wall_normal * 0.5f;
							}

							for ( size_t i = 1; i < scan_points.size( ); ++i ) {
								c_vector_2d screen_start, screen_end;
								if ( g_render.world_to_screen( scan_points[ i - 1 ], screen_start ) &&
								     g_render.world_to_screen( scan_points[ i ], screen_end ) ) {
									g_render.m_draw_data.emplace_back(
										e_draw_type::draw_type_line,
										std::make_any< line_draw_object_t >( screen_start, screen_end, line_color.get_u32( ), thickness ) );
								}
							}
						}
					}
				}
			}
		}
	}

	if ( g_ctx.m_local )
		if ( g_interfaces.m_engine_client->is_in_game( ) )
			if ( g_ctx.m_local->is_alive( ) ) {
				RenderPoints( m_points_check, g_ctx.m_local->get_origin( ), g_interfaces.m_engine_client->get_level_name_short( ) );
			}
	can = false;
}
void n_movement::impl_t::on_create_move_post( )
{
	lester_kill_poll_audio_wallclock( );
	if ( !g_ctx.m_local )
		return;
	if ( g_ctx.m_local->get_observer_mode( ) > 0 )
		return;
	if ( g_ctx.m_local ) {
		if ( g_ctx.m_local->get_health( ) != 0 && g_ctx.m_local->is_alive( ) ) {
			const float target_ps_velocity =
				-g_convars[ HASH_BT( "sv_gravity" ) ]->get_float( ) * 0.5f * g_interfaces.m_global_vars_base->m_interval_per_tick;

			// Обновляем состояние пиксельсюрфа
			g_movement.m_pixelsurf_data.m_in_pixel_surf =
				is_pixelsurf_velocity( g_prediction.backup_data.m_velocity.m_z ) && is_pixelsurf_velocity( g_ctx.m_local->get_velocity( ).m_z );

			// ========================================== //
			// ДЕТЕКТ ПИКСЕЛЬСЮРФА (ЧАТ ПРИНТ)        //
			// ========================================== //
			{
				static bool was_in_pixelsurf = false;
				bool currently_in_pixelsurf  = g_movement.m_pixelsurf_data.m_in_pixel_surf;

				if ( currently_in_pixelsurf && !was_in_pixelsurf ) {
					if ( GET_VARIABLE( g_variables.m_detections, bool ) && GET_VARIABLE( g_variables.m_pixelsurf_detect, bool ) ) {
						if ( n_interfaces::chat_element ) {
							n_interfaces::chat_element->chatprintf(
								0, 0, make_kami_chat_message( GET_VARIABLE( g_variables.m_accent, c_color ), "pixelsurfed" ).c_str( ) );
						}
					}
				}
				was_in_pixelsurf = currently_in_pixelsurf;
			}
			// ========================================== //

			// ========================================== //
			// ДЕТЕКТ ЭДЖБАГА (ЧАТ ПРИНТ)              //
			// ========================================== //
			{
				if ( GET_VARIABLE( g_variables.edge_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) ) ) {
					if ( g_edgebug.m_found &&
					     g_edgebug.m_prediction_timestamp + g_edgebug.m_prediction_ticks == g_interfaces.m_global_vars_base->m_tick_count ) {
						if ( GET_VARIABLE( g_variables.m_detections, bool ) && GET_VARIABLE( g_variables.m_detect_edgebug, bool ) ) {
							if ( n_interfaces::chat_element ) {
								n_interfaces::chat_element->chatprintf(
									0, 0, make_kami_chat_message( GET_VARIABLE( g_variables.m_accent, c_color ), "edgebugged" ).c_str( ) );
							}
						}
					}
				}
			}
			// ========================================== //

			if ( GET_VARIABLE( g_variables.m_edge_jump, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_edge_jump_key, key_bind_t ) ) )
				this->edge_jump( );
			if ( GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) ||
			     AlertJB || AlertJB2 )
				this->jump_bug( );

			if ( GET_VARIABLE( g_variables.m_pixel_surf_line_render, bool ) && g_ctx.m_local ) {
				const bool now_in_surf = g_movement.m_pixelsurf_data.m_in_pixel_surf;
				c_vector current_pos   = g_ctx.m_local->get_origin( );

				if ( now_in_surf ) {
					// pixelsurf solution = ровно у фитов игрока (origin.z в CSGO = feet).
					// Прежний +4u оффсет давал визуальный сдвиг линии вверх над
					// реальным surf'ом. Луч из feet-уровня в горизонт корректен,
					// потому что во время surf игрок airborne (фиты не в полу).
					c_vector mid_pos = current_pos;

					c_vector wall_pos = mid_pos;
					c_vector velocity = g_ctx.m_local->get_velocity( );
					velocity.m_z      = 0.f;

					bool     surface_found = false;
					c_vector wall_normal{ };
					c_vector tangent{ };

					// --- Круговой скан вокруг игрока, чтобы найти РЕАЛЬНУЮ surf-стену.
					// Раньше луч пускался в направлении velocity, но на pixelsurf'е скорость
					// обычно параллельна стене (тангенс), а не к ней → луч улетал в другую
					// геометрию, wall_normal брался с неё, линия уходила "в сторону".
					// Теперь берём ближайшую почти-вертикальную плоскость, нормаль которой
					// смотрит на нас.
					constexpr int   k_angle_steps     = 32;
					// FIX: было 48u — без pixelsurf-assist игрок при входе в surf может
					// быть на 50-70u от стены (момент схватывания пикселя), 360°-scan
					// её не находил → линия вообще не строилась. 128u надёжно покрывает
					// все варианты подхода, всё ещё ловит ближайшую стену через best_fraction.
					constexpr float k_probe_dist      = 128.0f;
					constexpr float k_normal_z_eps    = 0.35f;
					constexpr float k_facing_thresh   = -0.1f;
					constexpr float k_two_pi          = 6.28318530717958647692f;

					float          best_fraction = 1.0f;
					c_trace_filter flt_scan( g_ctx.m_local );

					for ( int i = 0; i < k_angle_steps; ++i ) {
						const float    ang = static_cast< float >( i ) / static_cast< float >( k_angle_steps ) * k_two_pi;
						const c_vector dir( std::cos( ang ), std::sin( ang ), 0.f );
						const c_vector end_pos = mid_pos + dir * k_probe_dist;

						trace_t t{ };
						ray_t   ray( mid_pos, end_pos );
						g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt_scan, &t );

						if ( t.m_fraction >= 1.0f )
							continue;
						if ( std::fabs( t.m_plane.m_normal.m_z ) > k_normal_z_eps )
							continue;
						// Нормаль должна смотреть против направления луча — иначе это
						// тыльная грань brush'а.
						if ( t.m_plane.m_normal.m_x * dir.m_x + t.m_plane.m_normal.m_y * dir.m_y > k_facing_thresh )
							continue;
						if ( t.m_fraction >= best_fraction )
							continue;

						best_fraction = t.m_fraction;
						wall_pos      = t.m_end;
						wall_normal   = t.m_plane.m_normal;
						surface_found = true;
					}

					if ( surface_found ) {
						// Тангенс — проекция горизонтальной скорости на плоскость стены.
						tangent = velocity - wall_normal * velocity.dot_product( wall_normal );

						// Fallback: игрок упёрся почти перпендикулярно или стоит на месте →
						// проекция ~0. Берём горизонтальный тангенс стены через cross(up,n).
						if ( tangent.length( ) < 0.01f )
							tangent = c_vector( -wall_normal.m_y, wall_normal.m_x, 0.f );

						if ( tangent.length( ) > 0.01f ) {
							tangent.normalize_in_place( );
							// Ориентируем по направлению движения, если оно значимо.
							if ( velocity.length_2d( ) > 1.0f && tangent.dot_product( velocity ) < 0.f )
								tangent = tangent * -1.0f;
						} else {
							surface_found = false;
						}
					}

					// Хелпер: сканирует стену от wall_pos в направлении dir, на каждом
					// шаге запускает prediction-проверку (как в pixel_finder): ставит
					// игрока в кандидатную позицию, выполняет один tick prediction и
					// проверяет, что vel.z ≈ half_grav (сигнатура pixelsurf solution).
					// Линия обрывается там, где solution физически перестаёт работать.
					constexpr float k_test_al     = 15.97803f;
					constexpr float k_step        = 16.0f;
					constexpr int   k_max_steps   = 128;   // safety-cap (~2048u)
					constexpr float k_probe_depth = 48.0f;
					const float     k_half_grav   = get_half_gravity_per_tick( );
					const int       k_pred_frame  = g_interfaces.m_prediction->m_commands_predicted - 1;
					const float     k_surf_z      = current_pos.m_z;

					// Сохраняем состояние, которое мутирует prediction.
					const c_vector saved_origin   = g_ctx.m_local->get_origin( );
					const c_vector saved_abs      = g_ctx.m_local->get_abs_origin( );
					const int      saved_buttons  = g_ctx.m_cmd ? g_ctx.m_cmd->m_buttons : 0;
					const float    saved_fwd_move = g_ctx.m_cmd ? g_ctx.m_cmd->m_forward_move : 0.f;
					const float    saved_side_mv  = g_ctx.m_cmd ? g_ctx.m_cmd->m_side_move : 0.f;

					auto is_solution_at = [ & ]( const c_vector& candidate_origin ) -> bool {
						if ( !g_ctx.m_cmd )
							return false;
						g_prediction.restore_entity_to_predicted_frame( k_pred_frame );
						*( int* )( ( uintptr_t )( c_base_entity* )g_ctx.m_local + 0x25C ) = 2;
						g_ctx.m_cmd->m_buttons |= in_jump;
						g_ctx.m_cmd->m_buttons |= in_duck;
						const c_vector ang{ wall_normal.m_x * -1.f, wall_normal.m_y * -1.f, 0.f };
						const c_angle  to_wall  = ang.to_angle( );
						const float    rotation = deg2rad( to_wall.m_y - g_ctx.m_cmd->m_view_point.m_y );
						g_ctx.m_cmd->m_forward_move = cosf( rotation ) * 10.f;
						g_ctx.m_cmd->m_side_move    = -sinf( rotation ) * 10.f;

						g_ctx.m_local->get_origin( ) = candidate_origin;
						g_ctx.m_local->set_abs_origin( candidate_origin );

						g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
						g_prediction.end( g_ctx.m_local );

						// Толерантность: численный дрейф prediction между итерациями
						// (особенно после restore_entity_to_predicted_frame, который
						// не всегда чистит velocity/move-state полностью) даёт разброс
						// до 1-2u/tick. Free-fall дельта = |sv_gravity * tick| ≈ 12.5
						// при 64tick → 3.0 надёжно отделяет solution от не-solution.
						return std::abs( g_ctx.m_local->get_velocity( ).m_z - k_half_grav ) < 3.0f;
					};

					auto scan_along_wall = [ & ]( const c_vector& dir ) -> std::vector< c_vector > {
						std::vector< c_vector > out;
						c_vector probe_ref = wall_pos;
						c_vector last_pt   = wall_pos;
						for ( int i = 0; i < k_max_steps; ++i ) {
							probe_ref = probe_ref + dir * k_step;

							// Геометрический пробник: убеждаемся, что стена ещё есть в этой точке.
							c_vector probe_start = probe_ref + wall_normal * k_probe_depth;
							c_vector probe_end   = probe_ref - wall_normal * k_probe_depth;
							trace_t  t{ };
							ray_t    ray2( probe_start, probe_end );
							c_trace_filter flt2( g_ctx.m_local );
							g_interfaces.m_engine_trace->trace_ray( ray2, mask_playersolid, &flt2, &t );
							if ( t.m_fraction >= 1.0f || t.m_start_solid )
								break;
							// Ужесточённая coherence: 0.8 пропускала повороты до 37°,
							// чего хватает чтобы зацепить слегка наклонную соседнюю
							// плоскость. 0.95 = до ~18°, реально совпадает с одной стеной.
							if ( t.m_plane.m_normal.dot_product( wall_normal ) < 0.95f )
								break;

							const c_vector delta = t.m_end - last_pt;
							// FIX 1: 2D-jump (вдоль tangent) — защита от пропусков шагов.
							if ( delta.length_2d( ) > k_step * 1.5f )
								break;
							// FIX 2: КЛЮЧЕВОЙ — компонент скачка ВДОЛЬ wall_normal.
							// При корректном движении вдоль одной стены контактная
							// точка не должна смещаться в направлении нормали. Если
							// смещается > 1u — значит trace зацепил параллельную/
							// смещённую поверхность (другая часть geometry).
							const float normal_jump = std::fabs( delta.dot_product( wall_normal ) );
							if ( normal_jump > 1.0f )
								break;

							// FIX: sign-based offset как в pixel_finder. Раньше использовал
							// wall_normal * test_al → для диагональных стен (nx=ny=0.707)
							// давало offset 11.3u по каждой оси, а player half-width
							// по диагонали = 22.6u → центр оказывался ВНУТРИ стены,
							// prediction выталкивал игрока, velocity ломался → predict-check
							// фейлил → линия не строилась дальше первого шага.
							auto sign_or_zero = []( float v ) -> float {
								if ( v < -0.001f ) return -1.f;
								if ( v > 0.001f ) return 1.f;
								return 0.f;
							};
							c_vector candidate( t.m_end.m_x + sign_or_zero( wall_normal.m_x ) * k_test_al,
							                    t.m_end.m_y + sign_or_zero( wall_normal.m_y ) * k_test_al, k_surf_z );

							const bool predict_ok = is_solution_at( candidate );
							// FIX: на ПЕРВОМ шаге прерывать только при провале geometry,
							// игнорируя predict. Игрок прямо сейчас в surf, а predict
							// иногда фейлит из-за нестабильного backup_data на entry-tick
							// (у assist'а оно чище → линия рисовалась). Это даёт хотя бы
							// 2-точечный segment, чтобы линия была видна; дальше скан
							// продолжается с обычной predict-проверкой.
							if ( !predict_ok && i > 0 )
								break;

							out.push_back( t.m_end );
							last_pt   = t.m_end;
							probe_ref = t.m_end + wall_normal * 0.5f;

							if ( !predict_ok )
								break;
						}
						return out;
					};

					if ( !g_movement.m_pixelsurf_data.m_was_in_surf ) {
						// Вход в сюрф — сразу сохраняем ПОЛНУЮ длину solution:
						// скан в обе стороны от точки контакта.
						trajectory_segment_t new_segment;
						new_segment.map_name = g_interfaces.m_engine_client->get_level_name_short( );

						if ( surface_found ) {
							const auto forward  = scan_along_wall( tangent );
							const auto backward = scan_along_wall( tangent * -1.0f );

							// Восстанавливаем игровое состояние после prediction-сессии.
							g_prediction.restore_entity_to_predicted_frame( k_pred_frame );
							g_ctx.m_local->get_origin( ) = saved_origin;
							g_ctx.m_local->set_abs_origin( saved_abs );
							if ( g_ctx.m_cmd ) {
								g_ctx.m_cmd->m_buttons      = saved_buttons;
								g_ctx.m_cmd->m_forward_move = saved_fwd_move;
								g_ctx.m_cmd->m_side_move    = saved_side_mv;
							}

							new_segment.points.reserve( backward.size( ) + 1 + forward.size( ) );
							// reversed backward → initial contact → forward
							for ( auto it = backward.rbegin( ); it != backward.rend( ); ++it )
								new_segment.points.push_back( *it );
							new_segment.points.push_back( wall_pos );
							for ( const auto& p : forward )
								new_segment.points.push_back( p );
						} else {
							// fallback: не нашли стену — просто точка контакта/тела
							new_segment.points.push_back( wall_pos );
						}

						for ( const auto& p : new_segment.points )
							g_movement.m_pixelsurf_data.m_trajectory_points.push_back( p );

						g_movement.m_pixelsurf_data.m_trajectory_segments.push_back( new_segment );
						g_movement.m_pixelsurf_data.m_last_position = wall_pos;
					} else {
						// Уже в сюрфе — solution уже полностью сохранён при входе.
						// Ничего не добавляем: это избавляет от лишних точек и скачков на
						// изломах стены, и даёт гладкую полилинию ровно на длину solution.
						g_movement.m_pixelsurf_data.m_last_position = wall_pos;
					}
				}

				g_movement.m_pixelsurf_data.m_was_in_surf = now_in_surf;
			}

			{
				static bool prev_in_pixel_surf = false;
				static bool prev_lirili_key    = false;
				const bool now_ps              = g_movement.m_pixelsurf_data.m_in_pixel_surf;
				const bool lirili_key          = GET_VARIABLE( g_variables.m_lirili_larila, bool ) &&
				                        g_input.check_input( &GET_VARIABLE( g_variables.m_lirili_larila_key, key_bind_t ) );
				if ( lirili_key && !prev_lirili_key )
					g_interfaces.m_engine_client->execute_client_cmd( "y6_ast 1" );
				else if ( !lirili_key && prev_lirili_key )
					g_interfaces.m_engine_client->execute_client_cmd( "y6_ast 0" );
				if ( GET_VARIABLE( g_variables.m_lirili_larila, bool ) && now_ps && !prev_in_pixel_surf && lirili_key && g_ctx.m_local ) {
					m_lirili_larila_pending_origin = g_ctx.m_local->get_origin( );
					m_lirili_larila_pending_render = true;
				}
				prev_in_pixel_surf = now_ps;
				prev_lirili_key    = lirili_key;
			}
			const bool aoh_active             = m_auto_one_hop_data.m_phase != auto_one_hop_data_t::phase_idle;
			const bool ladder_freelook_active = m_ladder_freelook_climb_data.m_phase != ladder_freelook_climb_data_t::phase_idle;
			const auto move_type              = g_prediction.backup_data.m_move_type;
			if ( move_type != e_move_types::move_type_noclip && move_type != e_move_types::move_type_fly &&
			     move_type != e_move_types::move_type_observer ) {
				if ( !aoh_active && !ladder_freelook_active ) {
					if ( GET_VARIABLE( g_variables.m_long_jump, bool ) &&
					     g_input.check_input( &GET_VARIABLE( g_variables.m_long_jump_key, key_bind_t ) ) )
						this->long_jump( );
					if ( GET_VARIABLE( g_variables.m_mini_jump, bool ) &&
					     g_input.check_input( &GET_VARIABLE( g_variables.m_mini_jump_key, key_bind_t ) ) )
						this->mini_jump( );
					if ( GET_VARIABLE( g_variables.m_pixel_surf_fix, bool ) )
						this->pixel_surf_fix( );
					// fire_man вызывается раз за тик из on_create_move_pre.
					this->pixel_surf( target_ps_velocity );
					this->fire_man( g_ctx.m_cmd );
					this->auto_duck( );
				}

				
					this->movement_fix( g_ctx.old_view_point );
			}
			if ( !aoh_active && !ladder_freelook_active ) {
				this->anti_ladder( g_ctx.m_cmd );
			}
		}
	}
}
void n_movement::impl_t::edge_jump( )
{
	const auto move_type = g_prediction.backup_data.m_move_type;
	if ( move_type == e_move_types::move_type_noclip || move_type == e_move_types::move_type_fly || move_type == e_move_types::move_type_observer )
		return;
	static int saved_tick{ };
	if ( g_interfaces.m_global_vars_base->m_tick_count - saved_tick > 1 && g_interfaces.m_global_vars_base->m_tick_count - saved_tick < 15 ) {
		g_ctx.m_cmd->m_forward_move = 0.f;
		g_ctx.m_cmd->m_side_move    = 0.f;
		g_ctx.m_cmd->m_buttons      = g_ctx.m_cmd->m_buttons & ~( in_forward | in_back | in_moveleft | in_moveleft );
		g_ctx.m_cmd->m_buttons |= in_duck;
	}
	if ( move_type == e_move_types::move_type_ladder ) {
		const int pre = g_ctx.m_local->get_move_type( );
		g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
		g_prediction.end( g_ctx.m_local );
		const int post = g_ctx.m_local->get_move_type( );
		if ( pre == e_move_types::move_type_ladder ) {
			if ( post != e_move_types::move_type_ladder ) {
				saved_tick = g_interfaces.m_global_vars_base->m_tick_count;
				g_ctx.m_cmd->m_buttons |= in_jump;
				g_ctx.m_cmd->m_forward_move = 0.f;
				g_ctx.m_cmd->m_side_move    = 0.f;
				g_ctx.m_cmd->m_buttons      = g_ctx.m_cmd->m_buttons & ~( in_forward | in_back | in_moveleft | in_moveleft );
			}
		}
		if ( g_interfaces.m_global_vars_base->m_tick_count - saved_tick > 1 && g_interfaces.m_global_vars_base->m_tick_count - saved_tick < 15 ) {
			g_ctx.m_cmd->m_forward_move = 0.f;
			g_ctx.m_cmd->m_side_move    = 0.f;
			g_ctx.m_cmd->m_buttons      = g_ctx.m_cmd->m_buttons & ~( in_forward | in_back | in_moveleft | in_moveleft );
			g_ctx.m_cmd->m_buttons |= in_duck;
		}
		return;
	}
	if ( ( g_prediction.backup_data.m_flags & e_flags::fl_onground ) && !( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) ) {
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_jump;
		m_edge_jump_data.m_detected = true;
	} else {
		m_edge_jump_data.m_detected = false;
	}
}
bool n_movement::impl_t::get_closest_wall_normal( c_vector& out_normal )
{
	if ( !g_ctx.m_local )
		return false;

	const auto start_pos = g_ctx.m_local->get_abs_origin( );

	// the mins and the maxs....
	const auto mins = g_ctx.m_local->get_collideable( )->get_obb_mins( );
	const auto maxs = g_ctx.m_local->get_collideable( )->get_obb_maxs( );

	constexpr float step = ( std::numbers::pi_v< float > * 2.f ) / 16.f;

	for ( float angle = 0.f; angle < ( std::numbers::pi_v< float > * 2.f ); angle += step ) {
		c_vector direction( cosf( angle ), sinf( angle ), 0.f );
		c_vector end_pos = start_pos + ( direction * 32.f );

		trace_t trace;
		c_trace_filter filter( g_ctx.m_local );
		ray_t ray( start_pos, end_pos, mins, maxs );

		g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &filter, &trace );

		if ( trace.m_fraction < 1.0f ) {
			out_normal = trace.m_plane.m_normal;
			return true;
		}
	}
	return false;
}

void n_movement::impl_t::air_stuck_serverside( c_user_cmd* cmd )
{
	static bool prev_key_pressed = false;
	static bool air_stuck_active = false;

	if ( !GET_VARIABLE( g_variables.m_air_stuck, bool ) ) {
		if ( air_stuck_active ) {
			g_interfaces.m_engine_client->execute_client_cmd( "sm_noclip @me" );
			air_stuck_active = false;
		}
		m_air_stuck_data.reset( );
		prev_key_pressed = false;
		return;
	}

	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) || !cmd ) {
		if ( air_stuck_active ) {
			g_interfaces.m_engine_client->execute_client_cmd( "sm_noclip @me" );
			air_stuck_active = false;
		}
		prev_key_pressed = false;
		return;
	}

	auto& key_bind = GET_VARIABLE( g_variables.m_air_stuck_key, key_bind_t );

	bool key_pressed = false;
	if ( key_bind.m_key != 0 ) {
		key_pressed = g_input.check_input( &key_bind );
	}

	bool on_ground = g_ctx.m_local->get_flags( ) & e_flags::fl_onground;

	if ( key_pressed && !prev_key_pressed && !on_ground ) {
		g_interfaces.m_engine_client->execute_client_cmd( "sm_noclip @me" );
		air_stuck_active       = true;
		

		// Детект server-side аирстака
		if ( GET_VARIABLE( g_variables.m_detections, bool ) && GET_VARIABLE( g_variables.m_detect_air_stuck, bool ) ) {
			if ( n_interfaces::chat_element ) {
				n_interfaces::chat_element->chatprintf( 0, 0, make_kami_chat_message( GET_VARIABLE( g_variables.m_accent, c_color ), "airstucked" ).c_str( ) );
			}
		}
	} else if ( !key_pressed && prev_key_pressed && air_stuck_active ) {
		g_interfaces.m_engine_client->execute_client_cmd( "sm_noclip @me" );
		air_stuck_active = false;
		m_air_stuck_data.reset( );
	}

	if ( air_stuck_active ) {
		m_air_stuck_data.reset( );
		cmd->m_forward_move = 0.f;
		cmd->m_side_move    = 0.f;
		cmd->m_up_move      = 0.f;
		cmd->m_buttons &= ~( in_forward | in_back | in_moveleft | in_moveright | in_jump | in_duck );
	}

	prev_key_pressed = key_pressed;
}


void n_movement::impl_t::pixel_surf_fix( )
{
	if ( !g_ctx.m_local )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	if ( g_prediction.backup_data.m_velocity.m_z > 0.f )
		return;
	if ( g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) ) )
		return;
	if ( g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) ) )
		return;
	if ( g_prediction.backup_data.m_velocity.length_2d( ) >= 285.91f ) {
		if ( g_ctx.m_local->get_flags( ) & 1 ) {
			static auto sv_airAcelerate = g_interfaces.m_convar->find_var( "sv_airaccelerate" );
			if ( !sv_airAcelerate ) {
				return;
			}
			float Razn                  = ( ( g_prediction.backup_data.m_velocity.length_2d( ) + 2.f - 285.91f ) / 12.f * 64.f );
			c_vector velocity           = g_prediction.backup_data.m_velocity * -1.f;
			velocity.m_z                = 0.f;
			float rotation              = deg2rad( velocity.to_angle2( ).m_y - g_ctx.m_cmd->m_view_point.m_y );
			float cos_rot               = cos( rotation );
			float sin_rot               = sin( rotation );
			float forwardmove           = cos_rot * Razn;
			float sidemove              = -sin_rot * Razn;
			g_ctx.m_cmd->m_forward_move = forwardmove;
			g_ctx.m_cmd->m_side_move    = sidemove;
		}
	}
}
void n_movement::impl_t::long_jump( )
{
	static int saved_tick  = 0;
	const bool left_ground = ( g_prediction.backup_data.m_flags & e_flags::fl_onground ) && !( g_ctx.m_local->get_flags( ) & e_flags::fl_onground );
	const bool jumped_this_tick = ( g_ctx.m_cmd->m_buttons & e_command_buttons::in_jump ) != 0;
	if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
		saved_tick = g_interfaces.m_global_vars_base->m_tick_count;
	if ( !( g_interfaces.m_global_vars_base->m_tick_count - saved_tick > 2 ) && !( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) ) {
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
		m_long_jump_data.m_detected = left_ground || jumped_this_tick;
	} else {
		m_long_jump_data.m_detected = false;
	}
}

void n_movement::impl_t::mini_jump( )
{
	static bool should_duck = false;
	const bool left_ground  = ( g_prediction.backup_data.m_flags & e_flags::fl_onground ) && !( g_ctx.m_local->get_flags( ) & e_flags::fl_onground );
	const bool jumped_this_tick = ( g_ctx.m_cmd->m_buttons & e_command_buttons::in_jump ) != 0;
	if ( g_prediction.backup_data.m_flags && g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
		should_duck = false;
	if ( g_prediction.backup_data.m_flags & e_flags::fl_onground && !( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) ) {
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_jump;
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
		m_mini_jump_data.m_detected = left_ground || jumped_this_tick;
		if ( GET_VARIABLE( g_variables.m_mini_jump_hold_duck, bool ) )
			should_duck = true;
	} else {
		m_mini_jump_data.m_detected = false;
	}
	if ( should_duck )
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
}
void n_movement::impl_t::auto_duck( )
{
	if ( !GET_VARIABLE( g_variables.m_auto_duck, bool ) || g_prediction.backup_data.m_flags & e_flags::fl_onground ||
	     g_movement.m_pixelsurf_data.m_in_pixel_surf ||
	     ( GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) ) ||
	     ( GET_VARIABLE( g_variables.m_edge_jump, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_edge_jump_key, key_bind_t ) ) ) ) {
		g_movement.m_autoduck_data.reset( );
		return;
	}
	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
	for ( int i = 0; i < 2; i++ ) {
		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
			break;
		c_user_cmd* simulated_cmd = new c_user_cmd( *g_ctx.m_cmd );
		simulated_cmd->m_buttons |= e_command_buttons::in_bullrush;
		simulated_cmd->m_buttons |= e_command_buttons::in_duck;
		g_prediction.begin( g_ctx.m_local, simulated_cmd );
		g_prediction.end( g_ctx.m_local );
		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
			g_movement.m_autoduck_data.m_did_land_ducking = true;
			g_movement.m_autoduck_data.m_ducking_vert     = g_ctx.m_local->get_abs_origin( ).m_z;
			break;
		}
		delete simulated_cmd;
	}
	g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
	g_prediction.end( g_ctx.m_local );
	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
	if ( !g_movement.m_autoduck_data.m_did_land_ducking )
		return;
	for ( int i = 0; i < 2; i++ ) {
		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
			break;
		c_user_cmd* simulated_cmd = new c_user_cmd( *g_ctx.m_cmd );
		simulated_cmd->m_buttons &= ~e_command_buttons::in_duck;
		g_prediction.begin( g_ctx.m_local, simulated_cmd );
		g_prediction.end( g_ctx.m_local );
		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
			g_movement.m_autoduck_data.m_did_land_standing = true;
			g_movement.m_autoduck_data.m_standing_vert     = g_ctx.m_local->get_abs_origin( ).m_z;
			break;
		}
		delete simulated_cmd;
	}
	g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
	g_prediction.end( g_ctx.m_local );
	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
	if ( g_movement.m_autoduck_data.m_did_land_ducking && g_movement.m_autoduck_data.m_did_land_standing ) {
		if ( g_movement.m_autoduck_data.m_ducking_vert > g_movement.m_autoduck_data.m_standing_vert )
			g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
	} else if ( g_movement.m_autoduck_data.m_did_land_ducking && !g_movement.m_autoduck_data.m_did_land_standing )
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
}
void n_movement::impl_t::pixel_surf_locking( float target_ps_velocity )
{
	if ( g_movement.m_pixelsurf_data.m_in_pixel_surf ) {
		if ( g_movement.m_pixelsurf_data.m_should_duck )
			g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
		else
			g_ctx.m_cmd->m_buttons &= ~e_command_buttons::in_duck;
		if ( g_prediction.backup_data.m_velocity.m_z == -6.25f ) {
			if ( b_buttons & in_duck )
				g_ctx.m_cmd->m_buttons |= in_duck;
		}
		if ( g_movement.m_pixelsurf_data.m_predicted_succesful && !( g_ctx.m_local->get_velocity( ).m_z == target_ps_velocity ) ) {
			g_movement.m_pixelsurf_data.m_in_pixel_surf       = false;
			g_movement.m_pixelsurf_data.m_predicted_succesful = false;
			return;
		}
		return;
	}
}
bool wall_detected = false;
void n_movement::impl_t::pixel_surf( float target_ps_velocity )
{
	if ( !( GET_VARIABLE( g_variables.m_pixel_surf, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ) ) ) ) {
		return;
	}
	{
		static int ticks = 0;
		if ( !g_ctx.m_local )
			return;
		if ( !g_ctx.m_local->is_alive( ) ) {
			ticks = 0;
			return;
		}
		if ( g_ctx.m_local->get_move_type( ) == move_type_noclip || g_ctx.m_local->get_move_type( ) == move_type_ladder )
			return;
		if ( g_ctx.m_local->get_flags( ) & 1 )
			return;
		if ( !wall_detected )
			return;
		if ( !m_pixelsurf_data.should_pixel_surf ) {
			int BackupButtons = g_ctx.m_cmd->m_buttons;
			for ( int i = 0; i < 2; i++ ) {
				g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
				if ( i == 0 )
					g_ctx.m_cmd->m_buttons &= ~in_duck;
				else
					g_ctx.m_cmd->m_buttons |= in_duck;
				for ( int z = 0; z < 8; z++ ) {
					float un_pred_velo = g_ctx.m_local->get_velocity( ).m_z;
					g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
					g_prediction.end( g_ctx.m_local );
					if ( g_ctx.m_local->get_flags( ) & 1 ) {
						break;
					}
					float zVelo           = g_ctx.m_local->get_velocity( ).m_z;
					const float half_grav = -( ( g_convars[ HASH_BT( "sv_gravity" ) ] ? g_convars[ HASH_BT( "sv_gravity" ) ]->get_float( ) : 800.f ) *
					                           g_interfaces.m_global_vars_base->m_interval_per_tick * 0.5f );
					m_pixelsurf_data.should_pixel_surf = std::abs( zVelo - half_grav ) < 0.01f;
					if ( m_pixelsurf_data.should_pixel_surf && i == 0 ) {
						m_pixelsurf_data.should_pixel_surf = false;
						g_ctx.m_cmd->m_buttons             = BackupButtons;
						return;
					}
					if ( m_pixelsurf_data.should_pixel_surf ) {
						ticks         = g_ctx.m_cmd->m_tick_count + z + 16;
						BackupButtons = g_ctx.m_cmd->m_buttons;
						break;
					}
				}
			}
			g_ctx.m_cmd->m_buttons = BackupButtons;
			g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
		} else {
			g_ctx.m_cmd->m_buttons |= in_duck;
			if ( g_ctx.m_cmd->m_tick_count > ticks ) {
				const float half_grav = -( ( g_convars[ HASH_BT( "sv_gravity" ) ] ? g_convars[ HASH_BT( "sv_gravity" ) ]->get_float( ) : 800.f ) *
				                           g_interfaces.m_global_vars_base->m_interval_per_tick * 0.5f );
				if ( std::abs( g_prediction.backup_data.m_velocity.m_z - half_grav ) >= 0.01f ) {
					m_pixelsurf_data.should_pixel_surf = false;
				}
			}
		}
	}
}

void n_movement::impl_t::jump_bug( )
{
	if ( g_prediction.backup_data.m_flags & 1 )
		g_movement.m_jumpbug_data.m_can_jb = false;
	static float btime;
	if ( btime < g_interfaces.m_global_vars_base->m_current_time )
		g_movement.m_jumpbug_data.m_can_jb = false;
	if ( ( GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_ctx.m_local->get_velocity( ).m_z > g_prediction.backup_data.m_velocity.m_z &&
	       !( g_prediction.backup_data.m_flags & 1 ) && !( g_ctx.m_local->get_flags( ) & 1 ) && g_ctx.m_local->get_move_type( ) != move_type_ladder &&
	       g_ctx.m_local->get_move_type( ) != move_type_noclip && g_ctx.m_local->get_move_type( ) != move_type_observer ) ) {
		btime                              = g_interfaces.m_global_vars_base->m_current_time + .2f;
		g_movement.m_jumpbug_data.m_can_jb = true;
	}
	[[unlikely]] if ( !( g_ctx.m_cmd->m_buttons & e_command_buttons::in_jump ) ) {
		static bool ducked = false;
		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground && !( g_prediction.backup_data.m_flags & e_flags::fl_onground ) && !ducked ) {
			g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
			ducked = true;
		} else
			ducked = false;
		if ( g_prediction.backup_data.m_flags & e_flags::fl_onground && ducked )
			ducked = false;
	} else {
		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground && !( g_prediction.backup_data.m_flags & e_flags::fl_onground ) )
			g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
			g_ctx.m_cmd->m_buttons &= ~e_command_buttons::in_jump;
		if ( !( g_ctx.m_local->get_flags( ) & fl_onground ) && g_prediction.backup_data.m_flags & fl_onground )
			g_ctx.m_cmd->m_buttons &= ~e_command_buttons::in_duck;
	}
}
void n_movement::impl_t::rotate_movement( c_user_cmd* cmd, c_angle& angle )
{
	if ( angle.m_x == 0 && angle.m_y == 0 && angle.m_z == 0 )
		g_interfaces.m_engine_client->get_view_angles( angle );
	c_vector vec_move    = c_vector( cmd->m_forward_move, cmd->m_side_move, 0.f );
	const float speed    = vec_move.length_2d( );
	const float rotation = deg2rad( cmd->m_view_point.m_y - angle.m_y );
	cmd->m_forward_move  = std::cosf( rotation ) * speed;
	cmd->m_side_move     = std::sinf( rotation ) * speed;
}
constexpr float RAY_LENGTH        = 6000.f;
constexpr float ALIGN_OFFSET      = 15.97803f;
constexpr float MOVE_SPEED        = 10.f;
constexpr float HEIGHT_OFFSET_NEG = -0.972092f;
constexpr float HEIGHT_OFFSET_POS = 0.0287018f;

inline float GetWallSupportDistance( c_base_entity* local, const c_vector& wallNormal )
{
	if ( !local )
		return ALIGN_OFFSET;

	auto* collideable = local->get_collideable( );
	if ( !collideable )
		return ALIGN_OFFSET;

	const c_vector mins = collideable->get_obb_mins( );
	const c_vector maxs = collideable->get_obb_maxs( );
	const c_vector half_extents( std::max( std::fabsf( mins.m_x ), std::fabsf( maxs.m_x ) ),
	                             std::max( std::fabsf( mins.m_y ), std::fabsf( maxs.m_y ) ),
	                             std::max( std::fabsf( mins.m_z ), std::fabsf( maxs.m_z ) ) );

	return std::fabsf( wallNormal.m_x ) * half_extents.m_x + std::fabsf( wallNormal.m_y ) * half_extents.m_y +
	       std::fabsf( wallNormal.m_z ) * half_extents.m_z;
}

c_vector GetOffsetFromWallNormal( const c_vector& wallNormal, float offset )
{
	float offsetX = ( wallNormal.m_x < 0 ? -offset : ( wallNormal.m_x > 0 ? offset : 0.f ) );
	float offsetY = ( wallNormal.m_y < 0 ? -offset : ( wallNormal.m_y > 0 ? offset : 0.f ) );
	return c_vector( offsetX, offsetY, 0.f );
}
struct AnimatedPoint {
	c_vector position;
	float animation_progress = 0.f;
	bool is_appearing        = true;
	bool is_removing         = false;
	float current_size       = 0.f;
};
std::vector< AnimatedPoint > animated_points;
void UpdateAndRenderPoints( std::vector< c_vector >& Points )
{
	float delta_time                   = ImGui::GetIO( ).DeltaTime;
	const float appearance_duration    = 0.5f;
	const float disappearance_duration = 0.5f;
	const float size_animation_speed   = 5.0f;
	const float normal_size            = 9.f;
	const float in_crosshair_size      = 12.f;
	for ( auto& ap : animated_points ) {
		ap.is_removing = true;
	}
	for ( const auto& p : Points ) {
		auto it = std::find_if( animated_points.begin( ), animated_points.end( ), [ &p ]( const AnimatedPoint& ap ) { return ap.position == p; } );
		if ( it == animated_points.end( ) ) {
			animated_points.push_back( { p, 0.f, true, false, 0.f } );
		} else {
			it->is_removing = false;
		}
	}
	for ( auto& ap : animated_points ) {
		if ( ap.is_appearing ) {
			ap.animation_progress += delta_time / appearance_duration;
			if ( ap.animation_progress >= 1.f ) {
				ap.animation_progress = 1.f;
				ap.is_appearing       = false;
			}
		} else if ( ap.is_removing ) {
			ap.animation_progress -= delta_time / disappearance_duration;
			if ( ap.animation_progress <= 0.f ) {
				ap.animation_progress = 0.f;
			}
		} else {
			ap.animation_progress = 1.f;
		}
		c_vector_2d screenPos;
		bool in_crosshair = false;
		if ( g_render.world_to_screen( ap.position, screenPos ) ) {
			in_crosshair = InCrosshair( screenPos.m_x, screenPos.m_y, 9.f );
		}
		float target_size;
		if ( ap.is_removing ) {
			target_size = 0.f;
		} else {
			target_size = in_crosshair ? in_crosshair_size : normal_size;
			if ( ap.is_appearing ) {
				target_size *= ap.animation_progress;
			}
		}
		ap.current_size += ( target_size - ap.current_size ) * ( 1.f - std::exp( -size_animation_speed * delta_time ) );
	}
	animated_points.erase( std::remove_if( animated_points.begin( ), animated_points.end( ),
	                                       []( const AnimatedPoint& ap ) { return ap.is_removing && ap.animation_progress <= 0.f; } ),
	                       animated_points.end( ) );
	static int stop     = 0;
	bool point_selected = false;
	for ( const auto& ap : animated_points ) {
		c_vector_2d screenPos;
		if ( g_render.world_to_screen( ap.position, screenPos ) ) {
			bool in_crosshair                            = InCrosshair( screenPos.m_x, screenPos.m_y, 9.f );
			g_movement.m_pixelsurf_assist_t.in_crosshair = in_crosshair;
			float filled_alpha                           = ap.animation_progress * 100.f;
			float outline_alpha                          = ap.animation_progress * 255.f;
			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_filled_circle,
			                                   std::make_any< filled_circle_draw_object_t >(
												   screenPos, ap.current_size, c_color( 0, 0, 0, static_cast< int >( filled_alpha ) ).get_u32( ) ) );
			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_circle,
				std::make_any< circle_draw_object_t >( screenPos, ap.current_size,
			                                           c_color( 0, 255, 0, static_cast< int >( outline_alpha ) ).get_u32( ) ) );
			if ( !point_selected && in_crosshair && g_input.check_input( &GET_VARIABLE( g_variables.m_bounce_assist_point_key, key_bind_t ) ) ) {
				if ( stop < GetTickCount64( ) ) {
					m_points_check.emplace_back( ap.position, g_interfaces.m_engine_client->get_level_name_short( ) );
					g_movement.m_pixelsurf_assist_t.set_point = true;
					Points.clear( );
					point_selected = true;
					stop           = GetTickCount64( ) + 3000;
				}
			}
		}
	}
}
std::vector< AnimatedPoint > animated_points2;
c_vector round_pos2( c_vector point )
{
	if ( point.m_z < 0.f )
		return c_vector( point.m_x, point.m_y, ( int )point.m_z - 0.969644f );
	else
		return c_vector( point.m_x, point.m_y, ( int )point.m_z + 0.04f );
}
bool bounce_set_point = false;
void UpdateAndRenderPoints2( std::vector< c_vector >& Points )
{
	float delta_time                   = ImGui::GetIO( ).DeltaTime;
	const float appearance_duration    = 0.5f;
	const float disappearance_duration = 0.5f;
	const float size_animation_speed   = 5.0f;
	const float normal_size            = 9.f;
	const float in_crosshair_size      = 12.f;
	for ( auto& ap : animated_points2 ) {
		ap.is_removing = true;
	}
	for ( const auto& p : Points ) {
		auto it = std::find_if( animated_points2.begin( ), animated_points2.end( ), [ &p ]( const AnimatedPoint& ap ) { return ap.position == p; } );
		if ( it == animated_points2.end( ) ) {
			animated_points2.push_back( { p, 0.f, true, false, 0.f } );
		} else {
			it->is_removing = false;
		}
	}
	for ( auto& ap : animated_points2 ) {
		if ( ap.is_appearing ) {
			ap.animation_progress += delta_time / appearance_duration;
			if ( ap.animation_progress >= 1.f ) {
				ap.animation_progress = 1.f;
				ap.is_appearing       = false;
			}
		} else if ( ap.is_removing ) {
			ap.animation_progress -= delta_time / disappearance_duration;
			if ( ap.animation_progress <= 0.f ) {
				ap.animation_progress = 0.f;
			}
		} else {
			ap.animation_progress = 1.f;
		}
		c_vector_2d screenPos;
		bool in_crosshair = false;
		if ( g_render.world_to_screen( ap.position, screenPos ) ) {
			in_crosshair = InCrosshair( screenPos.m_x, screenPos.m_y, 9.f );
		}
		float target_size;
		if ( ap.is_removing ) {
			target_size = 0.f;
		} else {
			target_size = in_crosshair ? in_crosshair_size : normal_size;
			if ( ap.is_appearing ) {
				target_size *= ap.animation_progress;
			}
		}
		ap.current_size += ( target_size - ap.current_size ) * ( 1.f - std::exp( -size_animation_speed * delta_time ) );
	}
	animated_points2.erase( std::remove_if( animated_points2.begin( ), animated_points2.end( ),
	                                        []( const AnimatedPoint& ap ) { return ap.is_removing && ap.animation_progress <= 0.f; } ),
	                        animated_points2.end( ) );
	static int stop     = 0;
	bool point_selected = false;
	for ( const auto& ap : animated_points2 ) {
		c_vector_2d screenPos;
		if ( g_render.world_to_screen( ap.position, screenPos ) ) {
			bool in_crosshair                            = InCrosshair( screenPos.m_x, screenPos.m_y, 9.f );
			g_movement.m_pixelsurf_assist_t.in_crosshair = in_crosshair;
			float filled_alpha                           = ap.animation_progress * 100.f;
			float outline_alpha                          = ap.animation_progress * 255.f;
			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_filled_circle,
			                                   std::make_any< filled_circle_draw_object_t >(
												   screenPos, ap.current_size, c_color( 0, 0, 0, static_cast< int >( filled_alpha ) ).get_u32( ) ) );
			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_circle,
				std::make_any< circle_draw_object_t >( screenPos, ap.current_size,
			                                           c_color( 255, 0, 0, static_cast< int >( outline_alpha ) ).get_u32( ) ) );
			if ( !point_selected && in_crosshair && !g_movement.m_pixelsurf_assist_t.set_point &&
			     g_input.check_input( &GET_VARIABLE( g_variables.m_bounce_assist_point_key, key_bind_t ) ) ) {
				if ( stop < GetTickCount64( ) ) {
					m_bounce_points_check.emplace_back( round_pos2( c_vector( ap.position.m_x, ap.position.m_y, ap.position.m_z + 1.f ) ),
					                                    g_interfaces.m_engine_client->get_level_name_short( ) );
					bounce_set_point = true;
					Points.clear( );
					point_selected = true;
					stop           = GetTickCount64( ) + 3000;
				}
			}
		}
	}
}
void n_movement::impl_t::pixel_finder( c_user_cmd* cmd )
{
	static c_vector StartPos, EndPos{ };
	static c_vector ViewAngles{ };
	static c_vector pixelsurfPoint{ };
	static c_vector WallNormal{ };
	static bool redLine = false;
	static std::vector< c_vector > Points{ };
	static std::vector< c_vector > Point_bounce{ };
	int to_muchPoints           = 0;
	static bool is_displacement = false;

	// Динамический расчёт гравитации под любой тикрейт
	const float half_grav = get_half_gravity_per_tick( );

	if ( cmd != nullptr ) {
		ViewAngles             = c_vector( cmd->m_view_point.m_x, cmd->m_view_point.m_y, 0.f );
		static bool FirstPoint = FALSE;
		if ( g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_finder_key, key_bind_t ) ) ) {
			if ( !FirstPoint ) {
				if ( Points.size( ) > 0 )
					Points.clear( );
				if ( Point_bounce.size( ) > 0 )
					Point_bounce.clear( );
				const auto endPos = g_ctx.m_local->get_eye_position( ) + c_vector::fromAngle( ViewAngles ) * 6000;
				trace_t trace     = { };
				ray_t ray( g_ctx.m_local->get_eye_position( ), endPos );
				c_trace_filter flt( g_ctx.m_local );
				g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
				WallNormal      = trace.m_plane.m_normal;
				is_displacement = strstr( trace.surface.m_name, "displacement" );
				StartPos        = g_ctx.m_local->get_eye_position( ) + ( endPos - g_ctx.m_local->get_eye_position( ) ) * trace.m_fraction;
				EndPos          = g_ctx.m_local->get_eye_position( ) + ( endPos - g_ctx.m_local->get_eye_position( ) ) * trace.m_fraction;
				FirstPoint      = true;
			} else {
				const auto endPos = g_ctx.m_local->get_eye_position( ) + c_vector::fromAngle( ViewAngles ) * 6000;
				trace_t trace     = { };
				ray_t ray( g_ctx.m_local->get_eye_position( ), endPos );
				c_trace_filter flt( g_ctx.m_local );
				g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
				c_vector SecPlane = trace.m_plane.m_normal;
				EndPos            = c_vector(
                    StartPos.m_x, StartPos.m_y,
                    c_vector( g_ctx.m_local->get_eye_position( ) + ( endPos - g_ctx.m_local->get_eye_position( ) ) * trace.m_fraction ).m_z );
			}
		} else if ( g_input.check_input( &GET_VARIABLE( g_variables.m_free_point_key, key_bind_t ) ) ) {
			const auto endPos = g_ctx.m_local->get_eye_position( ) + c_vector::fromAngle( ViewAngles ) * 2000;
			trace_t trace;
			ray_t ray( g_ctx.m_local->get_eye_position( ), endPos );
			c_trace_filter flt( g_ctx.m_local );
			g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
			points_check_t free_point;
			free_point.pos =
				round_pos( c_vector( g_ctx.m_local->get_eye_position( ) + ( endPos - g_ctx.m_local->get_eye_position( ) ) * trace.m_fraction ) );
			free_point.map           = g_interfaces.m_engine_client->get_level_name_short( );
			free_point.is_free_point = true;
			free_point.point_type    = 0;
			m_points_check.emplace_back( free_point );
		} else if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_finder_key, key_bind_t ) ) && FirstPoint ) {
			if ( redLine ) {
				redLine    = false;
				FirstPoint = false;
				return;
			}
			c_vector origin      = g_ctx.m_local->get_origin( );
			int buttons          = cmd->m_buttons;
			float ForwardMove    = cmd->m_forward_move;
			float Lerp           = 0;
			int PredictionFrames = g_interfaces.m_prediction->m_commands_predicted - 1;
			float test_al        = 15.97803f;
			if ( is_displacement )
				test_al = 16.001f;
			auto colid = g_ctx.m_local->get_collideable( );
			for ( int v = 0; v < 4; v++ ) {
				g_prediction.restore_entity_to_predicted_frame( PredictionFrames );
				bool done_and_break = false;
				for ( int i = 0; i < 10000; i++ ) {
					float align                                                       = 15.97803f;
					*( int* )( ( uintptr_t )( c_base_entity* )g_ctx.m_local + 0x25C ) = 2;
					cmd->m_buttons |= in_jump;
					cmd->m_buttons |= in_duck;
					c_vector Angles{ WallNormal.m_x * -1.f, WallNormal.m_y * -1.f, 0.f };
					c_angle to_wall     = Angles.to_angle( );
					float rotation      = deg2rad( to_wall.m_y - cmd->m_view_point.m_y );
					float cos_rot       = cos( rotation );
					float sin_rot       = sin( rotation );
					float forwardmove   = cos_rot * 10.f;
					float sidemove      = -sin_rot * 10.f;
					cmd->m_forward_move = forwardmove;
					cmd->m_side_move    = sidemove;
					if ( g_ctx.m_local->get_velocity( ).m_z > 0 ) {
						static float Height = 0.f;
						if ( StartPos.m_z > EndPos.m_z ) {
							if ( StartPos.m_z < 0.f )
								Height = ( int )StartPos.m_z - 0.02f - Lerp - 54.f;
							else
								Height = ( int )StartPos.m_z + 0.02f + Lerp - 54.f;
						} else {
							if ( EndPos.m_z < 0.f )
								Height = ( int )EndPos.m_z - 0.02f - Lerp - 54.f;
							else
								Height = ( int )EndPos.m_z + 0.02f + Lerp - 54.f;
						}
						int maxLerp;
						if ( StartPos.m_z > EndPos.m_z ) {
							maxLerp = StartPos.m_z - EndPos.m_z;
						} else {
							maxLerp = EndPos.m_z - StartPos.m_z;
						}
						if ( Lerp > maxLerp ) {
							cmd->m_buttons      = buttons;
							cmd->m_forward_move = ForwardMove;
							FirstPoint          = false;
							done_and_break      = true;
							break;
						} else {
							Lerp += 1.f;
						}
						if ( WallNormal.m_x < 0 && WallNormal.m_y < 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y - test_al, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x < 0 && WallNormal.m_y > 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y + test_al, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x > 0 && WallNormal.m_y < 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y - test_al, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x > 0 && WallNormal.m_y > 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y + test_al, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x == 0.f && WallNormal.m_y > 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x, StartPos.m_y + test_al, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x == 0.f && WallNormal.m_y < 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x, StartPos.m_y - test_al, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x < 0 && WallNormal.m_y == 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x > 0 && WallNormal.m_y == 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						}
						trace_t trace;
						c_trace_filter flt( g_ctx.m_local );
						ray_t ray(
							c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y, g_ctx.m_local->get_origin( ).m_z + 54.f ),
							c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y,
						              g_ctx.m_local->get_origin( ).m_z - 1000.f ) );
						g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
						c_vector down_point = trace.m_end;
						trace_t trace2;
						c_trace_filter flt2( g_ctx.m_local );
						ray_t ray2(
							c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y, g_ctx.m_local->get_origin( ).m_z + 54.f ),
							c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y,
						              g_ctx.m_local->get_origin( ).m_z + 1000.f ) );
						g_interfaces.m_engine_trace->trace_ray( ray2, mask_playersolid, &flt2, &trace2 );
						c_vector upper_point = trace2.m_end;
						if ( g_ctx.m_local->get_origin( ).m_z + 1.f < down_point.m_z )
							continue;
						if ( g_ctx.m_local->get_origin( ).m_z + 1.f > upper_point.m_z )
							continue;
					}
					c_vector un_pred_velo = g_ctx.m_local->get_velocity( );
					g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
					g_prediction.end( g_ctx.m_local );
					c_vector pred_velo = g_ctx.m_local->get_velocity( );
					{
						if ( std::abs( g_ctx.m_local->get_velocity( ).m_z - half_grav ) < 0.01f ) {
							if ( WallNormal.m_x < 0 && WallNormal.m_y < 0.f ) {
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x + test_al,
								                                     g_ctx.m_local->get_origin( ).m_y + test_al,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x < 0 && WallNormal.m_y > 0.f ) {
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x + test_al,
								                                     g_ctx.m_local->get_origin( ).m_y - test_al,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x > 0 && WallNormal.m_y < 0.f ) {
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x - test_al,
								                                     g_ctx.m_local->get_origin( ).m_y + test_al,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x > 0 && WallNormal.m_y > 0.f ) {
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x - test_al,
								                                     g_ctx.m_local->get_origin( ).m_y - test_al,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x == 0.f && WallNormal.m_y > 0.f ) {
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y - test_al,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x == 0.f && WallNormal.m_y < 0.f ) {
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y + test_al,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x < 0 && WallNormal.m_y == 0.f ) {
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x + test_al, g_ctx.m_local->get_origin( ).m_y,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x > 0 && WallNormal.m_y == 0.f ) {
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x - test_al, g_ctx.m_local->get_origin( ).m_y,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							}
						}
					}
					if ( un_pred_velo.m_z < 0.f && pred_velo.m_z < 0.f )
						break;
				}
				if ( done_and_break )
					break;
			}
			cmd->m_buttons      = buttons;
			cmd->m_forward_move = ForwardMove;
			g_prediction.restore_entity_to_predicted_frame( PredictionFrames );
			Lerp = 0.f;
			for ( int i = 0; i < 10000; i++ ) {
				float align                                                       = 15.97803f;
				*( int* )( ( uintptr_t )( c_base_entity* )g_ctx.m_local + 0x25C ) = 2;
				cmd->m_buttons |= in_jump;
				cmd->m_buttons |= in_duck;
				c_vector Angles{ WallNormal.m_x * -1.f, WallNormal.m_y * -1.f, 0.f };
				c_angle to_wall     = Angles.to_angle( );
				float rotation      = deg2rad( to_wall.m_y - cmd->m_view_point.m_y );
				float cos_rot       = cos( rotation );
				float sin_rot       = sin( rotation );
				float forwardmove   = cos_rot * 10.f;
				float sidemove      = -sin_rot * 10.f;
				cmd->m_forward_move = forwardmove;
				cmd->m_side_move    = sidemove;
				if ( g_ctx.m_local->get_velocity( ).m_z < 0 ) {
					static float Height = 0.f;
					if ( StartPos.m_z > EndPos.m_z ) {
						if ( StartPos.m_z < 0.f )
							Height = ( int )StartPos.m_z - 0.972092f - Lerp;
						else
							Height = ( int )StartPos.m_z + 0.0287018f - Lerp;
					} else {
						if ( EndPos.m_z < 0.f )
							Height = ( int )EndPos.m_z - 0.972092f - Lerp;
						else
							Height = ( int )EndPos.m_z + 0.0287018f - Lerp;
					}
					int maxLerp;
					if ( StartPos.m_z > EndPos.m_z ) {
						maxLerp = StartPos.m_z - EndPos.m_z;
					} else {
						maxLerp = EndPos.m_z - StartPos.m_z;
					}
					if ( Lerp > maxLerp ) {
						cmd->m_buttons      = buttons;
						cmd->m_forward_move = ForwardMove;
						FirstPoint          = false;
						break;
					} else {
						Lerp += 1.f;
					}
					if ( WallNormal.m_x < 0 && WallNormal.m_y < 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y - test_al, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x < 0 && WallNormal.m_y > 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y + test_al, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x > 0 && WallNormal.m_y < 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y - test_al, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x > 0 && WallNormal.m_y > 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y + test_al, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x == 0.f && WallNormal.m_y > 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x, StartPos.m_y + test_al, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x == 0.f && WallNormal.m_y < 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x, StartPos.m_y - test_al, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x < 0 && WallNormal.m_y == 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x > 0 && WallNormal.m_y == 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					}
					trace_t trace;
					c_trace_filter flt( g_ctx.m_local );
					ray_t ray(
						c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y, g_ctx.m_local->get_origin( ).m_z + 54.f ),
						c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y, g_ctx.m_local->get_origin( ).m_z - 1000.f ) );
					g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
					c_vector down_point = trace.m_end;
					trace_t trace2;
					c_trace_filter flt2( g_ctx.m_local );
					ray_t ray2(
						c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y, g_ctx.m_local->get_origin( ).m_z + 54.f ),
						c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y, g_ctx.m_local->get_origin( ).m_z + 1000.f ) );
					g_interfaces.m_engine_trace->trace_ray( ray2, mask_playersolid, &flt2, &trace2 );
					c_vector upper_point = trace2.m_end;
					if ( g_ctx.m_local->get_origin( ).m_z + 1.f < down_point.m_z )
						continue;
					if ( g_ctx.m_local->get_origin( ).m_z + 1.f > upper_point.m_z )
						continue;
				}
				c_vector un_pred_velo = g_ctx.m_local->get_velocity( );
				g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
				g_prediction.end( g_ctx.m_local );
				c_vector pred_velo = g_ctx.m_local->get_velocity( );

				if ( std::abs( g_ctx.m_local->get_velocity( ).m_z - half_grav ) < 0.01f ) {
					if ( WallNormal.m_x < 0 && WallNormal.m_y < 0.f ) {
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x + test_al, g_ctx.m_local->get_origin( ).m_y + test_al,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x < 0 && WallNormal.m_y > 0.f ) {
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x + test_al, g_ctx.m_local->get_origin( ).m_y - test_al,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x > 0 && WallNormal.m_y < 0.f ) {
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x - test_al, g_ctx.m_local->get_origin( ).m_y + test_al,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x > 0 && WallNormal.m_y > 0.f ) {
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x - test_al, g_ctx.m_local->get_origin( ).m_y - test_al,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x == 0.f && WallNormal.m_y > 0.f ) {
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y - test_al,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x == 0.f && WallNormal.m_y < 0.f ) {
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y + test_al,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x < 0 && WallNormal.m_y == 0.f ) {
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x + test_al, g_ctx.m_local->get_origin( ).m_y,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x > 0 && WallNormal.m_y == 0.f ) {
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x - test_al, g_ctx.m_local->get_origin( ).m_y,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					}
				}
			}
			cmd->m_buttons      = buttons;
			cmd->m_forward_move = ForwardMove;
			FirstPoint          = false;
			if ( Points.size( ) > 0 ) {
				for ( int i = 0; i < Points.size( ) - 1; ) {
					if ( ( int )Points[ i ].m_z - ( int )Points[ i + 1 ].m_z == 1 ) {
						Points.erase( Points.begin( ) + i );
					} else {
						++i;
					}
				}
			}
			int pixelsurf_count = Points.size( );
			int bounce_count    = Point_bounce.size( );
			int total_points    = pixelsurf_count + bounce_count;
			if ( total_points == 0 ) {
				myWindowDetect.AddWindow( 3.0f, "no points found :(" );
			} else if ( total_points == 1 ) {
				if ( pixelsurf_count == 1 ) {
					myWindowDetect.AddWindow( 3.0f, "1 points found: 1 pixelsurf" );
				} else {
					myWindowDetect.AddWindow( 3.0f, "1 points found: 1 headbounce" );
				}
			} else {
				std::string msg = std::to_string( total_points ) + " points found: ";
				if ( pixelsurf_count > 0 ) {
					msg += std::to_string( pixelsurf_count ) + " pixelsurf";
				}
				if ( bounce_count > 0 ) {
					if ( pixelsurf_count > 0 ) {
						msg += " and ";
					}
					msg += std::to_string( bounce_count ) + " headbounce";
				}
				myWindowDetect.AddWindow( 3.0f, msg );
			}
		}
	} else {
		UpdateAndRenderPoints( Points );
		UpdateAndRenderPoints2( Point_bounce );
		if ( g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_finder_key, key_bind_t ) ) ) {
			if ( !StartPos.is_zero( ) && !EndPos.is_zero( ) ) {
				c_vector_2d start2D, end2D;
				if ( g_render.world_to_screen( StartPos, start2D ) && g_render.world_to_screen( EndPos, end2D ) ) {
					c_color accent          = GET_VARIABLE( g_variables.m_accent, c_color );
					float outline_thickness = 3.f;
					c_color outline_color   = c_color( 0.f, 0.f, 0.f, 0.5f );
					float s                 = 10.f;
					float speed             = 50.f;
					float l                 = start2D.distance( end2D );
					if ( l > 0.f ) {
						static float offset = 0.f;
						offset              = fmod( offset + speed * ImGui::GetIO( ).DeltaTime, 2 * s );
						g_render.m_draw_data.emplace_back(
							e_draw_type::draw_type_line,
							std::make_any< line_draw_object_t >( start2D, end2D, outline_color.get_u32( ), outline_thickness ) );
						std::vector< float > transitions;
						transitions.push_back( 0.f );
						int m_start = static_cast< int >( ceil( offset / s ) );
						for ( int m = m_start;; m++ ) {
							float d = -offset + m * s;
							if ( d > l )
								break;
							float u = d / l;
							if ( u >= 0.f && u <= 1.f ) {
								transitions.push_back( u );
							}
						}
						transitions.push_back( 1.f );
						for ( size_t i = 0; i < transitions.size( ) - 1; i++ ) {
							float u0 = transitions[ i ];
							float u1 = transitions[ i + 1 ];
							if ( u1 > u0 ) {
								c_vector_2d p0 = start2D + ( end2D - start2D ) * u0;
								c_vector_2d p1 = start2D + ( end2D - start2D ) * u1;
								float u_mid    = ( u0 + u1 ) / 2.f;
								float d_mid    = u_mid * l;
								int phase      = static_cast< int >( floor( ( d_mid + offset ) / s ) ) % 2;
								c_color color  = ( phase == 0 ) ? accent : c_color( 128, 128, 128, 255 );
								g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line,
								                                   std::make_any< line_draw_object_t >( p0, p1, color.get_u32( ), 1.f ) );
							}
						}
					}
				}
			}
		}
	}
}
bool should_align{ };
bool hb = false;
bool should_ps_standing{ };

float difference( float a, float b )
{
	return std::max( abs( a ), abs( b ) ) - std::min( abs( a ), abs( b ) );
}

void n_movement::impl_t::auto_align( c_user_cmd* cmd )
{
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return;
	const auto move_type = g_ctx.m_local->get_move_type( );
	if ( move_type == e_move_types::move_type_ladder || move_type == e_move_types::move_type_noclip || move_type == e_move_types::move_type_fly ||
	     move_type == e_move_types::move_type_observer )
		return;
	if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
		return;

	if ( g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) ) )
		return;

	if ( g_input.check_input( &GET_VARIABLE( g_variables.m_antoha_bug_key, key_bind_t ) ) )
		return;

	if ( g_input.check_input( &GET_VARIABLE( g_variables.m_antoha_bug_land_key, key_bind_t ) ) )
		return;


	constexpr float TWO_PI    = 3.14159265358979323846f * 2.f;
	constexpr int SCAN_STEPS  = 64;
	constexpr float SCAN_STEP = TWO_PI / static_cast< float >( SCAN_STEPS );

	const float sv_gravity    = g_convars[ HASH_BT( "sv_gravity" ) ] ? g_convars[ HASH_BT( "sv_gravity" ) ]->get_float( ) : 800.f;
	const float tick_interval = g_interfaces.m_global_vars_base->m_interval_per_tick;
	const float target_surf_z = -( sv_gravity * tick_interval * 0.5f );
	auto is_surf_velocity     = [ & ]( float z ) -> bool { return std::fabsf( z - target_surf_z ) < 0.015f; };

	const c_vector start_pos = g_ctx.m_local->get_abs_origin( );
	const auto col           = g_ctx.m_local->get_collideable( );
	if ( !col )
		return;
	const auto mins         = col->get_obb_mins( );
	const auto maxs         = col->get_obb_maxs( );
	const c_vector velocity = g_ctx.m_local->get_velocity( );

	c_trace_filter filter( []( c_base_entity* entity, int ) {
		return entity && entity != g_ctx.m_local && !entity->is_player( );
	} );
	static float cached_angle = 0.f;
	bool wall_detected        = false;
	trace_t best_trace;
	float best_fraction = 1.f;
	float best_angle    = 0.f;
	for ( int i = 0; i < SCAN_STEPS; i++ ) {
		const float a = fmodf( cached_angle + i * SCAN_STEP, TWO_PI );
		c_vector end_pos;
		end_pos.m_x = cosf( a ) + start_pos.m_x;
		end_pos.m_y = sinf( a ) + start_pos.m_y;
		end_pos.m_z = start_pos.m_z;
		trace_t tmp_trace;
		ray_t ray( start_pos, end_pos, mins, maxs );
		g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &filter, &tmp_trace );
		if ( tmp_trace.m_fraction < 1.f && tmp_trace.m_plane.m_normal.m_z == 0.f ) {
			if ( tmp_trace.m_fraction < best_fraction ) {
				best_fraction = tmp_trace.m_fraction;
				best_trace    = tmp_trace;
				best_angle    = a;
				wall_detected = true;
			}
		}
	}
	if ( !wall_detected ) {
		cached_angle = 0.f;
		return;
	}
	cached_angle = best_angle;

	c_vector wall_dir( best_trace.m_plane.m_normal.m_x * -1.f, best_trace.m_plane.m_normal.m_y * -1.f, 0.f );
	{
		c_vector vel_2d = velocity;
		vel_2d.m_z      = 0.f;
		if ( vel_2d.length_2d( ) > 1.f ) {
			c_angle vel_ang  = vel_2d.to_angle2( );
			c_angle wall_ang = wall_dir.to_angle2( );
			c_angle delta    = vel_ang - wall_ang;
			delta.normalize( );
			if ( std::fabsf( delta.m_y ) > 100.f )
				return;
		}
	}

	c_vector normal_into_wall( best_trace.m_plane.m_normal.m_x * -0.005f, best_trace.m_plane.m_normal.m_y * -0.005f, 0.f );
	c_angle wall_angle = normal_into_wall.to_angle2( );
	wall_angle.normalize( );
	const float rotation = deg2rad( wall_angle.m_y - cmd->m_view_point.m_y );
	const float cos_rot  = cos( rotation );
	const float sin_rot  = sin( rotation );

	const float backup_fwd   = cmd->m_forward_move;
	const float backup_side  = cmd->m_side_move;
	const int backup_buttons = cmd->m_buttons;
	const bool manual_duck_held = ( backup_buttons & e_command_buttons::in_duck ) != 0;
	bool detect              = false;
	bool detected_with_duck  = false;
	const auto fix_air_stuck_move = [ & ]( ) {
		if ( !GET_VARIABLE( g_variables.m_fix_air_stucks, bool ) )
			return;
		const float inward_fwd  = cmd->m_forward_move;
		const float inward_side = cmd->m_side_move;
		if ( std::fabs( inward_fwd ) < 0.01f && std::fabs( inward_side ) < 0.01f )
			return;
		g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
		c_user_cmd test_cmd = *cmd;
		g_prediction.begin( g_ctx.m_local, &test_cmd );
		g_prediction.end( g_ctx.m_local );
		const float inward_speed = g_ctx.m_local->get_velocity( ).length_2d( );
		if ( inward_speed >= 1.f ) {
			g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
			return;
		}
		g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
		test_cmd                = *cmd;
		test_cmd.m_forward_move = -inward_fwd;
		test_cmd.m_side_move    = -inward_side;
		g_prediction.begin( g_ctx.m_local, &test_cmd );
		g_prediction.end( g_ctx.m_local );
		const float outward_speed = g_ctx.m_local->get_velocity( ).length_2d( );
		g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
		if ( outward_speed > inward_speed + 0.01f ) {
			cmd->m_forward_move = -inward_fwd;
			cmd->m_side_move    = -inward_side;
		}
	};
	const auto will_land_next_tick_with_current_cmd = [ & ]( ) {
		g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
		c_user_cmd probe_cmd = *cmd;
		g_prediction.begin( g_ctx.m_local, &probe_cmd );
		g_prediction.end( g_ctx.m_local );
		const bool will_land = ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) != 0;
		g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
		return will_land;
	};

	bool near_ceiling = false;
	if ( velocity.m_z > 0.f ) {
		c_vector head_pos( start_pos.m_x, start_pos.m_y, start_pos.m_z + 64.0f );
		float up_dist = std::max( velocity.m_z * tick_interval * 6.0f, 20.0f );
		c_vector head_up( start_pos.m_x, start_pos.m_y, start_pos.m_z + 64.0f + up_dist );
		ray_t ceil_ray( head_pos, head_up );
		trace_t ceil_trace;
		c_trace_filter ceil_flt( g_ctx.m_local );
		g_interfaces.m_engine_trace->trace_ray( ceil_ray, mask_playersolid, &ceil_flt, &ceil_trace );
		float dist_to_ceil = ceil_trace.m_fraction * up_dist;
		if ( ceil_trace.m_fraction < 1.0f && dist_to_ceil < 24.0f )
			near_ceiling = true;
	}

	constexpr float coarse_mults[] = { 0.f, 3.f, 8.f, 15.f, 25.f, 40.f, 60.f, 85.f, 115.f, 155.f, 200.f, 260.f, 330.f, 400.f, 450.f };
	constexpr int coarse_count     = sizeof( coarse_mults ) / sizeof( coarse_mults[ 0 ] );
	float coarse_hit               = -1.f;
	for ( int ci = 0; ci < coarse_count && !detect; ci++ ) {
		const float mult = coarse_mults[ ci ];
		const float fwd  = cos_rot * mult;
		const float side = -sin_rot * mult;
		for ( int duck_pass = 0; duck_pass < 2 && !detect; duck_pass++ ) {
			g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
			c_user_cmd test_cmd     = *cmd;
			test_cmd.m_forward_move = fwd;
			test_cmd.m_side_move    = side;
			if ( duck_pass == 1 || manual_duck_held )
				test_cmd.m_buttons |= e_command_buttons::in_duck;
			else
				test_cmd.m_buttons &= ~e_command_buttons::in_duck;
			g_prediction.begin( g_ctx.m_local, &test_cmd );
			g_prediction.end( g_ctx.m_local );
			if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground || g_ctx.m_local->get_move_type( ) == e_move_types::move_type_ladder )
				continue;
			const float new_z = g_ctx.m_local->get_velocity( ).m_z;
			if ( is_surf_velocity( new_z ) ) {
				cmd->m_forward_move = fwd;
				cmd->m_side_move    = side;
				detected_with_duck  = duck_pass == 1;
				if ( detected_with_duck ) {
					cmd->m_buttons |= e_command_buttons::in_duck;
				} else if ( !manual_duck_held )
					cmd->m_buttons &= ~e_command_buttons::in_duck;
				detect = true;
				break;
			}
			if ( coarse_hit < 0.f && new_z < 0.f && new_z > target_surf_z * 2.f )
				coarse_hit = mult;
		}
	}

	if ( !detect && coarse_hit >= 0.f ) {
		const float fine_start = std::max( 0.f, coarse_hit - 20.f );
		const float fine_end   = std::min( 450.f, coarse_hit + 20.f );
		for ( float mult = fine_start; mult <= fine_end && !detect; mult += 2.f ) {
			const float fwd  = cos_rot * mult;
			const float side = -sin_rot * mult;
			for ( int duck_pass = 0; duck_pass < 2 && !detect; duck_pass++ ) {
				g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
				c_user_cmd test_cmd     = *cmd;
				test_cmd.m_forward_move = fwd;
				test_cmd.m_side_move    = side;
				if ( duck_pass == 1 || manual_duck_held )
					test_cmd.m_buttons |= e_command_buttons::in_duck;
				else
					test_cmd.m_buttons &= ~e_command_buttons::in_duck;
				g_prediction.begin( g_ctx.m_local, &test_cmd );
				g_prediction.end( g_ctx.m_local );
				if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground || g_ctx.m_local->get_move_type( ) == e_move_types::move_type_ladder )
					continue;
				if ( is_surf_velocity( g_ctx.m_local->get_velocity( ).m_z ) ) {
					cmd->m_forward_move = fwd;
					cmd->m_side_move    = side;
					detected_with_duck  = duck_pass == 1;
					if ( detected_with_duck ) {
						cmd->m_buttons |= e_command_buttons::in_duck;
					} else if ( !manual_duck_held )
						cmd->m_buttons &= ~e_command_buttons::in_duck;
					detect = true;
					break;
				}
			}
		}
	}

	if ( !detect ) {
		for ( int i = 1; i <= 8 && !detect; i++ ) {
			const float fwd  = cos_rot * i * 7.f;
			const float side = -sin_rot * i * 7.f;
			for ( int duck_pass = 0; duck_pass < 2 && !detect; duck_pass++ ) {
				g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
				bool hit_ladder = false;
				for ( int t = 0; t < 4; t++ ) {
					c_user_cmd fake_cmd     = *cmd;
					fake_cmd.m_forward_move = fwd;
					fake_cmd.m_side_move    = side;
					if ( duck_pass == 1 || manual_duck_held )
						fake_cmd.m_buttons |= e_command_buttons::in_duck;
					else
						fake_cmd.m_buttons &= ~e_command_buttons::in_duck;
					g_prediction.begin( g_ctx.m_local, &fake_cmd );
					g_prediction.end( g_ctx.m_local );
					if ( g_ctx.m_local->get_move_type( ) == e_move_types::move_type_ladder ) {
						hit_ladder = true;
						break;
					}
				}
				if ( hit_ladder )
					continue;
				if ( is_surf_velocity( g_ctx.m_local->get_velocity( ).m_z ) ) {
					cmd->m_forward_move = fwd;
					cmd->m_side_move    = side;
					detected_with_duck  = duck_pass == 1;
					if ( detected_with_duck ) {
						cmd->m_buttons |= e_command_buttons::in_duck;
					} else if ( !manual_duck_held )
						cmd->m_buttons &= ~e_command_buttons::in_duck;
					detect = true;
					g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
					break;
				}
			}
		}
	}

	if ( !detect ) {
		cmd->m_forward_move = cos_rot * 10.f;
		cmd->m_side_move    = -sin_rot * 10.f;
		if ( !manual_duck_held )
			cmd->m_buttons &= ~e_command_buttons::in_duck;
	}
	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );

	const bool suppress_auto_duck_on_landing = detect && detected_with_duck && !manual_duck_held && will_land_next_tick_with_current_cmd( );

	if ( manual_duck_held || ( detect && detected_with_duck && !suppress_auto_duck_on_landing ) )
		cmd->m_buttons |= e_command_buttons::in_duck;
	else
		cmd->m_buttons &= ~e_command_buttons::in_duck;
	if ( is_surf_velocity( g_prediction.backup_data.m_velocity.m_z ) ) {
		const bool pure_side = ( ( backup_buttons & in_moveleft ) || ( backup_buttons & in_moveright ) ) && !( backup_buttons & in_forward ) &&
		                       !( backup_buttons & in_back );
		if ( pure_side ) {
			const bool align_right = cmd->m_side_move > 0.0f;
			const bool align_left  = cmd->m_side_move < 0.0f;
			const bool press_right = ( backup_buttons & in_moveright ) != 0;
			const bool press_left  = ( backup_buttons & in_moveleft ) != 0;
			if ( ( align_right && press_left ) || ( align_left && press_right ) ) {
				cmd->m_forward_move = backup_fwd;
				cmd->m_side_move    = backup_side;
				return;
			}
		}

		c_vector wishdir;
		bool has_wish = false;
		if ( ( backup_buttons & in_forward ) && !( backup_buttons & in_back ) && !( backup_buttons & in_moveleft ) &&
		     !( backup_buttons & in_moveright ) ) {
			wishdir  = { std::cos( deg2rad( cmd->m_view_point.m_y ) ) * 128.f, std::sin( deg2rad( cmd->m_view_point.m_y ) ) * 128.f, 0.f };
			has_wish = true;
		} else if ( ( backup_buttons & in_back ) && !( backup_buttons & in_forward ) && !( backup_buttons & in_moveleft ) &&
		            !( backup_buttons & in_moveright ) ) {
			wishdir  = { std::cos( deg2rad( cmd->m_view_point.m_y + 180.f ) ) * 128.f, std::sin( deg2rad( cmd->m_view_point.m_y + 180.f ) ) * 128.f,
				         0.f };
			has_wish = true;
		} else if ( ( backup_buttons & in_moveleft ) && !( backup_buttons & in_forward ) && !( backup_buttons & in_back ) &&
		            !( backup_buttons & in_moveright ) ) {
			wishdir  = { std::cos( deg2rad( cmd->m_view_point.m_y + 90.f ) ) * 128.f, std::sin( deg2rad( cmd->m_view_point.m_y + 90.f ) ) * 128.f,
				         0.f };
			has_wish = true;
		} else if ( ( backup_buttons & in_moveright ) && !( backup_buttons & in_forward ) && !( backup_buttons & in_back ) &&
		            !( backup_buttons & in_moveleft ) ) {
			wishdir  = { std::cos( deg2rad( cmd->m_view_point.m_y - 90.f ) ) * 128.f, std::sin( deg2rad( cmd->m_view_point.m_y - 90.f ) ) * 128.f,
				         0.f };
			has_wish = true;
		}

		if ( has_wish ) {
			trace_t trace_dir;
			c_vector st( start_pos.m_x, start_pos.m_y, best_trace.m_end.m_z );
			ray_t ray_dir( st, st + wishdir );
			g_interfaces.m_engine_trace->trace_ray( ray_dir, mask_playersolid, &filter, &trace_dir );
			if ( trace_dir.m_fraction < 1.f ) {
				float best_fwd_dir  = cmd->m_forward_move;
				float best_side_dir = cmd->m_side_move;
				for ( int i = 0; i <= 450; i += 45 ) {
					g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
					if ( backup_buttons & in_forward )
						cmd->m_forward_move = static_cast< float >( i );
					else if ( backup_buttons & in_back )
						cmd->m_forward_move = static_cast< float >( -i );
					if ( backup_buttons & in_moveleft )
						cmd->m_side_move = static_cast< float >( -i );
					else if ( backup_buttons & in_moveright )
						cmd->m_side_move = static_cast< float >( i );
					g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
					g_prediction.end( g_ctx.m_local );
					if ( is_surf_velocity( g_ctx.m_local->get_velocity( ).m_z ) ) {
						best_fwd_dir  = cmd->m_forward_move;
						best_side_dir = cmd->m_side_move;
					}
				}
				cmd->m_forward_move = best_fwd_dir;
				cmd->m_side_move    = best_side_dir;
			}
		}
	} else if ( !detect ) {
		if ( ( backup_buttons & in_forward ) || ( backup_buttons & in_back ) || ( backup_buttons & in_moveleft ) ||
		     ( backup_buttons & in_moveright ) ) {
			cmd->m_forward_move = backup_fwd;
			cmd->m_side_move    = backup_side;
		}
	}

	if ( cmd->m_forward_move < 0.f && backup_buttons & e_command_buttons::in_forward )
		cmd->m_forward_move = 450.f;
	if ( cmd->m_forward_move > 0.f && backup_buttons & e_command_buttons::in_back )
		cmd->m_forward_move = -450.f;
	if ( cmd->m_side_move < 0.f && backup_buttons & e_command_buttons::in_moveright )
		cmd->m_side_move = 450.f;
	if ( cmd->m_side_move > 0.f && backup_buttons & e_command_buttons::in_moveleft )
		cmd->m_side_move = -450.f;
	fix_air_stuck_move( );
}

void n_movement::impl_t::antoha_bug( c_user_cmd* cmd ) // todo: fix on locals (maybe some nigger reasons idk)
{
	static c_user_cmd b_cmd{ };
	static int i_ticks                       = 0;
	static bool duck_extension_pending       = false;
	static bool duck_extension_holding       = false;
	static int duck_extension_wait_ticks     = 0;
	static float duck_extension_yaw          = 0.0f;
	static float duck_extension_forward_move = 0.0f;
	static float duck_extension_side_move    = 0.0f;
	static int duck_extension_stand_buttons  = 0;
	static int duck_extension_duck_buttons   = 0;

	const auto reset_duck_extension = [ & ]( ) noexcept {
		duck_extension_pending       = false;
		duck_extension_holding       = false;
		duck_extension_wait_ticks    = 0;
		duck_extension_yaw           = 0.0f;
		duck_extension_forward_move  = 0.0f;
		duck_extension_side_move     = 0.0f;
		duck_extension_stand_buttons = 0;
		duck_extension_duck_buttons  = 0;
	};

	if ( !GET_VARIABLE( g_variables.m_antoha_bug, bool ) ) {
		g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
		reset_duck_extension( );
		return;
	}
	if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_antoha_bug_key, key_bind_t ) ) ) {
		g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
		reset_duck_extension( );
		return;
	}
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) ) {
		g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
		reset_duck_extension( );
		return;
	}

	const int stamina_cap          = GET_VARIABLE( g_variables.m_antoha_bug_max_stamina, int );
	const bool no_wall_slow_on_hit = GET_VARIABLE( g_variables.m_antoha_bug_no_wall_slow, bool );

	const int original_buttons                = cmd->m_buttons;
	const float original_forward_move         = cmd->m_forward_move;
	const float original_side_move            = cmd->m_side_move;
	const bool local_on_ground                = ( g_ctx.m_local->get_flags( ) & fl_onground ) != 0;
	const bool user_holding_jump              = ( original_buttons & in_jump ) != 0;
	const bool user_has_manual_movement_input = std::fabs( original_forward_move ) > 0.01f || std::fabs( original_side_move ) > 0.01f;

	if ( local_on_ground && !user_holding_jump ) {
		g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
		reset_duck_extension( );
		return;
	}

	const auto wall_to_player_fwd = [ & ]( float wall_yaw_deg, float fwd, float side ) noexcept -> float {
		const float delta = deg2rad( g_math.normalize_angle( wall_yaw_deg - cmd->m_view_point.m_y ) );
		return fwd * std::cos( delta ) + side * std::sin( delta );
	};
	const auto wall_to_player_side = [ & ]( float wall_yaw_deg, float fwd, float side ) noexcept -> float {
		const float delta = deg2rad( g_math.normalize_angle( wall_yaw_deg - cmd->m_view_point.m_y ) );
		return -fwd * std::sin( delta ) + side * std::cos( delta );
	};

	const c_vector live_velocity = g_prediction.backup_data.m_velocity;
	const float live_speed_2d    = live_velocity.length_2d( );
	constexpr float k_pi         = 3.14159265358979323846f;
	const float max_radius       = k_pi * 2.0f;
	const float step             = max_radius / 16.0f;
	const c_vector start_pos     = g_ctx.m_local->get_abs_origin( );
	const auto mins              = g_ctx.m_local->get_collideable( )->get_obb_mins( );
	const auto maxs              = g_ctx.m_local->get_collideable( )->get_obb_maxs( );
	static float start_circle    = 0.f;
	bool wall_detected           = false;
	c_trace_filter_trace_type_everything_filter_props fil;
	trace_t primary_trace{ };

	const auto detect_wall = []( const c_vector& start_pos, const c_vector& mins, const c_vector& maxs, float start_circle, float max_radius,
	                             float step, c_trace_filter_trace_type_everything_filter_props& fil, trace_t& out_trace, float& out_angle ) -> bool {
		const float extra_reach          = GET_VARIABLE( g_variables.m_antoha_bug_extra_reach, float );
		constexpr float z_step           = 1.f;
		constexpr float normal_z_epsilon = 0.05f;

		const float half_x = std::max( std::fabs( mins.m_x ), std::fabs( maxs.m_x ) );
		const float half_y = std::max( std::fabs( mins.m_y ), std::fabs( maxs.m_y ) );

		for ( float a = start_circle; a < start_circle + max_radius; a += step ) {
			const float angle = std::fmod( a, max_radius );
			const float dir_x = std::cos( angle );
			const float dir_y = std::sin( angle );

			const float box_reach  = std::min( half_x / std::max( std::fabs( dir_x ), 1e-6f ), half_y / std::max( std::fabs( dir_y ), 1e-6f ) );
			const float ray_length = box_reach + extra_reach;

			const float z_start = start_pos.m_z + mins.m_z;
			const float z_end   = start_pos.m_z + maxs.m_z;

			trace_t best_trace{ };
			best_trace.m_fraction  = 1.0f;
			bool hit_at_this_angle = false;

			for ( float z = z_start; z <= z_end; z += z_step ) {
				c_vector ray_start{ start_pos.m_x, start_pos.m_y, z };
				c_vector ray_end{ start_pos.m_x + dir_x * ray_length, start_pos.m_y + dir_y * ray_length, z };

				trace_t trace{ };
				ray_t ray( ray_start, ray_end );
				g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &fil, &trace );

				if ( trace.m_fraction >= 1.0f )
					continue;

				if ( std::fabs( trace.m_plane.m_normal.m_z ) > normal_z_epsilon )
					continue;

				const float facing = trace.m_plane.m_normal.m_x * dir_x + trace.m_plane.m_normal.m_y * dir_y;
				if ( facing > -0.1f )
					continue;

				hit_at_this_angle = true;
				if ( trace.m_fraction < best_trace.m_fraction )
					best_trace = trace;
			}

			if ( hit_at_this_angle ) {
				out_trace = best_trace;
				out_angle = a;
				return true;
			}
		}
		return false;
	};

	float found_angle = 0.f;
	if ( detect_wall( start_pos, mins, maxs, start_circle, max_radius, step, fil, primary_trace, found_angle ) ) {
		wall_detected = true;
		start_circle  = found_angle;
	}

	if ( !wall_detected ) {
		start_circle                                    = 0.0f;
		g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
		reset_duck_extension( );
		return;
	}

	struct prediction_state_guard_t {
		~prediction_state_guard_t( )
		{
			g_prediction.restore_entity_to_predicted_frame( -1 );
		}
	} prediction_state_guard;

	const auto apply_predicted_state = [ & ]( ) noexcept {
		g_ctx.m_local->set_abs_origin( start_pos );
		g_ctx.m_local->get_origin( )   = start_pos;
		g_ctx.m_local->get_velocity( ) = live_velocity;
	};

	const c_vector approach_normal = primary_trace.m_plane.m_normal * -1.0f;
	c_angle angle_wall             = approach_normal.to_angle( );
	angle_wall.normalize( );

	auto get_pidoras = [ & ]( ) -> float {
		trace_t trace_support{ };
		c_vector minusplane = primary_trace.m_plane.m_normal * -1.0f * 32.0f;
		auto origin         = g_ctx.m_local->get_abs_origin( );
		ray_t ray2( c_vector( origin.m_x, origin.m_y, primary_trace.m_end.m_z ),
		            c_vector( origin.m_x + minusplane.m_x, origin.m_y + minusplane.m_y, primary_trace.m_end.m_z ) );
		c_trace_filter_trace_type_everything_filter_props wlft;
		g_interfaces.m_engine_trace->trace_ray( ray2, mask_all, &wlft, &trace_support );
		const c_vector wall_normal( primary_trace.m_plane.m_normal.m_x, primary_trace.m_plane.m_normal.m_y, 0.0f );
		return trace_support.m_fraction * 32.0f - GetWallSupportDistance( g_ctx.m_local, wall_normal );
	};

	float pidoras               = 1.f;
	float best_forward_move     = cmd->m_forward_move;
	float best_side_move        = cmd->m_side_move;
	float best_yaw              = angle_wall.m_y;
	int best_buttos             = cmd->m_buttons;
	int best_hit_streak         = 0;
	bool best_is_duck_extension = false;
	int best_stand_buttons      = cmd->m_buttons;
	int best_duck_buttons       = cmd->m_buttons | in_duck;
	const bool manual_duck_held = ( original_buttons & in_duck ) != 0;

	c_vector velo = g_prediction.backup_data.m_velocity;
	float vel_yaw = rad2deg( atan2f( velo.m_y, velo.m_x ) );
	float rel     = g_math.normalize_angle( vel_yaw - angle_wall.m_y );
	float sign    = ( rel >= 0.f ) ? -1.f : 1.f;

	const float live_stamina                           = g_ctx.m_local->get_stamina( );
	const float target_surf_z                          = g_prediction.get_target_predict_z_velocity( );
	constexpr float k_forward_move_logged_values[]     = { 0.125f, 0.875f, 0.750f, 0.5f, 1.f, 2.125f, 2.875f, 5.f,15.f, 30.f };
	constexpr float k_duck_extension_pidoras_tolerance = 0.05f;
	constexpr int k_duck_extension_delay_ticks         = 0;
	constexpr float k_collision_stop_max_next_speed    = 8.0f;
	constexpr float k_collision_stop_speed_ratio       = 0.2f;
	constexpr float k_collision_stop_min_live_speed    = 40.0f;

	const auto would_release_tb_on_collision_stop = [ & ]( const float wall_yaw, const float fwd, const float side,
	                                                       const int buttons ) noexcept -> bool {
		if ( user_has_manual_movement_input || live_speed_2d < k_collision_stop_min_live_speed )
			return false;

		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		if ( stamina_cap > 0 )
			g_ctx.m_local->get_stamina( ) = std::max( g_ctx.m_local->get_stamina( ), static_cast< float >( stamina_cap ) );

		cmd->m_buttons      = buttons;
		cmd->m_forward_move = wall_to_player_fwd( wall_yaw, fwd, side );
		cmd->m_side_move    = wall_to_player_side( wall_yaw, fwd, side );

		g_prediction.begin( g_ctx.m_local, cmd );
		g_prediction.end( g_ctx.m_local );

		const float predicted_speed_2d = g_ctx.m_local->get_velocity( ).length_2d( );
		if ( predicted_speed_2d < 1.f )
			return false;
		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		return predicted_speed_2d <= k_collision_stop_max_next_speed && predicted_speed_2d <= live_speed_2d * k_collision_stop_speed_ratio;
	};
	const auto fix_air_stuck_move = [ & ]( ) noexcept {
		if ( !GET_VARIABLE( g_variables.m_fix_air_stucks, bool ) )
			return;
		const float inward_fwd  = cmd->m_forward_move;
		const float inward_side = cmd->m_side_move;
		if ( std::fabs( inward_fwd ) < 0.01f && std::fabs( inward_side ) < 0.01f )
			return;

		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );
		if ( stamina_cap > 0 )
			g_ctx.m_local->get_stamina( ) = std::max( g_ctx.m_local->get_stamina( ), static_cast< float >( stamina_cap ) );

		c_user_cmd test_cmd = *cmd;
		g_prediction.begin( g_ctx.m_local, &test_cmd );
		g_prediction.end( g_ctx.m_local );
		const float inward_speed = g_ctx.m_local->get_velocity( ).length_2d( );
		if ( inward_speed >= 1.f ) {
			g_prediction.restore_entity_to_predicted_frame( -1 );
			apply_predicted_state( );
			return;
		}

		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );
		if ( stamina_cap > 0 )
			g_ctx.m_local->get_stamina( ) = std::max( g_ctx.m_local->get_stamina( ), static_cast< float >( stamina_cap ) );

		test_cmd                = *cmd;
		test_cmd.m_forward_move = -inward_fwd;
		test_cmd.m_side_move    = -inward_side;
		g_prediction.begin( g_ctx.m_local, &test_cmd );
		g_prediction.end( g_ctx.m_local );
		const float outward_speed = g_ctx.m_local->get_velocity( ).length_2d( );
		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );
		if ( outward_speed > inward_speed + 0.01f ) {
			cmd->m_forward_move = -inward_fwd;
			cmd->m_side_move    = -inward_side;
		}
	};

	const auto simulate_antoha_variant = [ & ]( const float wall_yaw, const float forward_move, const bool force_duck, float& out_pidoras,
	                                            int& out_hit_streak, int& out_buttons, bool& out_is_headbounce ) {
		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		if ( stamina_cap > 0 )
			g_ctx.m_local->get_stamina( ) = std::max( g_ctx.m_local->get_stamina( ), static_cast< float >( stamina_cap ) );

		cmd->m_buttons = original_buttons;
		if ( force_duck )
			cmd->m_buttons |= in_duck;

		cmd->m_forward_move = wall_to_player_fwd( wall_yaw, forward_move, 0.0f );
		cmd->m_side_move    = wall_to_player_side( wall_yaw, forward_move, 0.0f );

		float pre_vel_z = g_ctx.m_local->get_velocity( ).m_z;

		g_prediction.begin( g_ctx.m_local, cmd );
		g_prediction.end( g_ctx.m_local );

		float post_vel_z               = g_ctx.m_local->get_velocity( ).m_z;
		const float predicted_speed_2d = g_ctx.m_local->get_velocity( ).length_2d( );
		if ( predicted_speed_2d < 1.f )
			return false;
		out_pidoras       = get_pidoras( );
		out_hit_streak    = 0;
		out_is_headbounce = false;

		if ( g_prediction.is_target_predict_z_velocity( post_vel_z ) ) {
			out_hit_streak = 1;
			if ( pre_vel_z > 0.0f && std::fabs( post_vel_z - target_surf_z ) < 0.01f ) {
				out_is_headbounce = true;
			}

			for ( int extra_tick = 0; extra_tick < 2; ++extra_tick ) {
				g_prediction.begin( g_ctx.m_local, cmd );
				g_prediction.end( g_ctx.m_local );
				if ( !g_prediction.is_target_predict_z_velocity( g_ctx.m_local->get_velocity( ).m_z ) )
					break;
				++out_hit_streak;
			}
		}
		out_buttons = cmd->m_buttons;
	};

	for ( auto mul : k_forward_move_logged_values ) {
		for ( float ang = 85.999f; ang < 90.f; ang += 1.f ) {
			const float current_yaw = angle_wall.m_y + ang * sign;

			float stand_pidoras      = 0.0f;
			int stand_hit_streak     = 0;
			int stand_buttons        = original_buttons;
			bool stand_is_headbounce = false;
			simulate_antoha_variant( current_yaw, mul, false, stand_pidoras, stand_hit_streak, stand_buttons, stand_is_headbounce );

			float candidate_pidoras          = stand_pidoras;
			int candidate_hit_streak         = stand_hit_streak;
			int candidate_buttons            = stand_buttons;
			bool candidate_is_duck_extension = false;
			int candidate_stand_buttons      = stand_buttons;
			int candidate_duck_buttons       = stand_buttons | in_duck;

			if ( !manual_duck_held ) {
				float duck_pidoras      = 0.0f;
				int duck_hit_streak     = 0;
				int duck_buttons        = original_buttons;
				bool duck_is_headbounce = false;
				simulate_antoha_variant( current_yaw, mul, true, duck_pidoras, duck_hit_streak, duck_buttons, duck_is_headbounce );

				const bool stand_hits = stand_hit_streak > 0;
				const bool duck_hits  = duck_hit_streak > 0;

				const bool should_use_duck_extension = stand_hits && duck_hits && duck_hit_streak > stand_hit_streak &&
				                                       duck_pidoras <= stand_pidoras + k_duck_extension_pidoras_tolerance;

				const bool duck_is_required_to_hit = !stand_hits && duck_hits && duck_is_headbounce;

				if ( should_use_duck_extension || duck_is_required_to_hit ) {
					candidate_pidoras           = duck_pidoras;
					candidate_hit_streak        = duck_hit_streak;
					candidate_buttons           = duck_buttons;
					candidate_is_duck_extension = should_use_duck_extension;
					candidate_duck_buttons      = duck_buttons;
				}
			}

			const bool candidate_uses_duck = ( candidate_buttons & in_duck ) != 0;
			const bool best_uses_duck      = ( best_buttos & in_duck ) != 0;
			const bool candidate_hits      = candidate_hit_streak > 0;
			const bool best_hits           = best_hit_streak > 0;

			const bool is_better_variant =
				( candidate_hits && !best_hits ) ||
				( candidate_hits == best_hits && candidate_uses_duck == best_uses_duck &&
			      ( candidate_hit_streak > best_hit_streak || ( candidate_hit_streak == best_hit_streak && candidate_pidoras < pidoras ) ) ) ||
				( candidate_hits && best_hits && best_uses_duck && !candidate_uses_duck ) ||
				( !candidate_hits && !best_hits &&
			      ( candidate_pidoras < pidoras ||
			        ( std::fabs( candidate_pidoras - pidoras ) <= 0.001f && !candidate_uses_duck && best_uses_duck ) ) );

			if ( is_better_variant ) {
				pidoras                = candidate_pidoras;
				best_hit_streak        = candidate_hit_streak;
				best_forward_move      = mul;
				best_side_move         = 0.0f;
				best_yaw               = current_yaw;
				best_buttos            = candidate_buttons;
				best_is_duck_extension = candidate_is_duck_extension;
				best_stand_buttons     = candidate_stand_buttons;
				best_duck_buttons      = candidate_duck_buttons;
			}
		}
	}

	g_movement.m_air_stuck_data.m_antoha_debug_fwd  = best_forward_move;
	g_movement.m_air_stuck_data.m_antoha_debug_side = best_side_move;
	g_movement.m_air_stuck_data.m_antoha_debug_dist = pidoras;

	const bool started_falling_from_stand = !g_prediction.is_target_predict_z_velocity( live_velocity.m_z ) && live_velocity.m_z < -1.0f;

	if ( manual_duck_held ) {
		if ( best_hit_streak > 0 && would_release_tb_on_collision_stop( best_yaw, best_forward_move, best_side_move, best_buttos ) ) {
			if ( no_wall_slow_on_hit ) {
				g_movement.m_air_stuck_data.m_antoha_bug_detect = true;
				cmd->m_buttons                                  = best_buttos;
				reset_duck_extension( );
				return;
			}
			g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
			reset_duck_extension( );
			return;
		}
		g_movement.m_air_stuck_data.m_antoha_bug_detect = best_hit_streak > 0;
		reset_duck_extension( );
	} else if ( duck_extension_holding ) {
		const int extension_buttons = started_falling_from_stand ? duck_extension_stand_buttons : duck_extension_duck_buttons;
		if ( would_release_tb_on_collision_stop( duck_extension_yaw, duck_extension_forward_move, duck_extension_side_move, extension_buttons ) ) {
			if ( no_wall_slow_on_hit ) {
				g_movement.m_air_stuck_data.m_antoha_bug_detect = true;
				cmd->m_buttons                                  = extension_buttons;
				if ( started_falling_from_stand )
					reset_duck_extension( );
				return;
			}
			g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
			reset_duck_extension( );
			return;
		}
		g_movement.m_air_stuck_data.m_antoha_bug_detect = true;
		if ( !no_wall_slow_on_hit ) {
			cmd->m_forward_move = wall_to_player_fwd( duck_extension_yaw, duck_extension_forward_move, duck_extension_side_move );
			cmd->m_side_move    = wall_to_player_side( duck_extension_yaw, duck_extension_forward_move, duck_extension_side_move );
		}
		cmd->m_buttons = extension_buttons;
		fix_air_stuck_move( );
		if ( started_falling_from_stand )
			reset_duck_extension( );
		return;
	} else if ( duck_extension_pending ) {
		if ( would_release_tb_on_collision_stop( duck_extension_yaw, duck_extension_forward_move, duck_extension_side_move,
		                                         duck_extension_stand_buttons ) ) {
			if ( no_wall_slow_on_hit ) {
				g_movement.m_air_stuck_data.m_antoha_bug_detect = true;
				cmd->m_buttons                                  = duck_extension_stand_buttons;
				if ( duck_extension_wait_ticks > 0 ) {
					--duck_extension_wait_ticks;
				} else {
					duck_extension_pending = false;
					duck_extension_holding = true;
				}
				return;
			}
			g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
			reset_duck_extension( );
			return;
		}
		g_movement.m_air_stuck_data.m_antoha_bug_detect = true;
		if ( !no_wall_slow_on_hit ) {
			cmd->m_forward_move = wall_to_player_fwd( duck_extension_yaw, duck_extension_forward_move, duck_extension_side_move );
			cmd->m_side_move    = wall_to_player_side( duck_extension_yaw, duck_extension_forward_move, duck_extension_side_move );
		}
		cmd->m_buttons = duck_extension_stand_buttons;
		fix_air_stuck_move( );
		if ( duck_extension_wait_ticks > 0 ) {
			--duck_extension_wait_ticks;
		} else {
			duck_extension_pending = false;
			duck_extension_holding = true;
		}
		return;
	}

	if ( best_is_duck_extension && !manual_duck_held ) {
		if ( best_hit_streak > 0 && would_release_tb_on_collision_stop( best_yaw, best_forward_move, best_side_move, best_stand_buttons ) ) {
			if ( no_wall_slow_on_hit ) {
				g_movement.m_air_stuck_data.m_antoha_bug_detect = true;
				cmd->m_buttons                                  = best_stand_buttons;
				reset_duck_extension( );
				return;
			}
			g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
			reset_duck_extension( );
			return;
		}
		g_movement.m_air_stuck_data.m_antoha_bug_detect = best_hit_streak > 0;

		const bool plan_changed = std::fabs( duck_extension_yaw - best_yaw ) > 0.001f ||
		                          std::fabs( duck_extension_forward_move - best_forward_move ) > 0.001f ||
		                          std::fabs( duck_extension_side_move - best_side_move ) > 0.001f ||
		                          duck_extension_stand_buttons != best_stand_buttons || duck_extension_duck_buttons != best_duck_buttons;

		if ( plan_changed || ( !duck_extension_pending && !duck_extension_holding ) ) {
			duck_extension_pending       = true;
			duck_extension_holding       = false;
			duck_extension_wait_ticks    = k_duck_extension_delay_ticks;
			duck_extension_yaw           = best_yaw;
			duck_extension_forward_move  = best_forward_move;
			duck_extension_side_move     = best_side_move;
			duck_extension_stand_buttons = best_stand_buttons;
			duck_extension_duck_buttons  = best_duck_buttons;
		}

		if ( !( no_wall_slow_on_hit && best_hit_streak > 0 ) ) {
			cmd->m_forward_move = wall_to_player_fwd( best_yaw, best_forward_move, best_side_move );
			cmd->m_side_move    = wall_to_player_side( best_yaw, best_forward_move, best_side_move );
		}
		cmd->m_buttons = best_stand_buttons;
		fix_air_stuck_move( );
		return;
	}

	reset_duck_extension( );
	if ( best_hit_streak > 0 && would_release_tb_on_collision_stop( best_yaw, best_forward_move, best_side_move, best_buttos ) ) {
		if ( no_wall_slow_on_hit ) {
			g_movement.m_air_stuck_data.m_antoha_bug_detect = true;
			cmd->m_buttons                                  = best_buttos;
			return;
		}
		g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
		return;
	}
	g_movement.m_air_stuck_data.m_antoha_bug_detect = best_hit_streak > 0;
	if ( !( no_wall_slow_on_hit && best_hit_streak > 0 ) ) {
		cmd->m_forward_move = wall_to_player_fwd( best_yaw, best_forward_move, best_side_move );
		cmd->m_side_move    = wall_to_player_side( best_yaw, best_forward_move, best_side_move );
	}
	cmd->m_buttons = best_buttos;
	fix_air_stuck_move( );
}
void n_movement::impl_t::wall_climb( c_user_cmd* cmd )
{
	static bool wall_climb_printed = false;

	m_air_stuck_data.m_wall_climb_detect = false;

	if ( !GET_VARIABLE( g_variables.m_wall_climb, bool ) ) {
		wall_climb_printed = false;
		return;
	}
	if ( !cmd || !g_ctx.m_local || !g_ctx.m_local->is_alive( ) ) {
		wall_climb_printed = false;
		return;
	}
	if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_wall_climb_key, key_bind_t ) ) ) {
		wall_climb_printed = false;
		return;
	}
	if ( !( g_ctx.m_local->get_flags( ) & fl_onground ) ) {
		wall_climb_printed = false;
		return;
	}
	if ( g_utilities.is_in< int >( g_ctx.m_local->get_move_type( ), invalid_move_types ) ) {
		wall_climb_printed = false;
		return;
	}

	struct wall_climb_wall_t {
		bool found = false;
		trace_t trace{ };
		float distance = FLT_MAX;
	};

	const c_vector start_pos = g_ctx.m_local->get_abs_origin( );
	const c_vector live_velocity = g_prediction.backup_data.m_velocity;
	const auto mins = g_ctx.m_local->get_collideable( )->get_obb_mins( );
	const auto maxs = g_ctx.m_local->get_collideable( )->get_obb_maxs( );
	constexpr float k_pi = 3.14159265358979323846f;
	constexpr float k_ray_distance = 8.0f;
	constexpr float k_max_align_distance = 2.25f;
	constexpr int k_scan_steps = 128;
	wall_climb_wall_t closest_wall{ };
	c_trace_filter_trace_type_everything_filter_props filter;

	for ( int i = 0; i < k_scan_steps; ++i ) {
		const float angle = ( k_pi * 2.0f ) * static_cast< float >( i ) / static_cast< float >( k_scan_steps );
		const c_vector direction( std::cos( angle ), std::sin( angle ), 0.0f );
		trace_t trace{ };
		ray_t ray( start_pos, start_pos + direction * k_ray_distance, mins, maxs );
		g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &filter, &trace );

		if ( trace.m_fraction >= 1.0f || trace.m_start_solid )
			continue;
		if ( std::fabs( trace.m_plane.m_normal.m_z ) > 0.05f )
			continue;

		const float normal_x = std::fabs( trace.m_plane.m_normal.m_x );
		const float normal_y = std::fabs( trace.m_plane.m_normal.m_y );
		if ( normal_x < 0.0005f || normal_y < 0.0005f || std::fabs( normal_x - 1.0f ) < 0.0005f || std::fabs( normal_y - 1.0f ) < 0.0005f )
			continue;

		const float distance = trace.m_fraction * k_ray_distance;
		if ( !closest_wall.found || distance < closest_wall.distance ) {
			closest_wall.found = true;
			closest_wall.trace = trace;
			closest_wall.distance = distance;
		}
	}

	if ( !closest_wall.found || closest_wall.distance > k_max_align_distance ) {
		wall_climb_printed = false;
		return;
	}

	const c_vector approach_normal = closest_wall.trace.m_plane.m_normal * -1.0f;
	c_angle wall_angle = approach_normal.to_angle( );
	wall_angle.normalize( ).clamp( );

	const auto apply_predicted_state = [ & ]( ) noexcept {
		g_ctx.m_local->set_abs_origin( start_pos );
		g_ctx.m_local->get_origin( ) = start_pos;
		g_ctx.m_local->get_velocity( ) = live_velocity;
	};

	const auto refresh_buttons = []( c_user_cmd& target_cmd ) noexcept {
		target_cmd.m_buttons &= ~( in_moveright | in_moveleft | in_back | in_forward );
		if ( target_cmd.m_side_move < -0.01f )
			target_cmd.m_buttons |= in_moveleft;
		else if ( target_cmd.m_side_move > 0.01f )
			target_cmd.m_buttons |= in_moveright;
		if ( target_cmd.m_forward_move < -0.01f )
			target_cmd.m_buttons |= in_back;
		else if ( target_cmd.m_forward_move > 0.01f )
			target_cmd.m_buttons |= in_forward;
	};

	const auto simulate = [ & ]( const float yaw, const float forward, const bool duck ) -> float {
		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		c_user_cmd test_cmd = *cmd;
		test_cmd.m_view_point.m_y = yaw;
		test_cmd.m_view_point.normalize( ).clamp( );
		test_cmd.m_forward_move = forward;
		test_cmd.m_side_move = 0.0f;
		test_cmd.m_buttons |= in_jump;
		if ( duck )
			test_cmd.m_buttons |= in_duck;
		else
			test_cmd.m_buttons &= ~in_duck;
		refresh_buttons( test_cmd );

		for ( int tick = 0; tick < 5; ++tick ) {
			g_prediction.begin( g_ctx.m_local, &test_cmd );
			g_prediction.end( g_ctx.m_local );
		}

		const float origin_gain = g_ctx.m_local->get_abs_origin( ).m_z - start_pos.m_z;
		const float velocity_score = g_ctx.m_local->get_velocity( ).m_z * 0.01f;

		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );
		return origin_gain + velocity_score;
	};

	c_user_cmd neutral_cmd = *cmd;
	neutral_cmd.m_forward_move = 0.0f;
	neutral_cmd.m_side_move = 0.0f;
	const float neutral_score = simulate( neutral_cmd.m_view_point.m_y, 0.0f, ( neutral_cmd.m_buttons & in_duck ) != 0 );

	float best_score = -FLT_MAX;
	float best_yaw = wall_angle.m_y;
	float best_forward = 450.0f;
	bool best_duck = false;
	constexpr std::array< float, 9 > yaw_offsets = { -6.0f, -3.0f, -1.0f, -0.25f, 0.0f, 0.25f, 1.0f, 3.0f, 6.0f };
	constexpr std::array< float, 7 > forward_values = { 450.0f, 320.0f, 180.0f, 80.0f, 30.0f, 5.0f, 1.0f };

	for ( const float yaw_offset : yaw_offsets ) {
		for ( const float forward : forward_values ) {
			for ( int duck_variant = 0; duck_variant < 2; ++duck_variant ) {
				const float score = simulate( std::remainder( wall_angle.m_y + yaw_offset, 360.0f ), forward, duck_variant != 0 );
				if ( score > best_score ) {
					best_score = score;
					best_yaw = std::remainder( wall_angle.m_y + yaw_offset, 360.0f );
					best_forward = forward;
					best_duck = duck_variant != 0;
				}
			}
		}
	}

	if ( best_score + 0.001f < neutral_score ) {
		wall_climb_printed = false;
		return;
	}

	const c_angle original_view = cmd->m_view_point;
	cmd->m_view_point.m_y = best_yaw;
	cmd->m_view_point.normalize( ).clamp( );
	cmd->m_forward_move = best_forward;
	cmd->m_side_move = 0.0f;
	cmd->m_buttons |= in_jump;
	if ( best_duck )
		cmd->m_buttons |= in_duck;
	else
		cmd->m_buttons &= ~in_duck;
	refresh_buttons( *cmd );
	start_movement_fix( cmd );
	cmd->m_view_point = original_view;
	end_movement_fix( cmd );
	refresh_buttons( *cmd );

	m_air_stuck_data.m_wall_climb_detect = true;

	if ( !wall_climb_printed ) {
		if ( n_interfaces::chat_element )
			n_interfaces::chat_element->chatprintf( 0, 0, make_kami_chat_message( GET_VARIABLE( g_variables.m_accent, c_color ), "wall climbed" ).c_str( ) );
		wall_climb_printed = true;
	}
}

void n_movement::impl_t::test( c_user_cmd* cmd )
{
	if ( !GET_VARIABLE( g_variables.test, bool ) ) {
		return;
	}
	if ( !g_input.check_input( &GET_VARIABLE( g_variables.test_key, key_bind_t ) ) ) {
		return;
	}
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) ) {
		return;
	}
	static c_user_cmd b_cmd{ };
	static int i_ticks = 0;

	const c_vector live_velocity = g_prediction.backup_data.m_velocity;
	constexpr float k_pi         = 3.14159265358979323846f;
	const float max_radius       = k_pi * 2.0f;
	const float step             = max_radius / 16.0f;
	const c_vector start_pos     = g_ctx.m_local->get_abs_origin( );
	const auto mins              = g_ctx.m_local->get_collideable( )->get_obb_mins( );
	const auto maxs              = g_ctx.m_local->get_collideable( )->get_obb_maxs( );
	static float start_circle    = 0.f;
	const float ray_dist         = maxs.m_x + 2.0f;
	bool wall_detected           = false;
	c_trace_filter_trace_type_everything_filter_props fil;
	trace_t primary_trace;
	for ( float a = start_circle; a < start_circle + max_radius; a += step ) {
		const float angle = std::fmod( a, max_radius );

		c_vector end_pos{ };
		end_pos.m_x = std::cos( angle ) * 1.f + start_pos.m_x;
		end_pos.m_y = std::sin( angle ) * 1.f + start_pos.m_y;
		end_pos.m_z = start_pos.m_z;

		trace_t trace{ };
		ray_t ray( start_pos, end_pos, mins, maxs );
		g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &fil, &trace );

		if ( trace.m_fraction != 1.0f && trace.m_plane.m_normal.m_z == 0.0f ) {
			if ( !wall_detected ) {
				wall_detected = true;
				primary_trace = trace;
				start_circle  = a;
			}
		}
	}
	if ( !wall_detected ) {
		start_circle = 0.0f;
		return;
	}
	const auto apply_predicted_state = [ & ]( ) noexcept {
		g_ctx.m_local->set_abs_origin( start_pos );
		g_ctx.m_local->get_origin( )   = start_pos;
		g_ctx.m_local->get_velocity( ) = live_velocity;
	};
	const c_vector approach_normal = primary_trace.m_plane.m_normal * -1.0f;
	c_angle angle_wall             = approach_normal.to_angle( );
	angle_wall.normalize( );

	auto get_pidoras = [ & ]( ) -> float {
		trace_t trace_support{ };
		c_vector minusplane = primary_trace.m_plane.m_normal * -1.0f * 32.0f;

		auto origin = g_ctx.m_local->get_abs_origin( );
		ray_t ray2( c_vector( origin.m_x, origin.m_y, primary_trace.m_end.m_z ),
		            c_vector( origin.m_x + minusplane.m_x, origin.m_y + minusplane.m_y, primary_trace.m_end.m_z ) );

		c_trace_filter_trace_type_everything_filter_props wlft;
		g_interfaces.m_engine_trace->trace_ray( ray2, mask_all, &wlft, &trace_support );

		const c_vector wall_normal( primary_trace.m_plane.m_normal.m_x, primary_trace.m_plane.m_normal.m_y, 0.0f );

		return trace_support.m_fraction * 32.0f - GetWallSupportDistance( g_ctx.m_local, wall_normal );
	};

	float pidoras                                                 = 1.f;
	float best_forward_move                                       = cmd->m_forward_move;
	float best_side_move                                          = cmd->m_side_move;
	float best_yaw                                                = cmd->m_view_point.m_y;
	float best_buttos                                             = cmd->m_buttons;
	constexpr std::array< float, 9 > k_forward_move_logged_values = { 5.f, 120.f };
	float angs[]                                                  = { 5.f, 85.f, 86.f, 87.f, 88.f, 89.f, 89.99f };
	c_vector velo                                                 = g_prediction.backup_data.m_velocity;
	float vel_yaw                                                 = rad2deg( atan2f( velo.m_y, velo.m_x ) );
	float rel                                                     = g_math.normalize_angle( vel_yaw - angle_wall.m_y ); // [-180, 180]
	float sign                                                    = ( rel >= 0.f ) ? -1.f : 1.f;
	bool end                                                      = false;
	bool befinduck                                                = cmd->m_buttons & in_duck;

	for ( auto mul : k_forward_move_logged_values ) {
		for ( float ang = 45.999f; ang < 90.f; ang += 1.f ) {
			g_prediction.restore_entity_to_predicted_frame( -1 );
			apply_predicted_state( );

			cmd->m_view_point.m_y = angle_wall.m_y + ang * sign;
			cmd->m_forward_move   = mul;
			cmd->m_side_move      = 0.0f;
			start_movement_fix( cmd );
			cmd->m_view_point = g_ctx.old_view_point;
			end_movement_fix( cmd );
			float b_huesos = g_ctx.m_local->get_velocity( ).m_z;
			g_prediction.begin( g_ctx.m_local, cmd );
			g_prediction.end( g_ctx.m_local );
			float p_pidoras = get_pidoras( );
			float p_huesos  = g_ctx.m_local->get_velocity( ).m_z;
			if ( p_pidoras < pidoras || p_huesos == -6.25f ) {
				pidoras           = p_pidoras;
				best_forward_move = cmd->m_forward_move;
				best_side_move    = cmd->m_side_move;
				best_yaw          = cmd->m_view_point.m_y;
				best_buttos       = cmd->m_buttons;
			}
		}
	}

	cmd->m_forward_move   = best_forward_move;
	cmd->m_side_move      = best_side_move;
	cmd->m_buttons        = best_buttos;
	cmd->m_view_point.m_y = best_yaw;
}
void n_movement::impl_t::air_stuck( c_user_cmd* cmd )
{
	static c_user_cmd backup_cmd{ };
	static c_user_cmd optimal_cmd{ };
	static bool have_saved_cmd                  = false;
	static bool have_optimal_cmd                = false;
	static float start_circle                   = 0.0f;
	static int armed_tick                       = -1;
	static int lock_until_tick                  = -1;
	static int last_bruteforce_tick             = -1000;
	static int optimal_fail_ticks               = 0;
	static int hold_bad_ticks                   = 0;
	static bool have_last_live_vel              = false;
	static int stuck_hold_ticks                 = 0;
	static bool stuck_msg_sent                  = false;
	static bool descend_wait_tick               = false;
	static bool point_descend_release_latch     = false;
	static int point_descend_release_until_tick = -1;
	static float stuck_last_yaw                 = FLT_MAX;
	static float stuck_last_move                = FLT_MAX;
	static float stuck_last_side                = 0.0f;
	static float stuck_last_ang                 = FLT_MAX;

	auto reset_state = [ & ]( ) noexcept {
		const int saved_auto_align_block_until_tick = m_air_stuck_data.m_auto_align_block_until_tick;
		const bool saved_m_wall_stuck = m_air_stuck_data.m_wall_stuck;
		have_saved_cmd                              = false;
		have_optimal_cmd                            = false;
		start_circle                                = 0.0f;
		armed_tick                                  = -1;
		lock_until_tick                             = -1;
		last_bruteforce_tick                        = -1000;
		optimal_fail_ticks                          = 0;
		hold_bad_ticks                              = 0;
		have_last_live_vel                          = false;
		stuck_hold_ticks                            = 0;
		stuck_msg_sent                              = false;
		descend_wait_tick                           = false;
		point_descend_release_latch                 = false;
		point_descend_release_until_tick            = -1;
		stuck_last_yaw                              = FLT_MAX;
		stuck_last_move                             = FLT_MAX;
		stuck_last_side                             = 0.0f;
		stuck_last_ang                              = FLT_MAX;
		m_air_stuck_data.reset( );
		m_air_stuck_data.m_wall_stuck = saved_m_wall_stuck;
		m_air_stuck_data.m_auto_align_block_until_tick = saved_auto_align_block_until_tick;
	};

	auto refresh_air_stuck_buttons = []( c_user_cmd& target_cmd ) noexcept {
		target_cmd.m_buttons &= ~( in_moveright | in_moveleft | in_back | in_forward | in_duck );

		if ( target_cmd.m_side_move < 0.0f ) {
			target_cmd.m_buttons |= in_moveleft;
		} else if ( target_cmd.m_side_move > 0.0f ) {
			target_cmd.m_buttons |= in_moveright;
		}

		if ( target_cmd.m_forward_move < 0.0f ) {
			target_cmd.m_buttons |= in_back;
		} else if ( target_cmd.m_forward_move > 0.0f ) {
			target_cmd.m_buttons |= in_forward;
		}
	};

	auto clear_air_stuck_movement = []( c_user_cmd& target_cmd ) noexcept {
		target_cmd.m_forward_move = 0.0f;
		target_cmd.m_side_move    = 0.0f;
		target_cmd.m_up_move      = 0.0f;
		target_cmd.m_buttons &= ~( in_moveright | in_moveleft | in_back | in_forward | in_duck | in_jump );
	};

	auto merge_air_stuck_action_buttons = []( const int live_buttons, const int stuck_buttons ) noexcept {
		constexpr int movement_mask = in_moveright | in_moveleft | in_back | in_forward | in_duck | in_jump;
		return ( stuck_buttons & movement_mask ) | ( live_buttons & ~movement_mask );
	};

	const c_user_cmd live_cmd                   = *cmd;
	const int saved_auto_align_block_until_tick = m_air_stuck_data.m_auto_align_block_until_tick;
	const bool saved_m_wall_stuck = m_air_stuck_data.m_wall_stuck;
	const c_angle air_stuck_original_view       = live_cmd.m_view_point;

	auto apply_air_stuck_view = [ & ]( c_user_cmd& target_cmd, const c_angle& desired_view ) {
		if ( GET_VARIABLE( g_variables.m_movement_fix, bool ) ) {
			target_cmd.m_view_point = desired_view;
			start_movement_fix( &target_cmd );
			target_cmd.m_view_point = air_stuck_original_view;
			end_movement_fix( &target_cmd );
		} else {
			target_cmd.m_view_point = desired_view;
		}
	};

	m_air_stuck_data.reset( );
	m_air_stuck_data.m_wall_stuck = saved_m_wall_stuck;
	m_air_stuck_data.m_auto_align_block_until_tick = saved_auto_align_block_until_tick;

	if ( !GET_VARIABLE( g_variables.m_air_stuck, bool ) ) {
		reset_state( );
		return;
	}
	if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_air_stuck_key, key_bind_t ) ) ) {
		reset_state( );
		return;
	}
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) ) {
		reset_state( );
		return;
	}
	const int cur_tick = g_interfaces.m_global_vars_base->m_tick_count;
	if ( point_descend_release_latch ) {
		if ( point_descend_release_until_tick >= cur_tick ) {
			clear_air_stuck_movement( *cmd );
			return;
		}

		point_descend_release_latch      = false;
		point_descend_release_until_tick = -1;
	}
	if ( g_ctx.m_local->get_flags( ) & fl_onground ) {
		reset_state( );
		return;
	}

	const c_vector live_velocity = g_prediction.backup_data.m_velocity;
	constexpr float k_pi         = 3.14159265358979323846f;

	const float max_radius        = k_pi * 2.0f;
	const float step              = max_radius / 16.0f;
	const c_vector start_pos      = g_ctx.m_local->get_abs_origin( );
	const auto mins               = g_ctx.m_local->get_collideable( )->get_obb_mins( );
	const auto maxs               = g_ctx.m_local->get_collideable( )->get_obb_maxs( );
	const std::string current_map = g_interfaces.m_engine_client->get_level_name_short( );

	struct pixel_jump_target_t {
		bool m_found                = false;
		int m_index                 = -1;
		float m_horizontal_distance = FLT_MAX;
		float m_vertical_gap        = 0.0f;
		c_vector m_pos{ };
	};
	

	c_trace_filter_trace_type_everything_filter_props fil;
	trace_t primary_trace{ };
	bool wall_detected = false;

	const auto detect_wall = []( const c_vector& start_pos, const c_vector& mins, const c_vector& maxs, float start_circle, float max_radius,
	                             float step, c_trace_filter_trace_type_everything_filter_props& fil, trace_t& out_trace, float& out_angle ) -> bool {
		const float extra_reach          = GET_VARIABLE( g_variables.m_antoha_bug_extra_reach, float );
		constexpr float z_step           = 1.f;
		constexpr float normal_z_epsilon = 0.05f;

		const float half_x = std::max( std::fabs( mins.m_x ), std::fabs( maxs.m_x ) );
		const float half_y = std::max( std::fabs( mins.m_y ), std::fabs( maxs.m_y ) );

		for ( float a = start_circle; a < start_circle + max_radius; a += step ) {
			const float angle = std::fmod( a, max_radius );
			const float dir_x = std::cos( angle );
			const float dir_y = std::sin( angle );

			const float box_reach  = std::min( half_x / std::max( std::fabs( dir_x ), 1e-6f ), half_y / std::max( std::fabs( dir_y ), 1e-6f ) );
			const float ray_length = box_reach + extra_reach;

			const float z_start = start_pos.m_z + mins.m_z;
			const float z_end   = start_pos.m_z + maxs.m_z;

			trace_t best_trace{ };
			best_trace.m_fraction  = 1.0f;
			bool hit_at_this_angle = false;

			for ( float z = z_start; z <= z_end; z += z_step ) {
				c_vector ray_start{ start_pos.m_x, start_pos.m_y, z };
				c_vector ray_end{ start_pos.m_x + dir_x * ray_length, start_pos.m_y + dir_y * ray_length, z };

				trace_t trace{ };
				ray_t ray( ray_start, ray_end );
				g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &fil, &trace );

				if ( trace.m_fraction >= 1.0f )
					continue;

				if ( std::fabs( trace.m_plane.m_normal.m_z ) > normal_z_epsilon )
					continue;

				const float facing = trace.m_plane.m_normal.m_x * dir_x + trace.m_plane.m_normal.m_y * dir_y;
				if ( facing > -0.1f )
					continue;

				hit_at_this_angle = true;
				if ( trace.m_fraction < best_trace.m_fraction )
					best_trace = trace;
			}

			if ( hit_at_this_angle ) {
				out_trace = best_trace;
				out_angle = a;
				return true;
			}
		}

		return false;
	};

	float found_angle = 0.0f;
	if ( detect_wall( start_pos, mins, maxs, start_circle, max_radius, step, fil, primary_trace, found_angle ) ) {
		wall_detected = true;
		start_circle  = found_angle;
	}

	if ( !wall_detected ) {
		start_circle     = 0.0f;
		stuck_hold_ticks = 0;
		stuck_msg_sent   = false;
		return;
	}

	const c_vector approach_normal = primary_trace.m_plane.m_normal * -1.0f;
	const c_angle angle_wall       = approach_normal.to_angle( );

	const auto apply_predicted_state = [ & ]( ) noexcept {
		g_ctx.m_local->set_abs_origin( start_pos );
		g_ctx.m_local->get_origin( )   = start_pos;
		g_ctx.m_local->get_velocity( ) = live_velocity;
	};

	const auto will_land_after_release_tick = [ & ]( const c_user_cmd& release_cmd ) -> bool {
		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		c_user_cmd probe_cmd = release_cmd;
		g_prediction.begin( g_ctx.m_local, &probe_cmd );
		g_prediction.end( g_ctx.m_local );

		const bool will_land = ( g_ctx.m_local->get_flags( ) & fl_onground ) != 0;

		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		return will_land;
	};

	struct release_tick_outcome_t {
		bool m_will_land = false;
		c_vector m_origin{ };
		c_vector m_velocity{ };
	};

	const auto simulate_release_tick = [ & ]( const c_user_cmd& release_cmd ) -> release_tick_outcome_t {
		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		c_user_cmd probe_cmd = release_cmd;
		g_prediction.begin( g_ctx.m_local, &probe_cmd );
		g_prediction.end( g_ctx.m_local );

		release_tick_outcome_t result;
		result.m_will_land = ( g_ctx.m_local->get_flags( ) & fl_onground ) != 0;
		result.m_origin    = g_ctx.m_local->get_abs_origin( );
		result.m_velocity  = g_ctx.m_local->get_velocity( );

		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		return result;
	};

	auto get_align = [ & ]( ) -> float {
		trace_t trace_support{ };
		c_vector minusplane = primary_trace.m_plane.m_normal * -1.0f * 32.0f;

		auto origin = g_ctx.m_local->get_abs_origin( );
		ray_t ray2( c_vector( origin.m_x, origin.m_y, primary_trace.m_end.m_z ),
		            c_vector( origin.m_x + minusplane.m_x, origin.m_y + minusplane.m_y, primary_trace.m_end.m_z ) );

		c_trace_filter_trace_type_everything_filter_props wlft;
		g_interfaces.m_engine_trace->trace_ray( ray2, mask_all, &wlft, &trace_support );

		const c_vector wall_normal( primary_trace.m_plane.m_normal.m_x, primary_trace.m_plane.m_normal.m_y, 0.0f );

		return trace_support.m_fraction * 32.0f - GetWallSupportDistance( g_ctx.m_local, wall_normal );
	};

	const float align = get_align( );

	if ( align > 0.0f ) {
		stuck_hold_ticks = 0;
		stuck_msg_sent   = false;
		c_angle desired_view = cmd->m_view_point;
		desired_view.m_y     = angle_wall.m_y;
		cmd->m_forward_move  = 5.0f;
		cmd->m_side_move     = 0.0f;
		apply_air_stuck_view( *cmd, desired_view );
		return;
	}

	if ( g_prediction.is_target_predict_z_velocity( g_prediction.backup_data.m_velocity.m_z ) ) {
		m_air_stuck_data.m_air_stuck = true;
		stuck_hold_ticks++;

		c_user_cmd hold_cmd = backup_cmd;
		if ( stuck_last_yaw != FLT_MAX && stuck_last_move != FLT_MAX ) {
			hold_cmd.m_view_point.m_y = stuck_last_yaw;
			hold_cmd.m_view_point.normalize( ).clamp( );
			hold_cmd.m_forward_move = stuck_last_move;
			hold_cmd.m_side_move    = stuck_last_side;
			hold_cmd.m_up_move      = 0.0f;
			refresh_air_stuck_buttons( hold_cmd );
		}

		descend_wait_tick = false;

		constexpr int k_stable_ticks = 12;
		if ( !stuck_msg_sent ) {
			if ( stuck_hold_ticks >= k_stable_ticks ) {
				if ( GET_VARIABLE( g_variables.m_air_stuck_display_screen, bool ) )
					myWindowDetect.AddWindow( 5, std::format( "stucked on: move: {:.4f} yaw: {:.4f}", stuck_last_move, stuck_last_yaw ) );
				stuck_msg_sent = true;
			}
		}

		const c_angle desired_view = hold_cmd.m_view_point;
		cmd->m_forward_move = hold_cmd.m_forward_move;
		cmd->m_side_move    = hold_cmd.m_side_move;
		cmd->m_up_move      = hold_cmd.m_up_move;
		cmd->m_buttons      = merge_air_stuck_action_buttons( live_cmd.m_buttons, hold_cmd.m_buttons );
		apply_air_stuck_view( *cmd, desired_view );
		backup_cmd          = hold_cmd;

		return;
	}

	m_air_stuck_data.m_air_stuck = false;
	stuck_hold_ticks             = 0;
	stuck_msg_sent               = false;

	const float side = ( cur_tick % 2 == 0 ) ? 1.0f : -1.0f;

	float best_yaw          = angle_wall.m_y;
	float best_align_pred   = 0.0f;
	float best_forward_move = 450.0f;
	int best_buttons        = cmd->m_buttons;
	float best_rel_ang      = 0.0f;
	bool first              = true;
	bool displayed_message  = false;

	const auto consider_fallback = [ & ]( const float rel_ang, const float forward_move, const float side_sign ) {
		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		cmd->m_view_point.m_y = std::remainder( angle_wall.m_y + rel_ang * side_sign, 360.0f );
		cmd->m_forward_move   = forward_move;
		cmd->m_side_move      = 0.0f;
		cmd->m_buttons &= ~in_duck;

		g_prediction.begin( g_ctx.m_local, cmd );
		g_prediction.end( g_ctx.m_local );
		c_vector new_velo = g_ctx.m_local->get_velocity( );

		const bool is_max_forward_move = fabsf( forward_move - 450.0f ) <= 0.001f;
		if ( is_max_forward_move ) {
			if ( g_prediction.is_target_predict_z_velocity( new_velo.m_z ) ) {
				best_align_pred   = -FLT_MAX;
				best_yaw          = cmd->m_view_point.m_y;
				best_forward_move = forward_move;
				best_rel_ang      = rel_ang;
				best_buttons      = cmd->m_buttons;
				first             = false;
				displayed_message = true;
			}

			return;
		}

		const float predicted_align = get_align( );

		if ( g_prediction.is_target_predict_z_velocity( new_velo.m_z ) ) {
			// g_prediction.restore_entity_to_predicted_frame( -1 ); shit niga
			// apply_predicted_state( );

			g_prediction.begin( g_ctx.m_local, cmd );
			g_prediction.end( g_ctx.m_local );
			new_velo = g_ctx.m_local->get_velocity( );

			if ( g_prediction.is_target_predict_z_velocity( new_velo.m_z ) ) {
				best_align_pred   = -FLT_MAX;
				best_yaw          = cmd->m_view_point.m_y;
				best_forward_move = forward_move;
				best_rel_ang      = rel_ang;
				best_buttons      = cmd->m_buttons;
				first             = false;
				displayed_message = true;
				return;
			}
		}

		if ( first || predicted_align < best_align_pred ) {
			best_align_pred   = predicted_align;
			best_yaw          = cmd->m_view_point.m_y;
			best_forward_move = forward_move;
			best_rel_ang      = rel_ang;
			best_buttons      = cmd->m_buttons;
			first             = false;
		}
	};

	constexpr std::array< float, 9 > k_forward_move_logged_values = { 1.f, 1.3856f, 5.f, 10.f, 22.f, 30.f, 60.f, 120.f, 450.f };

	for ( float ang = 88.0f; ang <= 89.999f; ang += 0.111f ) {
		for ( const float mul : k_forward_move_logged_values )
			consider_fallback( ang, mul, side );
	}

	g_prediction.restore_entity_to_predicted_frame( -1 );

	c_angle desired_view = cmd->m_view_point;
	desired_view.m_y     = best_yaw;
	cmd->m_forward_move   = best_forward_move;
	cmd->m_side_move      = 0.0f;
	cmd->m_buttons        = merge_air_stuck_action_buttons( live_cmd.m_buttons, best_buttons );
	apply_air_stuck_view( *cmd, desired_view );

	if ( displayed_message ) {
		stuck_last_yaw              = best_yaw;
		stuck_last_move             = best_forward_move;
		stuck_last_side             = 0.0f;
		stuck_last_ang              = best_rel_ang;
		backup_cmd                  = *cmd;
		backup_cmd.m_view_point.m_y = best_yaw;
		backup_cmd.m_view_point.normalize( ).clamp( );
		backup_cmd.m_forward_move = best_forward_move;
		backup_cmd.m_side_move    = 0.0f;
		backup_cmd.m_up_move      = 0.0f;
		refresh_air_stuck_buttons( backup_cmd );
	}
}




void n_movement::impl_t::wall_stuck( c_user_cmd* cmd )
{
	static c_user_cmd backup_cmd{ };
	static c_user_cmd optimal_cmd{ };
	static bool have_saved_cmd                  = false;
	static bool have_optimal_cmd                = false;
	static float start_circle                   = 0.0f;
	static int armed_tick                       = -1;
	static int lock_until_tick                  = -1;
	static int last_bruteforce_tick             = -1000;
	static int optimal_fail_ticks               = 0;
	static int hold_bad_ticks                   = 0;
	static bool have_last_live_vel              = false;
	static int stuck_hold_ticks                 = 0;
	static bool stuck_msg_sent                  = false;
	static bool descend_wait_tick               = false;
	static bool point_descend_release_latch     = false;
	static int point_descend_release_until_tick = -1;
	static float stuck_last_yaw                 = FLT_MAX;
	static float stuck_last_move                = FLT_MAX;
	static float stuck_last_side                = 0.0f;
	static float stuck_last_ang                 = FLT_MAX;

	auto reset_state = [ & ]( ) noexcept {
		const int saved_auto_align_block_until_tick = m_air_stuck_data.m_auto_align_block_until_tick;
		const bool saved_m_air_stuck = m_air_stuck_data.m_air_stuck;
		have_saved_cmd                              = false;
		have_optimal_cmd                            = false;
		start_circle                                = 0.0f;
		armed_tick                                  = -1;
		lock_until_tick                             = -1;
		last_bruteforce_tick                        = -1000;
		optimal_fail_ticks                          = 0;
		hold_bad_ticks                              = 0;
		have_last_live_vel                          = false;
		stuck_hold_ticks                            = 0;
		stuck_msg_sent                              = false;
		descend_wait_tick                           = false;
		point_descend_release_latch                 = false;
		point_descend_release_until_tick            = -1;
		stuck_last_yaw                              = FLT_MAX;
		stuck_last_move                             = FLT_MAX;
		stuck_last_side                             = 0.0f;
		stuck_last_ang                              = FLT_MAX;
		m_air_stuck_data.reset( );
		m_air_stuck_data.m_air_stuck = saved_m_air_stuck;
		m_air_stuck_data.m_auto_align_block_until_tick = saved_auto_align_block_until_tick;
	};

	auto refresh_air_stuck_buttons = []( c_user_cmd& target_cmd ) noexcept {
		target_cmd.m_buttons &= ~( in_moveright | in_moveleft | in_back | in_forward | in_duck );

		if ( target_cmd.m_side_move < 0.0f ) {
			target_cmd.m_buttons |= in_moveleft;
		} else if ( target_cmd.m_side_move > 0.0f ) {
			target_cmd.m_buttons |= in_moveright;
		}

		if ( target_cmd.m_forward_move < 0.0f ) {
			target_cmd.m_buttons |= in_back;
		} else if ( target_cmd.m_forward_move > 0.0f ) {
			target_cmd.m_buttons |= in_forward;
		}
	};

	auto clear_air_stuck_movement = []( c_user_cmd& target_cmd ) noexcept {
		target_cmd.m_forward_move = 0.0f;
		target_cmd.m_side_move    = 0.0f;
		target_cmd.m_up_move      = 0.0f;
		target_cmd.m_buttons &= ~( in_moveright | in_moveleft | in_back | in_forward | in_duck | in_jump );
	};

	auto merge_air_stuck_action_buttons = []( const int live_buttons, const int stuck_buttons ) noexcept {
		constexpr int movement_mask = in_moveright | in_moveleft | in_back | in_forward | in_duck | in_jump;
		return ( stuck_buttons & movement_mask ) | ( live_buttons & ~movement_mask );
	};

	const c_user_cmd live_cmd                   = *cmd;
	const int saved_auto_align_block_until_tick = m_air_stuck_data.m_auto_align_block_until_tick;
	const bool saved_m_air_stuck = m_air_stuck_data.m_air_stuck;
	const c_angle air_stuck_original_view       = live_cmd.m_view_point;

	auto apply_air_stuck_view = [ & ]( c_user_cmd& target_cmd, const c_angle& desired_view ) {
		if ( GET_VARIABLE( g_variables.m_movement_fix, bool ) ) {
			target_cmd.m_view_point = desired_view;
			start_movement_fix( &target_cmd );
			target_cmd.m_view_point = air_stuck_original_view;
			end_movement_fix( &target_cmd );
		} else {
			target_cmd.m_view_point = desired_view;
		}
	};

	m_air_stuck_data.reset( );
	m_air_stuck_data.m_air_stuck = saved_m_air_stuck;
	m_air_stuck_data.m_auto_align_block_until_tick = saved_auto_align_block_until_tick;

	if ( !GET_VARIABLE( g_variables.m_wall_stuck, bool ) ) {
		reset_state( );
		return;
	}
	if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_wall_stuck_key, key_bind_t ) ) ) {
		reset_state( );
		return;
	}
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) ) {
		reset_state( );
		return;
	}
	const int cur_tick = g_interfaces.m_global_vars_base->m_tick_count;
	if ( point_descend_release_latch ) {
		if ( point_descend_release_until_tick >= cur_tick ) {
			clear_air_stuck_movement( *cmd );
			return;
		}

		point_descend_release_latch      = false;
		point_descend_release_until_tick = -1;
	}
	if ( g_ctx.m_local->get_flags( ) & fl_onground ) {
		reset_state( );
		return;
	}

	const c_vector live_velocity = g_prediction.backup_data.m_velocity;
	constexpr float k_pi         = 3.14159265358979323846f;

	const float max_radius        = k_pi * 2.0f;
	const float step              = max_radius / 16.0f;
	const c_vector start_pos      = g_ctx.m_local->get_abs_origin( );
	const auto mins               = g_ctx.m_local->get_collideable( )->get_obb_mins( );
	const auto maxs               = g_ctx.m_local->get_collideable( )->get_obb_maxs( );
	const std::string current_map = g_interfaces.m_engine_client->get_level_name_short( );

	struct pixel_jump_target_t {
		bool m_found                = false;
		int m_index                 = -1;
		float m_horizontal_distance = FLT_MAX;
		float m_vertical_gap        = 0.0f;
		c_vector m_pos{ };
	};

	const auto find_pixel_jump_target = [ & ]( ) -> pixel_jump_target_t {
		pixel_jump_target_t result{ };

		for ( int i = 0; i < static_cast< int >( m_points_check.size( ) ); ++i ) {
			const auto& point              = m_points_check.at( i );
	

			if ( !point.active ||  point.map != current_map )
				continue;

			const float horizontal_distance = start_pos.dist_to_2d( point.pos );
			const float capture_radius      = std::max( point.radius, 96.0f );
			if ( horizontal_distance > capture_radius )
				continue;

			const float vertical_gap = start_pos.m_z - point.pos.m_z;
			if ( vertical_gap < -16.0f )
				continue;

			if ( !result.m_found || horizontal_distance < result.m_horizontal_distance ||
			     ( std::fabs( horizontal_distance - result.m_horizontal_distance ) <= 0.001f && vertical_gap < result.m_vertical_gap ) ) {
				result.m_found               = true;
				result.m_index               = i;
				result.m_horizontal_distance = horizontal_distance;
				result.m_vertical_gap        = vertical_gap;
				result.m_pos                 = point.pos;
			}
		}

		return result;
	};

	

	c_trace_filter_trace_type_everything_filter_props fil;
	trace_t primary_trace{ };
	bool wall_detected = false;

	const auto detect_wall = []( const c_vector& start_pos, const c_vector& mins, const c_vector& maxs, float start_circle, float max_radius,
	                             float step, c_trace_filter_trace_type_everything_filter_props& fil, trace_t& out_trace, float& out_angle ) -> bool {
		const float extra_reach          = GET_VARIABLE( g_variables.m_antoha_bug_extra_reach, float );
		constexpr float z_step           = 1.f;
		constexpr float normal_z_epsilon = 0.05f;

		const float half_x = std::max( std::fabs( mins.m_x ), std::fabs( maxs.m_x ) );
		const float half_y = std::max( std::fabs( mins.m_y ), std::fabs( maxs.m_y ) );

		for ( float a = start_circle; a < start_circle + max_radius; a += step ) {
			const float angle = std::fmod( a, max_radius );
			const float dir_x = std::cos( angle );
			const float dir_y = std::sin( angle );

			const float box_reach  = std::min( half_x / std::max( std::fabs( dir_x ), 1e-6f ), half_y / std::max( std::fabs( dir_y ), 1e-6f ) );
			const float ray_length = box_reach + extra_reach;

			const float z_start = start_pos.m_z + mins.m_z;
			const float z_end   = start_pos.m_z + maxs.m_z;

			trace_t best_trace{ };
			best_trace.m_fraction  = 1.0f;
			bool hit_at_this_angle = false;

			for ( float z = z_start; z <= z_end; z += z_step ) {
				c_vector ray_start{ start_pos.m_x, start_pos.m_y, z };
				c_vector ray_end{ start_pos.m_x + dir_x * ray_length, start_pos.m_y + dir_y * ray_length, z };

				trace_t trace{ };
				ray_t ray( ray_start, ray_end );
				g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &fil, &trace );

				if ( trace.m_fraction >= 1.0f )
					continue;

				if ( std::fabs( trace.m_plane.m_normal.m_z ) > normal_z_epsilon )
					continue;

				const float facing = trace.m_plane.m_normal.m_x * dir_x + trace.m_plane.m_normal.m_y * dir_y;
				if ( facing > -0.1f )
					continue;

				hit_at_this_angle = true;
				if ( trace.m_fraction < best_trace.m_fraction )
					best_trace = trace;
			}

			if ( hit_at_this_angle ) {
				out_trace = best_trace;
				out_angle = a;
				return true;
			}
		}

		return false;
	};

	float found_angle = 0.0f;
	if ( detect_wall( start_pos, mins, maxs, start_circle, max_radius, step, fil, primary_trace, found_angle ) ) {
		wall_detected = true;
		start_circle  = found_angle;
	}

	if ( !wall_detected ) {
		start_circle     = 0.0f;
		stuck_hold_ticks = 0;
		stuck_msg_sent   = false;
		return;
	}

	const c_vector approach_normal = primary_trace.m_plane.m_normal * -1.0f;
	const c_angle angle_wall       = approach_normal.to_angle( );

	const auto apply_predicted_state = [ & ]( ) noexcept {
		g_ctx.m_local->set_abs_origin( start_pos );
		g_ctx.m_local->get_origin( )   = start_pos;
		g_ctx.m_local->get_velocity( ) = live_velocity;
	};

	const auto will_land_after_release_tick = [ & ]( const c_user_cmd& release_cmd ) -> bool {
		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		c_user_cmd probe_cmd = release_cmd;
		g_prediction.begin( g_ctx.m_local, &probe_cmd );
		g_prediction.end( g_ctx.m_local );

		const bool will_land = ( g_ctx.m_local->get_flags( ) & fl_onground ) != 0;

		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		return will_land;
	};

	struct release_tick_outcome_t {
		bool m_will_land = false;
		c_vector m_origin{ };
		c_vector m_velocity{ };
	};

	const auto simulate_release_tick = [ & ]( const c_user_cmd& release_cmd ) -> release_tick_outcome_t {
		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		c_user_cmd probe_cmd = release_cmd;
		g_prediction.begin( g_ctx.m_local, &probe_cmd );
		g_prediction.end( g_ctx.m_local );

		release_tick_outcome_t result;
		result.m_will_land = ( g_ctx.m_local->get_flags( ) & fl_onground ) != 0;
		result.m_origin    = g_ctx.m_local->get_abs_origin( );
		result.m_velocity  = g_ctx.m_local->get_velocity( );

		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		return result;
	};

	auto get_align = [ & ]( ) -> float {
		trace_t trace_support{ };
		c_vector minusplane = primary_trace.m_plane.m_normal * -1.0f * 32.0f;

		auto origin = g_ctx.m_local->get_abs_origin( );
		ray_t ray2( c_vector( origin.m_x, origin.m_y, primary_trace.m_end.m_z ),
		            c_vector( origin.m_x + minusplane.m_x, origin.m_y + minusplane.m_y, primary_trace.m_end.m_z ) );

		c_trace_filter_trace_type_everything_filter_props wlft;
		g_interfaces.m_engine_trace->trace_ray( ray2, mask_all, &wlft, &trace_support );

		const c_vector wall_normal( primary_trace.m_plane.m_normal.m_x, primary_trace.m_plane.m_normal.m_y, 0.0f );

		return trace_support.m_fraction * 32.0f - GetWallSupportDistance( g_ctx.m_local, wall_normal );
	};

	const float align = get_align( );

	if ( align > 0.0f ) {
		stuck_hold_ticks = 0;
		stuck_msg_sent   = false;
		c_angle desired_view = cmd->m_view_point;
		desired_view.m_y     = angle_wall.m_y;
		cmd->m_forward_move  = 5.0f;
		cmd->m_side_move     = 0.0f;
		apply_air_stuck_view( *cmd, desired_view );
		return;
	}

	if ( g_prediction.is_target_predict_z_velocity( g_prediction.backup_data.m_velocity.m_z ) ) {
		m_air_stuck_data.m_wall_stuck = true;
		stuck_hold_ticks++;

		c_user_cmd hold_cmd = backup_cmd;
		if ( stuck_last_yaw != FLT_MAX && stuck_last_move != FLT_MAX ) {
			hold_cmd.m_view_point.m_y = stuck_last_yaw;
			hold_cmd.m_view_point.normalize( ).clamp( );
			hold_cmd.m_forward_move = stuck_last_move;
			hold_cmd.m_side_move    = stuck_last_side;
			hold_cmd.m_up_move      = 0.0f;
			refresh_air_stuck_buttons( hold_cmd );
		}

		descend_wait_tick = false;

		constexpr int k_stable_ticks = 12;
		if ( !stuck_msg_sent ) {
			if ( stuck_hold_ticks >= k_stable_ticks ) {
				if ( GET_VARIABLE( g_variables.m_wall_stuck_display_screen, bool ) )
					myWindowDetect.AddWindow( 5, std::format( "stucked on: move: {:.4f} yaw: {:.4f}", stuck_last_move, stuck_last_yaw ) );
				stuck_msg_sent = true;
			}
		}

		const c_angle desired_view = hold_cmd.m_view_point;
		cmd->m_forward_move = hold_cmd.m_forward_move;
		cmd->m_side_move    = hold_cmd.m_side_move;
		cmd->m_up_move      = hold_cmd.m_up_move;
		cmd->m_buttons      = merge_air_stuck_action_buttons( live_cmd.m_buttons, hold_cmd.m_buttons );
		apply_air_stuck_view( *cmd, desired_view );
		backup_cmd          = hold_cmd;

		return;
	}

	m_air_stuck_data.m_wall_stuck = false;
	stuck_hold_ticks             = 0;
	stuck_msg_sent               = false;

	const float side = ( cur_tick % 2 == 0 ) ? 1.0f : -1.0f;

	float best_yaw          = angle_wall.m_y;
	float best_align_pred   = 0.0f;
	float best_forward_move = 450.0f;
	int best_buttons        = cmd->m_buttons;
	float best_rel_ang      = 0.0f;
	bool first              = true;
	bool displayed_message  = false;

	const auto consider_fallback = [ & ]( const float rel_ang, const float forward_move, const float side_sign ) {
		g_prediction.restore_entity_to_predicted_frame( -1 );
		apply_predicted_state( );

		cmd->m_view_point.m_y = std::remainder( angle_wall.m_y + rel_ang * side_sign, 360.0f );
		cmd->m_forward_move   = forward_move;
		cmd->m_side_move      = 0.0f;
		cmd->m_buttons &= ~in_duck;

		g_prediction.begin( g_ctx.m_local, cmd );
		g_prediction.end( g_ctx.m_local );
		c_vector new_velo = g_ctx.m_local->get_velocity( );

		const bool is_max_forward_move = fabsf( forward_move - 450.0f ) <= 0.001f;
		if ( is_max_forward_move ) {
			if ( g_prediction.is_target_predict_z_velocity( new_velo.m_z ) ) {
				best_align_pred   = -FLT_MAX;
				best_yaw          = cmd->m_view_point.m_y;
				best_forward_move = forward_move;
				best_rel_ang      = rel_ang;
				best_buttons      = cmd->m_buttons;
				first             = false;
				displayed_message = true;
			}

			return;
		}

		const float predicted_align = get_align( );

		if ( g_prediction.is_target_predict_z_velocity( new_velo.m_z ) ) {
			// g_prediction.restore_entity_to_predicted_frame( -1 ); shit niga
			// apply_predicted_state( );

			g_prediction.begin( g_ctx.m_local, cmd );
			g_prediction.end( g_ctx.m_local );
			new_velo = g_ctx.m_local->get_velocity( );

			if ( g_prediction.is_target_predict_z_velocity( new_velo.m_z ) ) {
				best_align_pred   = -FLT_MAX;
				best_yaw          = cmd->m_view_point.m_y;
				best_forward_move = forward_move;
				best_rel_ang      = rel_ang;
				best_buttons      = cmd->m_buttons;
				first             = false;
				displayed_message = true;
				return;
			}
		}

		if ( first || predicted_align < best_align_pred ) {
			best_align_pred   = predicted_align;
			best_yaw          = cmd->m_view_point.m_y;
			best_forward_move = forward_move;
			best_rel_ang      = rel_ang;
			best_buttons      = cmd->m_buttons;
			first             = false;
		}
	};

	constexpr std::array< float, 9 > k_forward_move_logged_values = { 1.f, 1.3856f, 5.f, 10.f, 22.f, 30.f, 60.f, 120.f, 450.f };

	for ( float ang = 88.0f; ang <= 89.999f; ang += 0.111f ) {
		for ( const float mul : k_forward_move_logged_values )
			consider_fallback( ang, mul, side );
	}

	g_prediction.restore_entity_to_predicted_frame( -1 );

	c_angle desired_view = cmd->m_view_point;
	desired_view.m_y     = best_yaw;
	cmd->m_forward_move   = best_forward_move;
	cmd->m_side_move      = 0.0f;
	cmd->m_buttons        = merge_air_stuck_action_buttons( live_cmd.m_buttons, best_buttons );
	apply_air_stuck_view( *cmd, desired_view );

	if ( displayed_message ) {
		stuck_last_yaw              = best_yaw;
		stuck_last_move             = best_forward_move;
		stuck_last_side             = 0.0f;
		stuck_last_ang              = best_rel_ang;
		backup_cmd                  = *cmd;
		backup_cmd.m_view_point.m_y = best_yaw;
		backup_cmd.m_view_point.normalize( ).clamp( );
		backup_cmd.m_forward_move = best_forward_move;
		backup_cmd.m_side_move    = 0.0f;
		backup_cmd.m_up_move      = 0.0f;
		refresh_air_stuck_buttons( backup_cmd );
	}
}




void n_movement::impl_t::antoha_bug_land( c_user_cmd* cmd )
{
	if ( !GET_VARIABLE( g_variables.m_antoha_bug_land, bool ) )
		return;
	if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_antoha_bug_land_key, key_bind_t ) ) )
		return;
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return;

	// --- pending land forward: apply stored move on the ground tick ---
	static bool  s_pending_land   = false;
	static float s_pending_fwd    = 0.f;
	static float s_pending_side   = 0.f;
	static float s_pending_yaw    = 0.f;

	const bool local_on_ground   = ( g_ctx.m_local->get_flags( ) & fl_onground ) != 0;
	const bool user_holding_jump = ( cmd->m_buttons & in_jump ) != 0;

	if ( local_on_ground ) {
		if ( s_pending_land ) {
			const float delta = deg2rad( g_math.normalize_angle( s_pending_yaw - cmd->m_view_point.m_y ) );
			cmd->m_forward_move = s_pending_fwd * std::cos( delta ) + s_pending_side * std::sin( delta );
			cmd->m_side_move    = -s_pending_fwd * std::sin( delta ) + s_pending_side * std::cos( delta );
			s_pending_land = false;
			g_movement.m_air_stuck_data.m_antoha_bug_detect = true;
			return;
		}
		if ( !user_holding_jump ) {
			g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
			return;
		}
	}

	if ( g_ctx.m_local->get_velocity( ).m_z >= 0.f ) {
		g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
		return;
	}

	const c_vector live_velocity = g_prediction.backup_data.m_velocity;
	const float live_speed_2d    = live_velocity.length_2d( );
	const c_vector start_pos     = g_ctx.m_local->get_abs_origin( );
	const auto mins              = g_ctx.m_local->get_collideable( )->get_obb_mins( );
	const auto maxs              = g_ctx.m_local->get_collideable( )->get_obb_maxs( );
	constexpr float k_pi         = 3.14159265358979323846f;
	const float max_radius       = k_pi * 2.0f;
	const float step             = max_radius / 16.0f;
	static float start_circle    = 0.f;
	c_trace_filter_trace_type_everything_filter_props fil;
	trace_t primary_trace{ };
	bool wall_detected = false;

	// --- wall detection (same as antoha_bug) ---
	{
		constexpr float extra_reach      = 2.f;
		constexpr float z_step           = 1.f;
		constexpr float normal_z_epsilon = 0.35f;
		const float half_x = std::max( std::fabs( mins.m_x ), std::fabs( maxs.m_x ) );
		const float half_y = std::max( std::fabs( mins.m_y ), std::fabs( maxs.m_y ) );

		for ( float a = start_circle; a < start_circle + max_radius; a += step ) {
			const float angle      = std::fmod( a, max_radius );
			const float dir_x      = std::cos( angle );
			const float dir_y      = std::sin( angle );
			const float box_reach  = std::min( half_x / std::max( std::fabs( dir_x ), 1e-6f ), half_y / std::max( std::fabs( dir_y ), 1e-6f ) );
			const float ray_length = box_reach + extra_reach;
			const float z_start    = start_pos.m_z + mins.m_z;
			const float z_end      = start_pos.m_z + maxs.m_z;
			trace_t best_t{ };
			best_t.m_fraction      = 1.0f;
			bool hit               = false;
			for ( float z = z_start; z <= z_end; z += z_step ) {
				c_vector rs{ start_pos.m_x, start_pos.m_y, z };
				c_vector re{ start_pos.m_x + dir_x * ray_length, start_pos.m_y + dir_y * ray_length, z };
				trace_t tr{ };
				g_interfaces.m_engine_trace->trace_ray( ray_t( rs, re ), mask_playersolid, &fil, &tr );
				if ( tr.m_fraction >= 1.0f ) continue;
				if ( std::fabs( tr.m_plane.m_normal.m_z ) > normal_z_epsilon ) continue;
				const float facing = tr.m_plane.m_normal.m_x * dir_x + tr.m_plane.m_normal.m_y * dir_y;
				if ( facing > -0.1f ) continue;
				hit = true;
				if ( tr.m_fraction < best_t.m_fraction ) best_t = tr;
			}
			if ( hit ) {
				primary_trace = best_t;
				start_circle  = a;
				wall_detected = true;
				break;
			}
		}
	}

	if ( !wall_detected ) {
		start_circle = 0.f;
		g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
		return;
	}

	// --- predict one tick to check if player will land ---
	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );

	struct land_guard_t {
		~land_guard_t( ) { g_prediction.restore_entity_to_predicted_frame( -1 ); }
	} land_guard;

	const auto apply_state = [ & ]( ) noexcept {
		g_ctx.m_local->set_abs_origin( start_pos );
		g_ctx.m_local->get_origin( )   = start_pos;
		g_ctx.m_local->get_velocity( ) = live_velocity;
	};

	apply_state( );
	c_user_cmd neutral_cmd   = *cmd;
	neutral_cmd.m_forward_move = 0.f;
	neutral_cmd.m_side_move    = 0.f;
	g_prediction.begin( g_ctx.m_local, &neutral_cmd );
	g_prediction.end( g_ctx.m_local );
	const bool will_land = ( g_ctx.m_local->get_flags( ) & fl_onground ) != 0;

	if ( !will_land ) {
		g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
		return;
	}

	// --- landing detected: find best forwardmove towards wall ---
	const c_vector approach_normal = primary_trace.m_plane.m_normal * -1.0f;
	c_angle angle_wall             = approach_normal.to_angle( );
	angle_wall.normalize( );

	auto get_pidoras = [ & ]( ) -> float {
		trace_t ts{ };
		c_vector mp = primary_trace.m_plane.m_normal * -1.0f * 32.0f;
		auto orig   = g_ctx.m_local->get_abs_origin( );
		ray_t ray2( c_vector( orig.m_x, orig.m_y, primary_trace.m_end.m_z ),
		            c_vector( orig.m_x + mp.m_x, orig.m_y + mp.m_y, primary_trace.m_end.m_z ) );
		c_trace_filter_trace_type_everything_filter_props wlft;
		g_interfaces.m_engine_trace->trace_ray( ray2, mask_all, &wlft, &ts );
		const c_vector wn( primary_trace.m_plane.m_normal.m_x, primary_trace.m_plane.m_normal.m_y, 0.0f );
		return ts.m_fraction * 32.0f - GetWallSupportDistance( g_ctx.m_local, wn );
	};

	c_vector velo = g_prediction.backup_data.m_velocity;
	float vel_yaw = rad2deg( atan2f( velo.m_y, velo.m_x ) );
	float rel     = g_math.normalize_angle( vel_yaw - angle_wall.m_y );
	float sign    = ( rel >= 0.f ) ? -1.f : 1.f;

	float best_pidoras     = 1.f;
	float best_fwd         = 0.f;
	float best_side        = 0.f;
	float best_yaw         = angle_wall.m_y;
	const int orig_buttons = cmd->m_buttons;
	bool found_best        = false;

	const int stamina_cap = GET_VARIABLE( g_variables.m_antoha_bug_max_stamina, int );
	constexpr float k_default_standable_normal_z   = 0.7f;
	constexpr float k_post_hit_positive_z_epsilon  = 0.1f;
	constexpr float k_post_hit_validate_time       = 0.2f;
	constexpr float k_min_valid_predicted_speed_2d = 1.0f;

	const auto has_default_standable_ground = [ & ]( ) noexcept -> bool {
		if ( ( g_ctx.m_local->get_flags( ) & fl_onground ) == 0 )
			return false;

		trace_t trace{ };
		c_trace_filter filter( g_ctx.m_local );
		const c_vector origin = g_ctx.m_local->get_abs_origin( );
		ray_t ray( origin + c_vector( 0.0f, 0.0f, 2.0f ), origin - c_vector( 0.0f, 0.0f, 8.0f ), mins, maxs );
		g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &filter, &trace );
		return trace.m_fraction < 1.0f && trace.m_plane.m_normal.m_z >= k_default_standable_normal_z;
	};

	const auto validate_land_candidate = [ & ]( ) noexcept -> bool {
		const int validate_ticks =
			std::max( 1, static_cast< int >( std::ceil( k_post_hit_validate_time / g_interfaces.m_global_vars_base->m_interval_per_tick ) ) );

		for ( int tick = 0; tick < validate_ticks; ++tick ) {
			const c_vector predicted_velocity = g_ctx.m_local->get_velocity( );
			if ( predicted_velocity.length_2d( ) < k_min_valid_predicted_speed_2d )
				return false;
			if ( predicted_velocity.m_z > k_post_hit_positive_z_epsilon )
				return false;
			if ( has_default_standable_ground( ) )
				return true;

			g_prediction.begin( g_ctx.m_local, cmd );
			g_prediction.end( g_ctx.m_local );
		}

		return false;
	};

	for ( float mul = 0.125f; mul <= 5.f + 0.0001f; mul += 0.125f ) {
		for ( float ang = 85.999f; ang < 90.f; ang += 1.f ) {
			const float cur_yaw = angle_wall.m_y + ang * sign;

			apply_state( );
			if ( stamina_cap > 0 )
				g_ctx.m_local->get_stamina( ) = std::max( g_ctx.m_local->get_stamina( ), static_cast< float >( stamina_cap ) );

			cmd->m_buttons      = orig_buttons;
			const float delta_pred = deg2rad( g_math.normalize_angle( cur_yaw - cmd->m_view_point.m_y ) );
			cmd->m_forward_move = mul * std::cos( delta_pred );
			cmd->m_side_move    = -mul * std::sin( delta_pred );

			g_prediction.begin( g_ctx.m_local, cmd );
			g_prediction.end( g_ctx.m_local );

			const float spd2d = g_ctx.m_local->get_velocity( ).length_2d( );
			if ( spd2d < k_min_valid_predicted_speed_2d ) continue;
			if ( !validate_land_candidate( ) ) continue;

			const float p = get_pidoras( );
			if ( p < best_pidoras ) {
				best_pidoras = p;
				best_fwd     = mul;
				best_side    = 0.f;
				best_yaw     = cur_yaw;
				found_best   = true;
			}
		}
	}

	apply_state( );
	if ( !found_best ) {
		g_movement.m_air_stuck_data.m_antoha_bug_detect = false;
		return;
	}

	// --- store the result for application on the NEXT tick (on ground) ---
	s_pending_land = true;
	s_pending_fwd  = best_fwd;
	s_pending_side = best_side;
	s_pending_yaw  = best_yaw;

	g_movement.m_air_stuck_data.m_antoha_bug_detect = true;

	// current tick (air): do not override movement, let player fall naturally
	cmd->m_buttons = orig_buttons;
}



// --------------------------------
//
// "fireman" is a mechanic that takes advantage of a cs:go bug when you enter a ladder while holding space.
//
// when we first grab the ladder, "OnStartMoveTypeLadder" is called which sets
// "player->m_ignoreLadderJumpTime = gpGlobals->curtime + IGNORE_JUMP_TIME"
// giving us 0.2 seconds of time before you can even jump off the ladder after grabbing onto it.
//
// while your movetype is "MOVETYPE_LADDER" your gravity is set to zero and because we are holding space
// and the aformentioned jump cooldown is not over yet, we basically have 0.2 seconds of zero gravity
// where we maintain our initial velocity when we entered the ladder and cannot take fall damage.
//
// "auto fireman" abuses this mechanic by grabbing the ladder at a point ~ 0.2 seconds above the ground
// (the ladder cooldown). you slide down at constant velocity and hit the ground, taking no fall damage
// (because they're on the ladder still). the player then has the ability to jump off the ladder
// near-instantaneously since the majority of the ladder cooldown was spent gliding downwards.
//
// --------------------------------
void n_movement::impl_t::fire_man( c_user_cmd* cmd )
{
	m_fireman_data.is_ladder = false;

	// Предварительные проверки.
	if ( !g_ctx.m_local )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;

	if ( !GET_VARIABLE( g_variables.m_fire_man, bool ) || !g_input.check_input( &GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ) ) ) {
		m_fireman_data.fr_hit   = false;
		m_fireman_data.fr_hit_1 = false;
		return;
	}

	if ( m_fireman_data.fr_hit_1 ) {
		if ( g_prediction.backup_data.m_move_type == move_type_ladder ) {
			cmd->m_buttons |= in_jump;
			cmd->m_forward_move = 0.f;
			cmd->m_side_move    = 0.f;
		}
	}

	auto colidable = g_ctx.m_local->get_collideable( );
	if ( !colidable )
		return;

	// Восстановление предыдущего состояния.
	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
	m_fireman_data.awall = false;
	trace_t trace;
	float step                = std::numbers::pi_v< float > * 2.0f / 16.f;
	static float start_circle = 0.f;

	if ( g_ctx.m_local->get_velocity( ).m_z != -6.25f && g_prediction.backup_data.m_velocity != -6.25f )
		start_circle = 0.f;

	// --- Этап поиска стены: только поиск, без выполнения основной логики ---
	bool foundWall   = false;
	float foundAngle = 0.f;
	c_vector wall_position;

	for ( float a = start_circle; a < std::numbers::pi_v< float > * 2.0f; a += step ) {
		c_vector wishdir( cosf( a ), sinf( a ), 0.f );
		auto start_pos = g_ctx.m_local->get_abs_origin( );
		auto end_pos   = start_pos + wishdir;
		c_trace_filter flt( g_ctx.m_local );
		ray_t ray( start_pos, end_pos, colidable->get_obb_mins( ), colidable->get_obb_maxs( ) );

		g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );

		if ( trace.m_fraction < 1.f && trace.m_plane.m_normal.m_z < 0.4f ) {
			wall_position = trace.m_end;
			foundWall     = true;
			foundAngle    = a;
			break; // Выходим из цикла сразу после нахождения подходящей стены.
		}
	}

	// --- Если стена найдена, выполняем основную логику (ladder, предсказание, перемещение) ---
	if ( foundWall ) {
		m_fireman_data.awall   = true;
		int commands_predicted = g_interfaces.m_prediction->m_commands_predicted;

		// Вычисление направления стены.
		c_vector normal_plane( trace.m_plane.m_normal.m_x * -1.f, trace.m_plane.m_normal.m_y * -1.f, 0.f );
		c_angle wall_angle = normal_plane.to_angle( );
		c_vector d_velo    = g_ctx.m_local->get_velocity( );
		d_velo.m_z         = 0.f;
		c_angle velo_angle = d_velo.to_angle( );
		c_angle delta      = velo_angle - wall_angle;
		delta.normalize( );
		float rotation            = deg2rad( wall_angle.m_y - cmd->m_view_point.m_y );
		float cos_rot             = cos( rotation );
		float sin_rot             = sin( rotation );
		float multiplayer         = 450.f;
		float backup_forward_move = cmd->m_forward_move;
		float backup_side_move    = cmd->m_side_move;
		float forward_move        = cos_rot * multiplayer;
		float side_move           = -sin_rot * multiplayer;
		int backup_buttons        = cmd->m_buttons;

		g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
		cmd->m_forward_move = forward_move;
		cmd->m_side_move    = side_move;
		cmd->m_buttons &= ~in_jump;

		g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
		g_prediction.end( g_ctx.m_local );

		// Проверка на ladder.
		if ( !m_fireman_data.is_ladder ) {
			if ( ( g_prediction.backup_data.m_move_type != move_type_ladder && g_ctx.m_local->get_move_type( ) == move_type_ladder ) ||
			     g_prediction.backup_data.m_move_type == move_type_ladder ) {
				m_fireman_data.is_ladder = true;
				start_circle             = foundAngle;
			} else {
				cmd->m_buttons      = backup_buttons;
				cmd->m_forward_move = backup_forward_move;
				cmd->m_side_move    = backup_side_move;
			}
		}

		if ( m_fireman_data.is_ladder ) {
			g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
			int old_flags       = g_ctx.m_local->get_flags( );
			int old_move_type   = g_ctx.m_local->get_move_type( );
			cmd->m_forward_move = 0.f;
			cmd->m_side_move    = 0.f;

			if ( !m_fireman_data.fr_hit && !m_fireman_data.fr_hit_1 ) {
				for ( int i = 0; i < 12; i++ ) {
					g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
					g_prediction.end( g_ctx.m_local );
					if ( !( old_flags & 1 ) && ( g_ctx.m_local->get_flags( ) & 1 ) ) {
						if ( i < 10 ) {
							m_fireman_data.fr_hit_1 = true;
							break;
						}
						m_fireman_data.fr_hit = true;
					}
				}
			}
			if ( !m_fireman_data.fr_hit_1 ) {
				if ( !m_fireman_data.fr_hit ) {
					cmd->m_buttons &= ~in_jump;
					cmd->m_forward_move = 0.f;
					cmd->m_side_move    = 0.f;
				} else {
					if ( g_prediction.backup_data.m_move_type != move_type_ladder ) {
						cmd->m_forward_move = forward_move;
						cmd->m_side_move    = side_move;
						cmd->m_buttons |= in_jump;
					} else {
						cmd->m_forward_move = 0.f;
						cmd->m_side_move    = 0.f;
						cmd->m_buttons |= in_jump;
					}
				}
			} else {
				g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
				int oldflags        = g_ctx.m_local->get_flags( );
				int oldMoveType     = g_ctx.m_local->get_move_type( );
				cmd->m_forward_move = 0.f;
				cmd->m_side_move    = 0.f;

				g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
				g_prediction.end( g_ctx.m_local );

				if ( !( oldflags & 1 ) && ( g_ctx.m_local->get_flags( ) & 1 ) ) {
					cmd->m_forward_move = forward_move;
					cmd->m_side_move    = side_move;
					cmd->m_buttons &= ~in_jump;
				} else {
					cmd->m_buttons |= in_jump;
					cmd->m_forward_move = 0.f;
					cmd->m_side_move    = 0.f;
				}
			}
		} else {
			m_fireman_data.fr_hit   = false;
			m_fireman_data.fr_hit_1 = false;
		}
	}
}
struct fake_user_move {
	float sidemove     = 0.f;
	float forwardmove  = 0.f;
	int buttons        = 0;
	c_angle viewangles = c_angle( 0.f, 0.f, 0.f );
};
fake_user_move g_moveuser[ 1000 ];
void n_movement::impl_t::ladder_bug( c_user_cmd* cmd )
{
	m_ladder_bug_data.founded = false;
	static int ticks_to_stop  = 0;

	if ( !g_ctx.m_local )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;

	if ( !GET_VARIABLE( g_variables.m_ladder_bug, bool ) || !g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) ) ) {
		ticks_to_stop = 0;
		return;
	}
	auto cvar = g_interfaces.m_convar->find_var( "sv_autobunnyhopping" );
	if ( !cvar ) {
		return;
	}
	if ( cvar->get_int( ) == 0 )
		cvar->set_value( 1 );
	c_vector_2d direction_move = c_vector_2d( cmd->m_forward_move, cmd->m_side_move );
	c_vector_2d direction_view = c_vector_2d( cmd->m_view_point.m_x, cmd->m_view_point.m_y );
	int buttons                = cmd->m_buttons;
	bool detected_in_predicted = false;
	bool jump_do_it            = false;
	int max_ground             = 0;
	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
	int ticks_to_jb     = 0;
	int ticks_on_ladder = 0;
	int max_on_ladder   = 0;
	if ( !m_ladder_bug_data.founded && g_ctx.m_local->get_move_type( ) != move_type_ladder && !( g_ctx.m_local->get_flags( ) & 1 ) ) {
		g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
		int max_prediction_ticks = 128;
		if ( g_ctx.m_local->get_move_type( ) == move_type_ladder ) {
			cmd->m_up_move = 450.f;
			cmd->m_buttons |= in_jump;
			m_ladder_bug_data.founded = true;
		} else {
			m_ladder_bug_data.founded = false;
		}
	}
}
void n_movement::impl_t::anti_ladder( c_user_cmd* cmd )
{
	if ( !g_ctx.m_local )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	if ( !GET_VARIABLE( g_variables.m_ladder_bug, bool ) || !g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) ) ) {
		return;
	}
	static int ticks_on_ladder = 0;
	if ( g_prediction.backup_data.m_move_type == move_type_ladder ) {
		if ( ticks_on_ladder > 10 ) {
			c_vector_2d direction_move = c_vector_2d( cmd->m_forward_move, cmd->m_side_move );
			for ( int i = 0; i < 8; i++ ) {
				g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
				int old_move_type = g_ctx.m_local->get_move_type( );
				if ( i == 0 ) {
					cmd->m_forward_move = 450.f;
					cmd->m_side_move    = 0.f;
				} else if ( i == 1 ) {
					cmd->m_forward_move = -450.f;
					cmd->m_side_move    = 0.f;
				} else if ( i == 2 ) {
					cmd->m_forward_move = 0.f;
					cmd->m_side_move    = 450.f;
				} else if ( i == 3 ) {
					cmd->m_forward_move = 0.f;
					cmd->m_side_move    = -450.f;
				} else if ( i == 4 ) {
					cmd->m_forward_move = 0.f;
					cmd->m_side_move    = 0.f;
				} else if ( i == 5 ) {
					cmd->m_forward_move = 450.f;
					cmd->m_side_move    = -450.f;
				} else if ( i == 6 ) {
					cmd->m_side_move    = 450.f;
					cmd->m_forward_move = -450.f;
				} else if ( i == 7 ) {
					cmd->m_forward_move = 450.f;
					cmd->m_side_move    = 450.f;
				}
				g_prediction.begin( g_ctx.m_local, cmd );
				g_prediction.end( g_ctx.m_local );
				int new_move_type = g_ctx.m_local->get_move_type( );
				if ( old_move_type == move_type_ladder && new_move_type != move_type_ladder ) {
					direction_move = c_vector_2d( cmd->m_forward_move, cmd->m_side_move );
				}
			}
			cmd->m_forward_move = direction_move.m_x;
			cmd->m_side_move    = direction_move.m_y;
		}
		ticks_on_ladder++;
	} else {
		ticks_on_ladder = 0;
	}
}

void n_movement::impl_t::strafe_to_yaw( c_user_cmd* cmd, c_angle& angle, const float yaw )
{
	const static auto max_side_speed = g_convars[ HASH_BT( "cl_sidespeed" ) ]->get_float( );
	angle.m_y += yaw;
	cmd->m_side_move    = 0.f;
	cmd->m_forward_move = 0.f;
	const auto degrees  = rad2deg( std::atan2f( g_ctx.m_local->get_velocity( ).m_y, g_ctx.m_local->get_velocity( ).m_x ) );
	const auto delta    = g_math.normalize_angle( angle.m_y - degrees );
	cmd->m_side_move    = delta > 0.f ? -max_side_speed : max_side_speed;
	angle.m_y           = g_math.normalize_angle( angle.m_y - delta );
}
void n_movement::impl_t::detect_edgebug( c_user_cmd* cmd )
{
	bool edgebug_active = GET_VARIABLE( g_variables.edge_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) );
	if ( !edgebug_active ) {
		m_edgebug_data.m_will_edgebug = false;
		m_edgebug_data.m_will_fail    = true;
		return;
	}
	bool jumpbug_active =
		GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) );
	bool on_ladder = g_ctx.m_local->get_move_type( ) == move_type_ladder;
	bool fireman_active =
		GET_VARIABLE( g_variables.m_fire_man, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ) );
	bool ladderbug_active =
		GET_VARIABLE( g_variables.m_ladder_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) );
	if ( jumpbug_active ) {
		m_edgebug_data.m_will_edgebug = false;
		m_edgebug_data.m_will_fail    = true;
		return;
	}
	if ( on_ladder && ( fireman_active || ladderbug_active ) ) {
		m_edgebug_data.m_will_edgebug = false;
		m_edgebug_data.m_will_fail    = true;
		return;
	}
	if ( g_prediction.backup_data.m_velocity.m_z > 0 || g_utilities.is_in< int >( g_ctx.m_local->get_move_type( ), invalid_move_types ) ) {
		m_edgebug_data.m_will_edgebug = false;
		m_edgebug_data.m_will_fail    = true;
		return;
	}
	if ( round( g_ctx.m_local->get_velocity( ).m_z ) == 0 || g_prediction.backup_data.m_flags & fl_onground ) {
		m_edgebug_data.m_will_edgebug = false;
		m_edgebug_data.m_will_fail    = true;
		return;
	}
	if ( g_prediction.backup_data.m_velocity.m_z < -100.f && g_ctx.m_local->get_velocity( ).m_z > g_prediction.backup_data.m_velocity.m_z &&
	     g_ctx.m_local->get_velocity( ).m_z < -5.f && !( g_ctx.m_local->get_flags( ) & fl_onground ) &&
	     g_prediction.backup_data.m_origin.m_z > g_ctx.m_local->get_abs_origin( ).m_z ) {
		float velocity_change   = g_ctx.m_local->get_velocity( ).m_z - g_prediction.backup_data.m_velocity.m_z;
		bool sharp_change       = velocity_change > 15.0f;
		bool small_horizontal   = g_ctx.m_local->get_velocity( ).length_2d( ) < 300.0f;
		bool slow_fall          = g_ctx.m_local->get_velocity( ).m_z > -10.0f && g_ctx.m_local->get_velocity( ).m_z < -3.0f;
		bool not_jump_bug_style = !( g_ctx.m_local->get_velocity( ).m_z < 400.0f && g_ctx.m_local->get_velocity( ).length_2d( ) > 200.0f );
		bool not_fireman_style =
			!( g_prediction.backup_data.m_velocity.length_2d( ) < 200.0f && g_ctx.m_local->get_velocity( ).length_2d( ) > 250.0f );
		bool not_surf_style = !( fabsf( g_ctx.m_local->get_velocity( ).m_z - ( -6.25f ) ) < 1.0f );
		if ( std::floor( g_prediction.backup_data.m_velocity.m_z ) < -101 && std::floor( g_ctx.m_local->get_velocity( ).m_z ) == -7 &&
		     g_ctx.m_local->get_velocity( ).length_2d( ) >= g_prediction.backup_data.m_velocity.length_2d( ) && sharp_change && small_horizontal &&
		     slow_fall && not_jump_bug_style && not_fireman_style && not_surf_style ) {
			m_edgebug_data.m_will_edgebug = true;
			m_edgebug_data.m_will_fail    = false;
		} else {
			float previous_velocity = g_ctx.m_local->get_velocity( ).m_z;
			for ( int i = 0; i < 4; i++ ) {
				g_prediction.begin( g_ctx.m_local, cmd );
				g_prediction.end( g_ctx.m_local );
			}
			static auto sv_gravity_cvar = g_interfaces.m_convar->find_var( "sv_gravity" );
			float gravity               = 800.f;
			if ( sv_gravity_cvar ) {
				gravity = sv_gravity_cvar->get_float( );
			}
			float expected_vertical_velocity = std::roundf( ( -gravity ) * g_interfaces.m_global_vars_base->m_interval_per_tick + previous_velocity );
			bool velocity_match              = expected_vertical_velocity == std::roundf( g_ctx.m_local->get_velocity( ).m_z );
			bool still_stable_conditions = sharp_change && small_horizontal && slow_fall && not_jump_bug_style && not_fireman_style && not_surf_style;
			m_edgebug_data.m_will_edgebug = velocity_match && still_stable_conditions;
			m_edgebug_data.m_will_fail    = !( velocity_match && still_stable_conditions );
		}
	} else {
		m_edgebug_data.m_will_edgebug = false;
		m_edgebug_data.m_will_fail    = true;
	}
}
void n_movement::impl_t::edgebug_data_t::reset( )
{
	g_movement.m_edgebug_data.m_edgebug_method = edgebug_type_t::eb_standing;
	g_movement.m_edgebug_data.m_last_tick      = 0;
	g_movement.m_edgebug_data.m_ticks_to_stop  = 0;
	g_movement.m_edgebug_data.m_will_edgebug   = false;
	g_movement.m_edgebug_data.m_will_fail      = false;
}
void n_movement::impl_t::autoduck_data_t::reset( )
{
	g_movement.m_autoduck_data.m_did_land_ducking  = false;
	g_movement.m_autoduck_data.m_did_land_standing = false;
	g_movement.m_autoduck_data.m_ducking_vert      = 0.f;
	g_movement.m_autoduck_data.m_standing_vert     = 0.f;
}
void n_movement::impl_t::pixelsurf_data_t::reset( )
{
	g_movement.m_pixelsurf_data.m_predicted_succesful = false;
	g_movement.m_pixelsurf_data.m_in_pixel_surf       = false;
	g_movement.m_pixelsurf_data.m_should_duck         = false;
	g_movement.m_pixelsurf_data.m_prediction_ticks    = 0;
	g_movement.m_pixelsurf_data.m_was_in_surf         = false;
}

float n_movement::impl_t::point_to_line_distance( const c_vector_2d& point, const c_vector_2d& line_start, const c_vector_2d& line_end )
{
	float dx        = line_end.m_x - line_start.m_x;
	float dy        = line_end.m_y - line_start.m_y;
	float length_sq = dx * dx + dy * dy;

	if ( length_sq == 0.0f )
		return point.distance( line_start );

	float t = ( ( point.m_x - line_start.m_x ) * dx + ( point.m_y - line_start.m_y ) * dy ) / length_sq;
	t       = std::max( 0.0f, std::min( 1.0f, t ) );

	c_vector_2d projection( line_start.m_x + t * dx, line_start.m_y + t * dy );
	return point.distance( projection );
}
c_vector round_pos( const c_vector& point )
{
	const float epsilon  = 0.001f;
	const float z_offset = point.m_z < 0.0f ? -0.97f : 0.031f;
	return c_vector( point.m_x, point.m_y, floorf( point.m_z + epsilon ) + z_offset );
}
void n_movement::impl_t::movement_fix( const c_angle& old_view_point )
{
	c_vector forward, right, up;
	g_math.angle_vectors( old_view_point, &forward, &right, &up );
	c_vector old_forward, old_right, old_up;
	g_math.angle_vectors( g_ctx.m_cmd->m_view_point, &old_forward, &old_right, &old_up );
	forward.m_z = right.m_z = 0.0f;
	old_forward.m_z = old_right.m_z = 0.0f;
	up.m_x = up.m_y = old_up.m_x = old_up.m_y = 0.0f;
	const float forward_len                   = sqrtf( forward.m_x * forward.m_x + forward.m_y * forward.m_y );
	const float right_len                     = sqrtf( right.m_x * right.m_x + right.m_y * right.m_y );
	const float old_forward_len               = sqrtf( old_forward.m_x * old_forward.m_x + old_forward.m_y * old_forward.m_y );
	const float old_right_len                 = sqrtf( old_right.m_x * old_right.m_x + old_right.m_y * old_right.m_y );
	if ( forward_len > 0.001f ) {
		forward.m_x /= forward_len;
		forward.m_y /= forward_len;
	}
	if ( right_len > 0.001f ) {
		right.m_x /= right_len;
		right.m_y /= right_len;
	}
	if ( old_forward_len > 0.001f ) {
		old_forward.m_x /= old_forward_len;
		old_forward.m_y /= old_forward_len;
	}
	if ( old_right_len > 0.001f ) {
		old_right.m_x /= old_right_len;
		old_right.m_y /= old_right_len;
	}
	if ( fabsf( up.m_z ) > 0.001f )
		up.m_z = up.m_z > 0.0f ? 1.0f : -1.0f;
	if ( fabsf( old_up.m_z ) > 0.001f )
		old_up.m_z = old_up.m_z > 0.0f ? 1.0f : -1.0f;
	const float original_forward = g_ctx.m_cmd->m_forward_move;
	const float original_side    = g_ctx.m_cmd->m_side_move;
	const float original_up      = g_ctx.m_cmd->m_up_move;
	const float input_magnitude  = sqrtf( original_forward * original_forward + original_side * original_side );
	const float world_forward_x  = forward.m_x * original_forward + right.m_x * original_side;
	const float world_forward_y  = forward.m_y * original_forward + right.m_y * original_side;
	const float world_up_z       = up.m_z * original_up;
	const float cos_delta        = old_forward.m_x * forward.m_x + old_forward.m_y * forward.m_y;
	const float sin_delta        = old_forward.m_x * forward.m_y - old_forward.m_y * forward.m_x;
	const float rotated_x        = world_forward_x * cos_delta + world_forward_y * sin_delta;
	const float rotated_y        = -world_forward_x * sin_delta + world_forward_y * cos_delta;
	const float new_forward      = old_forward.m_x * rotated_x + old_forward.m_y * rotated_y + old_up.m_z * world_up_z;
	const float new_side         = old_right.m_x * rotated_x + old_right.m_y * rotated_y;
	const float output_magnitude = sqrtf( new_forward * new_forward + new_side * new_side );
	const float scale_factor = ( output_magnitude > 0.001f && input_magnitude > 0.001f ) ? fminf( 1.0f, input_magnitude / output_magnitude ) : 1.0f;
	const float cached_forward_speed = g_convars[ HASH_BT( "cl_forwardspeed" ) ]->get_float( );
	const float cached_side_speed    = g_convars[ HASH_BT( "cl_sidespeed" ) ]->get_float( );
	const float final_forward        = new_forward * scale_factor;
	const float final_side           = new_side * scale_factor;
	g_ctx.m_cmd->m_forward_move      = std::clamp( final_forward, -cached_forward_speed, cached_forward_speed );
	g_ctx.m_cmd->m_side_move         = std::clamp( final_side, -cached_side_speed, cached_side_speed );
}
bool seccheckcros2    = false;
bool HITGODA2         = false;
bool HEAD_CEILING_HIT = false;
bool check( float a, float b )
{
	int a1 = ( int )a;
	int b1 = ( int )b;
	if ( b < 0 ) {
		if ( a1 == b1 ) {
			int a2 = ( a - a1 ) * 100;
			int b2 = ( b - b1 ) * 100;
			int a3 = ( a - a1 ) * 1000;
			if ( b2 == a2 || a2 + 1 == b2 || a2 + 2 == b2 )
				return true;
			else
				return false;
		}
	} else {
		if ( a1 == b1 ) {
			int a2 = ( a - a1 ) * 100;
			int b2 = ( b - b1 ) * 100;
			int a3 = ( a - a1 ) * 1000;
			if ( b2 == a2 || a2 == b2 - 1 || a2 == b2 - 2 )
				return true;
			else
				return false;
		}
	}
	return false;
}
bool check2( float a, float b )
{
	int a1 = ( int )a;
	int b1 = ( int )b;
	if ( b < 0 ) {
		int aInt = static_cast< int >( a );
		int bInt = static_cast< int >( b );
		if ( aInt != bInt ) {
			return false;
		}
		float aFraction = std::fabs( a - aInt );
		return ( aFraction >= 0.969644f && aFraction <= 0.988999f );
	} else {
		if ( a1 == b1 ) {
			double fracA          = ( double )a - a1;
			int fracA_thousandths = ( int )( fracA * 1000 ) % 1000;
			int a2;
			if ( fracA_thousandths == 969 ) {
				a2 = 97;
			} else {
				a2 = ( int )( fracA * 100 );
			}
			int b2 = ( int )( ( b - b1 ) * 100 );
			return ( b2 == a2 || a2 == b2 - 1 );
		}
	}
	return false;
}
bool self_coded( double a, double b )
{
	if ( a > 0 && b > 0 ) {
		double frac_a = a - std::floor( a );
		double frac_b = b - std::floor( b );
		if ( frac_a > 0 && frac_a < 0.3 && frac_b > 0 && frac_b < 0.3 ) {
			return true;
		} else {
			return false;
		}
	} else if ( a < 0 && b < 0 ) {
		int i_a = ( int )a;
		int i_b = ( int )b;
		if ( i_a != i_b ) {
			return false;
		}
		float outofpoint_a = a - i_a;
		float outofpoint_b = b - i_b;
		int full_a         = std::abs( outofpoint_a * 1000000.f );
		int full_b         = std::abs( outofpoint_b * 1000000.f );
		if ( full_b <= full_a && full_a < 990000 ) {
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}
struct NewCmd1 {
	c_angle viewangle;
	float forwardmove;
	float sideMove;
	int buttons;
	c_vector origin;
};
NewCmd1 NewCmd[ 128 ];
void n_movement::impl_t::autobounce_assist( c_user_cmd* cmd )
{
	seccheckcros2 = false;
	static bool HitJump{ false };
	static bool HitMiniJump{ false };
	static bool HitLongJump{ false };
	static bool HitCHop{ false };
	static bool HitMiniChop{ false };
	static bool HitJumpBug{ false };
	if ( !GET_VARIABLE( g_variables.m_bouncee_assist, bool ) )
		return;
	if ( !g_ctx.m_local )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	if ( !g_ctx.m_local->get_collideable( ) )
		return;
	static int longjump_ticks = 0;
	static int toggletime     = 0;
	static int skip_one_tick  = 0;
	static int ljtickshold    = 0;
	static int crtickshold    = 0;
	static float bfwd         = 0.f;
	static float bside        = 0.f;
	static int skip_ground    = 0;
	static int jumpcrhld      = 0;
	static int holdducklj     = 0;
	static int holdduckmj     = 0;
	static bool ChatStand{ };
	static int skip_mini_jump       = 0;
	static int duck_in_jump_tick    = 0;
	static float last_velocity_z    = 0.0f;
	static bool head_hit_this_jump  = false;
	static bool was_on_ground       = false;
	static float jump_peak_velocity = 0.0f;
	float current_velocity_z        = g_ctx.m_local->get_velocity( ).m_z;
	bool is_on_ground               = ( g_ctx.m_local->get_flags( ) & 1 ) != 0;
	if ( is_on_ground && !was_on_ground ) {
		head_hit_this_jump = false;
		jump_peak_velocity = 0.0f;
	}
	if ( current_velocity_z > 0.0f && current_velocity_z > jump_peak_velocity ) {
		jump_peak_velocity = current_velocity_z;
	}
	if ( last_velocity_z > 0.0f && current_velocity_z < -50.0f && !head_hit_this_jump && !is_on_ground ) {
		c_vector head_pos  = g_ctx.m_local->get_origin( ) + c_vector( 0, 0, 72 );
		c_vector trace_end = head_pos + c_vector( 0, 0, 10 );
		ray_t ray( head_pos, trace_end );
		c_trace_filter filter( g_ctx.m_local );
		trace_t trace;
		g_interfaces.m_engine_trace->trace_ray( ray, e_mask::mask_solid, &filter, &trace );
		if ( trace.m_fraction < 0.5f ) {
			HEAD_CEILING_HIT   = true;
			head_hit_this_jump = true;
		}
	} else if ( is_on_ground ) {
		HEAD_CEILING_HIT = false;
	}
	last_velocity_z = current_velocity_z;
	was_on_ground   = is_on_ground;
	if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) ) ) {
		AlertJB2          = false;
		HITGODA2          = false;
		HitJump           = false;
		HitMiniJump       = false;
		HitLongJump       = false;
		HitMiniChop       = false;
		HitCHop           = false;
		HitJumpBug        = false;
		skip_one_tick     = 0;
		ljtickshold       = 0;
		crtickshold       = 0;
		bfwd              = 0.f;
		bside             = 0.f;
		skip_ground       = 0;
		jumpcrhld         = 0;
		holdducklj        = 0;
		holdduckmj        = 0;
		ChatStand         = false;
		longjump_ticks    = 0;
		skip_mini_jump    = 0;
		duck_in_jump_tick = 0;
		return;
	}
	if ( m_bounce_points_check.size( ) > 0 || m_points_check.size( ) > 0 ) {
		int index     = -1;
		float Nearest = FLT_MAX;
		std::vector< c_vector > surf{ };
		std::vector< int > surf_index{ };
		bool is_free_point   = false;
		int free_point_index = -1;
		for ( int i = 0; i < m_bounce_points_check.size( ); i++ ) {
			if ( g_interfaces.m_engine_client->get_level_name_short( ) == m_bounce_points_check.at( i ).map &&
			     m_bounce_points_check.at( i ).active ) {
				float Distance = g_ctx.m_local->get_origin( ).dist_to_2d( m_bounce_points_check.at( i ).pos );
				if ( Distance <= m_bounce_points_check.at( i ).radius ) {
					surf.emplace_back( m_bounce_points_check.at( i ).pos );
					surf_index.emplace_back( i );
					if ( Distance < Nearest ) {
						Nearest       = Distance;
						index         = i;
						is_free_point = false;
					}
				}
			}
		}
		for ( int i = 0; i < m_points_check.size( ); i++ ) {
			if ( g_interfaces.m_engine_client->get_level_name_short( ) == m_points_check.at( i ).map && m_points_check.at( i ).active &&
			     m_points_check.at( i ).is_free_point && m_points_check.at( i ).point_type == 1 ) {
				float Distance = g_ctx.m_local->get_origin( ).dist_to_2d( m_points_check.at( i ).pos );
				if ( Distance <= m_points_check.at( i ).radius ) {
					surf.emplace_back( m_points_check.at( i ).pos );
					surf_index.emplace_back( i );
					if ( Distance < Nearest ) {
						Nearest          = Distance;
						index            = i;
						is_free_point    = true;
						free_point_index = i;
					}
				}
			}
		}
		static c_vector StoredSurf = c_vector( 0.0f, 0.0f, 0.0f );
		if ( index != -1 ) {
			c_vector Surf = is_free_point ? m_points_check.at( free_point_index ).pos : m_bounce_points_check.at( index ).pos;
			StoredSurf    = Surf;
			c_vector Calculate{ };
			c_vector Calculate2{ };
			float iCalcilate{ };
			int BackupButtons   = cmd->m_buttons;
			float ForwaMove     = cmd->m_forward_move;
			float SideMove      = cmd->m_side_move;
			c_vector BackupVelo = g_ctx.m_local->get_velocity( );
			c_vector Origin     = g_ctx.m_local->get_origin( );
			float stamina       = g_ctx.m_local->get_stamina( );
			int BackupPredicted = g_interfaces.m_prediction->m_commands_predicted;
			static int timer    = 0;
			static int ticks    = 0;
			static int ljticks  = 0;
			static int mjticks  = 0;
			static c_angle last{ };
			c_angle angViewPointBackup = cmd->m_view_point;
			c_angle DeltaAngle         = ( angViewPointBackup - last ) * m_bounce_points_check.at( index ).delta_strafe;
			last                       = cmd->m_view_point;
			if ( !GET_VARIABLE( g_variables.m_bouncee_assist_brokehop, bool ) ) {
				if ( cmd->m_tick_count > ticks && g_prediction.backup_data.m_flags & 1 && g_ctx.m_local->get_stamina( ) != 0.f )
					return;
			}
			if ( g_prediction.backup_data.m_velocity.m_z < 0.f )
				ljticks = 0;
			int g = ( g_prediction.backup_data.m_flags & 1 ) ? 3 : 5;
			int hitgroudtick{ };
			c_vector ChatPositions{ g_ctx.m_local->get_origin( ) };
			bool ChatDuck{ };
			int bheight = 0;
			if ( !HITGODA2 ) {
				for ( int v = 0; v < g; v++ ) {
					cmd->m_side_move    = 0.f;
					cmd->m_forward_move = 0.f;
					cmd->m_buttons      = BackupButtons;
					bool jb             = false;
					bool secjb          = false;
					bool thrjb          = false;
					bool forthjb        = false;
					g_prediction.restore_entity_to_predicted_frame( BackupPredicted - 1 );
					int BackupPredicted2 = 0;
					int once             = 0;
					int ticksljinpred    = 0;
					if ( !HitJump && !HitMiniJump && !HitLongJump && !HitCHop && !HitJumpBug && !( g_ctx.m_local->get_flags( ) & fl_ducking ) &&
					     !HitMiniChop ) {
						for ( int i = 0; i < 48; i++ ) {
							if ( HitJump || HitMiniJump || HitLongJump || HitCHop || HitJumpBug )
								break;
							int max_ground_ticks = 0;
							if ( g_ctx.m_local->get_flags( ) & 1 ) {
								max_ground_ticks += 1;
								once += 1;
							}
							if ( once == 1 )
								ChatPositions = g_ctx.m_local->get_origin( );
							if ( v == 0 ) {
								if ( g_ctx.m_local->get_flags( ) & 1 )
									cmd->m_buttons |= in_jump;
							}
							if ( v == 1 ) {
								if ( g_ctx.m_local->get_flags( ) & 1 ) {
									cmd->m_buttons |= in_jump;
									cmd->m_buttons |= in_duck;
								}
							}
							if ( v == 2 ) {
								if ( ticksljinpred > 4 )
									ticksljinpred = 0;
								if ( ticksljinpred == 0 ) {
									cmd->m_buttons &= ~in_duck;
									cmd->m_buttons &= ~in_jump;
								}
								if ( g_ctx.m_local->get_flags( ) & 1 ) {
									cmd->m_buttons |= in_jump;
									cmd->m_buttons |= in_duck;
									ticksljinpred += 1;
								}
								if ( ticksljinpred > 0 ) {
									cmd->m_buttons |= in_jump;
									cmd->m_buttons |= in_duck;
									ticksljinpred += 1;
								}
							}
							if ( !( g_ctx.m_local->get_flags( ) & 1 ) && v != 2 && v != 4 ) {
								cmd->m_buttons &= ~in_duck;
								cmd->m_buttons &= ~in_jump;
							}
							if ( v == 3 ) {
								if ( g_ctx.m_local->get_flags( ) & 1 ) {
									cmd->m_buttons |= in_jump;
								}
								cmd->m_buttons |= in_duck;
							}
							if ( v == 4 ) {
								if ( i == hitgroudtick ) {
									cmd->m_buttons |= in_duck;
									cmd->m_buttons &= ~in_jump;
									once += 1;
								} else {
									cmd->m_buttons &= ~in_duck;
									cmd->m_buttons |= in_jump;
								}
							}
							c_vector BackupVelo2 = g_ctx.m_local->get_velocity( );
							c_vector Origin2     = g_ctx.m_local->get_origin( );
							int backflags        = g_ctx.m_local->get_flags( );
							g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
							g_prediction.end( g_ctx.m_local );
							if ( g_ctx.m_local->get_flags( ) & 1 )
								max_ground_ticks += 1;
							if ( v == 0 ) {
								if ( g_ctx.m_local->get_flags( ) & 1 && !( backflags & 1 ) ) {
									if ( !jb )
										hitgroudtick = i;
									jb = true;
								}
							}
							if ( max_ground_ticks > 1 )
								break;
							if ( !( g_ctx.m_local->get_flags( ) & 1 ) && v != 3 && v != 4 ) {
								cmd->m_buttons &= ~in_duck;
								cmd->m_buttons &= ~in_jump;
							}
							c_vector PredictedLocalPlayerOrigin   = g_ctx.m_local->get_abs_origin( );
							c_vector PredictedLocalPlayerVelocity = g_ctx.m_local->get_velocity( );
							int flags                             = g_ctx.m_local->get_flags( );
							iCalcilate                            = 0.f;
							Calculate = PredictedLocalPlayerOrigin + g_ctx.m_local->get_collideable( )->get_obb_maxs( ) + c_vector( 0.f, 0.f, 0.04f );
							int heightlol = ( int )g_ctx.m_local->get_collideable( )->get_obb_maxs( ).m_z;
							if ( PredictedLocalPlayerVelocity.m_z > 0.f ) {
								if ( once == 1 ) {
									for ( int j = 0; j < surf.size( ); j++ ) {
										auto& point = m_bounce_points_check.at( surf_index[ j ] );
										if ( v == 0 ) {
											if ( point.jump && self_coded( Calculate.m_z, surf[ j ].m_z ) ) {
												HitJump   = true;
												ChatStand = true;
												ticks     = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												break;
											}
											if ( point.c_jump && heightlol == 72 && self_coded( Calculate.m_z - 9.f, surf[ j ].m_z ) ) {
												HitJump   = true;
												ChatStand = false;
												ChatDuck  = true;
												ticks     = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z - 9.f ).c_str( ) );
												break;
											}
										} else if ( v == 1 ) {
											if ( point.minijump && self_coded( Calculate.m_z, surf[ j ].m_z ) ) {
												if ( !is_bind_restricted_in_radius( point, g_ctx.m_local->get_origin( ), "mj" ) ) {
													HitMiniJump = true;
													ChatStand   = true;
													ticks       = cmd->m_tick_count + i + 5;
													g_interfaces.m_engine_client->execute_client_cmd(
														std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												}
												break;
											}
											if ( point.c_minijump && heightlol == 72 && bheight == 72 &&
											     self_coded( Calculate.m_z - 9.f, surf[ j ].m_z ) ) {
												if ( !is_bind_restricted_in_radius( point, g_ctx.m_local->get_origin( ), "mj" ) ) {
													HitMiniJump = true;
													ChatStand   = false;
													ChatDuck    = true;
													ticks       = cmd->m_tick_count + i + 5;
													g_interfaces.m_engine_client->execute_client_cmd(
														std::format( "echo detected: {} \n", Calculate.m_z - 9.f ).c_str( ) );
												}
												break;
											}
											bheight = heightlol;
										} else if ( v == 3 ) {
											if ( point.crouch_hop && self_coded( Calculate.m_z, surf[ j ].m_z ) ) {
												if ( !is_bind_restricted_in_radius( point, g_ctx.m_local->get_origin( ), "ch" ) ) {
													HitCHop   = true;
													ChatStand = true;
													ticks     = cmd->m_tick_count + i + 5;
													g_interfaces.m_engine_client->execute_client_cmd(
														std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												}
												break;
											}
											if ( point.c_crouch_hop && heightlol == 54 && self_coded( Calculate.m_z + 9.f, surf[ j ].m_z ) ) {
												if ( !is_bind_restricted_in_radius( point, g_ctx.m_local->get_origin( ), "ch" ) ) {
													HitCHop   = true;
													ChatStand = false;
													ChatDuck  = true;
													ticks     = cmd->m_tick_count + i + 5;
													g_interfaces.m_engine_client->execute_client_cmd(
														std::format( "echo detected: {} \n", Calculate.m_z + 9.f ).c_str( ) );
												}
												break;
											}
											if ( point.c_mini_crouch_hop && self_coded( Calculate.m_z + 9.f, surf[ j ].m_z ) ) {
												if ( !is_bind_restricted_in_radius( point, g_ctx.m_local->get_origin( ), "mch" ) ) {
													HitMiniChop = true;
													ChatStand   = false;
													ChatDuck    = true;
													ticks       = cmd->m_tick_count + i + 5;
													g_interfaces.m_engine_client->execute_client_cmd(
														std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												}
												break;
											}
											if ( point.high_crouch_jump && self_coded( Calculate.m_z, surf[ j ].m_z ) ) {
												if ( !is_bind_restricted_in_radius( point, g_ctx.m_local->get_origin( ), "hcj" ) ) {
													HitCHop   = true;
													ChatStand = true;
													ticks     = cmd->m_tick_count + i + 5;
													g_interfaces.m_engine_client->execute_client_cmd(
														std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												}
												break;
											}
											if ( point.c_high_crouch_jump && heightlol == 54 && self_coded( Calculate.m_z + 9.f, surf[ j ].m_z ) ) {
												if ( !is_bind_restricted_in_radius( point, g_ctx.m_local->get_origin( ), "hcj" ) ) {
													HitCHop   = true;
													ChatStand = false;
													ChatDuck  = true;
													ticks     = cmd->m_tick_count + i + 5;
													g_interfaces.m_engine_client->execute_client_cmd(
														std::format( "echo detected: {} \n", Calculate.m_z + 9.f ).c_str( ) );
												}
												break;
											}
										} else if ( v == 4 ) {
											if ( point.jumpbug && self_coded( Calculate.m_z, surf[ j ].m_z ) ) {
												if ( !is_bind_restricted_in_radius( point, g_ctx.m_local->get_origin( ), "jb" ) ) {
													HitJumpBug = true;
													ChatStand  = true;
													ticks      = cmd->m_tick_count + i + 5;
													g_interfaces.m_engine_client->execute_client_cmd(
														std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												}
												break;
											}
										} else if ( v == 2 ) {
											if ( point.longjump && self_coded( Calculate.m_z, surf[ j ].m_z ) ) {
												if ( !is_bind_restricted_in_radius( point, g_ctx.m_local->get_origin( ), "lj" ) ) {
													HitLongJump = true;
													ChatStand   = true;
													ticks       = cmd->m_tick_count + i + 5;
													g_interfaces.m_engine_client->execute_client_cmd(
														std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												}
												break;
											}
											if ( point.c_longjump && heightlol == 72 && self_coded( Calculate.m_z - 9.f, surf[ j ].m_z ) ) {
												HitLongJump = true;
												ChatStand   = false;
												ChatDuck    = true;
												ticks       = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z - 9.f ).c_str( ) );
												break;
											}
										}
									}
								}
							}
							if ( HitJump || HitMiniJump || HitLongJump || HitCHop || HitJumpBug || HitMiniChop )
								break;
						}
					} else {
						break;
					}
				}
			}
			cmd->m_buttons      = BackupButtons;
			cmd->m_forward_move = ForwaMove;
			cmd->m_side_move    = SideMove;
			g_prediction.restore_entity_to_predicted_frame( BackupPredicted - 1 );
			g_prediction.restore_entity_to_predicted_frame( BackupPredicted - 1 );
			static int TicksOut = 0;
			if ( cmd->m_tick_count > ticks ) {
				ticks             = 0;
				HITGODA2          = false;
				skip_one_tick     = 0;
				skip_ground       = 0;
				HitJump           = false;
				jumpcrhld         = 0;
				holdducklj        = 0;
				holdduckmj        = 0;
				HitMiniJump       = false;
				HitMiniChop       = false;
				ChatStand         = false;
				longjump_ticks    = 0;
				skip_mini_jump    = 0;
				duck_in_jump_tick = 0;
			}
			if ( cmd->m_tick_count < ticks ) {
				HITGODA2 = true;
				if ( skip_ground == 0 ) {
					cmd->m_forward_move  = 0.f;
					cmd->m_side_move     = 0.f;
					cmd->m_buttons       = 0;
					cmd->m_mouse_delta_x = 0;
					cmd->m_mouse_delta_y = 0;
				}
				if ( g_prediction.backup_data.m_flags & 1 )
					skip_ground += 1;
				if ( g_prediction.backup_data.m_velocity.m_z > 0.f ) {
					if ( HitJumpBug ) {
						if ( !ChatPositions.is_zero( ) ) {
							if ( TicksOut < GetTickCount64( ) ) {
								TicksOut = GetTickCount64( ) + 500;
								if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
									myWindowDetect.AddWindow( 5, std::string( "jb to head bounce " )
									                                 .append( std::to_string( Surf.m_z ) )
									                                 .append( " with " )
									                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
									                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
							}
						}
					}
					HitJumpBug = false;
				}

				if ( HitJump && ChatStand ) {
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "jump to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
					}
				}
				if ( HitJump && !ChatStand ) {
					if ( duck_in_jump_tick > g_interfaces.m_global_vars_base->m_tick_count )
						cmd->m_buttons |= in_duck;
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "jump to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
						duck_in_jump_tick = g_interfaces.m_global_vars_base->m_tick_count + 32;
					}
				}
				if ( HitMiniJump && ChatStand ) {
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "mj to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
						cmd->m_buttons |= in_duck;
					}
				}
				if ( HitMiniJump && !ChatStand ) {
					if ( skip_mini_jump > 1 )
						cmd->m_buttons |= in_duck;
					if ( skip_mini_jump > 0 )
						skip_mini_jump += 1;
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "mj to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
						cmd->m_buttons |= in_duck;
						skip_mini_jump += 1;
					}
				}
				if ( HitLongJump && ChatStand ) {
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "lj to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
						cmd->m_buttons |= in_duck;
						longjump_ticks = g_interfaces.m_global_vars_base->m_tick_count + 2;
					}
					if ( longjump_ticks > g_interfaces.m_global_vars_base->m_tick_count )
						cmd->m_buttons |= in_duck;
				}
				if ( HitLongJump && !ChatStand ) {
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "lj to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
						cmd->m_buttons |= in_duck;
						longjump_ticks = g_interfaces.m_global_vars_base->m_tick_count + 32;
					}
					if ( longjump_ticks > g_interfaces.m_global_vars_base->m_tick_count )
						cmd->m_buttons |= in_duck;
				}
				if ( HitCHop && ChatStand ) {
					if ( g_prediction.backup_data.m_flags & 1 ) {
						cmd->m_buttons |= in_jump;
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "crouch jump to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
					}
					cmd->m_buttons |= in_duck;
				}
				if ( HitCHop && !ChatStand ) {
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "crouch jump to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
					}
					if ( !( g_prediction.backup_data.m_flags & 1 ) && g_prediction.backup_data.m_velocity.m_z < 0.f ) {
						cmd->m_buttons |= in_duck;
					}
				}

				if ( HitMiniChop ) {
					if ( !bounce_alert_mini_crouch_hop && GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
						myWindowDetect.AddWindow( 5, std::string( "crouch jb to head bounce " )
						                                 .append( std::to_string( Surf.m_z ) )
						                                 .append( " with " )
						                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
						                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
					bounce_alert_mini_crouch_hop = true;
				}
				AlertJB2 = HitJumpBug;
				if ( skip_ground != 0 ) {
					auto colidable = g_ctx.m_local->get_collideable( );
					if ( !colidable )
						return;
					trace_t trace;
					float step         = std::numbers::pi_v< float > * 2.0f / 16.f;
					float start_circle = 0.f;
					c_vector wall_position{ };
					for ( float a = start_circle; a < std::numbers::pi_v< float > * 2.0f; a += step ) {
						c_vector wishdir     = c_vector( cosf( a ), sinf( a ), 0.f );
						const auto start_pos = g_ctx.m_local->get_abs_origin( );
						const auto end_pos   = start_pos + wishdir;
						c_trace_filter flt( g_ctx.m_local );
						ray_t ray( start_pos, end_pos, colidable->get_obb_mins( ), colidable->get_obb_maxs( ) );
						g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
						if ( trace.m_fraction < 1.f && trace.m_plane.m_normal.m_z == 0.f ) {
							c_vector NormalPlane = c_vector( trace.m_plane.m_normal.m_x * -1.f, trace.m_plane.m_normal.m_y * -1.f, 0.f );
							c_angle WallAngle    = NormalPlane.to_angle( );
							float rotation       = deg2rad( WallAngle.m_y - ( cmd->m_view_point.m_y + DeltaAngle.m_y ) );
							float cos_rot        = cos( rotation );
							float sin_rot        = sin( rotation );
							float multiplayer    = 6.f;
							float forwardmove    = cos_rot * multiplayer;
							float sidemove       = -sin_rot * multiplayer;
							cmd->m_forward_move  = forwardmove;
							cmd->m_side_move     = sidemove;
						}
					}
				}
			} else {
				HITGODA2 = false;
			}
		} else {
			static uint64_t last_unreachable_msg_bounce = 0;
			uint64_t current_time                       = GetTickCount64( );
			if ( current_time - last_unreachable_msg_bounce > 3000 ) {
				if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) ) {
					myWindowDetect.AddWindow( 3.0f, "[-] point unreachable" );
				}
				last_unreachable_msg_bounce = current_time;
			}
		}
	}
}
bool seccheckcros = false;
float backupmyaw{ };
float prev_frametime  = -1.0f;
const float threshold = 0.016f;
bool HITGODA          = false;
bool float_ground     = false;
static int cmd_i      = 0;
extern void start_movement_fix( c_user_cmd* cmd );
extern void end_movement_fix( c_user_cmd* cmd );
static bool do_fucking_strafe = false;
c_angle aim_1{ };
c_angle back_ang = { };
c_angle save_delta{ };
c_angle save_cur{ };
void n_movement::impl_t::pixelsurf_assist( c_user_cmd* cmd )
{
	seccheckcros = false;
	static bool HitJump{ false };
	static bool HitMiniJump{ false };
	static bool HitLongJump{ false };
	static bool HitCHop{ false };
	static bool HitMiniChop{ false };
	static bool HitJumpBug{ false };
	if ( !g_ctx.m_local )
		return;
	back_ang = cmd->m_view_point;
	if ( !GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ) )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	static int toggletime = 0;
	if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) ) ) {
		AlertJB           = false;
		HITGODA           = false;
		HitJump           = false;
		HitMiniJump       = false;
		HitLongJump       = false;
		HitCHop           = false;
		HitJumpBug        = false;
		HitMiniChop       = false;
		float_ground      = false;
		cmd_i             = 0;
		do_fucking_strafe = false;
		return;
	}
	const std::string cached_map = g_interfaces.m_engine_client->get_level_name_short( );
	const c_vector cached_origin = g_ctx.m_local->get_origin( );
	if ( m_points_check.size( ) > 0 ) {
		int index     = -1;
		float Nearest = FLT_MAX;
		for ( int i = 0; i < m_points_check.size( ); i++ ) {
			if ( cached_map == m_points_check.at( i ).map && m_points_check.at( i ).active ) {
				if ( !m_points_check.at( i ).is_free_point || m_points_check.at( i ).point_type == 0 ) {
					float Distance = cached_origin.dist_to( m_points_check.at( i ).pos );
					if ( Distance <= m_points_check.at( i ).radius ) {
						if ( Distance < Nearest ) {
							Nearest = Distance;
							index   = i;
						}
					}
				}
			}
		}
		if ( index != -1 ) {
			c_vector Surf               = m_points_check.at( index ).pos;
			auto isLargeAngleDifference = []( float yaw1, float yaw2 ) -> bool {
				float yawDifference = std::abs( yaw1 - yaw2 );
				if ( yawDifference > 180.0f )
					yawDifference = 360.0f - yawDifference;
				return yawDifference >= 90.f;
			};
			int BackupButtons   = cmd->m_buttons;
			float ForwaMove     = cmd->m_forward_move;
			float SideMove      = cmd->m_side_move;
			c_vector BackupVelo = g_ctx.m_local->get_velocity( );
			c_vector Origin     = g_ctx.m_local->get_origin( );
			float stamina       = g_ctx.m_local->get_stamina( );
			int BackupPredicted = g_interfaces.m_prediction->m_commands_predicted;
			static int timer    = 0;
			static int ticks    = 0;
			static int ljticks  = 0;
			if ( !GET_VARIABLE( g_variables.m_pixel_surf_assist_brokehop, bool ) ) {
				if ( cmd->m_tick_count > ticks && g_prediction.backup_data.m_flags & 1 && g_ctx.m_local->get_stamina( ) != 0.f )
					return;
			}
			if ( g_prediction.backup_data.m_velocity.m_z < 0.f )
				ljticks = 0;
			int g = ( g_prediction.backup_data.m_flags & 1 ) ? 2 : 4;
			int hitgroudtick{ };
			c_vector ChatPositions{ g_ctx.m_local->get_origin( ) };
			bool ChatStand{ };
			bool ChatDuck{ };
			static int TicksOut = 0;
			static c_angle last{ };
			float BackupForwardMove    = cmd->m_forward_move;
			float BackupSideMove       = cmd->m_side_move;
			c_angle angViewPointBackup = cmd->m_view_point;
			float DeltaSalier          = 0.5f;
			c_angle DeltaAngle         = ( angViewPointBackup - last ) * m_points_check.at( index ).delta_strafe;
			last                       = cmd->m_view_point;
			for ( int v = 0; v < g; v++ ) {
				cmd->m_side_move    = BackupSideMove;
				cmd->m_forward_move = BackupForwardMove;
				cmd->m_buttons      = BackupButtons;
				cmd->m_view_point   = angViewPointBackup;
				bool jb             = false;
				bool secjb          = false;
				bool thrjb          = false;
				bool forthjb        = false;
				g_prediction.restore_entity_to_predicted_frame( BackupPredicted - 1 );
				int wait             = 0;
				int BackupPredicted2 = 0;
				int once             = 0;
				c_vector Calculate{ };
				float iCalcilate{ };
				int once_fix         = 0;
				c_angle currentAngle = cmd->m_view_point;
				if ( !HitJump && !HitMiniJump && !HitLongJump && !HitCHop && !HitJumpBug && !HitMiniChop ) {
					for ( int i = 0; i < GET_VARIABLE( g_variables.m_pixel_surf_assist_ticks, int ); i++ ) {
						int max_tick_on_ground = 0;
						cmd->m_side_move       = BackupSideMove;
						cmd->m_forward_move    = BackupForwardMove;
						if ( !( g_ctx.m_local->get_flags( ) & 1 ) ) {
							if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_type, bool ) ) {
								if ( !isLargeAngleDifference( currentAngle.m_y, angViewPointBackup.m_y ) )
									currentAngle = ( currentAngle + DeltaAngle ).normalize( ).clamp( );
								cmd->m_view_point = currentAngle;
								start_movement_fix( cmd );
								cmd->m_view_point = angViewPointBackup;
								end_movement_fix( cmd );
							}
						}
						if ( GET_VARIABLE( g_variables.m_pixel_surf_fix, bool ) ) {
							if ( g_prediction.backup_data.m_velocity.length_2d( ) >= 285.91f && once_fix == 0 ) {
								auto sv_airAcelerate = g_interfaces.m_convar->find_var( "sv_airaccelerate" );
								const float tickrate = 1.f / g_interfaces.m_global_vars_base->m_interval_per_tick;
								float Razn           = ( ( g_prediction.backup_data.m_velocity.length_2d( ) + 2.f - 285.91f ) / 12.f * tickrate );
								c_vector velocity    = g_prediction.backup_data.m_velocity * -1.f;
								velocity.m_z         = 0.f;
								float rotation       = deg2rad( velocity.to_angle2( ).m_y - g_ctx.m_cmd->m_view_point.m_y );
								float cos_rot        = cos( rotation );
								float sin_rot        = sin( rotation );
								float forwardmove    = cos_rot * Razn;
								float sidemove       = -sin_rot * Razn;
								cmd->m_forward_move  = forwardmove;
								cmd->m_side_move     = sidemove;
								once_fix             = 1;
							}
						}
						if ( g_ctx.m_local->get_flags( ) & 1 ) {
							max_tick_on_ground += 1;
							once += 1;
							trace_t trace;
							ray_t ray( g_ctx.m_local->get_abs_origin( ), g_ctx.m_local->get_abs_origin( ) + c_vector( 0, 0, 64 ) );
							c_trace_filter_world flt;
							g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
							if ( ( trace.m_fraction < 1.f ) && ( trace.m_plane.m_normal.m_z == 1.f ) )
								float_ground = true;
						}
						if ( once == 1 )
							ChatPositions = g_ctx.m_local->get_origin( );
						if ( v == 0 ) {
							if ( g_ctx.m_local->get_flags( ) & 1 )
								cmd->m_buttons |= in_jump;
						}
						if ( v == 1 ) {
							if ( g_ctx.m_local->get_flags( ) & 1 ) {
								cmd->m_buttons |= in_jump;
								cmd->m_buttons |= in_duck;
							}
						}
						if ( !( g_ctx.m_local->get_flags( ) & 1 ) && v != 2 && v != 3 && v != 4 ) {
							cmd->m_buttons &= ~in_duck;
							cmd->m_buttons &= ~in_jump;
						}
						if ( v == 2 ) {
							if ( g_ctx.m_local->get_flags( ) & 1 ) {
								cmd->m_buttons |= in_jump;
							}
							cmd->m_buttons |= in_duck;
						}
						if ( v == 3 ) {
							if ( i == hitgroudtick ) {
								cmd->m_buttons |= in_duck;
								cmd->m_buttons &= ~in_jump;
								once += 1;
							} else {
								cmd->m_buttons &= ~in_duck;
								cmd->m_buttons |= in_jump;
							}
						}
						c_vector BackupVelo2 = g_ctx.m_local->get_velocity( );
						c_vector Origin2     = g_ctx.m_local->get_origin( );
						int backflags        = g_ctx.m_local->get_flags( );
						g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
						g_prediction.end( g_ctx.m_local );
						if ( v == 1 ) {
							if ( g_ctx.m_local->get_flags( ) & 1 && !( backflags & 1 ) ) {
								if ( !jb )
									hitgroudtick = i;
								jb = true;
							}
						}
						if ( !( g_ctx.m_local->get_flags( ) & 1 ) && v != 3 && v != 4 ) {
							cmd->m_buttons &= ~in_duck;
							cmd->m_buttons &= ~in_jump;
						}
						int preedflags = g_ctx.m_local->get_flags( );
						if ( preedflags & 1 )
							max_tick_on_ground += 1;
						if ( max_tick_on_ground > 1 )
							break;
						c_vector PredictedLocalPlayerOrigin   = g_ctx.m_local->get_origin( );
						c_vector PredictedLocalPlayerVelocity = g_ctx.m_local->get_velocity( );
						iCalcilate                            = 0.f;
						if ( !( v == 2 ) ) {
							if ( !iCalcilate && once == 1 && !( backflags & fl_ducking ) && !( preedflags & fl_ducking ) && BackupVelo2.m_z > 0.f &&
							     PredictedLocalPlayerVelocity.m_z > 0.f ) {
								if ( wait > 2 ) {
									Calculate  = Origin2;
									iCalcilate = Origin2.m_z - PredictedLocalPlayerOrigin.m_z;
									once += 1;
								} else
									wait += 1;
							}
						} else {
							if ( !iCalcilate && once == 1 && BackupVelo2.m_z > 0.f && PredictedLocalPlayerVelocity.m_z > 0.f ) {
								if ( wait > 2 ) {
									Calculate  = Origin2;
									iCalcilate = Origin2.m_z - PredictedLocalPlayerOrigin.m_z;
									once += 1;
								} else
									wait += 1;
							}
						}
						NewCmd[ i ].forwardmove = cmd->m_forward_move;
						NewCmd[ i ].sideMove    = cmd->m_side_move;
						NewCmd[ i ].viewangle   = cmd->m_view_point;
						save_cur                = angViewPointBackup;
						save_delta              = DeltaAngle;
						if ( iCalcilate ) {
							float z = 0;
							for ( int k = 0; k < 512; k++ ) {
								float o = z * ( z + 1 ) / 2;
								const float grav_val =
									g_convars[ HASH_BT( "sv_gravity" ) ] ? g_convars[ HASH_BT( "sv_gravity" ) ]->get_float( ) : 800.f;
								const float ipt = g_interfaces.m_global_vars_base->m_interval_per_tick;
								float p         = iCalcilate + ( grav_val * ipt * ipt * z );
								if ( p > 0 ) {
									if ( m_points_check.at( index ).jump ) {
										if ( v == 0 ) {
											if ( check( Calculate.m_z, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "jump to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (stand)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (stand)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "j" ) ) {
													HitJump = true;
													ticks   = cmd->m_tick_count + i - 3;
												}
												break;
											}
										}
									}

									if ( m_points_check.at( index ).c_jump ) {
										if ( v == 0 ) {
											if ( check( Calculate.m_z + 9.f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "jump to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (crouch)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "j" ) ) {
													HitJump = true;
													ticks   = cmd->m_tick_count + i - 3;
												}
												break;
											}
										}
									}

									if ( m_points_check.at( index ).minijump ) {
										if ( v == 1 ) {
											if ( check( Calculate.m_z, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "mj to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (stand)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (stand)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "mj" ) ) {
													HitMiniJump = true;
													ticks       = cmd->m_tick_count + i - 3;
												}
												break;
											}
										}
									}
									if ( m_points_check.at( index ).c_minijump ) {
										if ( v == 1 ) {
											if ( check( Calculate.m_z + 9.f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "mj to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (crouch)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "mj" ) ) {
													HitMiniJump = true;
													ticks       = cmd->m_tick_count + i - 3;
												}
												break;
											}
										}
									}

									if ( m_points_check.at( index ).longjump ) {
										if ( v == 1 ) {
											if ( check( Calculate.m_z + 8.9999704f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "lj to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (stand)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (stand)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "lj" ) ) {
													HitLongJump = true;
													ticks       = cmd->m_tick_count + i - 3;
												}
												break;
											}
										}
									}

									if ( m_points_check.at( index ).c_longjump ) {
										if ( v == 1 ) {
											if ( check( Calculate.m_z + 9.f + 8.9999704f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "lj to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (crouch)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "lj" ) ) {
													HitLongJump = true;
													ticks       = cmd->m_tick_count + i - 3;
												}
												break;
											}
										}
									}

									if ( m_points_check.at( index ).crouch_hop ) {
										if ( v == 2 ) {
											if ( check( Calculate.m_z - 9.f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "crouch jump to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (stand)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (stand)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "ch" ) ) {
													HitCHop = true;
													ticks   = cmd->m_tick_count + i - 3;
												}
												break;
											}
										}
									}

									if ( m_points_check.at( index ).c_crouch_hop ) {
										if ( v == 2 ) {
											if ( check( Calculate.m_z, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "crouch jump to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (crouch)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "ch" ) ) {
													HitCHop = true;
													ticks   = cmd->m_tick_count + i - 3;
												}
												break;
											}
										}
									}

									if ( m_points_check.at( index ).jumpbug ) {
										if ( v == 3 ) {
											if ( check( Calculate.m_z, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "jb to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (stand)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (stand)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "jb" ) ) {
													HitJumpBug = true;
													ticks      = cmd->m_tick_count + i - 3;
												}
												break;
											}
										}
									}

									if ( m_points_check.at( index ).c_jumpbug ) {
										if ( v == 3 ) {
											if ( check( Calculate.m_z + 9.f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "jb to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (crouch)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "jb" ) ) {
													HitJumpBug = true;
													ticks      = cmd->m_tick_count + i - 3;
												}
												break;
											}
										}
									}

									if ( m_points_check.at( index ).c_mini_crouch_hop ) {
										if ( v == 2 ) {
											if ( check( Calculate.m_z + 9.f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "crouch jb to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (crouch)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "mch" ) ) {
													HitMiniChop = true;
													ticks       = cmd->m_tick_count + i + 10;
												}
												break;
											}
										}
									}

									if ( m_points_check.at( index ).high_crouch_jump ) {
										if ( v == 2 ) {
											if ( check( Calculate.m_z - 9.f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "crouch jb to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (stand)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (stand)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "hcj" ) ) {
													HitCHop = true;
													ticks   = cmd->m_tick_count + i - 3;
												}
												break;
											}
										}
									}

									if ( m_points_check.at( index ).c_high_crouch_jump ) {
										if ( v == 2 ) {
											if ( check( Calculate.m_z, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
														// Check if point is actually reachable
														myWindowDetect.AddWindow(
															5, std::string( "crouch jb to surf at " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
														// myWindowDetect.AddWindow( 5, std::string( "(debug) point hittable (crouch)" ) );
													}
												}
												//
												if ( !is_bind_restricted_in_radius( m_points_check.at( index ), g_ctx.m_local->get_origin( ),
												                                    "hcj" ) ) {
													HitCHop = true;
													ticks   = cmd->m_tick_count + i - 3;
												}
												break;
											}
										}
									}
								}

								Calculate.m_z = Calculate.m_z - p;
								z += 1;
							}
							break;
						}
					}
				} else {
					break;
				}
				cmd->m_buttons      = BackupButtons;
				cmd->m_forward_move = BackupForwardMove;
				cmd->m_side_move    = BackupSideMove;
			}

			cmd->m_buttons      = BackupButtons;
			cmd->m_forward_move = BackupForwardMove;
			cmd->m_side_move    = BackupSideMove;
			g_prediction.restore_entity_to_predicted_frame( BackupPredicted - 1 );
			if ( cmd->m_tick_count > ticks ) {
				ticks             = 0;
				HITGODA           = false;
				float_ground      = false;
				HitJump           = false;
				HitMiniJump       = false;
				HitLongJump       = false;
				HitCHop           = false;
				HitJumpBug        = false;
				HitMiniChop       = false;
				AlertJB           = false;
				cmd_i             = 0;
				do_fucking_strafe = false;
			}
			if ( cmd->m_tick_count < ticks ) {
				HITGODA = true;
				if ( m_pixelsurf_assist_t.pending_announce ) {
					if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
						myWindowDetect.AddWindow( 5, m_pixelsurf_assist_t.pending_info );
					}
					m_pixelsurf_assist_t.pending_announce = false;
					m_pixelsurf_assist_t.pending_info.clear( );
					m_pixelsurf_assist_t.pending_stance.clear( );
				}
				if ( g_prediction.backup_data.m_velocity.m_z < 0.f || g_ctx.m_local->get_velocity( ).m_z < 0.f ) {
					if ( cmd_i > 127 )
						cmd_i = 0;
					try {
						cmd->m_forward_move = NewCmd[ cmd_i ].forwardmove;
						cmd->m_side_move    = NewCmd[ cmd_i ].sideMove;
						if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_type, bool ) ) {
							cmd->m_view_point = NewCmd[ cmd_i ].viewangle;
							start_movement_fix( cmd );
							cmd->m_view_point = angViewPointBackup;
							end_movement_fix( cmd );
						}
						cmd_i += 1;
					} catch ( ... ) {
						cmd->m_forward_move = 0.f;
						cmd->m_side_move    = 0.f;
					}
					do_fucking_strafe = true;
					cmd->m_buttons    = 0;
				} else {
					do_fucking_strafe = false;
				}
				if ( g_prediction.backup_data.m_velocity.m_z > 0.f ) {
					if ( HitJumpBug ) { }
					HitCHop    = false;
					HitJumpBug = false;
				}
				if ( HitCHop ) {
					cmd->m_buttons |= in_duck;
					if ( g_prediction.backup_data.m_flags & 1 )
						cmd->m_buttons |= in_jump;
				}
				if ( HitJump && g_prediction.backup_data.m_flags & 1 ) {
					cmd->m_buttons |= in_jump;
					HitJump = false;
				}
				if ( HitMiniJump && g_prediction.backup_data.m_flags & 1 ) {
					cmd->m_buttons |= in_jump;
					cmd->m_buttons |= in_duck;
					HitMiniJump = false;
				}
				if ( HitLongJump && g_prediction.backup_data.m_flags & 1 ) {
					cmd->m_buttons |= in_jump;
					cmd->m_buttons |= in_duck;
					ljticks     = g_interfaces.m_global_vars_base->m_tick_count + 3;
					HitLongJump = false;
				}
				if ( HitMiniChop ) {
					alert_mini_crouch_hop = true;
				}
				if ( ljticks > g_interfaces.m_global_vars_base->m_tick_count )
					cmd->m_buttons |= in_duck;
				else {
					if ( g_prediction.backup_data.m_velocity.m_z > 0 )
						ticks = 0;
				}
				AlertJB = HitJumpBug;
			} else {
				HITGODA = false;
			}
		} else {
			static uint64_t last_unreachable_msg = 0;
			uint64_t current_time                = GetTickCount64( );
			if ( current_time - last_unreachable_msg > 3000 ) {
				if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) ) {
					myWindowDetect.AddWindow( 3.0f, "[-] point unreachable" );
				}
				last_unreachable_msg = current_time;
			}
		}
	}
}
bool n_movement::impl_t::can_reach_point( const c_vector& target_pos, float radius, const std::string& jump_type, const c_vector& current_vel )
{
	if ( !g_ctx.m_local )
		return false;
	c_vector start_pos         = g_ctx.m_local->get_origin( );
	c_vector start_vel         = current_vel;
	c_vector to_target         = target_pos - start_pos;
	float horizontal_dist      = c_vector( to_target.m_x, to_target.m_y, 0.f ).length_2d( );
	float height_diff          = target_pos.m_z - start_pos.m_z;
	const float tick           = g_interfaces.m_global_vars_base->m_interval_per_tick;
	const auto sv_gravity      = g_interfaces.m_convar->find_var( "sv_gravity" );
	const float gravity        = sv_gravity ? sv_gravity->get_float( ) : 800.f;
	const auto sv_airaccel     = g_interfaces.m_convar->find_var( "sv_airaccelerate" );
	const float airaccel       = sv_airaccel ? sv_airaccel->get_float( ) : 10.f;
	float jump_velocity        = 0.f;
	float max_horizontal_reach = 0.f;
	float max_height_reach     = 0.f;
	float min_speed_required   = 0.f;
	if ( jump_type == "jump" ) {
		jump_velocity        = 270.f;
		max_horizontal_reach = 220.f;
		max_height_reach     = 45.f;
		min_speed_required   = 180.f;
	} else if ( jump_type == "mj" ) {
		jump_velocity        = 200.f;
		max_horizontal_reach = 120.f;
		max_height_reach     = 25.f;
		min_speed_required   = 250.f;
	} else if ( jump_type == "lj" ) {
		jump_velocity        = 300.f;
		max_horizontal_reach = 280.f;
		max_height_reach     = 50.f;
		min_speed_required   = 150.f;
	} else if ( jump_type == "crouch" ) {
		jump_velocity        = 250.f;
		max_horizontal_reach = 200.f;
		max_height_reach     = 35.f;
		min_speed_required   = 200.f;
	} else if ( jump_type == "jb" ) {
		jump_velocity        = 280.f;
		max_horizontal_reach = 240.f;
		max_height_reach     = 40.f;
		min_speed_required   = 170.f;
	}
	if ( horizontal_dist > max_horizontal_reach || height_diff > max_height_reach ) {
		return false;
	}
	float current_speed = start_vel.length_2d( );
	if ( jump_type == "mj" ) {
		if ( horizontal_dist > 80.f ) {
			return false;
		}
		if ( std::abs( height_diff ) < 15.f && horizontal_dist > 60.f ) {
			return false;
		}
		if ( current_speed < 280.f && horizontal_dist > 50.f ) {
			return false;
		}
	}
	if ( jump_type == "crouch" ) {
		if ( horizontal_dist > 150.f ) {
			return false;
		}
		if ( height_diff < -20.f && horizontal_dist > 100.f ) {
			return false;
		}
		if ( current_speed < 220.f && horizontal_dist > 120.f ) {
			return false;
		}
	}
	if ( jump_type == "jump" ) {
		if ( current_speed < 200.f && horizontal_dist > 180.f ) {
			return false;
		}
		if ( height_diff > 30.f && horizontal_dist > 150.f ) {
			return false;
		}
	}
	if ( jump_type == "lj" ) {
		if ( current_speed < 150.f && horizontal_dist > 200.f ) {
			return false;
		}
	}
	if ( jump_type == "jb" ) {
		if ( current_speed < 180.f && horizontal_dist > 200.f ) {
			return false;
		}
	}
	if ( g_ctx.m_local->get_flags( ) & 1 ) {
		start_vel.m_z = jump_velocity;
	}
	const int max_ticks  = static_cast< int >( 2.5f / tick );
	c_vector pos         = start_pos;
	c_vector vel         = start_vel;
	c_vector optimal_dir = c_vector( to_target.m_x, to_target.m_y, 0.f ).normalized( );
	for ( int i = 0; i < max_ticks; ++i ) {
		vel.m_z -= gravity * tick;
		if ( !( g_ctx.m_local->get_flags( ) & 1 ) || i > 0 ) {
			c_vector current_dir  = c_vector( vel.m_x, vel.m_y, 0.f ).normalized( );
			c_vector accel_dir    = optimal_dir;
			float accel_magnitude = airaccel * 10.f * tick;
			c_vector accel        = accel_dir * accel_magnitude;
			vel.m_x += accel.m_x;
			vel.m_y += accel.m_y;
			float max_speed                = 320.f;
			float current_horizontal_speed = c_vector( vel.m_x, vel.m_y, 0.f ).length_2d( );
			if ( current_horizontal_speed > max_speed ) {
				vel.m_x = ( vel.m_x / current_horizontal_speed ) * max_speed;
				vel.m_y = ( vel.m_y / current_horizontal_speed ) * max_speed;
			}
		}
		pos.m_x += vel.m_x * tick;
		pos.m_y += vel.m_y * tick;
		pos.m_z += vel.m_z * tick;
		c_vector to_target_sim = target_pos - pos;
		to_target_sim.m_z      = 0.f;
		if ( to_target_sim.length_2d( ) <= radius ) {
			return true;
		}
		if ( pos.m_z <= start_pos.m_z && vel.m_z <= 0.f ) {
			break;
		}
		if ( vel.m_z < 0.f && pos.m_z < target_pos.m_z ) {
			break;
		}
	}
	return false;
}

void n_movement::impl_t::pixelsurf_assist_ground_help( c_user_cmd* cmd )
{
	if ( !GET_VARIABLE( g_variables.m_pixel_surf_assist_ground_help, bool ) )
		return;
	if ( !GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ) )
		return;
	if ( !g_ctx.m_local )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	if ( !( g_ctx.m_local->get_flags( ) & 1 ) )
		return;
	if ( m_points_check.size( ) == 0 )
		return;
	int index     = -1;
	float nearest = FLT_MAX;
	for ( int i = 0; i < m_points_check.size( ); i++ ) {
		if ( g_interfaces.m_engine_client->get_level_name_short( ) == m_points_check.at( i ).map && m_points_check.at( i ).active ) {
			float distance     = g_ctx.m_local->get_origin( ).dist_to( m_points_check.at( i ).pos );
			float check_radius = m_points_check.at( i ).radius;
			if ( m_points_check.at( i ).use_custom_first_jump_radius ) {
				check_radius = m_points_check.at( i ).first_jump_radius;
			}
			if ( distance <= check_radius ) {
				if ( distance < nearest ) {
					nearest = distance;
					index   = i;
				}
			}
		}
	}
	if ( index == -1 )
		return;
	c_vector target_pos   = m_points_check.at( index ).pos;
	c_vector direction    = ( target_pos - g_ctx.m_local->get_origin( ) ).normalized( );
	c_angle target_angle  = direction.to_angle2( );
	c_angle current_angle = cmd->m_view_point;
	float yaw_diff        = target_angle.m_y - current_angle.m_y;
	while ( yaw_diff > 180.0f )
		yaw_diff -= 360.0f;
	while ( yaw_diff < -180.0f )
		yaw_diff += 360.0f;
	float max_angle = GET_VARIABLE( g_variables.m_pixel_surf_assist_ground_angle_limit, float );
	if ( yaw_diff > max_angle )
		yaw_diff = max_angle;
	else if ( yaw_diff < -max_angle )
		yaw_diff = -max_angle;
	c_angle new_angle = current_angle;
	new_angle.m_y += yaw_diff;
	new_angle.normalize( ).clamp( );
	cmd->m_view_point = new_angle;
	CorrectMovement( cmd, new_angle, current_angle );
}
void n_movement::impl_t::on_frame_stage_notify( int stage )
{
	if ( stage != e_client_frame_stage::start )
		return;
}
void n_movement::impl_t::pixelsurf_assist_render( )
{
	if ( m_lirili_larila_pending_render ) {
		myWindowDetect.AddWindow( 5, std::format( "assisting to {}", static_cast< int >( m_lirili_larila_pending_origin.m_y ) ) );
		m_lirili_larila_pending_render = false;
	}
	if ( !GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ) && !GET_VARIABLE( g_variables.m_bouncee_assist, bool ) )
		return;
	if ( !g_ctx.m_local )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	static bool isBackspacePressed = false;
	if ( GetAsyncKeyState( VK_BACK ) & 0x8000 ) {
		isBackspacePressed = true;
	} else {
		isBackspacePressed = false;
	}
	if ( m_points_check.size( ) > 0 || m_bounce_points_check.size( ) > 0 ) { }
}
float hitmarkerTimer                     = 0.0f;
bool showHitmarker                       = false;
float lester_kill_notify_timer           = 0.f;
bool show_lester_kill_notify             = false;
static uint64_t lester_kill_start_ms     = 0;
static float lester_kill_duration_cached = 3.f;

void lester_kill_begin_notify( float duration )
{
	if ( duration < 0.25f )
		duration = 0.25f;
	else if ( duration > 60.f )
		duration = 60.f;
	lester_kill_duration_cached = duration;
	lester_kill_notify_timer    = duration;
	show_lester_kill_notify     = true;
	lester_kill_start_ms        = GetTickCount64( );
	lester_kill_sound_play( );
}

void lester_kill_sound_stop( )
{
	mciSendStringW( L"stop bebra_lester_ring", nullptr, 0, nullptr );
	mciSendStringW( L"close bebra_lester_ring", nullptr, 0, nullptr );
}

void lester_kill_sound_play( )
{
	lester_kill_sound_stop( );
	const wchar_t* path   = LR"(C:\Users\thtgg\Downloads\Voicy_Franklin Ringtone GTA 5.mp3)";
	std::wstring open_cmd = std::wstring( L"open \"" ) + path + L"\" type mpegvideo alias bebra_lester_ring";
	if ( mciSendStringW( open_cmd.c_str( ), nullptr, 0, nullptr ) != 0 )
		return;
	if ( mciSendStringW( L"set bebra_lester_ring time format milliseconds", nullptr, 0, nullptr ) != 0 ) {
		mciSendStringW( L"play bebra_lester_ring", nullptr, 0, nullptr );
		return;
	}
	const float total = lester_kill_duration_cached;
	float sd          = GET_VARIABLE( g_variables.m_lester_sound_duration, float );
	if ( sd <= 0.f )
		sd = total;
	sd     = std::min( sd, total );
	int ms = static_cast< int >( sd * 1000.f + 0.5f );
	if ( ms < 1 )
		ms = 1;
	wchar_t play_cmd[ 128 ];
	swprintf_s( play_cmd, std::size( play_cmd ), L"play bebra_lester_ring from 0 to %d", ms );
	if ( mciSendStringW( play_cmd, nullptr, 0, nullptr ) != 0 )
		mciSendStringW( L"play bebra_lester_ring", nullptr, 0, nullptr );
}

static void lester_kill_poll_audio_wallclock( )
{
	if ( !show_lester_kill_notify || !GET_VARIABLE( g_variables.m_lester_kill_notify, bool ) )
		return;
	const float duration = lester_kill_duration_cached;
	float sound_d        = GET_VARIABLE( g_variables.m_lester_sound_duration, float );
	if ( sound_d <= 0.f )
		sound_d = duration;
	sound_d             = std::min( sound_d, duration );
	const float elapsed = static_cast< float >( GetTickCount64( ) - lester_kill_start_ms ) * 0.001f;
	if ( elapsed >= sound_d )
		lester_kill_sound_stop( );
}

static void RenderLesterKillNotify( )
{
	if ( !GET_VARIABLE( g_variables.m_lester_kill_notify, bool ) ) {
		if ( show_lester_kill_notify ) {
			lester_kill_sound_stop( );
			show_lester_kill_notify  = false;
			lester_kill_notify_timer = 0.f;
		}
		return;
	}
	if ( !show_lester_kill_notify )
		return;

	const float duration     = lester_kill_duration_cached;
	const float elapsed      = static_cast< float >( GetTickCount64( ) - lester_kill_start_ms ) * 0.001f;
	lester_kill_notify_timer = std::max( 0.f, duration - elapsed );

	float sound_d = GET_VARIABLE( g_variables.m_lester_sound_duration, float );
	if ( sound_d <= 0.f )
		sound_d = duration;
	sound_d = std::min( sound_d, duration );
	if ( elapsed >= sound_d )
		lester_kill_sound_stop( );

	float kin  = GET_VARIABLE( g_variables.m_lester_slide_in, float );
	float kout = GET_VARIABLE( g_variables.m_lester_slide_out, float );
	if ( kin < 0.02f )
		kin = 0.02f;
	if ( kout < 0.02f )
		kout = 0.02f;
	const float anim_budget = std::max( 0.05f, duration - 0.04f );
	if ( kin + kout > anim_budget ) {
		const float s = anim_budget / ( kin + kout );
		kin *= s;
		kout *= s;
	}

	const float t_anim = std::min( elapsed, duration );

	if ( !g_render.m_lester_call_texture ) {
		if ( elapsed >= duration ) {
			lester_kill_sound_stop( );
			show_lester_kill_notify = false;
		}
		return;
	}
	D3DSURFACE_DESC desc{ };
	if ( FAILED( g_render.m_lester_call_texture->GetLevelDesc( 0, &desc ) ) ) {
		if ( elapsed >= duration ) {
			lester_kill_sound_stop( );
			show_lester_kill_notify = false;
		}
		return;
	}
	const ImVec2 display = ImGui::GetIO( ).DisplaySize;
	const float tex_w    = static_cast< float >( desc.Width );
	const float tex_h    = static_cast< float >( desc.Height );
	float max_h_frac     = GET_VARIABLE( g_variables.m_lester_image_max_height_frac, float );
	max_h_frac           = ImClamp( max_h_frac, 0.08f, 0.95f );
	const float max_h    = display.y * max_h_frac;
	const float scale    = max_h / tex_h;
	const float draw_w   = tex_w * scale;
	const float draw_h   = tex_h * scale;

	const float margin  = 18.f;
	const float ox      = GET_VARIABLE( g_variables.m_lester_pos_offset_x, float );
	const float oy      = GET_VARIABLE( g_variables.m_lester_pos_offset_y, float );
	const float x       = display.x - draw_w - margin + ox;
	const float y_rest  = display.y - draw_h - margin + oy;
	const float y_start = display.y + draw_h * 0.08f + oy;

	float y          = y_rest;
	float alpha_norm = 1.f;
	if ( t_anim < kin ) {
		float slide_t     = kin > 0.f ? t_anim / kin : 1.f;
		slide_t           = ImClamp( slide_t, 0.f, 1.f );
		const float eased = 1.f - ( 1.f - slide_t ) * ( 1.f - slide_t ) * ( 1.f - slide_t );
		y                 = ImLerp( y_start, y_rest, eased );
	} else if ( t_anim < duration - kout ) {
		y = y_rest;
	} else {
		float out_t           = kout > 0.f ? ( t_anim - ( duration - kout ) ) / kout : 1.f;
		out_t                 = ImClamp( out_t, 0.f, 1.f );
		const float eased_out = out_t * out_t * out_t;
		y                     = ImLerp( y_rest, y_start, eased_out );
		alpha_norm            = 1.f - eased_out;
	}
	const int a    = static_cast< int >( ImClamp( alpha_norm * 255.f, 0.f, 255.f ) );
	ImDrawList* dl = ImGui::GetForegroundDrawList( );
	dl->AddImage( reinterpret_cast< ImTextureID >( g_render.m_lester_call_texture ), ImVec2( x, y ), ImVec2( x + draw_w, y + draw_h ), ImVec2( 0, 0 ),
	              ImVec2( 1, 1 ), IM_COL32( 255, 255, 255, a ) );
	if ( elapsed >= duration ) {
		lester_kill_sound_stop( );
		show_lester_kill_notify = false;
	}
}

void RenderHitmarker( )
{
	if ( !GET_VARIABLE( g_variables.m_hit_marker, bool ) )
		return;
	const float hitmarkerDuration = 0.5f;
	const float extensionTime     = 0.1f;
	const float full_length       = 10.0f;
	const float gap               = 8.0f;
	const float move_distance     = 5.0f;
	const float angle             = 45.0f;
	const float thickness         = 1.0f;
	if ( showHitmarker ) {
		ImDrawList* drawList = ImGui::GetForegroundDrawList( );
		float elapsed_time   = hitmarkerDuration - hitmarkerTimer;
		float current_length = 0.0f;
		float alpha          = 0.0f;
		float offset         = 0.0f;
		if ( elapsed_time < extensionTime ) {
			float progress = elapsed_time / extensionTime;
			current_length = progress * full_length;
			alpha          = 255.0f;
		} else {
			current_length      = full_length;
			float fade_progress = ( elapsed_time - extensionTime ) / ( hitmarkerDuration - extensionTime );
			alpha               = 255.0f * ( 1.0f - fade_progress );
			offset              = fade_progress * move_distance;
		}
		ImU32 color     = IM_COL32( 255, 255, 255, ( int )alpha );
		float centerX   = ImGui::GetIO( ).DisplaySize.x / 2.0f;
		float centerY   = ImGui::GetIO( ).DisplaySize.y / 2.0f;
		float rad_angle = angle * 3.1415926535f / 180.0f;
		float dx        = cos( rad_angle );
		float dy        = sin( rad_angle );
		drawList->AddLine( ImVec2( centerX + offset * dx, centerY + offset * dy ),
		                   ImVec2( centerX + offset * dx + current_length * dx, centerY + offset * dy + current_length * dy ), color, thickness );
		drawList->AddLine( ImVec2( centerX - offset * dx, centerY - offset * dy ),
		                   ImVec2( centerX - offset * dx - current_length * dx, centerY - offset * dy - current_length * dy ), color, thickness );
		drawList->AddLine( ImVec2( centerX + offset * dy, centerY - offset * dx ),
		                   ImVec2( centerX + offset * dy + current_length * dy, centerY - offset * dx - current_length * dx ), color, thickness );
		drawList->AddLine( ImVec2( centerX - offset * dy, centerY + offset * dx ),
		                   ImVec2( centerX - offset * dy - current_length * dy, centerY + offset * dx + current_length * dx ), color, thickness );
		hitmarkerTimer -= ImGui::GetIO( ).DeltaTime;
		if ( hitmarkerTimer <= 0.0f ) {
			showHitmarker = false;
		}
	}
}
void n_movement::impl_t::on_end_scene( )
{
	RenderHitmarker( );
	RenderLesterKillNotify( );
	float deltaTime = ImGui::GetIO( ).DeltaTime;
	myWindowDetect.Update( deltaTime );
	ImDrawList* draw = ImGui::GetBackgroundDrawList( );
	myWindowDetect.Draw( draw, ImGui::GetIO( ).DisplaySize );
}
void n_movement::impl_t::pixel_calc_render( )
{
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return;
	if ( !GET_VARIABLE( g_variables.m_pixel_calc, bool ) || !GET_VARIABLE( g_variables.m_pixel_calc_show_point, bool ) )
		return;

	auto& data = m_pixel_calc_data;

	if ( data.point_vec.is_zero( ) )
		return;

	c_vector_2d screen_pos;
	if ( !g_render.world_to_screen( data.point_vec, screen_pos ) )
		return;

	auto accent  = GET_VARIABLE( g_variables.m_accent, c_color );
	float radius = 12.f;

	filled_circle_draw_object_t filled;
	filled.m_center = screen_pos;
	filled.m_radius = radius;
	filled.m_color  = c_color( 0, 0, 0, 255 ).get_u32( );
	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_filled_circle, std::make_any< filled_circle_draw_object_t >( filled ) );

	circle_draw_object_t outline;
	outline.m_center = screen_pos;
	outline.m_radius = radius;
	outline.m_color =
		c_color( static_cast< int >( accent.get< 0 >( ) ), static_cast< int >( accent.get< 1 >( ) ), static_cast< int >( accent.get< 2 >( ) ), 255 )
			.get_u32( );
	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_circle, std::make_any< circle_draw_object_t >( outline ) );

	auto* font = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_googlesans_addwindow ];
	if ( font ) {
		const char* label = "px";
		ImVec2 text_size  = font->CalcTextSizeA( font->FontSize, FLT_MAX, 0.0f, label );
		text_draw_object_t text;
		text.m_position = c_vector_2d( screen_pos.m_x - text_size.x * 0.5f, screen_pos.m_y - text_size.y * 0.5f - 1.f );
		text.m_color    = c_color( static_cast< int >( accent.get< 0 >( ) ), static_cast< int >( accent.get< 1 >( ) ),
		                           static_cast< int >( accent.get< 2 >( ) ), 255 )
		                   .get_u32( );
		text.m_text = label;
		text.m_font = font;
		g_render.m_draw_data.emplace_back( e_draw_type::draw_type_text, std::make_any< text_draw_object_t >( text ) );
	}
}

static float pixel_height_offset( float hk )
{
	return hk - 2.f;
}

static void try_pixel_combo( float start_z, float height, float gravity_inc, float gravity_step, float target, const char* name,
                             const char* name_crouch, std::vector< std::string >& out )
{
	float a = start_z + height;
	int i   = 0;
	while ( a > target - 20.f ) {
		if ( check( a, target ) )
			out.emplace_back( name );
		if ( check( a + 9.f, target ) )
			out.emplace_back( name_crouch );
		float p = gravity_inc + ( gravity_step * i );
		a -= p;
		i += 1;
	}
}

void n_movement::impl_t::pixel_calc( c_user_cmd* cmd )
{
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return;
	if ( !GET_VARIABLE( g_variables.m_pixel_calc, bool ) )
		return;

	auto& data = m_pixel_calc_data;

	if ( cmd ) {
		static bool set_point_prev = false;
		bool set_point_pressed     = g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_calc_aim_key, key_bind_t ) );
		if ( set_point_pressed && !set_point_prev ) {
			c_vector eye = g_ctx.m_local->get_eye_position( );
			c_vector view_angle( cmd->m_view_point.m_x, cmd->m_view_point.m_y, 0.f );
			c_vector forward = c_vector::fromAngle( view_angle );

			float best_screen_dist = FLT_MAX;
			c_vector best_point{ };
			bool snap_found                = false;
			const float snap_screen_radius = 30.f;

			auto try_snap = [ & ]( const std::vector< AnimatedPoint >& points ) {
				for ( const auto& ap : points ) {
					if ( ap.is_removing )
						continue;
					c_vector_2d screen_pos;
					if ( !g_render.world_to_screen( ap.position, screen_pos ) )
						continue;
					float dx          = screen_pos.m_x - g_ctx.m_width * 0.5f;
					float dy          = screen_pos.m_y - g_ctx.m_height * 0.5f;
					float screen_dist = sqrtf( dx * dx + dy * dy );
					if ( screen_dist < snap_screen_radius && screen_dist < best_screen_dist ) {
						best_screen_dist = screen_dist;
						best_point       = ap.position;
						snap_found       = true;
					}
				}
			};

			try_snap( animated_points );
			try_snap( animated_points2 );

			if ( snap_found ) {
				data.point_vec      = best_point;
				data.normal_pos_vec = c_vector( 0, 0, 1 );
				myWindowDetect.AddWindow( 3, "calc: snapped to finder point (z: " + std::to_string( best_point.m_z ) + ")" );
			} else {
				const auto end_pos = eye + forward * 6000.f;
				trace_t trace;
				ray_t ray( eye, end_pos );
				c_trace_filter flt( g_ctx.m_local );
				g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
				c_vector hit_point  = eye + ( end_pos - eye ) * trace.m_fraction;
				data.point_vec      = hit_point;
				data.normal_pos_vec = trace.m_plane.m_normal;
				myWindowDetect.AddWindow( 3, "calc: point set (z: " + std::to_string( hit_point.m_z ) + ")" );
			}
		}
		set_point_prev = set_point_pressed;
	}

	static bool calc_prev_pressed = false;
	bool calc_pressed             = g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_calc_solve_key, key_bind_t ) );
	if ( !calc_pressed ) {
		calc_prev_pressed = false;
		return;
	}
	if ( calc_prev_pressed )
		return;
	calc_prev_pressed = true;

	auto& pv = data.point_vec;
	if ( pv.is_zero( ) )
		return;

	const float sv_gravity        = g_convars[ HASH_BT( "sv_gravity" ) ] ? g_convars[ HASH_BT( "sv_gravity" ) ]->get_float( ) : 800.f;
	const float interval_per_tick = g_interfaces.m_global_vars_base->m_interval_per_tick;
	const float gravity_step      = sv_gravity * interval_per_tick * interval_per_tick;

	float point;
	if ( pv.m_z < 0.f )
		point = static_cast< int >( pv.m_z ) - 0.97125f;
	else
		point = static_cast< int >( pv.m_z ) + 0.03125f;

	c_vector backup_origin = g_ctx.m_local->get_origin( );
	data.combo.clear( );

	struct combo_entry_t {
		float height;
		float gravity_inc;
		const char* name;
		const char* name_crouch;
	};

	combo_entry_t entries[] = {
		{ pixel_height_offset( 2.0f ), pixel_height_offset( 2.195312f ), "drop down", "drop down" },
		{ pixel_height_offset( 44.49794f ), pixel_height_offset( 2.12471f ), "2 hop", "2 hop + crouch" },
		{ pixel_height_offset( 44.49794f ), pixel_height_offset( 2.12471f ), "mj + 2 hop", "mj + 2 hop + crouch" },
		{ pixel_height_offset( 46.585155f ), pixel_height_offset( 2.097992f ), "lj + 2 hop", "lj + 2 hop + crouch" },
		{ pixel_height_offset( 46.328964f ), pixel_height_offset( 2.112167f ), "lj hop + hop", "lj hop + hop + crouch" },
		{ pixel_height_offset( 46.349533f ), pixel_height_offset( 2.05442f ), "mj hop + hop", "mj hop + hop + crouch" },
		{ pixel_height_offset( 37.306175f ), pixel_height_offset( 2.03862f ), "hop + mj hop", "hop + mj hop + crouch" },
		{ pixel_height_offset( 46.306168f ), pixel_height_offset( 2.03862f ), "hop + lj hop", "hop + lj hop + crouch" },
		{ pixel_height_offset( 45.505562f ), pixel_height_offset( 2.076744f ), "mj + mj hop + hop", "mj + mj hop + hop + crouch" },
		{ pixel_height_offset( 45.694935f ), pixel_height_offset( 2.114349f ), "lj + lj hop + hop", "lj + lj hop + hop + crouch" },
		{ pixel_height_offset( 45.928982f ), pixel_height_offset( 2.05658f ), "lj + mj hop + hop", "lj + mj hop + hop + crouch" },
		{ pixel_height_offset( 44.292542f ), pixel_height_offset( 2.134491f ), "mj + lj hop + hop", "mj + lj hop + hop + crouch" },
		{ pixel_height_offset( 48.405243f ), pixel_height_offset( 2.011292f ), "lj + hop + lj hop", "lj + hop + lj hop + crouch" },
		{ pixel_height_offset( 39.405258f ), pixel_height_offset( 2.011307f ), "lj + hop + mj hop", "lj + hop + mj hop + crouch" },
		{ pixel_height_offset( 37.443584f ), pixel_height_offset( 2.032089f ), "mj + hop + mj hop", "mj + hop + mj hop + crouch" },
		{ pixel_height_offset( 46.4436f ), pixel_height_offset( 2.032074f ), "mj + hop + lj hop", "mj + hop + lj hop + crouch" },
		{ pixel_height_offset( 39.222183f ), pixel_height_offset( 2.162125f ), "2 mj hop", "2 mj hop + crouch" },
		{ pixel_height_offset( 48.142784f ), pixel_height_offset( 2.025795f ), "2 lj hop", "2 lj hop + crouch" },
		{ pixel_height_offset( 48.222176f ), pixel_height_offset( 2.162125f ), "mj hop + lj hop", "mj hop + lj hop + crouch" },
		{ pixel_height_offset( 39.142769f ), pixel_height_offset( 2.02578f ), "lj hop + mj hop", "lj hop + mj hop + crouch" },
		{ pixel_height_offset( 47.50787f ), pixel_height_offset( 2.028015f ), "lj + 2 lj hop", "lj + 2 lj hop + crouch" },
		{ pixel_height_offset( 38.345467f ), pixel_height_offset( 2.184952f ), "mj + 2 mj hop", "mj + 2 mj hop + crouch" },
		{ pixel_height_offset( 46.096443f ), pixel_height_offset( 2.048607f ), "mj + 2 lj hop", "mj + 2 lj hop + crouch" },
		{ pixel_height_offset( 38.798359f ), pixel_height_offset( 2.164352f ), "lj + 2 mj hop", "lj + 2 mj hop + crouch" },
		{ pixel_height_offset( 47.798344f ), pixel_height_offset( 2.164368f ), "lj + mj hop + lj hop", "lj + mj hop + lj hop + crouch" },
		{ pixel_height_offset( 37.096436f ), pixel_height_offset( 2.048592f ), "mj + lj hop + mj hop", "mj + lj hop + mj hop + crouch" },
		{ pixel_height_offset( 35.36142f ), pixel_height_offset( 2.168922f ), "lj + crouch hop", "lj + crouch hop + crouch" },
		{ pixel_height_offset( 37.303425f ), pixel_height_offset( 2.10994f ), "mj + crouch hop", "mj + crouch hop + crouch" },
		{ pixel_height_offset( 35.835266f ), pixel_height_offset( 2.15416f ), "crouch hop", "crouch hop + crouch" },
		{ pixel_height_offset( 37.388512f ), pixel_height_offset( 2.034714f ), "2 crouch hop", "2 crouch hop + crouch" },
		{ pixel_height_offset( 37.148033f ), pixel_height_offset( 2.046165f ), "3 crouch hop", "3 crouch hop + crouch" },
		{ pixel_height_offset( 37.111656f ), pixel_height_offset( 2.047867f ), "4 (or more) crouch hop", "4 (or more) crouch hop + crouch" },
		{ pixel_height_offset( 47.575405f ), pixel_height_offset( 2.034225f ), "crouch hop + hop", "crouch hop + hop + crouch" },
		{ pixel_height_offset( 36.067513f ), pixel_height_offset( 2.097588f ), "hop + crouch hop", "hop + crouch hop + crouch" },
		{ pixel_height_offset( 38.507915f ), pixel_height_offset( 2.02803f ), "mj + 2 crouch hop", "mj + 2 crouch hop + crouch" },
		{ pixel_height_offset( 37.169167f ), pixel_height_offset( 2.045128f ), "mj + 3 crouch hop", "mj + 3 crouch hop + crouch" },
		{ pixel_height_offset( 39.834682f ), pixel_height_offset( 2.007446f ), "lj + 2 crouch hop", "lj + 2 crouch hop + crouch" },
		{ pixel_height_offset( 38.78841f ), pixel_height_offset( 2.042053f ), "lj + 3 crouch hop", "lj + 3 crouch hop + crouch" },
		{ pixel_height_offset( 37.124794f ), pixel_height_offset( 2.047276f ), "lj + 4 crouch hop", "lj + 4 crouch hop + crouch" },
		{ pixel_height_offset( 49.997486f ), pixel_height_offset( 2.066513f ), "mj", "mj + crouch" },
		{ pixel_height_offset( 58.997456f ), pixel_height_offset( 2.066498f ), "lj", "lj + crouch" },
		{ pixel_height_offset( 56.653721f ), pixel_height_offset( 2.164169f ), "jump", "jump + crouch" },
		{ pixel_height_offset( 44.798252f ), pixel_height_offset( 2.18013f ), "hop", "hop + crouch" },
		{ pixel_height_offset( 44.526211f ), pixel_height_offset( 2.194565f ), "lj + hop", "lj + hop + crouch" },
		{ pixel_height_offset( 45.661243f ), pixel_height_offset( 2.13681f ), "mj + hop", "mj + hop + crouch" },
		{ pixel_height_offset( 37.581924f ), pixel_height_offset( 2.095177f ), "mj hop", "mj hop + crouch" },
		{ pixel_height_offset( 46.581947f ), pixel_height_offset( 2.095207f ), "lj hop", "lj hop + crouch" },
		{ pixel_height_offset( 37.303394f ), pixel_height_offset( 2.10994f ), "lj + mj hop", "lj + mj hop + crouch" },
		{ pixel_height_offset( 47.464214f ), pixel_height_offset( 2.050949f ), "mj + lj hop", "mj + lj hop + crouch" },
		{ pixel_height_offset( 46.303379f ), pixel_height_offset( 2.109924f ), "lj + lj hop", "lj + lj hop + crouch" },
		{ pixel_height_offset( 38.464184f ), pixel_height_offset( 2.050949f ), "mj + mj hop", "mj + mj hop + crouch" },
		{ pixel_height_offset( 36.30547f ), pixel_height_offset( 2.091064f ), "mj + hop + crouch hop", "mj + hop + crouch hop + crouch" },
		{ pixel_height_offset( 39.142776f ), pixel_height_offset( 2.025795f ), "mj hop + crouch hop", "mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 37.096451f ), pixel_height_offset( 2.048607f ), "mj + mj hop + crouch hop", "mj + mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 36.021801f ), pixel_height_offset( 2.099792f ), "lj + hop + crouch hop", "lj + hop + crouch hop + crouch" },
		{ pixel_height_offset( 35.717518f ), pixel_height_offset( 2.114273f ), "lj hop + crouch hop", "lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 37.650806f ), pixel_height_offset( 2.087006f ), "lj + lj hop + crouch hop", "lj + lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 38.50787f ), pixel_height_offset( 2.028f ), "lj + mj hop + crouch hop", "lj + mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 35.857803f ), pixel_height_offset( 2.10759f ), "mj + lj hop + crouch hop", "mj + lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 45.928982f ), pixel_height_offset( 2.056595f ), "mj + crouch hop + hop", "mj + crouch hop + hop + crouch" },
		{ pixel_height_offset( 40.477528f ), pixel_height_offset( 2.141525f ), "crouch hop + mj hop", "crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 38.798359f ), pixel_height_offset( 2.164352f ), "mj + crouch hop + mj hop", "mj + crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 46.790314f ), pixel_height_offset( 2.036423f ), "lj + crouch hop + hop", "lj + crouch hop + hop + crouch" },
		{ pixel_height_offset( 49.477528f ), pixel_height_offset( 2.141541f ), "crouch hop + lj hop", "crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 48.689182f ), pixel_height_offset( 2.143753f ), "lj + crouch hop + lj hop", "lj + crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 47.798344f ), pixel_height_offset( 2.164352f ), "mj + crouch hop + lj hop", "mj + crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 39.689213f ), pixel_height_offset( 2.143768f ), "lj + crouch hop + mj hop", "lj + crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 58.117512f ), pixel_height_offset( 2.164169f ), "jb", "jb + crouch" },
		{ pixel_height_offset( 58.070759f ), pixel_height_offset( 2.164169f ), "mj + jb", "mj + jb + crouch" },
		{ pixel_height_offset( 58.148655f ), pixel_height_offset( 2.164169f ), "lj + jb", "lj + jb + crouch" },
		{ pixel_height_offset( 57.831043f ), pixel_height_offset( 2.164169f ), "crouch hop + jb", "crouch hop + jb + crouch" },
		{ pixel_height_offset( 57.984516f ), pixel_height_offset( 2.164169f ), "2 crouch hop + jb", "2 crouch hop + jb + crouch" },
		{ pixel_height_offset( 57.526463f ), pixel_height_offset( 2.164169f ), "3 crouch hop + jb", "3 crouch hop + jb + crouch" },
		{ pixel_height_offset( 57.457188f ), pixel_height_offset( 2.164169f ), "4 crouch hop + jb", "4 crouch hop + jb + crouch" },
		{ pixel_height_offset( 57.028584f ), pixel_height_offset( 2.164169f ), "mj hop + jb", "mj hop + jb + crouch" },
		{ pixel_height_offset( 57.827671f ), pixel_height_offset( 2.164169f ), "hop + mj hop + jb", "hop + mj hop + jb + crouch" },
		{ pixel_height_offset( 57.397099f ), pixel_height_offset( 2.164169f ), "2 mj hop + jb", "2 mj hop + jb + crouch" },
		{ pixel_height_offset( 58.179234f ), pixel_height_offset( 2.164169f ), "lj + hop + jb", "lj + hop + jb + crouch" },
		{ pixel_height_offset( 58.221012f ), pixel_height_offset( 2.164169f ), "lj hop + jb", "lj hop + jb + crouch" },
		{ pixel_height_offset( 57.632755f ), pixel_height_offset( 2.164169f ), "lj + lj hop + jb", "lj + lj hop + jb + crouch" },
		{ pixel_height_offset( 57.11182f ), pixel_height_offset( 2.164169f ), "2 lj hop + jb", "2 lj hop + jb + crouch" },
		{ pixel_height_offset( 57.532032f ), pixel_height_offset( 2.164169f ), "mj + mj hop + hop + jb", "mj + mj hop + hop + jb + crouch" },
		{ pixel_height_offset( 58.08942f ), pixel_height_offset( 2.164169f ), "mj + hop + mj hop + jb", "mj + hop + mj hop + jb + crouch" },
		{ pixel_height_offset( 58.37888f ), pixel_height_offset( 2.164169f ), "lj + mj hop + hop + jb", "lj + mj hop + hop + jb + crouch" },
		{ pixel_height_offset( 56.723469f ), pixel_height_offset( 2.164169f ), "lj + hop + mj hop + jb", "lj + hop + mj hop + jb + crouch" },
		{ pixel_height_offset( 56.821491f ), pixel_height_offset( 2.164169f ), "lj + 4 hop + jb", "lj + 4 hop + jb + crouch" },
		{ pixel_height_offset( 57.538944f ), pixel_height_offset( 2.164169f ), "lj + 3 hop + jb", "lj + 3 hop + jb + crouch" },
		{ pixel_height_offset( 58.165714f ), pixel_height_offset( 2.164169f ), "lj + 2 hop + jb", "lj + 2 hop + jb + crouch" },
		{ pixel_height_offset( 36.243351f ), pixel_height_offset( 2.089249f ), "jb + lj hop + crouch hop", "jb + lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 37.482037f ), pixel_height_offset( 2.030258f ), "jb + mj hop + crouch hop", "jb + mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 48.202229f ), pixel_height_offset( 2.145996f ), "jb + crouch hop + lj hop", "jb + crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 39.202259f ), pixel_height_offset( 2.145996f ), "jb + crouch hop + mj hop", "jb + crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 36.607971f ), pixel_height_offset( 2.072472f ), "jb + hop + crouch hop", "jb + hop + crouch hop + crouch" },
		{ pixel_height_offset( 46.306488f ), pixel_height_offset( 2.038605f ), "jb + crouch hop + hop", "jb + crouch hop + hop + crouch" },
		{ pixel_height_offset( 46.833923f ), pixel_height_offset( 2.013489f ), "jb + hop + lj hop", "jb + hop + lj hop + crouch" },
		{ pixel_height_offset( 44.670063f ), pixel_height_offset( 2.116531f ), "jb + lj hop + hop", "jb + lj hop + hop + crouch" },
		{ pixel_height_offset( 37.833923f ), pixel_height_offset( 2.013489f ), "jb + hop + mj hop", "jb + hop + mj hop + crouch" },
		{ pixel_height_offset( 47.861366f ), pixel_height_offset( 2.029869f ), "jb + mj hop + hop", "jb + mj hop + hop + crouch" },
		{ pixel_height_offset( 37.482037f ), pixel_height_offset( 2.030243f ), "jb + lj hop + mj hop", "jb + lj hop + mj hop + crouch" },
		{ pixel_height_offset( 49.769958f ), pixel_height_offset( 2.137085f ), "jb + mj hop + lj hop", "jb + mj hop + lj hop + crouch" },
		{ pixel_height_offset( 40.769958f ), pixel_height_offset( 2.137085f ), "jb + 2 mj hop", "jb + 2 mj hop + crouch" },
		{ pixel_height_offset( 46.482052f ), pixel_height_offset( 2.030243f ), "jb + 2 lj hop", "jb + 2 lj hop + crouch" },
		{ pixel_height_offset( 37.108013f ), pixel_height_offset( 2.04805f ), "jb + 5 crouch hop", "jb + 5 crouch hop + crouch" },
		{ pixel_height_offset( 37.12368f ), pixel_height_offset( 2.04731f ), "jb + 4 crouch hop", "jb + 4 crouch hop + crouch" },
		{ pixel_height_offset( 37.289478f ), pixel_height_offset( 2.042374f ), "jb + 3 crouch hop", "jb + 3 crouch hop + crouch" },
		{ pixel_height_offset( 38.391796f ), pixel_height_offset( 2.009659f ), "jb + 2 crouch hop", "jb + 2 crouch hop + crouch" },
		{ pixel_height_offset( 34.260181f ), pixel_height_offset( 2.18367f ), "jb + crouch hop", "jb + crouch hop + crouch" },
		{ pixel_height_offset( 44.706718f ), pixel_height_offset( 2.114784f ), "jb + 4 hop", "jb + 4 hop + crouch" },
		{ pixel_height_offset( 44.746452f ), pixel_height_offset( 2.112892f ), "jb + 3 hop", "jb + 3 hop + crouch" },
		{ pixel_height_offset( 45.014797f ), pixel_height_offset( 2.100113f ), "jb + 2 hop", "jb + 2 hop + crouch" },
		{ pixel_height_offset( 44.498882f ), pixel_height_offset( 2.12468f ), "jb + lj hop", "jb + lj hop + crouch" },
		{ pixel_height_offset( 42.74173f ), pixel_height_offset( 2.013695f ), "jb + hop", "jb + hop + crouch" },
		{ pixel_height_offset( 35.498852f ), pixel_height_offset( 2.12468f ), "jb + mj hop", "jb + mj hop + crouch" },
		{ pixel_height_offset( 44.670029f ), pixel_height_offset( 2.116531f ), "3 hop", "3 hop + crouch" },
		{ pixel_height_offset( 44.695377f ), pixel_height_offset( 2.115326f ), "4 hop", "4 hop + crouch" },
		{ pixel_height_offset( 46.265015f ), pixel_height_offset( 2.112579f ), "lj + 3 hop", "lj + 3 hop + crouch" },
		{ pixel_height_offset( 45.592903f ), pixel_height_offset( 2.114738f ), "lj + 4 hop", "lj + 4 hop + crouch" },
		{ pixel_height_offset( 44.868847f ), pixel_height_offset( 2.115051f ), "lj + 5 hop", "lj + 5 hop + crouch" },
		{ pixel_height_offset( 44.699188f ), pixel_height_offset( 2.115143f ), "5 hop", "5 hop + crouch" },
		{ pixel_height_offset( 44.699795f ), pixel_height_offset( 2.115112f ), "6 hop", "6 hop + crouch" },
		{ pixel_height_offset( 45.666786f ), pixel_height_offset( 2.114677f ), "lj hop + 2 hop", "lj hop + 2 hop + crouch" },
		{ pixel_height_offset( 44.330849f ), pixel_height_offset( 2.132683f ), "hop + lj hop + hop", "hop + lj hop + hop + crouch" },
		{ pixel_height_offset( 46.481861f ), pixel_height_offset( 2.030258f ), "2 hop + lj hop", "2 hop + lj hop + crouch" },
		{ pixel_height_offset( 44.98024f ), pixel_height_offset( 2.114998f ), "lj + lj hop + 2 hop", "lj + lj hop + 2 hop + crouch" },
		{ pixel_height_offset( 44.363728f ), pixel_height_offset( 2.131119f ), "lj + 2 lj hop + hop", "lj + 2 lj hop + hop + crouch" },
		{ pixel_height_offset( 46.169182f ), pixel_height_offset( 2.045143f ), "lj + 3 lj hop", "lj + 3 lj hop + crouch" },
		{ pixel_height_offset( 44.82885f ), pixel_height_offset( 2.130783f ), "2 lj hop + hop", "2 lj hop + hop + crouch" },
		{ pixel_height_offset( 46.135601f ), pixel_height_offset( 2.046738f ), "hop + 2 lj hop", "hop + 2 lj hop + crouch" },
		{ pixel_height_offset( 46.634338f ), pixel_height_offset( 2.044807f ), "3 lj hop", "3 lj hop + crouch" },
		{ pixel_height_offset( 47.479485f ), pixel_height_offset( 2.028358f ), "lj hop + hop + lj hop", "lj hop + hop + lj hop + crouch" },
		{ pixel_height_offset( 48.078674f ), pixel_height_offset( 2.026215f ), "lj + 2 hop + lj hop", "lj + 2 hop + lj hop + crouch" },
		{ pixel_height_offset( 45.45496f ), pixel_height_offset( 2.128647f ), "lj + hop + lj hop + hop", "lj + hop + lj hop + hop + crouch" },
		{ pixel_height_offset( 47.261536f ), pixel_height_offset( 2.042618f ), "lj + hop + 2 lj hop", "lj + hop + 2 lj hop + crouch" },
		{ pixel_height_offset( 46.79277f ), pixel_height_offset( 2.028687f ), "lj + lj hop + hop + lj hop", "lj + lj hop + hop + lj hop + crouch" },
		{ pixel_height_offset( 44.337719f ), pixel_height_offset( 2.132355f ), "lj + crouch hop + 2 hop", "lj + crouch hop + 2 hop + crouch" },
		{ pixel_height_offset( 46.217319f ), pixel_height_offset( 2.070312f ), "lj + 2 crouch hop + hop", "lj + 2 crouch hop + hop + crouch" },
		{ pixel_height_offset( 45.960419f ), pixel_height_offset( 2.055084f ), "lj + hop + crouch hop + hop",
		  "lj + hop + crouch hop + hop + crouch" },
		{ pixel_height_offset( 37.560932f ), pixel_height_offset( 2.026489f ), "lj + hop + 2 crouch hop", "lj + hop + 2 crouch hop + crouch" },
		{ pixel_height_offset( 35.903938f ), pixel_height_offset( 2.105392f ), "lj + crouch hop + hop + crouch hop",
		  "lj + crouch hop + hop + crouch hop + crouch" },
		{ pixel_height_offset( 44.344452f ), pixel_height_offset( 2.132019f ), "crouch hop + 2 hop", "crouch hop + 2 hop + crouch" },
		{ pixel_height_offset( 47.493374f ), pixel_height_offset( 2.045456f ), "2 crouch hop + hop", "2 crouch hop + hop + crouch" },
		{ pixel_height_offset( 35.910835f ), pixel_height_offset( 2.10508f ), "crouch hop + hop + crouch hop",
		  "crouch hop + hop + crouch hop + crouch" },
		{ pixel_height_offset( 37.570156f ), pixel_height_offset( 2.026169f ), "hop + 2 crouch hop", "hop + 2 crouch hop + crouch" },
		{ pixel_height_offset( 45.967182f ), pixel_height_offset( 2.054779f ), "hop + crouch hop + hop", "hop + crouch hop + hop + crouch" },
		{ pixel_height_offset( 45.802128f ), pixel_height_offset( 2.062622f ), "lj + crouch hop + 2 lj hop", "lj + crouch hop + 2 lj hop + crouch" },
		{ pixel_height_offset( 44.004311f ), pixel_height_offset( 2.148216f ), "lj + crouch hop + lj hop + hop",
		  "lj + crouch hop + lj hop + hop + crouch" },
		{ pixel_height_offset( 46.142567f ), pixel_height_offset( 2.046391f ), "lj + crouch hop + hop + lj hop",
		  "lj + crouch hop + hop + lj hop + crouch" },
		{ pixel_height_offset( 44.01125f ), pixel_height_offset( 2.147903f ), "crouch hop + lj hop + hop", "crouch hop + lj hop + hop + crouch" },
		{ pixel_height_offset( 45.809193f ), pixel_height_offset( 2.062271f ), "crouch hop + 2 lj hop", "crouch hop + 2 lj hop + crouch" },
		{ pixel_height_offset( 46.14946f ), pixel_height_offset( 2.046066f ), "crouch hop + hop + lj hop", "crouch hop + hop + lj hop + crouch" },
		{ pixel_height_offset( 45.960419f ), pixel_height_offset( 2.055099f ), "lj + hop + crouch hop + hop",
		  "lj + hop + crouch hop + hop + crouch" },
		{ pixel_height_offset( 48.299812f ), pixel_height_offset( 2.028343f ), "lj hop + crouch hop + hop", "lj hop + crouch hop + hop + crouch" },
		{ pixel_height_offset( 47.839298f ), pixel_height_offset( 2.162491f ), "hop + crouch hop + lj hop", "hop + crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 50.210602f ), pixel_height_offset( 2.135513f ), "lj hop + crouch hop + lj hop",
		  "lj hop + crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 47.479485f ), pixel_height_offset( 2.028358f ), "lj + lj hop + crouch hop + hop",
		  "lj + lj hop + crouch hop + hop + crouch" },
		{ pixel_height_offset( 48.473785f ), pixel_height_offset( 2.160873f ), "lj + lj hop + crouch hop + lj hop",
		  "lj + lj hop + crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 47.83218f ), pixel_height_offset( 2.162811f ), "lj + hop + crouch hop + lj hop",
		  "lj + hop + crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 48.066753f ), pixel_height_offset( 2.17836f ), "lj + 2 crouch hop + lj hop", "lj + 2 crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 49.379147f ), pixel_height_offset( 2.152969f ), "2 crouch hop + lj hop", "2 crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 37.61808f ), pixel_height_offset( 2.087341f ), "lj hop + hop + crouch hop", "lj hop + hop + crouch hop + crouch" },
		{ pixel_height_offset( 35.896915f ), pixel_height_offset( 2.105728f ), "hop + lj hop + crouch hop", "hop + lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 36.555054f ), pixel_height_offset( 2.10379f ), "2 lj hop + crouch hop", "2 lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 35.930542f ), pixel_height_offset( 2.104126f ), "lj + 2 lj hop + crouch hop", "lj + 2 lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 35.563519f ), pixel_height_offset( 2.121597f ), "lj + crouch hop + lj hop + crouch hop",
		  "lj + crouch hop + lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 35.570614f ), pixel_height_offset( 2.121246f ), "crouch hop + lj hop + crouch hop",
		  "crouch hop + lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 39.409531f ), pixel_height_offset( 2.024544f ), "lj + lj hop + 2 crouch hop", "lj + lj hop + 2 crouch hop + crouch" },
		{ pixel_height_offset( 37.515041f ), pixel_height_offset( 2.028671f ), "lj hop + 2 crouch hop", "lj hop + 2 crouch hop + crouch" },
		{ pixel_height_offset( 45.697342f ), pixel_height_offset( 2.109436f ), "mj + mj hop + 2 hop", "mj + mj hop + 2 hop + crouch" },
		{ pixel_height_offset( 45.69606f ), pixel_height_offset( 2.067673f ), "mj + 2 mj hop + hop", "mj + 2 mj hop + hop + crouch" },
		{ pixel_height_offset( 38.549225f ), pixel_height_offset( 2.175674f ), "mj + 3 mj hop", "mj + 3 mj hop + crouch" },
		{ pixel_height_offset( 38.585472f ), pixel_height_offset( 2.174026f ), "mj + 4 (or more) mj hop", "mj + 4 (or more) mj hop + crouch" },
		{ pixel_height_offset( 47.606373f ), pixel_height_offset( 2.045074f ), "mj + hop + mj hop + hop", "mj + hop + mj hop + hop + crouch" },
		{ pixel_height_offset( 37.502224f ), pixel_height_offset( 2.029282f ), "mj + 2 hop + mj hop", "mj + 2 hop + mj hop + crouch" },
		{ pixel_height_offset( 44.281822f ), pixel_height_offset( 2.135017f ), "mj hop + 2 hop", "mj hop + 2 hop + crouch" },
		{ pixel_height_offset( 47.324383f ), pixel_height_offset( 2.046036f ), "hop + mj hop + hop", "hop + mj hop + hop + crouch" },
		{ pixel_height_offset( 37.481876f ), pixel_height_offset( 2.030258f ), "2 hop + mj hop", "2 hop + mj hop + crouch" },
		{ pixel_height_offset( 46.510254f ), pixel_height_offset( 2.064301f ), "2 mj hop + hop", "2 mj hop + hop + crouch" },
		{ pixel_height_offset( 40.209267f ), pixel_height_offset( 2.15358f ), "hop + 2 mj hop", "hop + 2 mj hop + crouch" },
		{ pixel_height_offset( 39.36842f ), pixel_height_offset( 2.172226f ), "3 mj hop", "3 mj hop + crouch" },
		{ pixel_height_offset( 37.085548f ), pixel_height_offset( 2.049126f ), "mj hop + hop + mj hop", "mj hop + hop + mj hop + crouch" },
		{ pixel_height_offset( 38.512302f ), pixel_height_offset( 2.02301f ), "mj + mj hop + hop + mj hop", "mj + mj hop + hop + mj hop + crouch" },
		{ pixel_height_offset( 40.492733f ), pixel_height_offset( 2.152603f ), "mj + hop + 2 mj hop", "mj + hop + 2 mj hop + crouch" },
		{ pixel_height_offset( 46.606709f ), pixel_height_offset( 2.106461f ), "mj + crouch hop + 2 hop", "mj + crouch hop + 2 hop + crouch" },
		{ pixel_height_offset( 35.840012f ), pixel_height_offset( 2.108437f ), "mj + crouch hop + hop + crouch hop",
		  "mj + crouch hop + hop + crouch hop + crouch" },
		{ pixel_height_offset( 45.987576f ), pixel_height_offset( 2.053802f ), "mj + hop + crouch hop + hop",
		  "mj + hop + crouch hop + hop + crouch" },
		{ pixel_height_offset( 45.576809f ), pixel_height_offset( 2.073334f ), "mj + 2 crouch hop + hop", "mj + 2 crouch hop + hop + crouch" },
		{ pixel_height_offset( 37.966171f ), pixel_height_offset( 2.025169f ), "mj + hop + 2 crouch hop", "mj + hop + 2 crouch hop + crouch" },
		{ pixel_height_offset( 38.894909f ), pixel_height_offset( 2.172562f ), "mj + crouch hop + 2 mj hop", "mj + crouch hop + 2 mj hop + crouch" },
		{ pixel_height_offset( 46.037247f ), pixel_height_offset( 2.064629f ), "mj + crouch hop + mj hop + hop",
		  "mj + crouch hop + mj hop + hop + crouch" },
		{ pixel_height_offset( 39.423092f ), pixel_height_offset( 2.019943f ), "mj + crouch hop + hop + mj hop",
		  "mj + crouch hop + hop + mj hop + crouch" },
		{ pixel_height_offset( 39.776566f ), pixel_height_offset( 2.155106f ), "mj + mj hop + crouch hop + mj hop",
		  "mj + mj hop + crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 38.861069f ), pixel_height_offset( 2.161484f ), "mj + hop + crouch hop + mj hop",
		  "mj + hop + crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 39.776566f ), pixel_height_offset( 2.155106f ), "mj + mj hop + crouch hop + hop",
		  "mj + mj hop + crouch hop + hop + crouch" },
		{ pixel_height_offset( 37.737106f ), pixel_height_offset( 2.03933f ), "mj + 2 mj hop + crouch hop", "mj + 2 mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 37.721718f ), pixel_height_offset( 2.082008f ), "mj + mj hop + hop + crouch hop",
		  "mj + mj hop + hop + crouch hop + crouch" },
		{ pixel_height_offset( 37.156357f ), pixel_height_offset( 2.045769f ), "mj + hop + mj hop + crouch hop",
		  "mj + hop + mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 38.421661f ), pixel_height_offset( 2.181488f ), "mj + 2 crouch hop + mj hop", "mj + 2 crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 37.103939f ), pixel_height_offset( 2.048248f ), "mj + hop + 2 crouch hop", "mj + mj hop + 2 crouch hop + crouch" },
		{ pixel_height_offset( 38.047909f ), pixel_height_offset( 2.003296f ), "crouch hop + 2 mj hop", "crouch hop + 2 mj hop + crouch" },
		{ pixel_height_offset( 45.224331f ), pixel_height_offset( 2.090118f ), "crouch hop + mj hop + hop", "crouch hop + mj hop + hop + crouch" },
		{ pixel_height_offset( 37.14946f ), pixel_height_offset( 2.046066f ), "crouch hop + hop + mj hop", "crouch hop + hop + mj hop + crouch" },
		{ pixel_height_offset( 38.429054f ), pixel_height_offset( 2.181152f ), "mj hop + crouch hop + mj hop",
		  "mj hop + crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 45.583729f ), pixel_height_offset( 2.073021f ), "mj hop + crouch hop + hop", "mj hop + crouch hop + hop + crouch" },
		{ pixel_height_offset( 38.839314f ), pixel_height_offset( 2.162506f ), "hop + crouch hop + mj hop", "hop + crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 39.16518f ), pixel_height_offset( 2.035904f ), "2 mj hop + crouch hop", "2 mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 35.846893f ), pixel_height_offset( 2.108109f ), "mj hop + hop + crouch hop", "mj hop + hop + crouch hop + crouch" },
		{ pixel_height_offset( 37.135555f ), pixel_height_offset( 2.046738f ), "hop + mj hop + crouch hop", "hop + mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 40.379131f ), pixel_height_offset( 2.152985f ), "2 crouch hop + mj hop", "2 crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 37.634323f ), pixel_height_offset( 2.044807f ), "mj hop + 2 crouch hop", "mj hop + 2 crouch hop + crouch" },
		{ pixel_height_offset( 39.776566f ), pixel_height_offset( 2.155106f ), "mj + lj hop + 2 mj hop", "mj + lj hop + 2 mj hop + crouch" },
		{ pixel_height_offset( 49.492733f ), pixel_height_offset( 2.152573f ), "mj + hop + mj hop + lj hop", "mj + hop + mj hop + lj hop + crouch" },
		{ pixel_height_offset( 46.893822f ), pixel_height_offset( 2.047516f ), "mj + lj hop + mj hop + hop", "mj + lj hop + mj hop + hop + crouch" },
		{ pixel_height_offset( 37.169182f ), pixel_height_offset( 2.045158f ), "lj + 2 lj hop + mj hop", "lj + 2 lj hop + mj hop + crouch" },
		{ pixel_height_offset( 47.421646f ), pixel_height_offset( 2.181458f ), "lj + lj hop + mj hop + lj hop",
		  "lj + lj hop + mj hop + lj hop + crouch" },
		{ pixel_height_offset( 37.450829f ), pixel_height_offset( 2.031715f ), "mj + lj hop + hop + mj hop", "mj + lj hop + hop + mj hop + crouch" },
		{ pixel_height_offset( 37.156342f ), pixel_height_offset( 2.045738f ), "mj + hop + lj hop + mj hop", "mj + hop + lj hop + mj hop + crouch" },
		{ pixel_height_offset( 38.421646f ), pixel_height_offset( 2.181458f ), "lj + lj hop + 2 mj hop", "lj + lj hop + 2 mj hop + crouch" },
		{ pixel_height_offset( 37.79277f ), pixel_height_offset( 2.028702f ), "lj + lj hop + hop + mj hop", "lj + lj hop + hop + mj hop + crouch" },
		{ pixel_height_offset( 45.576809f ), pixel_height_offset( 2.073334f ), "lj + lj hop + mj hop + hop", "lj + lj hop + mj hop + hop + crouch" },
		{ pixel_height_offset( 38.261536f ), pixel_height_offset( 2.042633f ), "lj + hop + lj hop + mj hop", "lj + hop + lj hop + mj hop + crouch" },
		{ pixel_height_offset( 45.878326f ), pixel_height_offset( 2.12241f ), "lj + mj hop + lj hop + hop", "lj + mj hop + lj hop + hop + crouch" },
		{ pixel_height_offset( 48.423138f ), pixel_height_offset( 2.019958f ), "lj + mj hop + hop + lj hop", "lj + mj hop + hop + lj hop + crouch" },
		{ pixel_height_offset( 47.546967f ), pixel_height_offset( 2.178947f ), "lj + hop + mj hop + lj hop", "lj + hop + mj hop + lj hop + crouch" },
		{ pixel_height_offset( 47.894924f ), pixel_height_offset( 2.172562f ), "lj + 2 mj hop + lj hop", "lj + 2 mj hop + lj hop + crouch" },
		{ pixel_height_offset( 47.687534f ), pixel_height_offset( 2.03624f ), "lj + mj hop + 2 lj hop", "lj + mj hop + 2 lj hop + crouch" },
		{ pixel_height_offset( 38.68755f ), pixel_height_offset( 2.03624f ), "lj + mj hop + lj hop + mj hop",
		  "lj + mj hop + lj hop + mj hop + crouch" },
		{ pixel_height_offset( 48.776558f ), pixel_height_offset( 2.15509f ), "mj + lj hop + mj hop + lj hop",
		  "mj + lj hop + mj hop + lj hop + crouch" },
		{ pixel_height_offset( 46.737099f ), pixel_height_offset( 2.03933f ), "mj + mj hop + 2 lj hop", "mj + mj hop + 2 lj hop + crouch" },
		{ pixel_height_offset( 46.450844f ), pixel_height_offset( 2.031731f ), "mj + lj hop + hop + lj hop", "mj + lj hop + hop + lj hop + crouch" },
		{ pixel_height_offset( 39.423107f ), pixel_height_offset( 2.019958f ), "lj + mj hop + hop + mj hop", "lj + mj hop + hop + mj hop + crouch" },
		{ pixel_height_offset( 37.103954f ), pixel_height_offset( 2.048248f ), "mj + 2 lj hop + mj hop", "mj + 2 lj hop + mj hop + crouch" },
		{ pixel_height_offset( 37.634338f ), pixel_height_offset( 2.044807f ), "2 lj hop + mj hop", "2 lj hop + mj hop + crouch" },
		{ pixel_height_offset( 47.549225f ), pixel_height_offset( 2.175674f ), "mj + 2 mj hop + lj hop", "mj + 2 mj hop + lj hop + crouch" },
		{ pixel_height_offset( 48.368435f ), pixel_height_offset( 2.172226f ), "2 mj hop + lj hop", "2 mj hop + lj hop + crouch" },
		{ pixel_height_offset( 47.512287f ), pixel_height_offset( 2.02301f ), "mj + mj hop + hop + lj hop", "mj + mj hop + hop + lj hop + crouch" },
		{ pixel_height_offset( 49.209267f ), pixel_height_offset( 2.15358f ), "hop + mj hop + lj hop", "hop + mj hop + lj hop + crouch" },
		{ pixel_height_offset( 46.085548f ), pixel_height_offset( 2.049126f ), "mj hop + hop + lj hop", "mj hop + hop + lj hop + crouch" },
		{ pixel_height_offset( 46.502239f ), pixel_height_offset( 2.029282f ), "mj + 2 hop + lj hop", "mj + 2 hop + lj hop + crouch" },
		{ pixel_height_offset( 37.737114f ), pixel_height_offset( 2.039345f ), "mj + mj hop + lj hop + mj hop",
		  "mj + mj hop + lj hop + mj hop + crouch" },
		{ pixel_height_offset( 39.165195f ), pixel_height_offset( 2.035904f ), "mj hop + lj hop + mj hop", "mj hop + lj hop + mj hop + crouch" },
		{ pixel_height_offset( 44.929222f ), pixel_height_offset( 2.125435f ), "mj + mj hop + lj hop + hop", "mj + mj hop + lj hop + hop + crouch" },
		{ pixel_height_offset( 37.135586f ), pixel_height_offset( 2.046738f ), "hop + lj hop + mj hop", "hop + lj hop + mj hop + crouch" },
		{ pixel_height_offset( 38.479485f ), pixel_height_offset( 2.028358f ), "lj hop + hop + mj hop", "lj hop + hop + mj hop + crouch" },
		{ pixel_height_offset( 45.583748f ), pixel_height_offset( 2.073021f ), "lj hop + mj hop + hop", "lj hop + mj hop + hop + crouch" },
		{ pixel_height_offset( 38.429054f ), pixel_height_offset( 2.181137f ), "lj hop + 2 mj hop", "lj hop + 2 mj hop + crouch" },
		{ pixel_height_offset( 46.355804f ), pixel_height_offset( 2.122063f ), "mj hop + lj hop + hop", "mj hop + lj hop + hop + crouch" },
		{ pixel_height_offset( 46.085548f ), pixel_height_offset( 2.049126f ), "mj hop + hop + lj hop", "mj hop + hop + lj hop + crouch" },
		{ pixel_height_offset( 48.16518f ), pixel_height_offset( 2.035904f ), "mj hop + 2 lj hop", "mj hop + 2 lj hop + crouch" },
		{ pixel_height_offset( 46.10397f ), pixel_height_offset( 2.048248f ), "mj + 3 lj hop", "mj + 3 lj hop + crouch" },
		{ pixel_height_offset( 46.156364f ), pixel_height_offset( 2.045753f ), "mj + hop + 2 lj hop", "mj + hop + 2 lj hop + crouch" },
		{ pixel_height_offset( 44.299892f ), pixel_height_offset( 2.134155f ), "mj + 2 lj hop + hop", "mj + 2 lj hop + hop + crouch" },
		{ pixel_height_offset( 44.351196f ), pixel_height_offset( 2.131714f ), "mj + hop + lj hop + hop", "mj + hop + lj hop + hop + crouch" },
		{ pixel_height_offset( 47.464214f ), pixel_height_offset( 2.050949f ), "mj + lj hop + 2 hop", "mj + lj hop + 2 hop + crouch" },
		{ pixel_height_offset( 38.894909f ), pixel_height_offset( 2.172562f ), "lj + 3 mj hop", "lj + 3 mj hop + crouch" },
		{ pixel_height_offset( 38.546967f ), pixel_height_offset( 2.178947f ), "lj + hop + 2 mj hop", "lj + hop + 2 mj hop + crouch" },
		{ pixel_height_offset( 46.037247f ), pixel_height_offset( 2.064629f ), "lj + 2 mj hop + hop", "lj + 2 mj hop + hop + crouch" },
		{ pixel_height_offset( 39.078674f ), pixel_height_offset( 2.026215f ), "lj + 2 hop + mj hop", "lj + 2 hop + mj hop + crouch" },
		{ pixel_height_offset( 45.698483f ), pixel_height_offset( 2.070877f ), "lj + hop + mj hop + hop", "lj + hop + mj hop + hop + crouch" },
		{ pixel_height_offset( 46.606709f ), pixel_height_offset( 2.106461f ), "lj + mj hop + 2 hop", "lj + mj hop + 2 hop + crouch" },
		{ pixel_height_offset( 40.245232f ), pixel_height_offset( 2.169456f ), "lj + crouch hop + 2 mj hop", "lj + crouch hop + 2 mj hop + crouch" },
		{ pixel_height_offset( 47.383125f ), pixel_height_offset( 2.061584f ), "lj + crouch hop + mj hop + hop",
		  "lj + crouch hop + mj hop + hop + crouch" },
		{ pixel_height_offset( 37.142578f ), pixel_height_offset( 2.04641f ), "lj + crouch hop + hop + mj hop",
		  "lj + crouch hop + hop + mj hop + crouch" },
		{ pixel_height_offset( 38.421661f ), pixel_height_offset( 2.181473f ), "lj + mj hop + crouch hop + mj hop",
		  "lj + mj hop + crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 45.576824f ), pixel_height_offset( 2.073349f ), "lj + mj hop + crouch hop + hop",
		  "lj + mj hop + crouch hop + hop + crouch" },
		{ pixel_height_offset( 38.832165f ), pixel_height_offset( 2.162811f ), "lj + hop + crouch hop + mj hop",
		  "lj + hop + crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 38.68758f ), pixel_height_offset( 2.03624f ), "lj + 2 mj hop + crouch hop", "lj + 2 mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 35.840012f ), pixel_height_offset( 2.108437f ), "lj + mj hop + hop + crouch hop",
		  "lj + mj hop + hop + crouch hop + crouch" },
		{ pixel_height_offset( 38.261536f ), pixel_height_offset( 2.042618f ), "lj + hop + mj hop + crouch hop",
		  "lj + hop + mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 39.066738f ), pixel_height_offset( 2.17836f ), "lj + 2 crouch hop + mj hop", "lj + 2 crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 37.169182f ), pixel_height_offset( 2.045143f ), "lj + mj hop + 2 crouch hop", "lj + mj hop + 2 crouch hop + crouch" },
		{ pixel_height_offset( 36.802143f ), pixel_height_offset( 2.062622f ), "lj + couch hop + mj hop + crouch hop",
		  "lj + couch hop + mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 36.802143f ), pixel_height_offset( 2.062622f ), "lj + crouch hop + lj hop + mj hop",
		  "lj + crouch hop + lj hop + mj hop + crouch" },
		{ pixel_height_offset( 49.245232f ), pixel_height_offset( 2.169456f ), "lj + crouch hop + mj hop + lj hop",
		  "lj + crouch hop + mj hop + lj hop + crouch" },
		{ pixel_height_offset( 39.473785f ), pixel_height_offset( 2.160889f ), "lj + lj hop + crouch hop + mj hop",
		  "lj + lj hop + crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 47.421646f ), pixel_height_offset( 2.181458f ), "lj + mj hop + crouch hop + lj hop",
		  "lj + mj hop + crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 37.169182f ), pixel_height_offset( 2.045143f ), "lj + lj hop + mj hop + crouch hop",
		  "lj + lj hop + mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 37.721828f ), pixel_height_offset( 2.095215f ), "lj + mj hop + lj hop + crouch hop",
		  "lj + mj hop + lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 45.878326f ), pixel_height_offset( 2.122391f ), "mj + crouch hop + lj hop + hop",
		  "mj + crouch hop + lj hop + hop + crouch" },
		{ pixel_height_offset( 48.423138f ), pixel_height_offset( 2.019958f ), "mj + crouch hop + hop + lj hop",
		  "mj + crouch hop + hop + lj hop + crouch" },
		{ pixel_height_offset( 47.687534f ), pixel_height_offset( 2.03624f ), "mj + crouch hop + 2 lj hop", "mj + crouch hop + 2 lj hop + crouch" },
		{ pixel_height_offset( 47.806198f ), pixel_height_offset( 2.163994f ), "mj + lj hop + crouch hop + lj hop",
		  "mj + lj hop + crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 45.936226f ), pixel_height_offset( 2.056236f ), "mj + lj hop + crouch hop + hop",
		  "mj + lj hop + crouch hop + hop + crouch" },
		{ pixel_height_offset( 47.861084f ), pixel_height_offset( 2.161499f ), "mj + hop + crouch hop + lj hop",
		  "mj + hop + crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 35.865238f ), pixel_height_offset( 2.107239f ), "mj + 2 lj hop + crouch hop", "mj + 2 lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 36.212204f ), pixel_height_offset( 2.090729f ), "mj + lj hop + hop + crouch hop",
		  "mj + lj hop + hop + crouch hop + crouch" },
		{ pixel_height_offset( 35.917717f ), pixel_height_offset( 2.104736f ), "mj + hop + lj hop + crouch hop",
		  "mj + hop + lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 47.421661f ), pixel_height_offset( 2.181473f ), "mj + 2 crouch hop + lj hop", "mj + 2 crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 37.536232f ), pixel_height_offset( 2.027664f ), "mj + lj hop + 2 crouch hop", "mj + lj hop + 2 crouch hop + crouch" },
		{ pixel_height_offset( 37.721813f ), pixel_height_offset( 2.0952f ), "mj + crouch hop + lj hop + crouch hop",
		  "mj + crouch hop + lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 38.68755f ), pixel_height_offset( 2.03624f ), "mj + crouch hop + lj hop + mj hop",
		  "mj + crouch hop + lj hop + mj hop + crouch" },
		{ pixel_height_offset( 47.894924f ), pixel_height_offset( 2.172562f ), "mj + crouch hop + mj hop + lj hop",
		  "mj + crouch hop + mj hop + lj hop + crouch" },
		{ pixel_height_offset( 38.806198f ), pixel_height_offset( 2.163994f ), "mj + lj hop + crouch hop + mj hop",
		  "mj + lj hop + crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 48.776558f ), pixel_height_offset( 2.155106f ), "mj + mj hop + crouch hop + lj hop",
		  "mj + mj hop + crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 36.730148f ), pixel_height_offset( 2.098312f ), "mj + mj hop + lj hop + crouch hop",
		  "mj + mj hop + lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 37.103954f ), pixel_height_offset( 2.048248f ), "mj + lj hop + mj hop + crouch hop",
		  "mj + lj hop + mj hop + crouch hop + crouch" },
		{ pixel_height_offset( 47.047939f ), pixel_height_offset( 2.003311f ), "crouch hop + mj hop + lj hop",
		  "crouch hop + mj hop + lj hop + crouch" },
		{ pixel_height_offset( 36.809223f ), pixel_height_offset( 2.062286f ), "crouch hop + lj hop + mj hop",
		  "crouch hop + lj hop + mj hop + crouch" },
		{ pixel_height_offset( 41.210602f ), pixel_height_offset( 2.135529f ), "lj hop + crouch hop + mj hop",
		  "lj hop + crouch hop + mj hop + crouch" },
		{ pixel_height_offset( 47.429047f ), pixel_height_offset( 2.181152f ), "mj hop + crouch hop + lj hop",
		  "mj hop + crouch hop + lj hop + crouch" },
		{ pixel_height_offset( 35.505325f ), pixel_height_offset( 2.12439f ), "mj hop + lj hop + crouch hop",
		  "mj hop + lj hop + crouch hop + crouch" },
		{ pixel_height_offset( 37.634338f ), pixel_height_offset( 2.044807f ), "lj hop + mj hop + crouch hop",
		  "lj hop + mj hop + crouch hop + crouch" },
	};

	for ( const auto& e : entries )
		try_pixel_combo( backup_origin.m_z, e.height, e.gravity_inc, gravity_step, point, e.name, e.name_crouch, data.combo );

	std::sort( data.combo.begin( ), data.combo.end( ), []( const std::string& a, const std::string& b ) { return a.length( ) < b.length( ); } );

	if ( data.combo.empty( ) ) {
		myWindowDetect.AddWindow( 3, "no solutions found" );
	} else {
		myWindowDetect.AddWindow( 6, "calc: " + std::to_string( data.combo.size( ) ) + " solutions (z: " + std::to_string( pv.m_z ) + ")" );
		int show_count = static_cast< int >( data.combo.size( ) ) < 15 ? static_cast< int >( data.combo.size( ) ) : 15;
		for ( int i = 0; i < show_count; ++i )
			myWindowDetect.AddWindow( 6, data.combo[ i ] );
	}
}

bool WriteStringToFile( HANDLE hFile, const std::string& str )
{
	DWORD written;
	DWORD length = str.size( );
	if ( !WriteFile( hFile, &length, sizeof( length ), &written, nullptr ) ) {
		return false;
	}
	if ( !WriteFile( hFile, str.c_str( ), length, &written, nullptr ) ) {
		return false;
	}
	return true;
}
bool ReadStringFromFile( HANDLE hFile, std::string& str )
{
	DWORD read;
	DWORD length;
	if ( !ReadFile( hFile, &length, sizeof( length ), &read, nullptr ) ) {
		return false;
	}
	str.resize( length );
	if ( !ReadFile( hFile, &str[ 0 ], length, &read, nullptr ) ) {
		return false;
	}
	return true;
}
bool n_movement::impl_t::is_bind_restricted_in_radius( const points_check_t& point, const c_vector& player_pos, const std::string& bind_type )
{
	if ( !point.enable_restricted_binds || point.selected_restricted_bind == 0 ) {
		return false;
	}
	float distance       = player_pos.dist_to( point.pos );
	bool has_restriction = false;
	if ( bind_type == "mj" && point.selected_restricted_bind == 1 ) {
		has_restriction = true;
	} else if ( bind_type == "jb" && point.selected_restricted_bind == 2 ) {
		has_restriction = true;
	} else if ( bind_type == "lj" && point.selected_restricted_bind == 3 ) {
		has_restriction = true;
	} else if ( bind_type == "j" && point.selected_restricted_bind == 4 ) {
		has_restriction = true;
	} else if ( bind_type == "ch" && point.selected_restricted_bind == 5 ) {
		has_restriction = true;
	} else if ( bind_type == "mch" && point.selected_restricted_bind == 6 ) {
		has_restriction = true;
	} else if ( bind_type == "hcj" && point.selected_restricted_bind == 7 ) {
		has_restriction = true;
	}
	if ( has_restriction && distance <= point.restricted_binds_radius ) {
		return true;
	}
	return false;
}
void n_movement::impl_t::detect_head_ceiling_hit( )
{
	if ( !g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game( ) )
		return;

	// Получаем позицию головы игрока
	c_vector head_pos = g_ctx.m_local->get_abs_origin( );
	head_pos.m_z += 72.0f; // Высота головы игрока

	// Создаем луч вверх от головы на небольшое расстояние
	c_vector trace_end = head_pos;
	trace_end.m_z += 5.0f; // 5 единиц вверх от головы

	// Выполняем трассировку луча
	ray_t ray( head_pos, trace_end );
	c_trace_filter filter( g_ctx.m_local );
	trace_t trace;
	g_interfaces.m_engine_trace->trace_ray( ray, e_mask::mask_solid, &filter, &trace );

	// Если луч попал в препятствие на расстоянии менее 5 единиц, проверяем что это не игрок
	if ( trace.m_fraction < 1.0f ) {
		// Проверяем, что это не игрок (исключаем столкновения с другими игроками)
		if ( trace.m_hit_entity && trace.m_hit_entity->is_player( ) ) {
			// Это игрок - не считаем ударом головой об потолок
			m_head_ceiling_data.m_was_detected_last_tick = false;
			return;
		}
		// Проверяем, что это не первый тик детекции (чтобы избежать повторного начисления очков)
		if ( !m_head_ceiling_data.m_was_detected_last_tick ) {
			m_head_ceiling_data.m_detected = true;
		}
		m_head_ceiling_data.m_was_detected_last_tick = true;
	} else {
		// Сбрасываем состояние, если удар головой больше не детектируется
		if ( m_head_ceiling_data.m_was_detected_last_tick ) {
			m_head_ceiling_data.m_detected = false;
		}
		m_head_ceiling_data.m_was_detected_last_tick = false;
	}
}
void n_movement::impl_t::fast_ladder( c_user_cmd* cmd )
{
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return;
	if ( !GET_VARIABLE( g_variables.m_fast_ladder, bool ) )
		return;
	if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_fast_ladder_key, key_bind_t ) ) )
		return;
	if ( g_ctx.m_local->get_move_type( ) == move_type_ladder ) {
		if ( cmd->m_side_move == 0 ) {
			cmd->m_view_point.m_y += 45.0f;
		}
		if ( cmd->m_buttons & in_forward ) {
			if ( cmd->m_side_move > 0 )
				cmd->m_view_point.m_y -= 1.0f;
			if ( cmd->m_side_move < 0 )
				cmd->m_view_point.m_y += 1.0f;
		}
		if ( cmd->m_buttons & in_back ) {
			if ( cmd->m_side_move > 0 )
				cmd->m_view_point.m_y += 1.0f;
			if ( cmd->m_side_move < 0 )
				cmd->m_view_point.m_y -= 1.0f;
		}
		if ( cmd->m_buttons & in_forward ) {
			cmd->m_forward_move = 450.f;
		}
		if ( cmd->m_buttons & in_back ) {
			cmd->m_forward_move = -450.f;
		}
		if ( cmd->m_side_move > 0 ) {
			cmd->m_side_move = 450.f;
		}
		if ( cmd->m_side_move < 0 ) {
			cmd->m_side_move = -450.f;
		}
	}
}
void n_movement::impl_t::lirili_larila( c_user_cmd* cmd ) { }
void n_movement::impl_t::god_mode( c_user_cmd* cmd ) { }
void n_movement::impl_t::avoid_collision( c_user_cmd* cmd ) { }
void n_movement::impl_t::AutoStrafe( c_user_cmd* cmd )
{
	if ( !GET_VARIABLE( g_variables.m_auto_strafe, bool ) || !g_input.check_input( &GET_VARIABLE( g_variables.m_auto_strafe_key, key_bind_t ) ) )
		return;

	if ( g_ctx.m_local->get_flags( ) & fl_onground )
		return;

	const bool inAir               = !( g_ctx.m_local->get_flags( ) & fl_onground );
	const static auto cl_sidespeed = g_interfaces.m_convar->find_var( "cl_sidespeed" )->get_float( );
	const float speed              = g_ctx.m_local->get_velocity( ).length_2d( );

	if ( inAir ) {
		auto calcDelta = [ = ]( ) {
			const static float maxSpeed        = g_ctx.m_local->get_max_speed( );
			const static auto sv_airaccelerate = g_interfaces.m_convar->find_var( ( "sv_airaccelerate" ) );
			const float term                   = 30.0f / sv_airaccelerate->get_float( ) / maxSpeed * 100.0f / speed;

			if ( term < 1.0f && term > -1.0f )
				return std::acos( term );

			return 0.0f;
		};

		const float deltaAir = calcDelta( );

		if ( deltaAir != 0.0f ) {
			const float yaw               = deg2rad( cmd->m_view_point.m_y );
			const auto velocityVec        = g_ctx.m_local->get_velocity( );
			const float velocityDirection = std::atan2( velocityVec.m_y, velocityVec.m_x ) - yaw;
			const float bestAngleMove     = std::atan2( -cmd->m_side_move, cmd->m_forward_move );

			auto deltaAngle = []( float first, float second ) // used to point out angle to finally calculate, detection of dir
			{
				const float delta = first - second;
				float res         = std::isfinite( delta ) ? std::remainder( delta, M_PIN * 2.0f ) : 0.0f;

				if ( first > second ) {
					if ( res >= M_PIN )
						res -= M_PIN * 2.0f;
				} else {
					if ( res <= -M_PIN )
						res += M_PIN * 2.0f;
				}

				return res;
			};

			const float delta     = deltaAngle( velocityDirection, bestAngleMove );
			const float finalMove = delta < 0.0f ? velocityDirection + deltaAir : velocityDirection - deltaAir;

			cmd->m_forward_move = std::cos( finalMove ) * cl_sidespeed;
			cmd->m_side_move    = -std::sin( finalMove ) * cl_sidespeed;
		}
	}
}
extern void start_movement_fix( c_user_cmd* cmd );
extern void end_movement_fix( c_user_cmd* cmd );
void n_movement::impl_t::ladder_freelook_climb( c_user_cmd* cmd )
{
	auto& data = m_ladder_freelook_climb_data;

	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) || !cmd ) {
		data.reset( );
		return;
	}

	const bool enabled     = GET_VARIABLE( g_variables.m_ladder_freelook_climb, bool );
	const bool key_pressed = g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_freelook_climb_key, key_bind_t ) );
	if ( !enabled ) {
		data.reset( );
		return;
	}

	const int move_type  = g_ctx.m_local->get_move_type( );
	const bool on_ladder = move_type == e_move_types::move_type_ladder;
	const bool on_ground = ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) != 0;

	auto clear_walk_input = [ & ]( ) { cmd->m_buttons &= ~( in_forward | in_back | in_moveleft | in_moveright ); };

	auto apply_walk_buttons = [ & ]( float forward_move, float side_move ) {
		clear_walk_input( );
		if ( forward_move > 0.0f )
			cmd->m_buttons |= in_forward;
		else if ( forward_move < 0.0f )
			cmd->m_buttons |= in_back;
		if ( side_move > 0.0f )
			cmd->m_buttons |= in_moveright;
		else if ( side_move < 0.0f )
			cmd->m_buttons |= in_moveleft;
	};

	auto normalize_radians = []( float angle ) {
		while ( angle > M_PIN )
			angle -= M_PIN * 2.0f;
		while ( angle < -M_PIN )
			angle += M_PIN * 2.0f;
		return angle;
	};

	auto restore_freelook_camera = []( const c_angle& visual ) {
		c_angle v = visual;
		g_interfaces.m_engine_client->set_view_angles( v );
	};

	auto apply_server_locked_climb = [ & ]( ) {
		cmd->m_buttons &= ~in_jump;
		clear_walk_input( );
		cmd->m_forward_move = 450.0f;
		cmd->m_side_move    = 0.0f;
		cmd->m_up_move      = 0.0f;
		apply_walk_buttons( cmd->m_forward_move, cmd->m_side_move );
	};

	auto apply_server_jump_into_ladder = [ & ]( ) {
		cmd->m_buttons |= in_jump;
		clear_walk_input( );
		cmd->m_forward_move = 450.0f;
		cmd->m_side_move    = 0.0f;
		cmd->m_up_move      = 0.0f;
		apply_walk_buttons( cmd->m_forward_move, cmd->m_side_move );
	};

	static constexpr int k_ladder_freelook_simple_air_ticks = 8;

	auto apply_forward_air_strafe = [ & ]( float target_yaw, bool simple_wish_only ) {
		cmd->m_up_move = 0.0f;
		cmd->m_buttons &= ~in_jump;
		clear_walk_input( );

		const c_vector velocity       = g_ctx.m_local->get_velocity( );
		const float speed             = velocity.length_2d( );
		static auto sv_airaccelerate  = g_interfaces.m_convar->find_var( "sv_airaccelerate" );
		static auto cl_sidespeed_cvar = g_interfaces.m_convar->find_var( "cl_sidespeed" );

		if ( !simple_wish_only && speed > 15.0f && sv_airaccelerate && cl_sidespeed_cvar ) {
			const float max_speed = g_ctx.m_local->get_max_speed( );
			const float term      = 30.0f / sv_airaccelerate->get_float( ) / max_speed * 100.0f / speed;
			float delta_air       = 0.0f;
			if ( term > -1.0f && term < 1.0f )
				delta_air = std::acos( term );

			if ( delta_air != 0.0f ) {
				const float yaw                = deg2rad( cmd->m_view_point.m_y );
				const float velocity_direction = std::atan2( velocity.m_y, velocity.m_x ) - yaw;
				const float wish_direction     = deg2rad( target_yaw ) - yaw;
				const float delta              = normalize_radians( velocity_direction - wish_direction );
				const float final_move         = delta < 0.0f ? velocity_direction + delta_air : velocity_direction - delta_air;
				cmd->m_forward_move            = std::cos( final_move ) * cl_sidespeed_cvar->get_float( );
				cmd->m_side_move               = -std::sin( final_move ) * cl_sidespeed_cvar->get_float( );
			} else {
				const float rotation = deg2rad( g_math.normalize_angle( target_yaw - cmd->m_view_point.m_y ) );
				cmd->m_forward_move  = std::cos( rotation ) * 450.0f;
				cmd->m_side_move     = -std::sin( rotation ) * 450.0f;
			}
		} else {
			const float rotation = deg2rad( g_math.normalize_angle( target_yaw - cmd->m_view_point.m_y ) );
			cmd->m_forward_move  = std::cos( rotation ) * 450.0f;
			cmd->m_side_move     = -std::sin( rotation ) * 450.0f;
		}
		apply_walk_buttons( cmd->m_forward_move, cmd->m_side_move );
	};

	if ( data.m_phase == ladder_freelook_climb_data_t::phase_strafing ) {
		if ( !key_pressed ) {
			data.reset( );
			return;
		}

		const float speed_2d = g_ctx.m_local->get_velocity( ).length_2d( );
		data.m_keep_strafing = speed_2d < 245.0f;

		if ( on_ground || on_ladder ) {
			data.reset( );
			return;
		}

		const c_angle visual_angles = cmd->m_view_point;
		cmd->m_view_point           = c_angle( 0.0f, data.m_server_yaw, 0.0f );
		const bool simple_air       = data.m_air_ticks < k_ladder_freelook_simple_air_ticks || data.m_keep_strafing;
		apply_forward_air_strafe( data.m_server_yaw, simple_air );
		data.m_air_ticks += 1;
		restore_freelook_camera( visual_angles );
		return;
	}

	if ( data.m_phase == ladder_freelook_climb_data_t::phase_jumping ) {
		if ( !on_ladder ) {
			data.m_phase                = ladder_freelook_climb_data_t::phase_strafing;
			data.m_air_ticks            = 0;
			data.m_keep_strafing        = true;
			const c_angle visual_angles = cmd->m_view_point;
			cmd->m_view_point           = c_angle( 0.0f, data.m_server_yaw, 0.0f );
			apply_forward_air_strafe( data.m_server_yaw, true );
			data.m_air_ticks += 1;
			restore_freelook_camera( visual_angles );
			return;
		}

		const c_angle visual_angles = cmd->m_view_point;
		cmd->m_view_point           = c_angle( 0.0f, data.m_server_yaw, 0.0f );
		apply_server_jump_into_ladder( );
		restore_freelook_camera( visual_angles );
		return;
	}

	if ( !key_pressed ) {
		data.reset( );
		return;
	}

	auto colidable = g_ctx.m_local->get_collideable( );
	if ( !colidable ) {
		data.reset( );
		return;
	}

	const int pred_frame   = g_interfaces.m_prediction->m_commands_predicted - 1;
	bool found_ladder      = false;
	c_vector ladder_normal = { };

	const bool reuse_climb_geometry =
		data.m_phase == ladder_freelook_climb_data_t::phase_climbing && on_ladder && data.m_ladder_normal.length_2d( ) > 0.01f;

	if ( reuse_climb_geometry ) {
		found_ladder  = true;
		ladder_normal = data.m_ladder_normal;
	} else {
		trace_t trace;
		const float step    = 3.14159265f * 2.0f / 32.0f;
		float best_fraction = 1.0f;

		for ( float a = 0.0f; a < 3.14159265f * 2.0f; a += step ) {
			c_vector wishdir( cosf( a ), sinf( a ), 0.0f );
			auto start_pos = g_ctx.m_local->get_abs_origin( );
			auto end_pos   = start_pos + wishdir * 2.0f;
			c_trace_filter flt( g_ctx.m_local );
			ray_t ray( start_pos, end_pos, colidable->get_obb_mins( ), colidable->get_obb_maxs( ) );
			g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );

			if ( trace.m_fraction >= best_fraction || trace.m_plane.m_normal.m_z >= 0.4f || trace.m_plane.m_normal.m_z <= -0.4f )
				continue;

			const c_vector wall_normal = trace.m_plane.m_normal;

			if ( on_ladder ) {
				if ( trace.m_fraction < best_fraction ) {
					best_fraction = trace.m_fraction;
					ladder_normal = wall_normal;
					found_ladder  = true;
				}
			} else {
				const c_vector into_dir( -wall_normal.m_x, -wall_normal.m_y, 0.0f );
				const c_angle into_angle = into_dir.to_angle( );
				const float rotation     = deg2rad( into_angle.m_y - cmd->m_view_point.m_y );

				const float backup_forward = cmd->m_forward_move;
				const float backup_side    = cmd->m_side_move;
				const float backup_up      = cmd->m_up_move;
				const int backup_buttons   = cmd->m_buttons;

				g_prediction.restore_entity_to_predicted_frame( pred_frame );
				cmd->m_forward_move = std::cos( rotation ) * 450.0f;
				cmd->m_side_move    = -std::sin( rotation ) * 450.0f;
				cmd->m_up_move      = 0.0f;
				cmd->m_buttons &= ~in_jump;

				g_prediction.begin( g_ctx.m_local, cmd );
				g_prediction.end( g_ctx.m_local );
				const bool became_ladder = g_ctx.m_local->get_move_type( ) == e_move_types::move_type_ladder;

				g_prediction.restore_entity_to_predicted_frame( pred_frame );
				cmd->m_forward_move = backup_forward;
				cmd->m_side_move    = backup_side;
				cmd->m_up_move      = backup_up;
				cmd->m_buttons      = backup_buttons;

				if ( became_ladder ) {
					found_ladder  = true;
					ladder_normal = wall_normal;
					break;
				}
			}
		}
	}

	if ( !found_ladder ) {
		if ( data.m_phase == ladder_freelook_climb_data_t::phase_idle )
			return;

		data.reset( );
		return;
	}

	c_vector ladder_n = ladder_normal;
	const c_vector into_ladder( -ladder_n.m_x, -ladder_n.m_y, 0.0f );
	if ( !reuse_climb_geometry ) {
		data.m_ladder_normal = ladder_n;
		data.m_server_yaw    = into_ladder.to_angle( ).m_y;
	}
	data.m_exit_yaw = data.m_server_yaw;
	data.m_phase    = ladder_freelook_climb_data_t::phase_climbing;

	const c_angle visual_angles = cmd->m_view_point;
	cmd->m_view_point           = c_angle( 0.0f, data.m_server_yaw, 0.0f );
	apply_server_locked_climb( );

	if ( !on_ladder ) {
		restore_freelook_camera( visual_angles );
		return;
	}

	const float backup_forward = cmd->m_forward_move;
	const float backup_side    = cmd->m_side_move;
	const float backup_up      = cmd->m_up_move;
	const int backup_buttons   = cmd->m_buttons;
	const c_angle backup_view  = cmd->m_view_point;

	bool will_leave_ladder = false;
	g_prediction.restore_entity_to_predicted_frame( pred_frame );

	cmd->m_view_point   = c_angle( 0.0f, data.m_server_yaw, 0.0f );
	cmd->m_forward_move = 450.0f;
	cmd->m_side_move    = 0.0f;
	cmd->m_up_move      = 0.0f;
	cmd->m_buttons &= ~in_jump;

	for ( int i = 0; i < 2; ++i ) {
		g_prediction.begin( g_ctx.m_local, cmd );
		if ( g_ctx.m_local->get_move_type( ) != e_move_types::move_type_ladder ) {
			will_leave_ladder = true;
			g_prediction.end( g_ctx.m_local );
			break;
		}
		g_prediction.end( g_ctx.m_local );
	}
	g_prediction.restore_entity_to_predicted_frame( pred_frame );

	cmd->m_forward_move = backup_forward;
	cmd->m_side_move    = backup_side;
	cmd->m_up_move      = backup_up;
	cmd->m_buttons      = backup_buttons;
	cmd->m_view_point   = backup_view;

	if ( will_leave_ladder ) {
		movement_add_window( 5, std::string( "ladder dbg sy=" )
		                            .append( std::to_string( static_cast< int >( std::round( data.m_server_yaw ) ) ) )
		                            .append( " fw=" )
		                            .append( std::to_string( static_cast< int >( std::round( cmd->m_forward_move ) ) ) )
		                            .append( " sd=" )
		                            .append( std::to_string( static_cast< int >( std::round( cmd->m_side_move ) ) ) ) );
		data.m_phase = ladder_freelook_climb_data_t::phase_jumping;
	}

	restore_freelook_camera( visual_angles );
}
void n_movement::impl_t::auto_one_hop( c_user_cmd* cmd )
{
	const bool enabled     = GET_VARIABLE( g_variables.m_auto_one_hop, bool );
	const bool key_pressed = g_input.check_input( &GET_VARIABLE( g_variables.m_auto_one_hop_key, key_bind_t ) );

	if ( !enabled || !g_ctx.m_local || !g_ctx.m_local->is_alive( ) || !cmd ) {
		m_auto_one_hop_data.reset( );
		return;
	}

	if ( !key_pressed && m_auto_one_hop_data.m_phase == auto_one_hop_data_t::phase_idle ) {
		m_auto_one_hop_data.reset( );
		return;
	}

	auto colidable = g_ctx.m_local->get_collideable( );
	if ( !colidable )
		return;

	const int move_type  = g_ctx.m_local->get_move_type( );
	const bool on_ladder = move_type == e_move_types::move_type_ladder;
	const bool on_ground = g_ctx.m_local->get_flags( ) & e_flags::fl_onground;
	auto& data           = m_auto_one_hop_data;

	if ( data.m_phase == auto_one_hop_data_t::phase_strafing ) {
		if ( on_ground || on_ladder ) {
			data.reset( );
			return;
		}

		if ( data.m_ladder_normal.length( ) < 0.1f )
			return;

		const c_vector into_ladder( -data.m_ladder_normal.m_x, -data.m_ladder_normal.m_y, 0.0f );
		float target_yaw = rad2deg( std::atan2( into_ladder.m_y, into_ladder.m_x ) );
		if ( data.m_air_ticks == 0 ) {
			float rotation      = deg2rad( target_yaw - cmd->m_view_point.m_y );
			cmd->m_forward_move = std::cos( rotation ) * 380.f;
			cmd->m_side_move    = -std::sin( rotation ) * 380.f + 225.f;
			data.m_air_ticks += 1;
			return;
		}
		if ( data.m_air_ticks == 1 ) {
			float rotation      = deg2rad( target_yaw - cmd->m_view_point.m_y );
			cmd->m_forward_move = std::cos( rotation ) * 420.f;
			cmd->m_side_move    = -std::sin( rotation ) * 420.f + 110.f;
			data.m_air_ticks += 1;
			return;
		}

		const float speed = g_ctx.m_local->get_velocity( ).length_2d( );

		if ( speed > 15.0f ) {
			static auto sv_airaccelerate  = g_interfaces.m_convar->find_var( "sv_airaccelerate" );
			static auto cl_sidespeed_cvar = g_interfaces.m_convar->find_var( "cl_sidespeed" );
			if ( sv_airaccelerate && cl_sidespeed_cvar ) {
				const float cl_sidespeed = cl_sidespeed_cvar->get_float( );
				const float max_speed    = g_ctx.m_local->get_max_speed( );
				const float term         = 30.0f / sv_airaccelerate->get_float( ) / max_speed * 100.0f / speed;
				float delta_air          = 0.0f;
				if ( term > -1.0f && term < 1.0f )
					delta_air = std::acos( term );

				if ( delta_air != 0.0f ) {
					const float yaw      = deg2rad( cmd->m_view_point.m_y );
					const auto vel       = g_ctx.m_local->get_velocity( );
					const float vel_dir  = std::atan2( vel.m_y, vel.m_x ) - yaw;
					const float wish_dir = deg2rad( target_yaw ) - yaw;

					float angle_delta = vel_dir - wish_dir;
					if ( angle_delta > M_PIN )
						angle_delta -= M_PIN * 2.0f;
					if ( angle_delta < -M_PIN )
						angle_delta += M_PIN * 2.0f;

					const float final_move = angle_delta < 0.0f ? vel_dir + delta_air : vel_dir - delta_air;
					cmd->m_forward_move    = std::cos( final_move ) * cl_sidespeed;
					cmd->m_side_move       = -std::sin( final_move ) * cl_sidespeed;
				}
			}
		} else {
			float rotation      = deg2rad( target_yaw - cmd->m_view_point.m_y );
			cmd->m_forward_move = std::cos( rotation ) * 450.f;
			cmd->m_side_move    = -std::sin( rotation ) * 450.f;
		}
		data.m_air_ticks += 1;
		return;
	}

	if ( data.m_phase == auto_one_hop_data_t::phase_jumped ) {
		if ( !on_ladder ) {
			data.m_phase     = auto_one_hop_data_t::phase_strafing;
			data.m_air_ticks = 0;
			return;
		}
		cmd->m_buttons |= e_command_buttons::in_jump;
		cmd->m_forward_move = 0.f;
		cmd->m_side_move    = 0.f;
		return;
	}

	if ( !key_pressed ) {
		if ( data.m_phase == auto_one_hop_data_t::phase_climbing )
			data.reset( );
		return;
	}

	const int pred_frame = g_interfaces.m_prediction->m_commands_predicted - 1;

	// --- fireman-style: find wall, then predict if it's a ladder ---
	bool found_ladder = false;
	c_vector ladder_normal;

	if ( on_ladder ) {
		found_ladder = true;
		if ( data.m_ladder_normal.length( ) < 0.1f ) {
			trace_t trace;
			constexpr float step = 3.14159265f * 2.0f / 16.0f;
			for ( float a = 0; a < 3.14159265f * 2.0f; a += step ) {
				c_vector wishdir( cosf( a ), sinf( a ), 0.f );
				auto start_pos = g_ctx.m_local->get_abs_origin( );
				auto end_pos   = start_pos + wishdir;
				c_trace_filter flt( g_ctx.m_local );
				ray_t ray( start_pos, end_pos, colidable->get_obb_mins( ), colidable->get_obb_maxs( ) );
				g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
				if ( trace.m_fraction < 1.0f && trace.m_plane.m_normal.m_z < 0.4f ) {
					ladder_normal = trace.m_plane.m_normal;
					break;
				}
			}
		}
	} else {
		trace_t trace;
		constexpr float step = 3.14159265f * 2.0f / 16.0f;
		for ( float a = 0; a < 3.14159265f * 2.0f; a += step ) {
			c_vector wishdir( cosf( a ), sinf( a ), 0.f );
			auto start_pos = g_ctx.m_local->get_abs_origin( );
			auto end_pos   = start_pos + wishdir;
			c_trace_filter flt( g_ctx.m_local );
			ray_t ray( start_pos, end_pos, colidable->get_obb_mins( ), colidable->get_obb_maxs( ) );
			g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
			if ( trace.m_fraction < 1.0f && trace.m_plane.m_normal.m_z < 0.4f ) {
				c_vector wall_n = trace.m_plane.m_normal;
				c_vector into_wall( -wall_n.m_x, -wall_n.m_y, 0.f );
				c_angle wall_ang = into_wall.to_angle( );
				float rot        = deg2rad( wall_ang.m_y - cmd->m_view_point.m_y );

				g_prediction.restore_entity_to_predicted_frame( pred_frame );
				float save_fwd      = cmd->m_forward_move;
				float save_side     = cmd->m_side_move;
				int save_buttons    = cmd->m_buttons;
				cmd->m_forward_move = std::cos( rot ) * 450.f;
				cmd->m_side_move    = -std::sin( rot ) * 450.f;
				cmd->m_buttons &= ~e_command_buttons::in_jump;

				g_prediction.begin( g_ctx.m_local, cmd );
				g_prediction.end( g_ctx.m_local );
				bool became_ladder = g_ctx.m_local->get_move_type( ) == e_move_types::move_type_ladder;

				cmd->m_forward_move = save_fwd;
				cmd->m_side_move    = save_side;
				cmd->m_buttons      = save_buttons;
				g_prediction.restore_entity_to_predicted_frame( pred_frame );

				if ( became_ladder ) {
					found_ladder  = true;
					ladder_normal = wall_n;
					break;
				}
			}
		}
	}

	if ( !found_ladder ) {
		if ( data.m_phase == auto_one_hop_data_t::phase_climbing )
			data.reset( );
		return;
	}

	if ( ladder_normal.length( ) > 0.1f )
		data.m_ladder_normal = ladder_normal;

	if ( data.m_phase == auto_one_hop_data_t::phase_idle )
		data.m_phase = auto_one_hop_data_t::phase_climbing;

	cmd->m_buttons |= in_forward;
	cmd->m_buttons &= ~in_back;
	cmd->m_forward_move = 450.f;
	cmd->m_side_move    = 0.f;
	cmd->m_up_move      = 0.f;

	if ( !on_ladder )
		return;

	// --- predict 1 tick: will we leave the ladder? ---
	g_prediction.restore_entity_to_predicted_frame( pred_frame );

	float backup_forward = cmd->m_forward_move;
	float backup_side    = cmd->m_side_move;
	float backup_up      = cmd->m_up_move;
	int backup_buttons   = cmd->m_buttons;

	bool will_leave = false;
	for ( int i = 0; i < 1; i++ ) {
		g_prediction.begin( g_ctx.m_local, cmd );
		if ( g_ctx.m_local->get_move_type( ) != e_move_types::move_type_ladder ) {
			will_leave = true;
			g_prediction.end( g_ctx.m_local );
			break;
		}
		g_prediction.end( g_ctx.m_local );
	}
	g_prediction.restore_entity_to_predicted_frame( pred_frame );

	cmd->m_forward_move = backup_forward;
	cmd->m_side_move    = backup_side;
	cmd->m_up_move      = backup_up;
	cmd->m_buttons      = backup_buttons;

	if ( will_leave ) {
		// source engine jump off ladder: velocity = 270 * plane.normal (automatic)
		cmd->m_buttons |= e_command_buttons::in_jump;
		cmd->m_forward_move = 0.f;
		cmd->m_side_move    = 0.f;
		data.m_phase        = auto_one_hop_data_t::phase_jumped;
	}
}

void n_movement::impl_t::auto_crouch( c_user_cmd* cmd )
{
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) ) {
		m_auto_crouch_data.m_ducking_velo  = 0.f;
		m_auto_crouch_data.m_standing_velo = 0.f;
		m_auto_crouch_data.m_founded       = false;
		return;
	}
	if ( const auto mt = g_ctx.m_local->get_move_type( );
	     mt == move_type_ladder || mt == move_type_noclip || mt == move_type_fly || mt == move_type_observer ) {
		m_auto_crouch_data.m_ducking_velo  = 0.f;
		m_auto_crouch_data.m_standing_velo = 0.f;
		m_auto_crouch_data.m_founded       = false;
		return;
	}
	const bool enabled     = GET_VARIABLE( g_variables.m_auto_crouch, bool );
	const bool key_pressed = g_input.check_input( &GET_VARIABLE( g_variables.m_auto_crouch_key, key_bind_t ) );
	const bool jump_bug_active =
		GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) );

	if ( !enabled || !key_pressed || jump_bug_active ) {
		m_auto_crouch_data.m_founded = false;
		return;
	}

	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) || !cmd )
		return;

	// Only process if in air and not on ladder
	if ( ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) || g_ctx.m_local->get_move_type( ) == e_move_types::move_type_ladder ) {
		m_auto_crouch_data.m_founded = false;
		return;
	}

	const int ticks = GET_VARIABLE( g_variables.m_auto_crouch_ticks, int );

	bool can_land_standing = false;
	bool can_land_ducking  = false;
	c_vector standing_land_origin( 0, 0, 0 );
	c_vector ducking_land_origin( 0, 0, 0 );
	c_vector current_origin = g_ctx.m_local->get_abs_origin( );

	// 1. Predict Standing
	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
	for ( int i = 0; i < ticks; i++ ) {
		g_prediction.begin( g_ctx.m_local, cmd );
		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
			can_land_standing    = true;
			standing_land_origin = g_ctx.m_local->get_abs_origin( );
			g_prediction.end( g_ctx.m_local );
			break;
		}
		g_prediction.end( g_ctx.m_local );
	}

	// 2. Predict Ducking
	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
	c_user_cmd duck_cmd = *cmd;
	duck_cmd.m_buttons |= e_command_buttons::in_duck | e_command_buttons::in_bullrush;

	for ( int i = 0; i < ticks; i++ ) {
		g_prediction.begin( g_ctx.m_local, &duck_cmd );
		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
			can_land_ducking    = true;
			ducking_land_origin = g_ctx.m_local->get_abs_origin( );
			g_prediction.end( g_ctx.m_local );
			break;
		}
		g_prediction.end( g_ctx.m_local );
	}

	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );

	// 3. Logic: Only crouch if strictly necessary and it's a "climb" onto a ledge
	if ( !can_land_standing && can_land_ducking ) {
		// Only trigger if landing surface is higher than our current origin (ledge climb)
		// This prevents unnecessary crouching when dropping out of high ceiling areas (like booths)
		if ( ducking_land_origin.m_z > current_origin.m_z - 1.0f ) {
			cmd->m_buttons |= e_command_buttons::in_duck | e_command_buttons::in_bullrush;
			m_auto_crouch_data.m_founded = true;
		} else {
			m_auto_crouch_data.m_founded = false;
		}
	} else {
		m_auto_crouch_data.m_founded = false;
	}
}
void n_movement::impl_t::ladder_glide( c_user_cmd* cmd )
{
	if ( GET_VARIABLE( g_variables.m_ladder_glide_jetpack, bool ) ) {
		static bool s_jetpack_active = false;
		static int s_interval_counter = 0;

		const bool on_ladder    = g_ctx.m_local && g_ctx.m_local->get_move_type( ) == move_type_ladder;
		const bool bind_active  = on_ladder && GET_VARIABLE( g_variables.m_ladder_bug, bool ) &&
		                          g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) );

		const int interval = GET_VARIABLE( g_variables.m_ladder_glide_jetpack_interval, int );

		if ( bind_active ) {
			++s_interval_counter;
			if ( s_interval_counter <= interval ) {
				if ( !s_jetpack_active ) {
					g_interfaces.m_engine_client->client_cmd_unrestricted( "+jetpack" );
					s_jetpack_active = true;
				}
			} else {
				if ( s_jetpack_active ) {
					g_interfaces.m_engine_client->client_cmd_unrestricted( "-jetpack" );
					s_jetpack_active = false;
				}
				s_interval_counter = 0;
			}
		} else {
			if ( s_jetpack_active ) {
				g_interfaces.m_engine_client->client_cmd_unrestricted( "-jetpack" );
				s_jetpack_active = false;
			}
			s_interval_counter = 0;
		}
	}

	if ( GET_VARIABLE( g_variables.m_podlet, bool ) ) {
		static bool s_podlet_active = false;
		const bool on_ladder         = g_ctx.m_local && g_ctx.m_local->get_move_type( ) == move_type_ladder;
		const bool bind_active       = on_ladder && GET_VARIABLE( g_variables.m_ladder_bug, bool ) &&
		                         g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) );

		if ( bind_active && !s_podlet_active ) {
			if ( auto cvar = g_interfaces.m_convar->find_var( "sv_jump_impulse" ) ) {
				cvar->set_value( 450.f );
				
			}
			s_podlet_active = true;
		} else if ( !bind_active && s_podlet_active ) {
			if ( auto cvar = g_interfaces.m_convar->find_var( "sv_jump_impulse" ) ) {
				cvar->set_value( 301.993377f );
			
			}
			s_podlet_active = false;
		}
	}

	if ( !GET_VARIABLE( g_variables.m_ladder_glide, bool ) || !g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_glide_key, key_bind_t ) ) )
		return;
	if ( g_ctx.m_local->get_move_type( ) == move_type_ladder ) {
		if ( cmd->m_side_move == 0 ) {
			cmd->m_view_point.m_y += 45.0f;
		}
		if ( cmd->m_buttons & in_forward ) {
			if ( cmd->m_side_move > 0 )
				cmd->m_view_point.m_y -= 1.0f;
			if ( cmd->m_side_move < 0 )
				cmd->m_view_point.m_y += 90.0f;
			cmd->m_buttons &= ~in_moveleft;
			cmd->m_buttons |= in_moveright;
		}
		if ( cmd->m_buttons & in_back ) {
			if ( cmd->m_side_move < 0 )
				cmd->m_view_point.m_y -= 1.0f;
			if ( cmd->m_side_move > 0 )
				cmd->m_view_point.m_y += 90.0f;
			cmd->m_buttons &= ~in_moveright;
			cmd->m_buttons |= in_moveleft;
		}
	}
}
