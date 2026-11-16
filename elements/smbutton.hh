#pragma once
#include "../instalisation fonts/fonts2.h"
#include "../globals/includes/includes.h"
#include "animations.h"
#include <unordered_map>
#include <cfloat>

inline bool smbutton_hh( const char* name )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return false;

	auto* font = fonts_for_gui::tasorbiter_addwindow;
	if ( !font )
		return ImGui::Button( name );

	const auto name_text_size = font->CalcTextSizeA( font->FontSize, FLT_MAX, NULL, name );
	const float width = name_text_size.x + 24.0f;
	const float height = 20.0f;
	const float rounding = 4.0f;
	const ImVec2 position = ImGui::GetCurrentWindow( )->DC.CursorPos;
	ImRect total_bb( position, position + ImVec2( width, height ) );

	ImGui::ItemSize( total_bb, height );
	if ( !ImGui::ItemAdd( total_bb, ImGui::GetID( name ) ) )
		return false;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( total_bb, ImGui::GetID( name ), &hovered, &held );

	float dt = ImGui::GetIO( ).DeltaTime;
	static std::unordered_map< ImGuiID, float > press_anim;
	static std::unordered_map< ImGuiID, float > hover_anim;
	ImGuiID id = ImGui::GetID( name );

	if ( press_anim.find( id ) == press_anim.end( ) )
		press_anim[ id ] = 0.0f;
	if ( hover_anim.find( id ) == hover_anim.end( ) )
		hover_anim[ id ] = 0.0f;

	press_anim[ id ] = smoothLerp( press_anim[ id ], held ? 1.0f : 0.0f, dt * 12.0f );
	hover_anim[ id ] = smoothLerp( hover_anim[ id ], hovered ? 1.0f : 0.0f, dt * 8.0f );

	ImColor base_color( 16, 16, 16, int( 255 * g_menu.m_animation_progress ) );
	ImColor hover_color( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) );
	ImColor accent_color( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) );
	ImColor final_color = smoothLerpColor(
		smoothLerpColor( base_color, hover_color, hover_anim[ id ] ),
		accent_color,
		press_anim[ id ]
	);

	ImGui::GetWindowDrawList( )->AddRectFilled( total_bb.Min, total_bb.Max, final_color, rounding, ImDrawFlags_RoundCornersAll );
	ImGui::GetWindowDrawList( )->AddRect( total_bb.Min, total_bb.Max, ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ), rounding, ImDrawFlags_RoundCornersAll, 0.65f );

	ImColor base_text_color( 255, 255, 255, int( 220 * g_menu.m_animation_progress ) );
	ImColor hover_text_color( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) );
	ImColor final_text_color = smoothLerpColor( base_text_color, hover_text_color, hover_anim[ id ] );

	const float text_x = total_bb.Min.x + ( width - name_text_size.x ) * 0.5f;
	const float text_y = total_bb.Min.y + ( height - name_text_size.y ) * 0.5f - 1.0f;
	ImGui::GetWindowDrawList( )->AddText( font, font->FontSize, ImVec2( text_x, text_y ), final_text_color, name );

	return pressed;
}
