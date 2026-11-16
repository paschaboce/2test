#pragma once

#include "combobox.hh"
#include "../instalisation fonts/fonts2.h"
#include "../popup_elements/popup_elements.h"
#include "animations.h"
#include "custom_color_picker.hh"
#include <algorithm>

// Функция для создания элемента с текстом и цветовым пикером (без чекбокса)
inline void text_with_color_picker(
	const char* name, c_color* color, bool disabled )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return;

	const static auto name_text_size = ImGui::CalcTextSize( name, NULL, true );
	const static float width{ 120.0f };
	const static float size{ 15.0f };
	const static float spacing{ 5.0f };
	const static float rounding{ 4.0f };
	const static float fix_shit{ 5.0f };

	ImVec2 position = ImGui::GetCurrentWindow( )->DC.CursorPos;
	position.x += 261.0f; // Сдвигаем элемент на 261 пиксель вправо (200-7+80-12=261)
	position.y -= 10.0f; // Поднимаем элемент на 10 пикселей выше

	ImRect total_bb( position, ImVec2( position.x + width, position.y + size ) );

	ImGui::ItemSize( total_bb, size + 15.f );

	if ( !ImGui::ItemAdd( total_bb , ImGui::GetID( name ) ) )
		return;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( total_bb, ImGui::GetID( name ), &hovered, &held );

	// Enhanced animation logic with hover effects
	ImGuiID id               = ImGui::GetID( name );
	ImGuiStorage* storage    = ImGui::GetStateStorage( );
	float hover_progress     = storage->GetFloat( id + 1000, 0.0f ); // Different ID for hover animation
	
	float hover_target       = hovered ? 1.0f : 0.0f;
	
	float dt = ImGui::GetIO( ).DeltaTime;
	hover_progress     = smoothLerp( hover_progress, hover_target, dt * 6.0f );
	
	storage->SetFloat( id + 1000, hover_progress );

	// Draw color picker
	bool color_picker_hovered = false;
	if ( color ) {
		const float color_picker_width = 13.0f;
		const float color_picker_height = 12.0f;
		const float color_picker_spacing = 12.0f;
		ImVec2 color_picker_pos = ImVec2( total_bb.Min.x - color_picker_spacing - color_picker_width + 26.0f, 
		                                 total_bb.Min.y + ( size - color_picker_height ) / 2 );
		
		// Создаем область для цветового пикера для проверки наведения
		ImRect color_picker_bb( color_picker_pos, ImVec2( color_picker_pos.x + color_picker_width, color_picker_pos.y + color_picker_height ) );
		color_picker_hovered = color_picker_bb.Contains( ImGui::GetIO( ).MousePos );

		// Рисуем цветовой пикер напрямую
		ImGui::PushClipRect( color_picker_bb.Min, color_picker_bb.Max, false );
		
		// Фон цветового пикера
		ImColor picker_bg_color = color_picker_hovered ? ImColor( 25, 25, 25, 255 ) : ImColor( 16, 16, 16, 255 );
		ImGui::GetWindowDrawList( )->AddRectFilled( color_picker_bb.Min, color_picker_bb.Max, picker_bg_color, 4.0f, ImDrawFlags_RoundCornersAll );
		
		// Обводка цветового пикера
		ImColor picker_outline_color = color_picker_hovered ? ImColor( 50, 50, 50, 255 ) : ImColor( 33, 33, 33, 255 );
		ImGui::GetWindowDrawList( )->AddRect( color_picker_bb.Min, color_picker_bb.Max, picker_outline_color, 4.0f, ImDrawFlags_RoundCornersAll, 1.0f );
		
		// Цвет внутри пикера с отображением прозрачности (как в ImGui ColorEdit)
		ImRect color_bb( color_picker_bb.Min + ImVec2( 1.0f, 1.0f ), color_picker_bb.Max - ImVec2( 1.0f, 1.0f ) );
		
		// Всегда рисуем шахматный паттерн с фиксированными закруглениями для консистентности
		ImU32 col = color->get_u32( 1.0f );
		
		// Рисуем фон
		ImGui::GetWindowDrawList( )->AddRectFilled( color_bb.Min, color_bb.Max, IM_COL32( 204, 204, 204, 255 ), 3.0f, ImDrawFlags_RoundCornersAll );
		
		// Рисуем шахматный паттерн с фиксированными закруглениями
		const float grid_step = 4.0f;
		int yi = 0;
		for ( float y = color_bb.Min.y; y < color_bb.Max.y; y += grid_step, yi++ ) {
			for ( float x = color_bb.Min.x + ( yi & 1 ) * grid_step; x < color_bb.Max.x; x += grid_step * 2.0f ) {
				ImVec2 p1( x, y );
				ImVec2 p2( ImMin( x + grid_step, color_bb.Max.x ), ImMin( y + grid_step, color_bb.Max.y ) );
				ImGui::GetWindowDrawList( )->AddRectFilled( p1, p2, IM_COL32( 128, 128, 128, 255 ), 3.0f, ImDrawFlags_RoundCornersAll );
			}
		}
		
		// Затем рисуем цвет поверх
		ImGui::GetWindowDrawList( )->AddRectFilled( color_bb.Min, color_bb.Max, col, 3.0f, ImDrawFlags_RoundCornersAll );
		
		ImGui::PopClipRect( );
	}

	// Draw label with smooth color animation
	ImColor inactive_text_color = ImColor( 150, 150, 150, int( 255 * 1.0f ) );
	ImColor hover_text_color    = ImColor( 200, 200, 200, int( 255 * 1.0f ) );
	
	ImColor text_color = smoothLerpColor( inactive_text_color, hover_text_color, hover_progress );
	
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::tasorbiter_addwindow, fonts_for_gui::tasorbiter_addwindow->FontSize,
	                                      ImVec2( total_bb.Min.x + spacing - 260.0f, total_bb.Min.y + ( size - name_text_size.y - fix_shit ) / 2 ),
	                                      text_color, name );

	// Color picker handling - открываем стандартное окно настройки цвета ImGui при клике на цветовой пикер
	if ( color_picker_hovered && !disabled && color ) {
		if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
			// Открываем стандартное окно настройки цвета ImGui с уникальным ID
			ImGui::OpenPopup( ( std::string( "##ColorPicker_" ) + name ).c_str( ) );
		}
	}

	// Используем кастомное окно цветового пикера
	custom_color_picker_window( ( std::string( "##ColorPicker_" ) + name ).c_str( ), color, ImVec2( 200.0f, 280.0f ) );
}
