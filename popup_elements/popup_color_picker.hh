#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/imgui_internal.h"
#include "../hacks/animations/animations.h"
#include "../game/sdk/classes/c_color.h"
#include "../elements/custom_color_picker.hh"
#include <string>

/* Google Pixel Pro стиль улучшенного цветового пикера для popup элементов */

bool popup_color_picker_button( const char* name, c_color* color, const ImVec2& size = ImVec2( 15.0f, 14.0f ) )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return false;

	// Улучшенные размеры цветового пикера в стиле Google Pixel Pro
	const float width = size.x;
	const float height = size.y;
	const float rounding = 6.0f; // Увеличенное закругление для современного вида
	const float thickness = 1.5f; // Увеличенная толщина обводки

	// Определяем прямоугольник кнопки
	const ImVec2 position = ImGui::GetCurrentWindow( )->DC.CursorPos;
	ImRect total_bb( position, position + ImVec2( width, height ) );

	// Регистрируем пространство для элемента
	ImGui::ItemSize( total_bb, height );
	if ( !ImGui::ItemAdd( total_bb, ImGui::GetID( name ) ) )
		return false;

	// Получаем состояние кнопки (hover, held, pressed)
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( total_bb, ImGui::GetID( name ), &hovered, &held );

	// Плавная анимация нажатия и наведения (как в основном меню)
	float dt = ImGui::GetIO( ).DeltaTime;
	static std::unordered_map< ImGuiID, float > pressAnim;
	static std::unordered_map< ImGuiID, float > hoverAnim;
	ImGuiID id = ImGui::GetID( name );
	
	// Инициализация анимаций
	if ( pressAnim.find( id ) == pressAnim.end( ) )
		pressAnim[ id ] = 0.0f;
	if ( hoverAnim.find( id ) == hoverAnim.end( ) )
		hoverAnim[ id ] = 0.0f;
	
	// Целевые значения для анимаций
	float pressTarget = held ? 1.0f : 0.0f;
	float hoverTarget = hovered ? 1.0f : 0.0f;
	
	// Обновляем анимационные значения с использованием плавной интерполяции
	pressAnim[ id ] = smoothLerp( pressAnim[ id ], pressTarget, dt * 12.0f );
	hoverAnim[ id ] = smoothLerp( hoverAnim[ id ], hoverTarget, dt * 8.0f );

	/* Google Pixel Pro стиль цветов для различных состояний */
	ImColor baseColor( 12, 12, 12, 255 ); // Более темный базовый цвет
	ImColor hoverColor( 20, 20, 20, 255 ); // Более контрастный hover
	ImColor accentColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 255 ) );
	
	// Смешиваем цвета с учетом анимаций
	ImColor finalColor = smoothLerpColor( 
		smoothLerpColor( baseColor, hoverColor, hoverAnim[ id ] ),
		accentColor, 
		pressAnim[ id ] 
	);

	// Отрисовка фона кнопки с плавным изменением цвета
	ImGui::GetWindowDrawList( )->AddRectFilled( total_bb.Min, total_bb.Max, finalColor, rounding, ImDrawFlags_RoundCornersAll );
	
	/* Google Pixel Pro стиль анимированной обводки */
	ImColor baseOutlineColor( 28, 28, 28, 255 ); // Более тонкая обводка
	ImColor accentOutlineColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 255 ) );
	ImColor hoverOutlineColor( 45, 45, 45, 255 ); // Более контрастная hover обводка
	
	ImColor finalOutlineColor = smoothLerpColor( 
		smoothLerpColor( baseOutlineColor, hoverOutlineColor, hoverAnim[ id ] ),
		accentOutlineColor, 
		pressAnim[ id ] 
	);
	
	// Толщина обводки с анимацией
	float outlineThickness = thickness + hoverAnim[ id ] * 0.5f;
	ImGui::GetWindowDrawList( )->AddRect( total_bb.Min, total_bb.Max, finalOutlineColor, rounding, ImDrawFlags_RoundCornersAll, outlineThickness );

	// Отрисовка цвета внутри пикера с отображением прозрачности (как в основном меню)
	ImRect color_bb( total_bb.Min + ImVec2( 1.0f, 1.0f ), total_bb.Max - ImVec2( 1.0f, 1.0f ) );
	
	// Всегда рисуем шахматный паттерн с фиксированными закруглениями для консистентности
	ImU32 col = color->get_u32( 1.0f );
	
	// Рисуем фон
	ImGui::GetWindowDrawList( )->AddRectFilled( color_bb.Min, color_bb.Max, IM_COL32( 204, 204, 204, 255 ), rounding - 1.0f, ImDrawFlags_RoundCornersAll );
	
	// Рисуем шахматный паттерн с фиксированными закруглениями
	const float grid_step = 4.0f;
	int yi = 0;
	for ( float y = color_bb.Min.y; y < color_bb.Max.y; y += grid_step, yi++ ) {
		for ( float x = color_bb.Min.x + ( yi & 1 ) * grid_step; x < color_bb.Max.x; x += grid_step * 2.0f ) {
			ImVec2 p1( x, y );
			ImVec2 p2( ImMin( x + grid_step, color_bb.Max.x ), ImMin( y + grid_step, color_bb.Max.y ) );
			ImGui::GetWindowDrawList( )->AddRectFilled( p1, p2, IM_COL32( 128, 128, 128, 255 ), rounding - 1.0f, ImDrawFlags_RoundCornersAll );
		}
	}
	
	// Затем рисуем цвет поверх
	ImGui::GetWindowDrawList( )->AddRectFilled( color_bb.Min, color_bb.Max, col, rounding - 1.0f, ImDrawFlags_RoundCornersAll );

	// Color picker handling - открываем стандартное окно настройки цвета ImGui при клике
	if ( hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
		// Открываем стандартное окно настройки цвета ImGui с уникальным ID
		ImGui::OpenPopup( ( std::string( "##PopupColorPicker_" ) + name ).c_str( ) );
	}

	// Используем улучшенное кастомное окно цветового пикера в стиле Google Pixel Pro
	custom_color_picker_window( ( std::string( "##PopupColorPicker_" ) + name ).c_str( ), color, ImVec2( 205.0f, 330.0f ) );

	return pressed;
}
