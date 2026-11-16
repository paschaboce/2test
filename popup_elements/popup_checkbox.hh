#pragma once

#include "../elements/combobox.hh"
#include "../instalisation fonts/fonts2.h"
#include "../globals/config/variables.h"
#include <algorithm>
#include <cstring>


inline void popup_checkbox(
	const char* name, bool* variable, bool disabled = false, bool popup = false, const ImVec2& popup_size = ImVec2( ),
	const std::function< void( ) >& popup_function = [] { } )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return;

	const bool k_lester_labels = GET_VARIABLE( g_variables.m_lester_mode, bool );
	const char* label_draw =
	    ( k_lester_labels && std::strcmp( name, "LESTER MODE" ) != 0 ) ? "LESTER" : name;
	ImFont* label_font = fonts_for_gui::tasorbiter_addwindow;
	const ImVec2 name_text_size =
	    label_font ? label_font->CalcTextSizeA( label_font->FontSize, FLT_MAX, 0.f, label_draw ) : ImGui::CalcTextSize( label_draw, NULL, true );
	const static float width{ 180.0f };
	const static float size{ 15.0f };
	const static float spacing{ 5.0f };
	const static float rounding{ 4.0f };
	const static float fix_shit{ 5.0f };

	// Координаты адаптированы для popup - начинаем с левого края
	ImVec2 position = ImGui::GetCurrentWindow( )->DC.CursorPos;
	position.x += 261.0f; // Сдвигаем чекбокс на 261px вправо от левого края popup (270 - 9 = 261)

	ImRect total_bb( position, ImVec2( position.x + width, position.y + size ) );
	ImRect check_bb( total_bb.Min, ImVec2( total_bb.Min.x + size, total_bb.Min.y + size ) );

	ImGui::ItemSize( total_bb, size + 15.f );

	if ( !ImGui::ItemAdd( total_bb , ImGui::GetID( name ) ) )
		return;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( total_bb, ImGui::GetID( name ), &hovered, &held );

	if ( pressed && !disabled )
		*variable = !( *variable );

	// Animation logic
	ImGuiID id               = ImGui::GetID( name );
	ImGuiStorage* storage    = ImGui::GetStateStorage( );
	float animation_progress = storage->GetFloat( id, *variable ? 1.0f : 0.0f );
	float target             = *variable ? 1.0f : 0.0f;
	float step               = 5.0f * ImGui::GetIO( ).DeltaTime;
	if ( animation_progress < target )
		animation_progress = std::min( animation_progress + step, target );
	else if ( animation_progress > target )
		animation_progress = std::max( animation_progress - step, target );
	storage->SetFloat( id, animation_progress );

	// Draw checkbox with animation - квадратный как в основном меню
	ImGui::PushClipRect( check_bb.Min, check_bb.Max, false );
	ImColor unchecked_color = ImColor( 13, 13, 13, int( 255 * 1.0f ) );
	ImColor checked_color   = ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * 1.0f ) ) );
	ImColor background_color;
	background_color.Value = ImLerp( unchecked_color.Value, checked_color.Value, animation_progress );
	ImGui::GetWindowDrawList( )->AddRectFilled( check_bb.Min, check_bb.Max, background_color, rounding, ImDrawFlags_::ImDrawFlags_RoundCornersAll );
	
	// Добавляем обводку для неактивного чекбокса
	if ( animation_progress < 1.0f ) {
		ImGui::GetWindowDrawList( )->AddRect( check_bb.Min, check_bb.Max, ImColor( 40, 40, 40, int( 255 * 1.0f ) ), rounding, ImDrawFlags_::ImDrawFlags_RoundCornersAll, 1.0f );
	}
	
	// Убрана галочка по запросу пользователя

	ImGui::PopClipRect( );

	// Draw keyboard icon - только если есть возможность настройки (popup = true)
	bool keyboard_icon_hovered = false;
	if ( popup ) {
		const float keyboard_icon_size = 12.0f;
		const float keyboard_icon_spacing = 12.0f;
		ImVec2 keyboard_icon_pos = ImVec2( check_bb.Min.x - keyboard_icon_spacing - keyboard_icon_size, 
		                                  check_bb.Min.y + ( size - keyboard_icon_size ) / 2 );
		
		// Создаем область для иконки клавиатуры для проверки наведения
		ImRect keyboard_icon_bb( keyboard_icon_pos, ImVec2( keyboard_icon_pos.x + keyboard_icon_size, keyboard_icon_pos.y + keyboard_icon_size ) );
		keyboard_icon_hovered = keyboard_icon_bb.Contains( ImGui::GetIO( ).MousePos );

		// Цвет иконки клавиатуры: 150,150,150 по умолчанию, 255,255,255 при наведении
		ImColor keyboard_icon_color = keyboard_icon_hovered ? ImColor( 255, 255, 255, int( 255 * 1.0f ) )  // Белый при наведении
		                                                    : ImColor( 150, 150, 150, int( 255 * 1.0f ) ); // Серый по умолчанию
		
		// Рисуем иконку клавиатуры (символ 'k' в шрифте fontello)
		ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::keyboard_icon_font, keyboard_icon_size,
		                                      keyboard_icon_pos, keyboard_icon_color, "k" );
	}

	// Draw label - серый для неактивных функций, белый для активных
	ImColor text_color = *variable ? ImColor( 255, 255, 255, int( 255 * 1.0f ) )  // Белый для активных
	                               : ImColor( 150, 150, 150, int( 255 * 1.0f ) ); // Серый для неактивных (как неактивный таб)
	
	// Координаты текста адаптированы для popup
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::tasorbiter_addwindow, fonts_for_gui::tasorbiter_addwindow->FontSize,
	                                      ImVec2( check_bb.Max.x + spacing - 275.0f, check_bb.Min.y + ( size - name_text_size.y - fix_shit ) / 2 ),
	                                      text_color, label_draw );

	// Popup handling - показываем popup только при наведении на иконку клавиатуры
	if ( *variable && keyboard_icon_hovered && !disabled && popup ) {
		if ( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
			ImGui::OpenPopup( name );
	}

	ImGui::SetNextWindowSize( popup_size );
	if ( ImGui::BeginPopup( name, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
	                                  ImGuiWindowFlags_NoScrollWithMouse ) ) {
		ImVec2 pos = ImGui::GetWindowPos( ), size = ImGui::GetWindowSize( );
		
		// Основной фон popup
		ImGui::GetWindowDrawList( )->AddRectFilled( pos, pos + size, ImColor( 13, 13, 13, int( 255 * 1.0f ) ), 12.0f );
		
		// Обводка вокруг popup
		ImGui::GetWindowDrawList( )->AddRect( pos, pos + size, ImColor( 33, 33, 33, int( 255 * 1.0f ) ), 12.0f, 0, 1.0f );

		ImGui::BeginGroup( );
		popup_function( );
		ImGui::EndGroup( );

		ImGui::EndPopup( );
	}
}
