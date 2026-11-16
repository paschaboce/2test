#pragma once
#include "../instalisation fonts/fonts2.h"
#include "animations.h"
#include "../globals/config/variables.h"
#include <string>
#include <unordered_map> // Не забудьте подключить для работы unordered_map

static float slider_dpi_scale( )
{
	const int dpi = GET_VARIABLE( g_variables.m_menu_dpi_scale, int );
	return static_cast< float >( dpi < 75 ? 75 : dpi > 200 ? 200 : dpi ) / 100.0f;
}

bool draw_slider( const char* name, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format,
                  float width = 75.f, float value_box_width = -1.f )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return false;

	const float scale = slider_dpi_scale( );
	width *= scale;
	if ( value_box_width > 0.0f )
		value_box_width *= scale;

	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = ImGui::GetCurrentWindow( );

	char value_buf[ 64 ];
	const char* value_buf_end = value_buf + ImGui::DataTypeFormatString( value_buf, IM_ARRAYSIZE( value_buf ), data_type, p_data, format );

	const float height{ 20.0f * scale };
	const float grabber_size{ 12.0f * scale }; // Размер круглого ползунка
	const float slide_height{ 1.0f * scale }; // Высота дорожки слайдера
	const float rounding{ 2.0f * scale };
	const static auto name_text_size = ImGui::CalcTextSize( name, NULL, true );
	const auto value_text_size       = ImGui::CalcTextSize( value_buf, value_buf_end, true );

	const ImRect frame_bb( ImGui::GetCurrentWindow( )->DC.CursorPos + ImVec2( 15.0f * scale, -2.0f * scale ),
	                       ImGui::GetCurrentWindow( )->DC.CursorPos + ImVec2( 15.0f * scale + width, height - 2.0f * scale ) );
	
	// Вычисляем позицию бокса с цифрами (правый край на расстоянии 292px от левого края чилда)
	const float box_right_edge_offset = 292.0f * scale; // Расстояние правого края бокса от левого края чилда
	ImVec2 child_pos = ImGui::GetCurrentWindow()->Pos;
	
	// Определяем ширину бокса
	float box_width;
	if ( value_box_width > 0 ) {
		box_width = value_box_width;
	} else {
		box_width = 60.0f * scale;
		switch ( data_type ) {
		case ImGuiDataType_Float:
			box_width = 70.0f * scale;
			break;
		case ImGuiDataType_S32:
			box_width = 50.0f * scale;
			break;
		case ImGuiDataType_Double:
			box_width = 80.0f * scale;
			break;
		default:
			box_width = 60.0f * scale;
			break;
		}
	}
	
	// Позиция бокса с цифрами
	ImVec2 box_pos = ImVec2( 
		child_pos.x + box_right_edge_offset - box_width, // Левая позиция = правая позиция - ширина
		ImGui::GetCurrentWindow( )->DC.CursorPos.y - (height - slide_height) / 2 - 2.0f * scale + 15.0f * scale - 6.0f * scale - 16.0f * scale + 13.0f * scale
	);
	
	// Позиция слайдера: правый край в 12px от левого края бокса
	const float slider_end_x = box_pos.x - 12.0f * scale; // 12px от левого края бокса
	const float slider_start_x = slider_end_x - width; // Начинается на ширину слайдера левее
	
	const ImRect total_bb( 
		ImVec2( slider_start_x, ImGui::GetCurrentWindow( )->DC.CursorPos.y + (height - slide_height) / 2 - 2.0f * scale + 6.0f * scale - 6.0f * scale ),
		ImVec2( slider_end_x, ImGui::GetCurrentWindow( )->DC.CursorPos.y + (height - slide_height) / 2 + slide_height - 2.0f * scale + 6.0f * scale - 6.0f * scale )
	);

	// Вычисляем прогресс для определения позиции ползунка
	float progress = 0.0f;
	switch ( data_type ) {
	case ImGuiDataType_Float:
		progress = (*static_cast< float* >( p_data ) - *static_cast< const float* >( p_min )) / 
		          (*static_cast< const float* >( p_max ) - *static_cast< const float* >( p_min ));
		break;
	case ImGuiDataType_S32:
		progress = (*static_cast< int* >( p_data ) - *static_cast< const int* >( p_min )) / 
		          (float)(*static_cast< const int* >( p_max ) - *static_cast< const int* >( p_min ));
		break;
	case ImGuiDataType_Double:
		progress = (*static_cast< double* >( p_data ) - *static_cast< const double* >( p_min )) / 
		          (*static_cast< const double* >( p_max ) - *static_cast< const double* >( p_min ));
		break;
	default:
		progress = (*static_cast< int* >( p_data ) - *static_cast< const int* >( p_min )) / 
		          (float)(*static_cast< const int* >( p_max ) - *static_cast< const int* >( p_min ));
		break;
	}
	progress = std::clamp( progress, 0.0f, 1.0f );

	// Вычисляем позицию ползунка
	ImVec2 grabber_center = ImVec2( 
		total_bb.Min.x + progress * total_bb.GetWidth(), // X позиция по прогрессу
		total_bb.Min.y + total_bb.GetHeight() / 2        // Y позиция по центру дорожки
	);

	// Создаем область для ползунка (больше чем сам ползунок для удобства клика)
	ImRect grabber_bb( 
		ImVec2( grabber_center.x - grabber_size, grabber_center.y - grabber_size ),
		ImVec2( grabber_center.x + grabber_size, grabber_center.y + grabber_size )
	);

	// Расширяем область взаимодействия для всей дорожки + ползунка
	ImRect interaction_bb = total_bb;
	interaction_bb.Expand( ImVec2( 0, grabber_size ) );

	ImGui::ItemSize( frame_bb, height );

	// Используем расширенную область взаимодействия для ItemAdd
	const ImGuiID slider_id = ImGui::GetID( name );
	if ( !ImGui::ItemAdd( interaction_bb, slider_id ) )
		return false;

	const bool hovered = ImGui::ItemHoverable( interaction_bb, slider_id );
	const bool clicked = hovered && ImGui::IsMouseClicked( ImGuiMouseButton_::ImGuiMouseButton_Left, false );

	// Дополнительная проверка для клика по ползунку
	const bool grabber_hovered = ImGui::IsMouseHoveringRect( grabber_bb.Min, grabber_bb.Max );
	const bool grabber_clicked = grabber_hovered && ImGui::IsMouseClicked( ImGuiMouseButton_::ImGuiMouseButton_Left, false );
	
	// Animation system for slider
	ImGuiID id = slider_id;
	static std::unordered_map< ImGuiID, float > hover_anim;
	static std::unordered_map< ImGuiID, float > grabber_hover_anim;
	
	if ( hover_anim.find( id ) == hover_anim.end( ) )
		hover_anim[ id ] = 0.0f;
	if ( grabber_hover_anim.find( id ) == grabber_hover_anim.end( ) )
		grabber_hover_anim[ id ] = 0.0f;
	
	float dt = ImGui::GetIO( ).DeltaTime;
	hover_anim[ id ] = smoothLerp( hover_anim[ id ], hovered ? 1.0f : 0.0f, dt * 8.0f );
	grabber_hover_anim[ id ] = smoothLerp( grabber_hover_anim[ id ], grabber_hovered ? 1.0f : 0.0f, dt * 10.0f );

	// Отладочная информация (можно убрать после тестирования)
	#ifdef _DEBUG
	if ( grabber_hovered ) {
		ImGui::GetWindowDrawList( )->AddRect( grabber_bb.Min, grabber_bb.Max, ImColor( 255, 0, 0, 100 ), 0, 1.0f );
	}
	if ( hovered ) {
		ImGui::GetWindowDrawList( )->AddRect( interaction_bb.Min, interaction_bb.Max, ImColor( 0, 255, 0, 100 ), 0, 1.0f );
	}
	#endif

	if ( clicked || grabber_clicked ) {
		ImGui::SetActiveID( slider_id, window );
		ImGui::SetFocusID( slider_id, window );
		ImGui::FocusWindow( window );
		g.ActiveIdUsingNavDirMask |= ( 1 << ImGuiDir_Left ) | ( 1 << ImGuiDir_Right );
	}

	int percentage = 0;
	ImRect grab_bb;
	const bool value_changed =
		ImGui::SliderBehavior( total_bb, slider_id, data_type, p_data, p_min, p_max, format,
	                           ImGuiSliderFlags_::ImGuiSliderFlags_NoInput | ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp, &grab_bb, &percentage );

	if ( value_changed )
		ImGui::MarkItemEdited( slider_id );

	/*
	 * Отрисовка элементов слайдера
	 */

	// Анимированный фон дорожки слайдера
	ImColor base_track_color = ImColor( 88, 88, 88, 255 );
	ImColor hover_track_color = ImColor( 100, 100, 100, 255 );
	ImColor final_track_color = smoothLerpColor( base_track_color, hover_track_color, hover_anim[ id ] );
	
	ImGui::GetWindowDrawList( )->AddRectFilled( total_bb.Min, total_bb.Max, final_track_color, rounding,
	                                            ImDrawFlags_::ImDrawFlags_RoundCornersAll );

	// Анимированная заливка пройденной части слайдера
	ImVec2 filled_end = ImVec2( total_bb.Min.x + progress * total_bb.GetWidth(), total_bb.Max.y );
	ImRect filled_bb( total_bb.Min, filled_end );
	
	if ( progress > 0.0f ) {
		ImColor base_fill_color = ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 200 ) );
		ImColor hover_fill_color = ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 255 ) );
		ImColor final_fill_color = smoothLerpColor( base_fill_color, hover_fill_color, hover_anim[ id ] );
		
		ImGui::GetWindowDrawList( )->AddRectFilled( filled_bb.Min, filled_bb.Max, 
			final_fill_color, rounding,
			ImDrawFlags_::ImDrawFlags_RoundCornersAll );
	}

	// Анимированное название
	ImColor base_text_color = ImColor( 150, 150, 150, 255 );
	ImColor hover_text_color = ImColor( 200, 200, 200, 255 );
	ImColor final_text_color = smoothLerpColor( base_text_color, hover_text_color, hover_anim[ id ] );
	
	const float font_size = fonts_for_gui::tasorbiter_addwindow->FontSize * scale;
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::tasorbiter_addwindow, font_size,
	                                      frame_bb.Min + ImVec2( -13.0f * scale, 0.0f ),
	                                      final_text_color, name );

	// Анимированный ползунок
	ImColor base_grabber_color = ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 255 ) );
	ImColor hover_grabber_color = ImColor( 
		std::min( 255, (int)(base_grabber_color.Value.x * 255 * 1.3f) ),
		std::min( 255, (int)(base_grabber_color.Value.y * 255 * 1.3f) ),
		std::min( 255, (int)(base_grabber_color.Value.z * 255 * 1.3f) ),
		255
	);
	ImColor final_grabber_color = smoothLerpColor( base_grabber_color, hover_grabber_color, grabber_hover_anim[ id ] );
	
	ImGui::GetWindowDrawList( )->AddCircleFilled( grabber_center, grabber_size / 2, final_grabber_color );
	
	// Анимированная обводка ползунка
	ImColor base_outline_color = ImColor( 255, 255, 255, 100 );
	ImColor hover_outline_color = ImColor( 255, 255, 255, 200 );
	ImColor final_outline_color = smoothLerpColor( base_outline_color, hover_outline_color, grabber_hover_anim[ id ] );
	
	ImGui::GetWindowDrawList( )->AddCircle( grabber_center, grabber_size / 2,
		final_outline_color, 0, 1.0f * scale );

	// Бокс для отображения значения слайдера
	const float box_height = 20.0f * scale;
	
	// Используем уже вычисленную позицию бокса
	ImRect value_box( box_pos, ImVec2( box_pos.x + box_width, box_pos.y + box_height ) );
	const std::string value_box_label = std::string( name ) + "##_value";
	const ImGuiID value_box_id = ImGui::GetID( value_box_label.c_str( ) );
	bool value_box_hovered = false;
	bool value_box_held = false;
	bool value_box_pressed = false;
	bool value_box_active = ImGui::TempInputIsActive( value_box_id );
	if ( !value_box_active ) {
		if ( ImGui::ItemAdd( value_box, value_box_id ) )
			value_box_pressed = ImGui::ButtonBehavior( value_box, value_box_id, &value_box_hovered, &value_box_held );
		value_box_active = value_box_pressed;
	}
	
	// Анимированный фон бокса
	ImColor base_box_color = ImColor( 16, 16, 16, 255 );
	ImColor hover_box_color = ImColor( 22, 22, 22, 255 );
	ImColor final_box_color = smoothLerpColor( base_box_color, hover_box_color, value_box_hovered ? 1.0f : hover_anim[ id ] );
	
	ImGui::GetWindowDrawList( )->AddRectFilled( value_box.Min, value_box.Max, final_box_color, 2.0f * scale );
	
	// Анимированная обводка бокса
	ImColor base_box_outline = ImColor( 32, 32, 32, 255 );
	ImColor hover_box_outline = ImColor( 50, 50, 50, 255 );
	ImColor final_box_outline = smoothLerpColor( base_box_outline, hover_box_outline, value_box_hovered ? 1.0f : hover_anim[ id ] );
	
	float box_outline_thickness = ( 1.0f + hover_anim[ id ] * 0.5f ) * scale;
	ImGui::GetWindowDrawList( )->AddRect( value_box.Min, value_box.Max, final_box_outline, 2.0f * scale, 0, box_outline_thickness );

	if ( value_box_active ) {
		if ( ImGui::TempInputScalar( value_box, value_box_id, value_box_label.c_str( ), data_type, p_data, format, p_min, p_max ) )
			ImGui::MarkItemEdited( value_box_id );
		return value_changed || ImGui::IsItemEdited( );
	}
	
	// Вычисляем максимальное значение для центрирования
	char max_value_buf[ 64 ];
	const char* max_value_buf_end = max_value_buf + ImGui::DataTypeFormatString( max_value_buf, IM_ARRAYSIZE( max_value_buf ), data_type, (void*)p_max, format );
	const auto max_value_text_size = ImGui::CalcTextSize( max_value_buf, max_value_buf_end, true );
	
	// Текст значения в боксе (привязан к левой части, центрируется только максимальное значение)
	ImVec2 text_pos = ImVec2( 
		value_box.Min.x + 8.0f * scale, // Отступ от левого края
		value_box.Min.y + (box_height - value_text_size.y) / 2 
	);
	
	// Если текущее значение максимальное, центрируем его
	if ( value_text_size.x >= max_value_text_size.x * 0.9f ) { // 90% от максимального размера
		text_pos.x = value_box.Min.x + (box_width - value_text_size.x) / 2;
	}
	
	// Анимированный текст значения
	ImColor base_value_color = ImColor( 150, 150, 150, 255 );
	ImColor hover_value_color = ImColor( 200, 200, 200, 255 );
	ImColor final_value_color = smoothLerpColor( base_value_color, hover_value_color, hover_anim[ id ] );
	
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::tasorbiter_addwindow, font_size,
		text_pos, final_value_color, value_buf, value_buf_end );

	return value_changed;
}


bool slider_float(const char* label, float* v, float v_min, float v_max, const char* format, float width = 75.f, float value_box_width = -1.f)
{
	return draw_slider( label, ImGuiDataType_Float, v, &v_min, &v_max, format, width, value_box_width );
}

bool slider_int( const char* label, int* v, int v_min, int v_max, const char* format, float width = 75.f, float value_box_width = -1.f )
{
    return draw_slider(label, ImGuiDataType_S32, v, &v_min, &v_max, format, width, value_box_width);
}

bool slider_double( const char* label, double* v, double v_min, double v_max, const char* format, float width = 75.f, float value_box_width = -1.f )
{
	return draw_slider( label, ImGuiDataType_Double, v, &v_min, &v_max, format, width, value_box_width );
}
