#pragma once
#include "../instalisation fonts/fonts2.h"
#include <string>
#include <unordered_map>

bool popup_draw_slider( const char* name, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format,
                  float width = 75.f, float value_box_width = -1.f )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return false;

	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = ImGui::GetCurrentWindow( );

	char value_buf[ 64 ];
	const char* value_buf_end = value_buf + ImGui::DataTypeFormatString( value_buf, IM_ARRAYSIZE( value_buf ), data_type, p_data, format );

	const static float height{ 20.0f };
	const static float grabber_size{ 12.0f };
	const static float slide_height{ 1.0f };
	const static float rounding{ 2.0f };
	const static auto name_text_size = ImGui::CalcTextSize( name, NULL, true );
	const auto value_text_size       = ImGui::CalcTextSize( value_buf, value_buf_end, true );

	// Координаты адаптированы для popup - начинаем с левого края
	ImVec2 position = ImGui::GetCurrentWindow( )->DC.CursorPos;
	position.x += 10.0f; // Отступ от левого края popup
	
	const ImRect frame_bb( position + ImVec2( 15.0f, -2.0f ), 
	                       position + ImVec2( 15.0f + width, height - 2.0f ) );
	
	// Вычисляем позицию бокса с цифрами (правый край на расстоянии 284px от левого края чилда)
	const float box_right_edge_offset = 284.0f;
	ImVec2 child_pos = ImGui::GetCurrentWindow()->Pos;
	
	// Определяем ширину бокса на основе точных данных по размерам
	float box_width;
	if ( value_box_width > 0 ) {
		box_width = value_box_width;
	} else {
		switch ( data_type ) {
		case ImGuiDataType_Float:
			box_width = 47.0f; // Для чисел типа "0.700", "100.0"
			break;
		case ImGuiDataType_S32:
			box_width = 35.0f; // Для 3-значных чисел
			break;
		case ImGuiDataType_Double:
			box_width = 52.0f; // Для 5-значных чисел
			break;
		default:
			box_width = 35.0f; // Для 3-значных чисел по умолчанию
			break;
		}
	}
	
	// Позиция бокса с цифрами - возвращаем как было
	ImVec2 box_pos = ImVec2( 
		child_pos.x + box_right_edge_offset - box_width,
		ImGui::GetCurrentWindow( )->DC.CursorPos.y - (height - slide_height) / 2 - 2.0f + 15.0f - 6.0f - 16.0f + 13.0f + 5.0f // +5px вниз
	);
	
	// Позиция слайдера: правый край в 16px от левого края бокса, сдвигаем на 2px вниз и 8px вправо
	const float slider_end_x = box_pos.x - 16.0f + 8.0f;
	const float slider_start_x = slider_end_x - width;
	
	const ImRect total_bb( 
		ImVec2( slider_start_x, ImGui::GetCurrentWindow( )->DC.CursorPos.y + (height - slide_height) / 2 - 2.0f + 6.0f - 6.0f + 2.0f ), // +2px вниз
		ImVec2( slider_end_x, ImGui::GetCurrentWindow( )->DC.CursorPos.y + (height - slide_height) / 2 + slide_height - 2.0f + 6.0f - 6.0f + 2.0f ) // +2px вниз
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
		total_bb.Min.x + progress * total_bb.GetWidth(),
		total_bb.Min.y + total_bb.GetHeight() / 2
	);

	// Создаем область для ползунка
	ImRect grabber_bb( 
		ImVec2( grabber_center.x - grabber_size, grabber_center.y - grabber_size ),
		ImVec2( grabber_center.x + grabber_size, grabber_center.y + grabber_size )
	);

	// Расширяем область взаимодействия для всей дорожки + ползунка
	ImRect interaction_bb = total_bb;
	interaction_bb.Expand( ImVec2( 0, grabber_size ) );

	ImGui::ItemSize( frame_bb, height - 8.0f );

	const ImGuiID slider_id = ImGui::GetID( name );
	if ( !ImGui::ItemAdd( interaction_bb, slider_id ) )
		return false;

	const bool hovered = ImGui::ItemHoverable( interaction_bb, slider_id );
	const bool clicked = hovered && ImGui::IsMouseClicked( ImGuiMouseButton_::ImGuiMouseButton_Left, false );

	const bool grabber_hovered = ImGui::IsMouseHoveringRect( grabber_bb.Min, grabber_bb.Max );
	const bool grabber_clicked = grabber_hovered && ImGui::IsMouseClicked( ImGuiMouseButton_::ImGuiMouseButton_Left, false );

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

	// Фон дорожки слайдера
	ImGui::GetWindowDrawList( )->AddRectFilled( total_bb.Min, total_bb.Max, ImColor( 88, 88, 88, 255 ), rounding,
	                                            ImDrawFlags_::ImDrawFlags_RoundCornersAll );

	// Заливка пройденной части слайдера
	ImVec2 filled_end = ImVec2( total_bb.Min.x + progress * total_bb.GetWidth(), total_bb.Max.y );
	ImRect filled_bb( total_bb.Min, filled_end );
	
	if ( progress > 0.0f ) {
		ImGui::GetWindowDrawList( )->AddRectFilled( filled_bb.Min, filled_bb.Max, 
			ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 200 ) ), rounding,
			ImDrawFlags_::ImDrawFlags_RoundCornersAll );
	}

	// Название - сдвигаем на 6px влево, поднимаем на 2px выше
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::tasorbiter_addwindow, fonts_for_gui::tasorbiter_addwindow->FontSize, 
	                                      frame_bb.Min + ImVec2( -13.0f - 6.0f, 2.0f ), // -6px влево (было -10px), +2px вниз (было +4px)
	                                      ImColor( 150, 150, 150, 255 ), name );

	// Отрисовка круглого ползунка
	ImColor grabber_color = ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 255 ) );
	if ( grabber_hovered ) {
		grabber_color = ImColor( 
			std::min( 255, (int)(grabber_color.Value.x * 255 * 1.2f) ),
			std::min( 255, (int)(grabber_color.Value.y * 255 * 1.2f) ),
			std::min( 255, (int)(grabber_color.Value.z * 255 * 1.2f) ),
			255
		);
	}
	
	ImGui::GetWindowDrawList( )->AddCircleFilled( grabber_center, grabber_size / 2, grabber_color );
	
	float outline_thickness = grabber_hovered ? 2.0f : 1.0f;
	ImGui::GetWindowDrawList( )->AddCircle( grabber_center, grabber_size / 2, 
		ImColor( 255, 255, 255, grabber_hovered ? 150 : 100 ), 0, outline_thickness );

	// Бокс для отображения значения слайдера
	const float box_height = 20.0f;
	ImRect value_box( box_pos, ImVec2( box_pos.x + box_width, box_pos.y + box_height ) );
	const std::string value_box_label = std::string( name ) + "##_value";
	const ImGuiID value_box_id = ImGui::GetID( value_box_label.c_str( ) );
	bool value_box_hovered = false;
	bool value_box_held = false;
	bool value_box_pressed = false;
	if ( ImGui::ItemAdd( value_box, value_box_id ) )
		value_box_pressed = ImGui::ButtonBehavior( value_box, value_box_id, &value_box_hovered, &value_box_held );
	const bool value_box_active = value_box_pressed || ImGui::TempInputIsActive( value_box_id );
	
	// Фон бокса
	ImGui::GetWindowDrawList( )->AddRectFilled( value_box.Min, value_box.Max,
		value_box_hovered ? ImColor( 22, 22, 22, 255 ) : ImColor( 16, 16, 16, 255 ), 2.0f );
	
	// Обводка бокса
	ImGui::GetWindowDrawList( )->AddRect( value_box.Min, value_box.Max,
		value_box_hovered ? ImColor( 50, 50, 50, 255 ) : ImColor( 32, 32, 32, 255 ), 2.0f, 0, 1.0f );

	if ( value_box_active ) {
		if ( ImGui::TempInputScalar( value_box, value_box_id, value_box_label.c_str( ), data_type, p_data, format, p_min, p_max ) )
			ImGui::MarkItemEdited( value_box_id );
		return value_changed || ImGui::IsItemEdited( );
	}
	
	// Текст значения в боксе - сдвигаем левее если есть процент
	ImVec2 text_pos = ImVec2( 
		value_box.Min.x + 8.0f,
		value_box.Min.y + (box_height - value_text_size.y) / 2 
	);
	
	// Проверяем есть ли процент в формате и сдвигаем левее
	bool has_percentage = (strstr(format, "%") != nullptr);
	if (has_percentage) {
		text_pos.x -= 3.0f; // Сдвигаем на 3px левее для процентов
	}
	
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::tasorbiter_addwindow, fonts_for_gui::tasorbiter_addwindow->FontSize,
		text_pos, ImColor( 150, 150, 150, 255 ), value_buf, value_buf_end );

	return value_changed;
}

bool popup_slider_float(const char* label, float* v, float v_min, float v_max, const char* format, float width = 75.f, float value_box_width = -1.f)
{
	return popup_draw_slider( label, ImGuiDataType_Float, v, &v_min, &v_max, format, width, value_box_width );
}

bool popup_slider_int( const char* label, int* v, int v_min, int v_max, const char* format, float width = 75.f, float value_box_width = -1.f )
{
    return popup_draw_slider(label, ImGuiDataType_S32, v, &v_min, &v_max, format, width, value_box_width);
}

bool popup_slider_double( const char* label, double* v, double v_min, double v_max, const char* format, float width = 75.f, float value_box_width = -1.f )
{
	return popup_draw_slider( label, ImGuiDataType_Double, v, &v_min, &v_max, format, width, value_box_width );
}
