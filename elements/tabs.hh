#pragma once
#include <vector>

#include "../instalisation fonts/fonts2.h"
#include "animations.h"

struct tab_t {
	std::string name;
    std::string icon;
	std::vector<std::string> sub_tabs;
};
inline int tab_number    = 0;
inline int subtab_number = 0;
float lerp( float a, float b, float t )
{
	return a + ( b - a ) * t;
}

// Линейная интерполяция для ImColor (аналогично можно добавить отдельную функцию для ImU32)
// Здесь выполняется побитовая разбивка на компоненты
ImColor lerpColor( const ImColor& a, const ImColor& b, float t )
{
	int r     = int( lerp( a.Value.x * 255.0f, b.Value.x * 255.0f, t ) );
	int g     = int( lerp( a.Value.y * 255.0f, b.Value.y * 255.0f, t ) );
	int b_col = int( lerp( a.Value.z * 255.0f, b.Value.z * 255.0f, t ) );
	int a_col = int( lerp( a.Value.w * 255.0f, b.Value.w * 255.0f, t ) );
	return ImColor( r, g, b_col, a_col );
}

#include <unordered_map>



void custom_tabs( std::vector< tab_t > tabs )
{
	float y_offset_adjustment = -110.0f; // Поднимаем табы на 110 пикселей вверх

	const static float spacing[ 2 ]{ 13.0f, 143.0f }; // LEFT (0) – UP (1)
	const static float size[ 2 ]{ 175.0f, 30.0f };    // WIDTH (0) – HEIGHT (1)
	const static float text_spacing{ 37.0f };
	const static float rounding{ 2.0f }; // Увеличенное закругление для hover эффекта

	// Для анимации раскрытия сабтабов – ключ: индекс основного таба, значение: [0..1]
	static std::unordered_map< size_t, float > subTabsAnimProgress;
	// Для анимации hover эффекта табов
	static std::unordered_map< size_t, float > tabHoverAnim;
	float dt = ImGui::GetIO( ).DeltaTime;
	// Множитель сглаживания – настраивается экспериментально
	float smoothing_factor = dt * 8.0f;

	for ( size_t i = 0; i < tabs.size( ); ++i ) {
		auto windowPos = ImGui::GetWindowPos( );

		float x_offset     = windowPos.x + spacing[ 0 ] + text_spacing;
		float y_offset     = windowPos.y + spacing[ 1 ] + ( i * size[ 1 ] ) + 7 + y_offset_adjustment;
		auto text_position = ImVec2( x_offset, y_offset );

		auto tab_min = ImVec2( windowPos.x + spacing[ 0 ], windowPos.y + spacing[ 1 ] + i * size[ 1 ] + y_offset_adjustment );
		auto tab_max = ImVec2( windowPos.x + size[ 0 ], windowPos.y + spacing[ 1 ] + ( i + 1 ) * size[ 1 ] + y_offset_adjustment );

		bool hovered  = ImGui::IsMouseHoveringRect( tab_min, tab_max );
		bool selected = ( i == tab_number );

		// Анимация hover эффекта для табов
		if ( tabHoverAnim.find( i ) == tabHoverAnim.end( ) )
			tabHoverAnim[ i ] = 0.0f;
		
		float hover_target = hovered ? 1.0f : 0.0f;
		tabHoverAnim[ i ] = smoothLerp( tabHoverAnim[ i ], hover_target, dt * 8.0f );

		if ( selected ) {
			// Активный таб имеет ширину 175 пикселей
			auto active_tab_max = ImVec2( windowPos.x + spacing[ 0 ] + 175.0f, windowPos.y + spacing[ 1 ] + ( i + 1 ) * size[ 1 ] + y_offset_adjustment );
			
			// Анимированный фон активного таба - возвращаем прежний цвет
			ImColor base_active_color = ImColor( 23, 23, 23, 255 );
			ImColor hover_active_color = ImColor( 28, 28, 28, 255 );
			ImColor final_active_color = smoothLerpColor( base_active_color, hover_active_color, tabHoverAnim[ i ] );
			
			ImGui::GetWindowDrawList( )->AddRectFilled( tab_min, active_tab_max, final_active_color, rounding,
			                                            ImDrawFlags_RoundCornersAll );
		} else if ( hovered ) {
			// Hover эффект для неактивных табов - более прямоугольный
			ImColor hover_color = ImColor( 18, 18, 18, int( 255 * tabHoverAnim[ i ] ) );
			ImGui::GetWindowDrawList( )->AddRectFilled( tab_min, tab_max, hover_color, rounding,
			                                            ImDrawFlags_RoundCornersAll );
		}

		// Анимированная отрисовка названия таба
		ImColor base_text_color = ImColor( 160, 160, 160, 255 ); // Цвет неактивного текста
		ImColor selected_text_color = ImColor( 245, 245, 245, 255 ); // Белый текст для активного таба
		ImColor hover_text_color = ImColor( 200, 200, 200, 255 );
		
		ImColor final_text_color = smoothLerpColor( 
			smoothLerpColor( base_text_color, hover_text_color, tabHoverAnim[ i ] ),
			selected_text_color, 
			selected ? 1.0f : 0.0f 
		);
		
		ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::tas_orbiter_menu, fonts_for_gui::tas_orbiter_menu->FontSize, text_position,
		                                      final_text_color, tabs[ i ].name.c_str( ) );
		
		// Специальное позиционирование для определенных иконок
		float icon_x_offset = -20.0f; // Стандартное смещение (сдвинуто на 12 пикселей влево)
		if ( tabs[ i ].icon == "m" ) { // movement
			icon_x_offset = -18.0f; // Сдвигаем на 2 пикселя вправо
		} else if ( tabs[ i ].icon == "c" ) { // fonts
			icon_x_offset = -22.0f; // Сдвигаем на 2 пикселя влево
		}
		
		// Анимированная отрисовка иконки
		ImColor base_icon_color = ImColor( 160, 160, 160, 255 ); // Цвет неактивной иконки
		ImColor selected_icon_color = ImColor( 220, 215, 220, 255 ); // Светлая иконка для активного таба
		ImColor hover_icon_color = ImColor( 180, 180, 180, 255 );
		
		ImColor final_icon_color = smoothLerpColor( 
			smoothLerpColor( base_icon_color, hover_icon_color, tabHoverAnim[ i ] ),
			selected_icon_color, 
			selected ? 1.0f : 0.0f 
		);
		
		if ( tabs[ i ].icon == "s" ) {
			const ImVec2 icon_origin( text_position.x + icon_x_offset - 1.0f, text_position.y + 2.5f );
			const ImVec2 bar_size( 11.0f, 2.2f );
			const float bar_gap = 3.8f;
			const float bar_rounding = 1.3f;
			for ( int bar = 0; bar < 3; ++bar ) {
				const ImVec2 bar_min( icon_origin.x, icon_origin.y + bar * bar_gap );
				const ImVec2 bar_max( bar_min.x + bar_size.x, bar_min.y + bar_size.y );
				ImGui::GetWindowDrawList( )->AddRectFilled( bar_min, bar_max, final_icon_color, bar_rounding, ImDrawFlags_RoundCornersAll );
			}
		} else {
			ImGui::GetWindowDrawList( )->AddText(
				fonts_for_gui::fontello_font, fonts_for_gui::fontello_font->FontSize, ImVec2( text_position.x + icon_x_offset, text_position.y ),
				final_icon_color, tabs[ i ].icon.c_str( ) );
		}

		if ( hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
			tab_number    = i;
			subtab_number = 0;
		}

		// Отрисовка сабтабов только если таковые имеются
		// Выполняется отрисовка только если таб выбран или анимация раскрытия ещё не завершена
		if ( !tabs[ i ].sub_tabs.empty( ) && ( selected || subTabsAnimProgress[ i ] > 0.001f ) ) {
			// Целевое значение анимации: если выбран – раскрываем (1.0f), иначе – сворачиваем (0.0f)
			float targetAnim = selected ? 1.0f : 0.0f;
			if ( subTabsAnimProgress.find( i ) == subTabsAnimProgress.end( ) ) {
				subTabsAnimProgress[ i ] = targetAnim;
			} else {
				// Плавно приближаем текущее значение к целевому с использованием easing
				subTabsAnimProgress[ i ] = smoothLerp( subTabsAnimProgress[ i ], targetAnim, smoothing_factor );
			}
			// Вычисляем высоту группы сабтабов с учётом анимации
			float groupHeight = tabs[ i ].sub_tabs.size( ) * size[ 1 ] * subTabsAnimProgress[ i ];

			auto group_min = ImVec2( windowPos.x + spacing[ 0 ], windowPos.y + spacing[ 1 ] + ( i + 1 ) * size[ 1 ] + y_offset_adjustment );
			auto group_max =
				ImVec2( windowPos.x + size[ 0 ], windowPos.y + spacing[ 1 ] + ( i + 1 ) * size[ 1 ] + y_offset_adjustment + groupHeight );

			// Фон группы сабтабов
			ImGui::GetWindowDrawList( )->AddRectFilled( group_min, group_max, ImColor( 10, 10, 10, int( 255 * g_menu.m_animation_progress ) ),
			                                            rounding, ImDrawFlags_RoundCornersBottom );

			// Рендеринг каждого сабтаба с анимациями
			for ( size_t j = 0; j < tabs[ i ].sub_tabs.size( ); ++j ) {
				auto sub_tab_name      = tabs[ i ].sub_tabs[ j ];
				auto sub_tab_min       = ImVec2( group_min.x, group_min.y + j * size[ 1 ] );
				auto sub_tab_max       = ImVec2( group_max.x, group_min.y + ( j + 1 ) * size[ 1 ] );
				auto sub_text_position = ImVec2( x_offset, group_min.y + j * size[ 1 ] + 5 );

				bool sub_hovered  = ImGui::IsMouseHoveringRect( sub_tab_min, sub_tab_max );
				bool sub_selected = ( subtab_number == j );
				
				// Hover эффект для сабтабов - увеличенное закругление
				if ( sub_hovered ) {
					ImColor sub_hover_color = ImColor( 15, 15, 15, 255 );
					ImGui::GetWindowDrawList( )->AddRectFilled( sub_tab_min, sub_tab_max, sub_hover_color, 2.0f,
					                                            ImDrawFlags_RoundCornersAll );
				}
				
				if ( selected ) {
					// Простой текст сабтаба без анимации
					ImColor sub_text_color = sub_selected ? ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 255 ) ) : ImColor( 255, 255, 255, 255 );
					ImGui::GetWindowDrawList( )->AddText(
						fonts_for_gui::tasorbiter_addwindow, fonts_for_gui::tasorbiter_addwindow->FontSize, sub_text_position,
						sub_text_color, sub_tab_name.c_str( ) );
				}

				if ( sub_hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
					subtab_number = j;
			}

			// Корректировка вертикального смещения для последующих табов
			y_offset_adjustment += tabs[ i ].sub_tabs.size( ) * size[ 1 ] * subTabsAnimProgress[ i ];
		}
	}
}
