#pragma once
#include <functional>
#include "../instalisation fonts/fonts2.h"
#include "../hacks/menu/menu.h"
#include "../globals/config/variables.h"
#include "animations.h"

static float childbox_dpi_scale( )
{
	const int dpi = GET_VARIABLE( g_variables.m_menu_dpi_scale, int );
	return static_cast< float >( dpi < 75 ? 75 : dpi > 200 ? 200 : dpi ) / 100.0f;
}

// Scrollable child - same as child but with vertical scrollbar
void scrollable_child(const char* name, const ImVec2& size, const std::function<void()>& func) {
	const float scale = childbox_dpi_scale( );
	const float title_bar_height{ 25.0f * scale };
	const float spacing[2]{ 10.0f * scale, 5.0f * scale };
	const float rounding{ 4.0f * scale };
	const ImVec2 scaled_size( size.x * scale, size.y * scale );
	
	static std::unordered_map< const char*, float > child_anim;
	if ( child_anim.find( name ) == child_anim.end( ) )
		child_anim[ name ] = 0.0f;
	
	float dt = ImGui::GetIO( ).DeltaTime;
	child_anim[ name ] = smoothLerp( child_anim[ name ], 1.0f, dt * 6.0f );

	ImGui::SetCursorPos( ImGui::GetCursorPos( ) * scale );
	ImGui::BeginChild(name, scaled_size, 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);

	const auto draw_position = ImVec2(ImGui::GetCurrentWindow()->Pos.x, ImGui::GetCurrentWindow()->Pos.y);
	float anim_progress = child_anim[ name ];
	ImColor base_title_color = ImColor( 16, 16, 16, 0 );
	ImColor full_title_color = ImColor( 16, 16, 16, int( 255 * g_menu.m_animation_progress ) );
	ImColor final_title_color = smoothLerpColor( base_title_color, full_title_color, anim_progress );
	
	ImGui::GetWindowDrawList()->AddRectFilled(draw_position, draw_position + ImVec2(scaled_size.x, title_bar_height),
	                                            final_title_color, rounding, ImDrawFlags_RoundCornersTop );

	ImColor base_bg_color = ImColor( 16, 16, 16, 0 );
	ImColor full_bg_color = ImColor( 16, 16, 16, int( 255 * g_menu.m_animation_progress ) );
	ImColor final_bg_color = smoothLerpColor( base_bg_color, full_bg_color, anim_progress );
	
	ImGui::GetWindowDrawList()->AddRectFilled(draw_position + ImVec2(0.0f, title_bar_height), draw_position + scaled_size,
	                                            final_bg_color, rounding,
	                                            ImDrawFlags_RoundCornersBottom );

	ImColor base_glow_color = ImColor( 11, 11, 11, 0 );
	ImColor full_glow_color = ImColor( 11, 11, 11, int( 255 * g_menu.m_animation_progress ) );
	ImColor final_glow_color = smoothLerpColor( base_glow_color, full_glow_color, anim_progress );
	
	ImGui::GetWindowDrawList()->AddRect(draw_position, draw_position + scaled_size, 
	                                      final_glow_color, 
	                                      rounding, ImDrawFlags_RoundCornersAll, 1.0f * scale );

	ImColor base_text_color = ImColor( 150, 150, 150, 0 );
	ImColor full_text_color = ImColor( 150, 150, 150, int( 255 * g_menu.m_animation_progress ) );
	ImColor final_text_color = smoothLerpColor( base_text_color, full_text_color, anim_progress );
	
	ImGui::GetForegroundDrawList()->AddText(fonts_for_gui::tas_orbiter_small, fonts_for_gui::tas_orbiter_small->FontSize * scale, 
		draw_position + ImVec2(10.0f * scale, (title_bar_height / 2) - 5.0f * scale),
		final_text_color, name );

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + title_bar_height);
	ImGui::SetCursorPos({ ImGui::GetCursorPosX() + spacing[0] , ImGui::GetCursorPosY() + spacing[1] - 2.0f * scale });
	ImGui::BeginGroup();
	func();
	ImGui::EndGroup();

	ImGui::EndChild();
}

void child(const char* name, const ImVec2& size, const std::function<void()>& func) {
	const float scale = childbox_dpi_scale( );
	const float title_bar_height{ 25.0f * scale };
	const float spacing[2]{ 10.0f * scale, 5.0f * scale };
	const float rounding{ 4.0f * scale };
	const ImVec2 scaled_size( size.x * scale, size.y * scale );
	
	static std::unordered_map< const char*, float > child_anim;
	if ( child_anim.find( name ) == child_anim.end( ) )
		child_anim[ name ] = 0.0f;
	
	float dt = ImGui::GetIO( ).DeltaTime;
	child_anim[ name ] = smoothLerp( child_anim[ name ], 1.0f, dt * 6.0f );

	ImGui::SetCursorPos( ImGui::GetCursorPos( ) * scale );
	ImGui::BeginChild(name, scaled_size, 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollWithMouse);

	const auto draw_position = ImVec2(ImGui::GetCurrentWindow()->Pos.x, ImGui::GetCurrentWindow()->Pos.y);
	float anim_progress = child_anim[ name ];
	ImColor base_title_color = ImColor( 16, 16, 16, 0 );
	ImColor full_title_color = ImColor( 16, 16, 16, int( 255 * g_menu.m_animation_progress ) );
	ImColor final_title_color = smoothLerpColor( base_title_color, full_title_color, anim_progress );
	
	ImGui::GetWindowDrawList()->AddRectFilled(draw_position, draw_position + ImVec2(scaled_size.x, title_bar_height),
	                                            final_title_color, rounding, ImDrawFlags_RoundCornersTop );

	ImColor base_bg_color = ImColor( 16, 16, 16, 0 );
	ImColor full_bg_color = ImColor( 16, 16, 16, int( 255 * g_menu.m_animation_progress ) );
	ImColor final_bg_color = smoothLerpColor( base_bg_color, full_bg_color, anim_progress );
	
	ImGui::GetWindowDrawList()->AddRectFilled(draw_position + ImVec2(0.0f, title_bar_height), draw_position + scaled_size,
	                                            final_bg_color, rounding,
	                                            ImDrawFlags_RoundCornersBottom );

	ImColor base_glow_color = ImColor( 11, 11, 11, 0 );
	ImColor full_glow_color = ImColor( 11, 11, 11, int( 255 * g_menu.m_animation_progress ) );
	ImColor final_glow_color = smoothLerpColor( base_glow_color, full_glow_color, anim_progress );
	
	ImGui::GetWindowDrawList()->AddRect(draw_position, draw_position + scaled_size, 
	                                      final_glow_color, 
	                                      rounding, ImDrawFlags_RoundCornersAll, 1.0f * scale );

	ImColor base_text_color = ImColor( 150, 150, 150, 0 );
	ImColor full_text_color = ImColor( 150, 150, 150, int( 255 * g_menu.m_animation_progress ) );
	ImColor final_text_color = smoothLerpColor( base_text_color, full_text_color, anim_progress );
	
	ImGui::GetForegroundDrawList()->AddText(fonts_for_gui::tas_orbiter_small, fonts_for_gui::tas_orbiter_small->FontSize * scale, 
		draw_position + ImVec2(10.0f * scale, (title_bar_height / 2) - 5.0f * scale),
		final_text_color, name );

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + title_bar_height);
	ImGui::SetCursorPos({ ImGui::GetCursorPosX() + spacing[0] , ImGui::GetCursorPosY() + spacing[1] - 2.0f * scale });
	ImGui::BeginGroup();
	func();
	ImGui::EndGroup();

	ImGui::EndChild();
}
