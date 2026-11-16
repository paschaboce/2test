#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/imgui_internal.h"
#include "animations.h"
#include "../globals/config/variables.h"
#include <string>

static float combo_dpi_scale( )
{
	const int dpi = GET_VARIABLE( g_variables.m_menu_dpi_scale, int );
	return static_cast< float >( dpi < 75 ? 75 : dpi > 200 ? 200 : dpi ) / 100.0f;
}

// Константа для отступа текста в выпадающих списках
constexpr float COMBO_TEXT_OFFSET_Y = 3.0f;

static float CalcMaxPopupHeightFromItemCount(int items_count)
{
	ImGuiContext& g = *GImGui;
	if (items_count <= 0)
		return FLT_MAX;
	return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2) + 20.0f; // Добавляем 20px снизу
}

bool header_box( const char* name, const char* preview_value, int items_count, ImVec2 sz )
{
	ImGuiContext& g = *GImGui;
	const float scale = combo_dpi_scale( );
	sz = sz * scale;

	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return false;

	const auto name_text_size = ImGui::CalcTextSize( name, NULL, true );
	const float height{ sz.y };
	const float rounding{ 4.0f * scale };
	const float thickness{ 1.0f * scale };
	const float padding{ 5.0f * scale };
	const float spacing{ 5.0f * scale };
	
	// �спользуем переданную ширину комбо бокса
	const float combo_width = sz.x;
	const float combo_x = ImGui::GetCurrentWindow( )->DC.CursorPos.x;
	const float combo_y = ImGui::GetCurrentWindow( )->DC.CursorPos.y - 5.0f * scale; // Поднимаем на 5 пикселей выше
	
	// Проверяем, что комбо бокс имеет разумные размеры
	if ( combo_width <= 0 || height <= 0 )
		return false;
	
	const ImRect frame_bb( ImVec2( combo_x, combo_y ), 
	                      ImVec2( combo_x + combo_width, combo_y + height ) );
	const ImRect total_bb( ImVec2( combo_x, combo_y + name_text_size.y + 5.0f * scale ), frame_bb.Max );

	ImGui::ItemSize( frame_bb, height );

	if ( !ImGui::ItemAdd( frame_bb, ImGui::GetID( name ) ) )
		return false;

	bool hovered, held;
	bool pressed    = ImGui::ButtonBehavior( total_bb, ImGui::GetID( name ), &hovered, &held );
	bool popup_open = ImGui::IsPopupOpen( ImGui::GetID( name ), ImGuiPopupFlags_::ImGuiPopupFlags_None );

	ImGui::GetWindowDrawList( )->PushClipRect( total_bb.Min, total_bb.Max );
	
	// Фон комбо бокса (без изменений)
	ImColor final_bg_color = ImColor( 16, 16, 16, int( 255 * g_menu.m_animation_progress ) );
	
	ImGui::GetWindowDrawList( )->AddRectFilled( total_bb.Min, total_bb.Max, final_bg_color, rounding,
	                                            ImDrawFlags_::ImDrawFlags_RoundCornersAll );

	// Анимированная обводка (ярче при наведении, акцентный при открытии)
	ImColor base_outline_color = ImColor( 33, 33, 33, int( 255 * g_menu.m_animation_progress ) );
	ImColor hover_outline_color = ImColor( 60, 60, 60, int( 255 * g_menu.m_animation_progress ) );
	ImColor accent_outline_color = ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) );
	
	ImColor final_outline_color;
	if ( popup_open ) {
		final_outline_color = accent_outline_color;
	} else {
		final_outline_color = smoothLerpColor( base_outline_color, hover_outline_color, hovered ? 1.0f : 0.0f );
	}
	
	float animated_thickness = thickness + ( popup_open ? 0.5f : 0.0f );
	ImGui::GetWindowDrawList( )->AddRect(
		total_bb.Min, total_bb.Max,
		final_outline_color, rounding,
		ImDrawFlags_::ImDrawFlags_RoundCornersAll, animated_thickness );

	// Анимированный текст (без hover эффекта)
	ImColor base_text_color = ImColor( 150, 150, 150, int( 255 * g_menu.m_animation_progress ) );
	ImColor final_text_color = base_text_color;
	
	ImVec2 text_pos = total_bb.Min + ImVec2( 5.0f, 3.0f ) * scale;
	
	// Вычисляем доступную ширину для текста (оставляем место для стрелочки)
	float available_width = total_bb.GetWidth() - 30.0f * scale; // 30px для стрелочки и отступов
	
	// Получаем размер текста
	ImVec2 text_size = ImGui::CalcTextSize( preview_value );
	
	// Если текст помещается, рисуем его обычным способом
	if ( text_size.x <= available_width ) {
		ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::tasorbiter_addwindow, fonts_for_gui::tasorbiter_addwindow->FontSize * scale,
			text_pos, final_text_color, preview_value );
	} else {
		// Если текст не помещается, рисуем с градиентным затуханием
		ImVec2 text_end_pos = text_pos + ImVec2( available_width, text_size.y );
		
		// Рисуем текст с маской для градиентного затухания
		ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::tasorbiter_addwindow, fonts_for_gui::tasorbiter_addwindow->FontSize * scale,
			text_pos, final_text_color, preview_value );
		
		// Добавляем градиентную маску для плавного затухания
		ImVec2 gradient_start = text_pos + ImVec2( available_width - 20.0f * scale, 0.0f );
		ImVec2 gradient_end = text_end_pos;
		
		ImGui::GetWindowDrawList( )->AddRectFilledMultiColor(
			gradient_start, gradient_end,
			ImColor( 0, 0, 0, 0 ), // Прозрачный слева
			final_bg_color,        // Цвет фона справа
			final_bg_color,        // Цвет фона справа
			ImColor( 0, 0, 0, 0 )  // Прозрачный слева
		);
	}
	
	// Добавляем анимированную стрелочку
	static std::unordered_map< ImGuiID, float > arrow_rotation;
	ImGuiID combo_id = ImGui::GetID( name );
	if ( arrow_rotation.find( combo_id ) == arrow_rotation.end( ) )
		arrow_rotation[ combo_id ] = 0.0f;
	
	float dt = ImGui::GetIO( ).DeltaTime;
	float target_rotation = popup_open ? 180.0f : 0.0f;
	arrow_rotation[ combo_id ] = smoothLerp( arrow_rotation[ combo_id ], target_rotation, dt * 8.0f );
	
	// Позиция стрелочки в правой части комбобокса
	ImVec2 arrow_center = ImVec2( total_bb.Max.x - 15.0f * scale, total_bb.Min.y + total_bb.GetHeight() / 2.0f );
	
	// Цвет стрелочки (без hover эффекта)
	ImColor base_arrow_color = ImColor( 150, 150, 150, int( 255 * g_menu.m_animation_progress ) );
	ImColor final_arrow_color = base_arrow_color;
	
	// Рисуем стрелочку - точные параметры для симметрии
	float line_thickness = 1.0f * scale; // Толщина линий
	float arrow_size = 5.0f * scale; // Размер стрелочки
	float arrow_angle = 70.0f; // Угол между линиями (шире)
	
	// Рисуем стрелочку с правильным поворотом
	float angle_rad = arrow_rotation[ combo_id ] * 3.14159f / 180.0f;
	
	// Создаем идеально симметричную стрелочку с тригонометрией
	float half_angle_rad = (arrow_angle * 0.5f) * 3.14159f / 180.0f;
	float left_angle = -half_angle_rad;
	float right_angle = half_angle_rad;
	
	ImVec2 p1, p2, p3;
	
	// В зависимости от угла поворота, рисуем стрелочку в нужном направлении
	if ( arrow_rotation[ combo_id ] < 90.0f ) {
		// Стрелочка направлена вниз - идеально симметрично
		p1 = ImVec2( arrow_center.x, arrow_center.y - arrow_size * 0.5f );
		p2 = ImVec2( 
			arrow_center.x + arrow_size * 0.5f * sinf(left_angle), 
			arrow_center.y + arrow_size * 0.5f * cosf(left_angle) 
		);
		p3 = ImVec2( 
			arrow_center.x + arrow_size * 0.5f * sinf(right_angle), 
			arrow_center.y + arrow_size * 0.5f * cosf(right_angle) 
		);
	} else {
		// Стрелочка направлена вверх - идеально симметрично
		p1 = ImVec2( arrow_center.x, arrow_center.y + arrow_size * 0.5f );
		p2 = ImVec2( 
			arrow_center.x + arrow_size * 0.5f * sinf(left_angle), 
			arrow_center.y - arrow_size * 0.5f * cosf(left_angle) 
		);
		p3 = ImVec2( 
			arrow_center.x + arrow_size * 0.5f * sinf(right_angle), 
			arrow_center.y - arrow_size * 0.5f * cosf(right_angle) 
		);
	}
	
	// Рисуем стрелочку как две линии, образующие "V"
	ImGui::GetWindowDrawList( )->AddLine( p1, p2, final_arrow_color, line_thickness );
	ImGui::GetWindowDrawList( )->AddLine( p1, p3, final_arrow_color, line_thickness );
	
	ImGui::GetWindowDrawList( )->PopClipRect( );
	// Не отображаем название здесь, так как оно уже отображено в функции combo

	if ( ( pressed || g.NavActivateId == ImGui::GetID( name ) ) && !popup_open ) {
		if ( ImGui::GetCurrentWindow( )->DC.NavLayerCurrent == 0 )
			ImGui::GetCurrentWindow( )->NavLastIds[ 0 ] = ImGui::GetID( name );

		ImGui::OpenPopupEx( ImGui::GetID( name ), ImGuiPopupFlags_::ImGuiPopupFlags_None );
		popup_open = true;
	}

	// Если popup не открыт, выходим, больше ничего не делая.
	if ( !popup_open )
		return false;

	ImGui::SetNextWindowPos( total_bb.Min + ImVec2( 0, ( height / 2.0f ) + spacing + 8.0f * scale ) );

	// --- Улучшенная анимация плавного открытия ---
	static float popup_anim_start = -1.0f;
	if ( popup_open && popup_anim_start < 0.0f )
		popup_anim_start = ImGui::GetTime( );
	if ( !popup_open )
		popup_anim_start = -1.0f;

	float anim_duration = 0.3f; // Увеличиваем продолжительность анимации до 300 мс
	float anim_progress = 1.0f;
	if ( popup_open ) {
		float elapsed = ImGui::GetTime( ) - popup_anim_start;
		anim_progress = elapsed < anim_duration ? elapsed / anim_duration : 1.0f;
		// Применяем easing для более плавной анимации
		anim_progress = static_cast<float>( Easing::OutCubic( anim_progress ) );
	}

	// Расчёт полной высоты popup-а на основе количества элементов.
	float full_popup_height = CalcMaxPopupHeightFromItemCount( items_count );
	// Ограничиваем максимальную высоту до 300.f, чтобы появился scrollbar.
	const float max_popup_height = 300.f * scale;
	float constrained_height = full_popup_height > max_popup_height ? max_popup_height : full_popup_height;
	// Анимированное значение высоты - плавное увеличение от 0 до итоговой ограниченной высоты.
	float animated_height = constrained_height * anim_progress;

	// Устанавливаем ограничения размера окна с учётом анимации и максимальной высоты.
	ImGui::SetNextWindowSizeConstraints( ImVec2( combo_width, animated_height ), ImVec2( FLT_MAX, max_popup_height ) );
	// --- Конец изменений для анимации и ограничения высоты ---

	// Флаги окна popup-а.
	ImGuiWindowFlags flags = ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar |
	                         ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
	                         ImGuiWindowFlags_NoMove;

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( padding, padding ) );

	bool call_once = ImGui::BeginPopup( name, flags );

	if ( call_once ) {
		ImGui::PushClipRect( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ), false );
		// Фон popup: 13, 13, 13 с обводкой 10, 10, 10
		ImGui::GetWindowDrawList( )->AddRectFilled( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ),
		                                            ImColor( 13, 13, 13, int( 255 * g_menu.m_animation_progress ) ), rounding,
		                                            ImDrawFlags_RoundCornersAll );
		// Обводка popup
		ImGui::GetWindowDrawList( )->AddRect( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ),
		                                      ImColor( 10, 10, 10, int( 255 * g_menu.m_animation_progress ) ), rounding,
		                                      ImDrawFlags_RoundCornersAll, thickness );
		ImGui::PopClipRect( );
		
		// Опускаем popup на 3px вниз (было -2, стало +1)
		ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 1.0f );
	} else {
		ImGui::EndPopup( );
		return false;
	}

	ImGui::PopStyleVar( );

	return true;
}


void combo(const char* label, int& selected_index, const char* items[], int items_count,ImVec2 sz = ImVec2(180,39))
{
	// Получаем размер текста для правильного позиционирования
	const auto label_text_size = ImGui::CalcTextSize( label, NULL, true );
	
	// Отображаем название слева серым цветом с отступом 2px вправо
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 2.0f );
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(150.0f/255.0f, 150.0f/255.0f, 150.0f/255.0f, 1.0f));
	ImGui::Text( label );
	ImGui::PopStyleColor();
	
	// Перемещаемся на ту же строку и позиционируемся после текста + 9px (3+6)
	ImGui::SameLine();
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 9.0f );
	
	// Поднимаем комбо бокс на 15 пикселей выше
	ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 15.0f );
	
	// Вычисляем ширину комбо бокса: от текущей позиции до конца child - 12px
	const float combo_width = ImGui::GetContentRegionAvail().x - 12.0f;

	// --- Mouse-wheel scrolling on the collapsed combo ---
	// Compute the same frame rect as header_box() to hit-test hover.
	{
		const ImVec2 frame_min = ImGui::GetCurrentWindow( )->DC.CursorPos + ImVec2( 0.0f, -5.0f );
		const ImVec2 frame_max = frame_min + ImVec2( combo_width, sz.y );
		const bool popup_already_open = ImGui::IsPopupOpen( ImGui::GetID( label ), ImGuiPopupFlags_None );
		if ( !popup_already_open && items_count > 0 && ImGui::IsMouseHoveringRect( frame_min, frame_max ) ) {
			const float wheel = ImGui::GetIO( ).MouseWheel;
			if ( wheel != 0.0f ) {
				const int delta = ( wheel > 0.0f ) ? -1 : 1;
				selected_index  = ( selected_index + delta + items_count ) % items_count;
				// Consume the wheel so parent scrollable containers don't also scroll.
				ImGui::GetIO( ).MouseWheel = 0.0f;
			}
		}
	}

	// later do a custom text selector
	if ( header_box( label, items[ selected_index ], items_count, ImVec2( combo_width, sz.y ) ) ) {
		for (int i = 0; i < items_count; i++) {
			// Устанавливаем отступ 25px для каждого элемента
			ImGui::SetCursorPosX( 25.0f );
			
			// Получаем позицию до Selectable
			ImVec2 item_start_pos = ImGui::GetCursorScreenPos();
			
			// Получаем размер элемента
			ImVec2 item_size = ImVec2( ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y );
			
			// Проверяем наведение
			bool item_hovered = ImGui::IsMouseHoveringRect( 
				item_start_pos, 
				item_start_pos + item_size 
			);
			
			// Анимированный hover эффект
			static std::unordered_map< int, float > item_hover_anim;
			if ( item_hover_anim.find( i ) == item_hover_anim.end( ) )
				item_hover_anim[ i ] = 0.0f;
			
			float dt = ImGui::GetIO( ).DeltaTime;
			item_hover_anim[ i ] = smoothLerp( item_hover_anim[ i ], item_hovered ? 1.0f : 0.0f, dt * 10.0f );
			
			if ( item_hover_anim[ i ] > 0.0f ) {
				ImVec2 hover_start = item_start_pos + ImVec2( -15.0f, 0.0f );
				ImVec2 hover_end = item_start_pos + item_size - ImVec2( 5.0f, 0.0f );
				
				ImColor base_hover_color = ImColor( 21, 21, 21, 0 );
				ImColor full_hover_color = ImColor( 21, 21, 21, int( 255 * g_menu.m_animation_progress ) );
				ImColor final_hover_color = smoothLerpColor( base_hover_color, full_hover_color, item_hover_anim[ i ] );
				
				ImGui::GetWindowDrawList()->AddRectFilled(
					hover_start,
					hover_end,
					final_hover_color,
					4.0f, // Закругление
					ImDrawFlags_RoundCornersAll
				);
			}
			
			// Добавляем отступ для текста
			ImGui::SetCursorPosY( ImGui::GetCursorPosY() + COMBO_TEXT_OFFSET_Y );
			
			ImGui::PushStyleColor( ImGuiCol_Text,
			                       ( selected_index == i )
			                           ? ImVec4( 255, 255, 255, float( 1.f * g_menu.m_animation_progress ) )
								   : ImVec4( 150, 150, 150, float( 1.f * g_menu.m_animation_progress ) ) );
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));

			bool selected = (selected_index == i);
			if (ImGui::Selectable(items[i], selected)) {
				selected_index = i;
			}
			
			// Рисуем закругленную точку для активного элемента (левее на 21px)
			if ( selected ) {
				ImVec2 dot_pos = item_start_pos + ImVec2( -8.0f, item_size.y / 2.0f - 2.0f ); // Позиция точки правее
				ImGui::GetWindowDrawList()->AddCircleFilled(
					dot_pos + ImVec2( 2.0f, 2.0f ), // Центр круга
					3.0f, // Радиус
					ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) ) // Акцентный цвет
				);
			}

			ImGui::PopStyleColor(4);
		}
		ImGui::EndCombo();
	}
}

void multi_combo( const char* label, std::vector< bool >& combos, const char* items[], int items_count, ImVec2 sz = ImVec2( 180, 39 ) )
{
	std::vector< std::string > selected;

	for ( int i = 0; i < items_count; i++ )
		if ( combos[ i ] )
			selected.push_back( items[ i ] );

	std::string preview = selected.empty( ) ? "none" : selected[ 0 ];

	for ( int i = 1; i < selected.size( ); i++ )
		preview += ", " + selected[ i ];

	// Отображаем название слева серым цветом с отступом 2px вправо
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 2.0f );
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(150.0f/255.0f, 150.0f/255.0f, 150.0f/255.0f, 1.0f));
	ImGui::Text( label );
	ImGui::PopStyleColor();
	
	// Перемещаемся на ту же строку и позиционируемся после текста + 9px (3+6)
	ImGui::SameLine();
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 9.0f );
	
	// Поднимаем комбо бокс на 15 пикселей выше
	ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 15.0f );
	
	// Вычисляем ширину комбо бокса: от текущей позиции до конца child - 12px
	const float combo_width = ImGui::GetContentRegionAvail().x - 12.0f;

	// later do a custom text selector
	if ( header_box( label, preview.c_str( ), items_count, ImVec2( combo_width, sz.y ) ) ) {
		for ( int i = 0; i < items_count; i++ ) {
			// Устанавливаем отступ 25px для каждого элемента
			ImGui::SetCursorPosX( 25.0f );
			
			// Получаем позицию до Selectable
			ImVec2 item_start_pos = ImGui::GetCursorScreenPos();
			
			// Получаем размер элемента
			ImVec2 item_size = ImVec2( ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y );
			
			// Проверяем наведение
			bool item_hovered = ImGui::IsMouseHoveringRect( 
				item_start_pos, 
				item_start_pos + item_size 
			);
			
			// Анимированный hover эффект
			static std::unordered_map< int, float > item_hover_anim;
			if ( item_hover_anim.find( i ) == item_hover_anim.end( ) )
				item_hover_anim[ i ] = 0.0f;
			
			float dt = ImGui::GetIO( ).DeltaTime;
			item_hover_anim[ i ] = smoothLerp( item_hover_anim[ i ], item_hovered ? 1.0f : 0.0f, dt * 10.0f );
			
			if ( item_hover_anim[ i ] > 0.0f ) {
				ImVec2 hover_start = item_start_pos + ImVec2( -15.0f, 0.0f );
				ImVec2 hover_end = item_start_pos + item_size - ImVec2( 5.0f, 0.0f );
				
				ImColor base_hover_color = ImColor( 21, 21, 21, 0 );
				ImColor full_hover_color = ImColor( 21, 21, 21, int( 255 * g_menu.m_animation_progress ) );
				ImColor final_hover_color = smoothLerpColor( base_hover_color, full_hover_color, item_hover_anim[ i ] );
				
				ImGui::GetWindowDrawList()->AddRectFilled(
					hover_start,
					hover_end,
					final_hover_color,
					4.0f, // Закругление
					ImDrawFlags_RoundCornersAll
				);
			}
			
			// Добавляем отступ для текста
			ImGui::SetCursorPosY( ImGui::GetCursorPosY() + COMBO_TEXT_OFFSET_Y );
			
			ImGui::PushStyleColor( ImGuiCol_Text,
			                       combos[ i ]
			                           ? ImVec4( 255, 255, 255, float( 1.f * g_menu.m_animation_progress ) )
			                                                  : ImVec4( 150, 150, 150, float( 1.f * g_menu.m_animation_progress ) ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderActive, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_Header, ImVec4( 0, 0, 0, 0 ) );

			// Use a temporary variable to handle the proxy object issue
			bool temp = combos[ i ];
			if ( ImGui::Selectable( items[ i ], &temp, ImGuiSelectableFlags_DontClosePopups ) )
				combos[ i ] = temp;
			
			// Рисуем закругленную точку для активного элемента (левее на 21px)
			if ( combos[ i ] ) {
				ImVec2 dot_pos = item_start_pos + ImVec2( -8.0f, item_size.y / 2.0f - 2.0f ); // Позиция точки правее
				ImGui::GetWindowDrawList()->AddCircleFilled(
					dot_pos + ImVec2( 2.0f, 2.0f ), // Центр круга
					3.0f, // Радиус
					ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) ) // Акцентный цвет
				);
			}

			ImGui::PopStyleColor( 4 );
		}
		ImGui::EndCombo( );
	}
}
void multi_combo( const char* label, bool combos[], const char* items[], int items_count, ImVec2 sz = ImVec2( 180, 39 ) )
{
	std::vector< std::string > selected;

	for ( int i = 0; i < items_count; i++ )
		if ( combos[ i ] )
			selected.push_back( items[ i ] );

	std::string preview = selected.empty( ) ? "none" : selected[ 0 ];

	for ( int i = 1; i < selected.size( ); i++ )
		preview += ", " + selected[ i ];

	// Отображаем название слева серым цветом с отступом 2px вправо
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 2.0f );
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(150.0f/255.0f, 150.0f/255.0f, 150.0f/255.0f, 1.0f));
	ImGui::Text( label );
	ImGui::PopStyleColor();
	
	// Перемещаемся на ту же строку и позиционируемся после текста + 9px (3+6)
	ImGui::SameLine();
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 9.0f );
	
	// Поднимаем комбо бокс на 15 пикселей выше
	ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 15.0f );
	
	// Вычисляем ширину комбо бокса: от текущей позиции до конца child - 12px
	const float combo_width = ImGui::GetContentRegionAvail().x - 12.0f;

	// later do a custom text selector
	if ( header_box( label, preview.c_str( ), items_count, ImVec2( combo_width, sz.y ) ) ) {
		for ( int i = 0; i < items_count; i++ ) {
			// Устанавливаем отступ 25px для каждого элемента
			ImGui::SetCursorPosX( 25.0f );
			
			// Получаем позицию до Selectable
			ImVec2 item_start_pos = ImGui::GetCursorScreenPos();
			
			// Получаем размер элемента
			ImVec2 item_size = ImVec2( ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y );
			
			// Проверяем наведение
			bool item_hovered = ImGui::IsMouseHoveringRect( 
				item_start_pos, 
				item_start_pos + item_size 
			);
			
			// Анимированный hover эффект
			static std::unordered_map< int, float > item_hover_anim;
			if ( item_hover_anim.find( i ) == item_hover_anim.end( ) )
				item_hover_anim[ i ] = 0.0f;
			
			float dt = ImGui::GetIO( ).DeltaTime;
			item_hover_anim[ i ] = smoothLerp( item_hover_anim[ i ], item_hovered ? 1.0f : 0.0f, dt * 10.0f );
			
			if ( item_hover_anim[ i ] > 0.0f ) {
				ImVec2 hover_start = item_start_pos + ImVec2( -15.0f, 0.0f );
				ImVec2 hover_end = item_start_pos + item_size - ImVec2( 5.0f, 0.0f );
				
				ImColor base_hover_color = ImColor( 21, 21, 21, 0 );
				ImColor full_hover_color = ImColor( 21, 21, 21, int( 255 * g_menu.m_animation_progress ) );
				ImColor final_hover_color = smoothLerpColor( base_hover_color, full_hover_color, item_hover_anim[ i ] );
				
				ImGui::GetWindowDrawList()->AddRectFilled(
					hover_start,
					hover_end,
					final_hover_color,
					4.0f, // Закругление
					ImDrawFlags_RoundCornersAll
				);
			}
			
			// Добавляем отступ для текста
			ImGui::SetCursorPosY( ImGui::GetCursorPosY() + COMBO_TEXT_OFFSET_Y );
			
			ImGui::PushStyleColor( ImGuiCol_Text,
			                       combos[ i ] ? ImVec4( 255, 255, 255, float( 1.f * g_menu.m_animation_progress ) ) : ImVec4( 150, 150, 150, float( 1.f * g_menu.m_animation_progress ) ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderActive, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_Header, ImVec4( 0, 0, 0, 0 ) );

			// Use a temporary variable to handle the proxy object issue
			bool temp = combos[ i ];
			if ( ImGui::Selectable( items[ i ], &temp, ImGuiSelectableFlags_DontClosePopups ) )
				combos[ i ] = temp;
			
			// Рисуем закругленную точку для активного элемента (левее на 21px)
			if ( combos[ i ] ) {
				ImVec2 dot_pos = item_start_pos + ImVec2( -8.0f, item_size.y / 2.0f - 2.0f ); // Позиция точки правее
				ImGui::GetWindowDrawList()->AddCircleFilled(
					dot_pos + ImVec2( 2.0f, 2.0f ), // Центр круга
					3.0f, // Радиус
					ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) ) // Акцентный цвет
				);
			}

			ImGui::PopStyleColor( 4 );
		}
		ImGui::EndCombo( );
	}
}
