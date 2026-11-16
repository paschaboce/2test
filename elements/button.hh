#pragma once
#include "../instalisation fonts/fonts2.h"
#include "animations.h"
#include "../globals/config/variables.h"
#include <unordered_map>
#include <cfloat>

static float button_dpi_scale( )
{
	const int dpi = GET_VARIABLE( g_variables.m_menu_dpi_scale, int );
	return static_cast< float >( dpi < 75 ? 75 : dpi > 200 ? 200 : dpi ) / 100.0f;
}

// Функция линейной интерполяции для float
float lerp23( float a, float b, float t )
{
	return a + ( b - a ) * t;
}

// Функция для линейной интерполяции между двумя цветами
ImColor lerpColor23( const ImColor& a, const ImColor& b, float t )
{
	// Получаем нормированные компоненты цвета
	float ra = a.Value.x, ga = a.Value.y, ba = a.Value.z, aa = a.Value.w;
	float rb = b.Value.x, gb = b.Value.y, bb_val = b.Value.z, ab = b.Value.w;
	float r     = ra + ( rb - ra ) * t;
	float g     = ga + ( gb - ga ) * t;
	float b_col = ba + ( bb_val - ba ) * t;
	float a_col = aa + ( ab - aa ) * t;
	return ImColor( r, g, b_col, a_col );
}

bool button( const char* name, ImVec2 pos = ImVec2( 280, 22 ) )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return false;

	const float scale = button_dpi_scale( );
	pos = ImVec2( pos.x * scale, pos.y * scale );

	// Вычисляем размер текста через шрифт
	const float font_size = fonts_for_gui::tasorbiter_addwindow->FontSize * scale;
	const auto name_text_size = fonts_for_gui::tasorbiter_addwindow->CalcTextSizeA( font_size, FLT_MAX, NULL, name );

	// Задаём размеры кнопки: ширина – pos.x, высота – pos.y
	const float width            = pos.x;
	const float height           = pos.y;
	const float rounding         = 4.0f * scale;

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

	// Плавная анимация нажатия и наведения
	float dt = ImGui::GetIO( ).DeltaTime;
	// Статический контейнер для хранения состояния анимации по ID кнопки
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

	// Определяем цвета для различных состояний
	ImColor baseColor( 16, 16, 16, int( 255 * g_menu.m_animation_progress ) );
	ImColor accentColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) );
	ImColor hoverColor( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) );
	
	// Смешиваем цвета с учетом анимаций
	ImColor finalColor = smoothLerpColor( 
		smoothLerpColor( baseColor, hoverColor, hoverAnim[ id ] ),
		accentColor, 
		pressAnim[ id ] 
	);

	/*
	 * Отрисовка кнопки
	 */
	// Убираем clip rect, который может обрезать текст
	// Отрисовка фона кнопки с плавным изменением цвета
	ImGui::GetWindowDrawList( )->AddRectFilled( total_bb.Min, total_bb.Max, finalColor, rounding, ImDrawFlags_RoundCornersAll );
	
	// Анимированная обводка
	ImColor baseOutlineColor( 33, 33, 33, int( 255 * g_menu.m_animation_progress ) );
	ImColor accentOutlineColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) );
	ImColor hoverOutlineColor( 50, 50, 50, int( 255 * g_menu.m_animation_progress ) );
	
	ImColor finalOutlineColor = smoothLerpColor( 
		smoothLerpColor( baseOutlineColor, hoverOutlineColor, hoverAnim[ id ] ),
		accentOutlineColor, 
		pressAnim[ id ] 
	);
	
	// Толщина обводки с анимацией
	float outlineThickness = ( 1.0f + hoverAnim[ id ] * 0.5f ) * scale;
	ImGui::GetWindowDrawList( )->AddRect( total_bb.Min, total_bb.Max, finalOutlineColor, rounding, ImDrawFlags_RoundCornersAll, outlineThickness );

	// Вычисляем положение текста, центрируя его по горизонтали и вертикали
	const float text_x = total_bb.Min.x + ( width - name_text_size.x ) * 0.5f;
	const float text_y = total_bb.Min.y + ( height - name_text_size.y ) * 0.5f - 1.0f * scale;
	
	// Анимированный цвет текста
	ImColor baseTextColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) );
	ImColor accentTextColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) );
	ImColor hoverTextColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) );
	
	ImColor finalTextColor = smoothLerpColor( 
		smoothLerpColor( baseTextColor, hoverTextColor, hoverAnim[ id ] ),
		accentTextColor, 
		pressAnim[ id ] 
	);
	
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::tasorbiter_addwindow, font_size, ImVec2( text_x, text_y ),
	                                      finalTextColor, name );

	return pressed;
}
