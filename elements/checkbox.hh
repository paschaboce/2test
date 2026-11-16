#pragma once

#include "combobox.hh"
#include "../instalisation fonts/fonts2.h"
#include "../../popup_elements/popup_elements.h"

// Объявление функции color_picker_button
bool color_picker_button( const char* name, c_color* color, const ImVec2& size );

// Объявление функции popup_color_picker_button
bool popup_color_picker_button( const char* name, c_color* color, const ImVec2& size );

// Объявление функции text_with_color_picker
void text_with_color_picker( const char* name, c_color* color, bool disabled = false );
#include "animations.h"
#include <algorithm>
#include <cstring>
#include "../globals/config/variables.h"

static float checkbox_dpi_scale( )
{
	const int dpi = GET_VARIABLE( g_variables.m_menu_dpi_scale, int );
	return static_cast< float >( dpi < 75 ? 75 : dpi > 200 ? 200 : dpi ) / 100.0f;
}


inline void checkbox(
	const char* name, bool* variable, bool disabled = false, bool popup = false, const ImVec2& popup_size = ImVec2( ),
	const std::function< void( ) >& popup_function = [] { } )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return;

	const bool k_lester_labels = GET_VARIABLE( g_variables.m_lester_mode, bool );
	const float scale = checkbox_dpi_scale( );
	const char* label_draw =
	    ( k_lester_labels && std::strcmp( name, "LESTER MODE" ) != 0 ) ? "LESTER" : name;
	ImFont* label_font = fonts_for_gui::tasorbiter_addwindow;
	const float label_font_size = label_font ? label_font->FontSize * scale : ImGui::GetFontSize( );
	const ImVec2 name_text_size =
	    label_font ? label_font->CalcTextSizeA( label_font_size, FLT_MAX, 0.f, label_draw ) : ImGui::CalcTextSize( label_draw, NULL, true );
	const float width{ 120.0f * scale };
	const float size{ 15.0f * scale };
	const float spacing{ 5.0f * scale };
	const float rounding{ 4.0f * scale };
	const float fix_shit{ 5.0f * scale };

	ImVec2 position = ImGui::GetCurrentWindow( )->DC.CursorPos;
	position.x += 261.0f * scale; // Сдвигаем чекбокс на 261 пиксель вправо (200-7+80-12=261)

	ImRect total_bb( position, ImVec2( position.x + width, position.y + size ) );
	ImRect check_bb( total_bb.Min, ImVec2( total_bb.Min.x + size, total_bb.Min.y + size ) );

	ImGui::ItemSize( total_bb, size + 15.f * scale );

	if ( !ImGui::ItemAdd( total_bb , ImGui::GetID( name ) ) )
		return;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( total_bb, ImGui::GetID( name ), &hovered, &held );

	if ( pressed && !disabled )
		*variable = !( *variable );

	// Enhanced animation logic with hover effects
	ImGuiID id               = ImGui::GetID( name );
	ImGuiStorage* storage    = ImGui::GetStateStorage( );
	float animation_progress = storage->GetFloat( id, *variable ? 1.0f : 0.0f );
	float hover_progress     = storage->GetFloat( id + 1000, 0.0f ); // Different ID for hover animation
	
	float target             = *variable ? 1.0f : 0.0f;
	float hover_target       = hovered ? 1.0f : 0.0f;
	
	float dt = ImGui::GetIO( ).DeltaTime;
	animation_progress = smoothLerp( animation_progress, target, dt * 8.0f );
	hover_progress     = smoothLerp( hover_progress, hover_target, dt * 6.0f );
	
	storage->SetFloat( id, animation_progress );
	storage->SetFloat( id + 1000, hover_progress );

	// Draw checkbox with enhanced animation
	ImGui::PushClipRect( check_bb.Min, check_bb.Max, false );
	
	// Base colors
	ImColor unchecked_color = ImColor( 13, 13, 13, int( 255 * 1.0f ) );
	ImColor checked_color   = ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * 1.0f ) ) );
	ImColor hover_color     = ImColor( 20, 20, 20, int( 255 * 1.0f ) );
	
	// Mix colors with smooth interpolation
	ImColor background_color = smoothLerpColor( 
		smoothLerpColor( unchecked_color, hover_color, hover_progress ),
		checked_color, 
		animation_progress 
	);
	
	ImGui::GetWindowDrawList( )->AddRectFilled( check_bb.Min, check_bb.Max, background_color, rounding, ImDrawFlags_::ImDrawFlags_RoundCornersAll );
	
	// Animated outline
	ImColor base_outline_color = ImColor( 40, 40, 40, int( 255 * 1.0f ) );
	ImColor hover_outline_color = ImColor( 60, 60, 60, int( 255 * 1.0f ) );
	ImColor accent_outline_color = ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * 1.0f ) ) );
	
	ImColor outline_color = smoothLerpColor( 
		smoothLerpColor( base_outline_color, hover_outline_color, hover_progress ),
		accent_outline_color, 
		animation_progress 
	);
	
	ImGui::GetWindowDrawList( )->AddRect( check_bb.Min, check_bb.Max, outline_color, rounding, ImDrawFlags_::ImDrawFlags_RoundCornersAll, 1.0f * scale );

	// Убрана галочка по запросу пользователя
	// if ( animation_progress > 0.0f ) {
	//	float pad               = ImMax( 1.0f, size / 6.0f );
	//	ImColor checkmark_color = ImColor( 0, 0, 0, ( int )( 255 * animation_progress ) );
	//	ImGui::RenderCheckMark( ImGui::GetWindowDrawList( ), ImVec2( check_bb.Min.x + pad, check_bb.Min.y + pad ), checkmark_color, size - pad * 2.0f );
	// }
	ImGui::PopClipRect( );

	// Draw keyboard icon - только если есть возможность настройки (popup = true)
	bool keyboard_icon_hovered = false;
	if ( popup ) {
		const float keyboard_icon_size = 12.0f * scale;
		const float keyboard_icon_spacing = 12.0f * scale;
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

	// Draw label with smooth color animation
	ImColor inactive_text_color = ImColor( 150, 150, 150, int( 255 * 1.0f ) );
	ImColor active_text_color   = ImColor( 255, 255, 255, int( 255 * 1.0f ) );
	ImColor hover_text_color    = ImColor( 200, 200, 200, int( 255 * 1.0f ) );
	
	ImColor text_color = smoothLerpColor( 
		smoothLerpColor( inactive_text_color, hover_text_color, hover_progress ),
		active_text_color, 
		animation_progress 
	);
	
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::tasorbiter_addwindow, label_font_size,
	                                      ImVec2( check_bb.Max.x + spacing - 279.0f * scale, check_bb.Min.y + ( size - name_text_size.y - fix_shit ) / 2 ),
	                                      text_color, label_draw );

	// Popup handling - показываем popup при клике на иконку клавиатуры (ЛКМ)
	if ( *variable && keyboard_icon_hovered && !disabled && popup ) {
		if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
			ImGui::OpenPopup( name );
	}

	ImGui::SetNextWindowSize( ImVec2( popup_size.x * scale, popup_size.y * scale ) );
	if ( ImGui::BeginPopup( name, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
	                                  ImGuiWindowFlags_NoScrollWithMouse ) ) {
		ImVec2 pos = ImGui::GetWindowPos( ), size = ImGui::GetWindowSize( );
		
		// Основной фон popup с закруглением
		ImGui::GetWindowDrawList( )->AddRectFilled( pos, pos + size, ImColor( 13, 13, 13, int( 255 * 1.0f ) ), 12.0f * scale );
		
		// Обводка вокруг popup с закруглением
		ImGui::GetWindowDrawList( )->AddRect( pos, pos + size, ImColor( 33, 33, 33, int( 255 * 1.0f ) ), 12.0f * scale, 0, 1.0f * scale );

		ImGui::BeginGroup( );
		popup_function( );
		ImGui::EndGroup( );

		ImGui::EndPopup( );
	}
}

// Функция checkbox с цветовым пикером вместо иконки клавиатуры для popup с одной строчкой выбора цвета
inline void checkbox_with_color_picker(
	const char* name, bool* variable, bool disabled = false, bool popup = false, const ImVec2& popup_size = ImVec2( ),
	const std::function< void( ) >& popup_function = [] { }, c_color* color = nullptr )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return;

	const bool k_lester_labels = GET_VARIABLE( g_variables.m_lester_mode, bool );
	const char* label_draw =
	    ( k_lester_labels && std::strcmp( name, "LESTER MODE" ) != 0 ) ? "LESTER" : name;
	ImFont* label_font = fonts_for_gui::tasorbiter_addwindow;
	const ImVec2 name_text_size =
	    label_font ? label_font->CalcTextSizeA( label_font->FontSize, FLT_MAX, 0.f, label_draw ) : ImGui::CalcTextSize( label_draw, NULL, true );
	const static float width{ 120.0f };
	const static float size{ 15.0f };
	const static float spacing{ 5.0f };
	const static float rounding{ 4.0f };
	const static float fix_shit{ 5.0f };

	ImVec2 position = ImGui::GetCurrentWindow( )->DC.CursorPos;
	position.x += 261.0f; // Сдвигаем чекбокс на 261 пиксель вправо (200-7+80-12=261)

	ImRect total_bb( position, ImVec2( position.x + width, position.y + size ) );
	ImRect check_bb( total_bb.Min, ImVec2( total_bb.Min.x + size, total_bb.Min.y + size ) );

	ImGui::ItemSize( total_bb, size + 15.f );

	if ( !ImGui::ItemAdd( total_bb , ImGui::GetID( name ) ) )
		return;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( total_bb, ImGui::GetID( name ), &hovered, &held );

	if ( pressed && !disabled )
		*variable = !( *variable );

	// Enhanced animation logic with hover effects
	ImGuiID id               = ImGui::GetID( name );
	ImGuiStorage* storage    = ImGui::GetStateStorage( );
	float animation_progress = storage->GetFloat( id, *variable ? 1.0f : 0.0f );
	float hover_progress     = storage->GetFloat( id + 1000, 0.0f ); // Different ID for hover animation
	
	float target             = *variable ? 1.0f : 0.0f;
	float hover_target       = hovered ? 1.0f : 0.0f;
	
	float dt = ImGui::GetIO( ).DeltaTime;
	animation_progress = smoothLerp( animation_progress, target, dt * 8.0f );
	hover_progress     = smoothLerp( hover_progress, hover_target, dt * 6.0f );
	
	storage->SetFloat( id, animation_progress );
	storage->SetFloat( id + 1000, hover_progress );

	// Draw checkbox with enhanced animation
	ImGui::PushClipRect( check_bb.Min, check_bb.Max, false );
	
	// Base colors
	ImColor unchecked_color = ImColor( 13, 13, 13, int( 255 * 1.0f ) );
	ImColor checked_color   = ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * 1.0f ) ) );
	ImColor hover_color     = ImColor( 20, 20, 20, int( 255 * 1.0f ) );
	
	// Mix colors with smooth interpolation
	ImColor background_color = smoothLerpColor( 
		smoothLerpColor( unchecked_color, hover_color, hover_progress ),
		checked_color, 
		animation_progress 
	);
	
	ImGui::GetWindowDrawList( )->AddRectFilled( check_bb.Min, check_bb.Max, background_color, rounding, ImDrawFlags_::ImDrawFlags_RoundCornersAll );
	
	// Animated outline
	ImColor base_outline_color = ImColor( 40, 40, 40, int( 255 * 1.0f ) );
	ImColor hover_outline_color = ImColor( 60, 60, 60, int( 255 * 1.0f ) );
	ImColor accent_outline_color = ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * 1.0f ) ) );
	
	ImColor outline_color = smoothLerpColor( 
		smoothLerpColor( base_outline_color, hover_outline_color, hover_progress ),
		accent_outline_color, 
		animation_progress 
	);
	
	ImGui::GetWindowDrawList( )->AddRect( check_bb.Min, check_bb.Max, outline_color, rounding, ImDrawFlags_::ImDrawFlags_RoundCornersAll, 1.0f );

	// Убрана галочка по запросу пользователя
	// if ( animation_progress > 0.0f ) {
	//	float pad               = ImMax( 1.0f, size / 6.0f );
	//	ImColor checkmark_color = ImColor( 0, 0, 0, ( int )( 255 * animation_progress ) );
	//	ImGui::RenderCheckMark( ImGui::GetWindowDrawList( ), ImVec2( check_bb.Min.x + pad, check_bb.Min.y + pad ), checkmark_color, size - pad * 2.0f );
	// }
	ImGui::PopClipRect( );

	// Draw color picker instead of keyboard icon - только если есть возможность настройки (popup = true) и передан цвет
	bool color_picker_hovered = false;
	if ( popup && color ) {
		const float color_picker_width = 13.0f;
		const float color_picker_height = 12.0f;
		const float color_picker_spacing = 12.0f;
		ImVec2 color_picker_pos = ImVec2( check_bb.Min.x - color_picker_spacing - color_picker_width, 
		                                 check_bb.Min.y + ( size - color_picker_height ) / 2 );
		
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
	ImColor active_text_color   = ImColor( 255, 255, 255, int( 255 * 1.0f ) );
	ImColor hover_text_color    = ImColor( 200, 200, 200, int( 255 * 1.0f ) );
	
	ImColor text_color = smoothLerpColor( 
		smoothLerpColor( inactive_text_color, hover_text_color, hover_progress ),
		active_text_color, 
		animation_progress 
	);
	
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::tasorbiter_addwindow, fonts_for_gui::tasorbiter_addwindow->FontSize,
	                                      ImVec2( check_bb.Max.x + spacing - 279.0f, check_bb.Min.y + ( size - name_text_size.y - fix_shit ) / 2 ),
	                                      text_color, label_draw );

	// Color picker handling - открываем стандартное окно настройки цвета ImGui при клике на цветовой пикер
	if ( *variable && color_picker_hovered && !disabled && popup && color ) {
		if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
			// Открываем стандартное окно настройки цвета ImGui с уникальным ID
			ImGui::OpenPopup( ( std::string( "##ColorPicker_" ) + name ).c_str( ) );
		}
	}

	// Кастомное окно настройки цвета в стиле меню
	if ( ImGui::BeginPopup( ( std::string( "##ColorPicker_" ) + name ).c_str( ), 
	                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | 
	                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) ) {
		
		// Устанавливаем размер окна с отступами 5px
		ImGui::SetNextWindowSize( ImVec2( 200.0f, 120.0f ) );
		
		const auto draw_list = ImGui::GetWindowDrawList( );
		const ImVec2 position = ImGui::GetWindowPos( );
		const ImVec2 size = ImGui::GetWindowSize( );
		
		// Фон popup в стиле меню с закруглением
		draw_list->AddRectFilled( position, position + size, ImColor( 13, 13, 13, 255 ), 8.0f, ImDrawFlags_RoundCornersAll );
		
		// Обводка вокруг popup в стиле меню
		draw_list->AddRect( position, position + size, ImColor( 33, 33, 33, 255 ), 8.0f, ImDrawFlags_RoundCornersAll, 1.0f );
		
		// Устанавливаем отступы 5px от краев
		ImGui::SetCursorPos( ImVec2( 5.0f, 5.0f ) );
		
		// Создаем компактный цветовой пикер
		ImVec4 color_vec4 = color->get_vec4( 1.0f );
		
		// Основной цветовой пикер (упрощенный)
		if ( ImGui::ColorPicker4( ( std::string( "##color_picker_" ) + name ).c_str( ), &color_vec4.x, 
		                         ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview | 
		                         ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview ) ) {
			// Обновляем цвет из ImVec4 обратно в c_color
			*color = c_color( color_vec4.x, color_vec4.y, color_vec4.z, color_vec4.w );
		}
		
		// Добавляем мини кнопки для быстрого выбора цветов
		ImGui::SetCursorPosY( ImGui::GetCursorPosY( ) + 5.0f );
		ImGui::SetCursorPosX( 5.0f );
		
		// Создаем мини кнопки для предустановленных цветов
		const ImVec2 mini_button_size = ImVec2( 25.0f, 20.0f );
		const float mini_button_spacing = 5.0f;
		
		// Массив предустановленных цветов
		const c_color preset_colors[] = {
			c_color( 255, 255, 255, 255 ), // Белый
			c_color( 0, 0, 0, 255 ),       // Черный
			c_color( 255, 0, 0, 255 ),     // Красный
			c_color( 0, 255, 0, 255 ),     // Зеленый
			c_color( 0, 0, 255, 255 ),     // Синий
			c_color( 255, 255, 0, 255 ),   // Желтый
			c_color( 255, 0, 255, 255 ),   // Пурпурный
			c_color( 0, 255, 255, 255 )    // Голубой
		};
		
		// Рисуем мини кнопки для быстрого выбора
		for ( int i = 0; i < 8; i++ ) {
			ImVec2 button_pos = ImVec2( 5.0f + ( i % 4 ) * ( mini_button_size.x + mini_button_spacing ), 
			                           85.0f + ( i / 4 ) * ( mini_button_size.y + mini_button_spacing ) );
			
			ImRect button_bb( button_pos, button_pos + mini_button_size );
			
			// Проверяем наведение и клик
			bool hovered = ImGui::IsMouseHoveringRect( button_bb.Min, button_bb.Max );
			bool clicked = hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left );
			
			// Цвет кнопки
			ImColor button_color = hovered ? ImColor( 25, 25, 25, 255 ) : ImColor( 16, 16, 16, 255 );
			ImColor outline_color = hovered ? ImColor( 50, 50, 50, 255 ) : ImColor( 33, 33, 33, 255 );
			
			// Рисуем кнопку
			draw_list->AddRectFilled( button_bb.Min, button_bb.Max, button_color, 4.0f, ImDrawFlags_RoundCornersAll );
			draw_list->AddRect( button_bb.Min, button_bb.Max, outline_color, 4.0f, ImDrawFlags_RoundCornersAll, 1.0f );
			
			// Рисуем цвет внутри кнопки
			ImRect color_bb( button_bb.Min + ImVec2( 1.0f, 1.0f ), button_bb.Max - ImVec2( 1.0f, 1.0f ) );
			draw_list->AddRectFilled( color_bb.Min, color_bb.Max, preset_colors[ i ].get_u32( 255 ), 3.0f, ImDrawFlags_RoundCornersAll );
			
			// Обработка клика
			if ( clicked ) {
				*color = preset_colors[ i ];
			}
		}
		
		ImGui::EndPopup( );
	}
}

inline const char* key_names[] = {
	"-",   "m1",  "m2",  "cn",  "m3",  "m4",  "m5",  "-",   "bac", "tab", "-",   "-",   "clr", "ret", "-",   "-",
	"shi", "ctl", "men", "pau", "cap", "kan", "-",   "jun", "fin", "kan", "-",   "esc", "con", "nco", "acc", "mad",
	"spa", "pgu", "pgd", "end", "hom", "lef", "up",  "rig", "dow", "sel", "pri", "exe", "pri", "ins", "del", "hel",
	"0",   "1",   "2",   "3",   "4",   "5",   "6",   "7",   "8",   "9",   "-",   "-",   "-",   "-",   "-",   "-",
	"-",   "a",   "b",   "c",   "d",   "e",   "f",   "g",   "h",   "i",   "j",   "k",   "l",   "m",   "n",   "o",
	"p",   "q",   "r",   "s",   "t",   "u",   "v",   "w",   "x",   "y",   "z",   "win", "win", "app", "-",   "sle",
	"num", "num", "num", "num", "num", "num", "num", "num", "num", "num", "mul", "add", "sep", "min", "dec", "div",
	"f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",  "f8",  "f9",  "f10", "f11", "f12", "f13", "f14", "f15", "f16",
	"f17", "f18", "f19", "f20", "f21", "f22", "f23", "f24", "-",   "-",   "-",   "-",   "-",   "-",   "-",   "-",
	"num", "scr", "equ", "mas", "toy", "oya", "oya", "-",   "-",   "-",   "-",   "-",   "-",   "-",   "-",   "-",
	"shi", "shi", "ctr", "ctr", "alt", "alt"
};

enum e_key_styles {
	always_on = 0,
	hold,
	toggle
};



bool keybind( const char* name, key_bind_t* key, bool set_key_mode = true )
{
	if (ImGui::GetCurrentWindow()->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;

	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;

	char buf_display[64] = ("none");

	if (key->m_key != 0 && g.ActiveId != ImGui::GetCurrentWindow()->GetID(name))
		strcpy_s(buf_display, key_names[key->m_key]);

	const static auto name_text_size = ImGui::CalcTextSize(key_names[key->m_key], NULL, true);
	const auto buffer_text_size = ImGui::CalcTextSize(buf_display, NULL, true);
	const static float width{ 40.0f };
	const static float height{ 20.0f };
	const static float spacing{ 5.0f };
	const static float fix_shit{ 5.0f };
	const static float rounding{ 2.0f };

	ImGui::SameLine(ImGui::GetCurrentWindow()->Size.x - (width + (height - spacing)) - 9.0f);

	const ImRect frame_bb(ImGui::GetCurrentWindow()->DC.CursorPos, ImGui::GetCurrentWindow()->DC.CursorPos + ImVec2(width, height));

	ImGui::ItemSize(frame_bb, height);

	if (!ImGui::ItemAdd(frame_bb, ImGui::GetCurrentWindow()->GetID(name)))
		return false;

	bool value_changed = false;
	int k = key->m_key;

	const bool hovered = ImGui::IsItemHovered();

	if (hovered && io.MouseClicked[0]) {
		if (g.ActiveId != ImGui::GetCurrentWindow()->GetID(name)) {
			memset(io.MouseDown, 0, sizeof(io.MouseDown));
			memset(io.KeysDown, 0, sizeof(io.KeysDown));
			key->m_key = 0;
		}
		ImGui::SetActiveID(ImGui::GetCurrentWindow()->GetID(name), ImGui::GetCurrentWindow());
		ImGui::FocusWindow(ImGui::GetCurrentWindow());
	}
	else if (io.MouseClicked[0]) {
		if (g.ActiveId == ImGui::GetCurrentWindow()->GetID(name))
			ImGui::ClearActiveID();
	}
	else if (hovered && io.MouseClicked[1] && set_key_mode)
		ImGui::OpenPopup(name);

	if (g.ActiveId == ImGui::GetCurrentWindow()->GetID(name)) {
		for (auto i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++) {
			if (io.MouseDown[i]) {
				switch (i) {
				case 0:
					k = 0x01;
					break;
				case 1:
					k = 0x02;
					break;
				case 2:
					k = 0x04;
					break;
				case 3:
					k = 0x05;
					break;
				case 4:
					k = 0x06;
					break;
				}

				value_changed = true;
				ImGui::ClearActiveID();
			}
		}

		if (!value_changed) {
			for (auto i = 0x08; i <= 0xA5; i++) {
				if (io.KeysDown[i]) {
					k = i;
					value_changed = true;
					ImGui::ClearActiveID();
				}
			}
		}

		if (ImGui::IsKeyPressedMap(ImGuiKey_Escape)) {
			key->m_key = 0;
			ImGui::ClearActiveID();
		}
		else
			key->m_key = k;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowPadding, ImVec2(spacing, spacing));

	ImGui::SetNextWindowSize(ImVec2(70.f, CalcMaxPopupHeightFromItemCount(sizeof(e_key_styles) - 1) + spacing), ImGuiCond_Always);
	ImGui::SetNextWindowPos(frame_bb.Min + ImVec2(0.0f, height + spacing), ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_Popup
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoScrollWithMouse
		| ImGuiWindowFlags_NoMove;

	if (ImGui::BeginPopup(name, flags)) {
		ImGui::PushClipRect(ImGui::GetCurrentWindow()->Pos, ImGui::GetCurrentWindow()->Pos + ImGui::GetWindowSize(), false);
		ImGui::GetCurrentWindow( )->DrawList->AddRectFilled( ImGui::GetCurrentWindow( )->Pos,
		                                                     ImGui::GetCurrentWindow( )->Pos + ImGui::GetWindowSize( ),
		                                                     ImColor( 7, 7, 7, int( 255 * 1.0f ) ), rounding );
		ImGui::GetCurrentWindow( )->DrawList->AddRect( ImGui::GetCurrentWindow( )->Pos, ImGui::GetCurrentWindow( )->Pos + ImGui::GetWindowSize( ),
		                                               ImColor( 50, 50, 50, int( 255 * 1.0f ) ), rounding );
		ImGui::PopClipRect();

		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor( ImGuiCol_Text, key->m_key_style == always_on
		                                          ? ImVec4( 230 / 255.f, 210 / 255.f, 255 / 255.f, float( 1.f * 1.0f ) )
		                                          : ImVec4( 1.0f, 1.0f, 1.0f, float( 1.f * 1.0f ) ) );
        if (ImGui::Selectable("always on", key->m_key_style == always_on))  
           key->m_key_style = always_on;  
		ImGui::PopStyleColor();

		ImGui::PushStyleColor( ImGuiCol_Text, key->m_key_style == hold
		                                          ? ImVec4( 230 / 255.f, 210 / 255.f, 255 / 255.f, float( 1.f * 1.0f ) )
		                                                               : ImVec4( 1.0f, 1.0f, 1.0f, float( 1.f * 1.0f ) ) );
        if (ImGui::Selectable("hold", key->m_key_style == hold))  
           key->m_key_style = hold;  
        ImGui::PopStyleColor();  

		ImGui::PushStyleColor( ImGuiCol_Text, key->m_key_style == toggle
		                                          ? ImVec4( 230 / 255.f, 210 / 255.f, 255 / 255.f, float( 1.f * 1.0f ) )
		                                          : ImVec4( 1.0f, 1.0f, 1.0f, float( 1.f * 1.0f ) ) );
        if (ImGui::Selectable("toggle", key->m_key_style == toggle))  
           key->m_key_style = toggle;  
        ImGui::PopStyleColor();
		ImGui::PopStyleColor(3);

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar();

	// Заливка цветом 16, 16, 16
	ImGui::GetCurrentWindow( )->DrawList->AddRectFilled( frame_bb.Min, frame_bb.Max, ImColor( 16, 16, 16, int( 255 * 1.0f ) ),
	                                                     rounding );
	
	// Обводка в 1 пиксель цветом 33, 33, 33
	ImGui::GetCurrentWindow( )->DrawList->AddRect( frame_bb.Min, frame_bb.Max, ImColor( 33, 33, 33, int( 255 * 1.0f ) ),
	                                               rounding, ImDrawFlags_RoundCornersAll, 1.0f );

    ImVec2 text_pos = frame_bb.Min + (frame_bb.GetSize() - buffer_text_size - ImVec2(0, fix_shit)) * 0.5f;

    ImGui::GetCurrentWindow( )->DrawList->AddText( fonts_for_gui::tasorbiter_addwindow, fonts_for_gui::tasorbiter_addwindow->FontSize, text_pos,
	                                               g.ActiveId == ImGui::GetCurrentWindow( )->GetID( name )
	                                                   ? ImColor( 230, 210, 255, int( 255 * 1.0f ) )
	                                               : key->m_key != 0 ? ImColor( 230, 210, 255, int( 255 * 1.0f ) )
	                                                                 : ImColor( 150, 150, 150, int( 255 * 1.0f ) ),
	                                               buf_display );

	return value_changed;
}
