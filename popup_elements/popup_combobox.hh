#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/imgui_internal.h"
#include <string>

// Константа для отступа текста в выпадающих списках
constexpr float POPUP_COMBO_TEXT_OFFSET_Y = 3.0f;

static float popup_CalcMaxPopupHeightFromItemCount(int items_count)
{
	ImGuiContext& g = *GImGui;
	if (items_count <= 0)
		return FLT_MAX;
	return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2) + 5.0f; // Добавляем 5px снизу (было 20px)
}

bool popup_header_box( const char* name, const char* preview_value, int items_count, ImVec2 sz )
{
	ImGuiContext& g = *GImGui;

	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return false;

	const auto name_text_size = ImGui::CalcTextSize( name, NULL, true );
	const float height{ sz.y };
	const float rounding{ 4.0f };
	const float thickness{ 1.0f };
	const float padding{ 5.0f };
	const float spacing{ 5.0f };
	
	// �спользуем переданную ширину комбо бокса
	const float combo_width = sz.x;
	// Координаты адаптированы для popup - начинаем с левого края
	const float combo_x = ImGui::GetCurrentWindow( )->DC.CursorPos.x + 4.0f; // Отступ от левого края popup (-6px)
	const float combo_y = ImGui::GetCurrentWindow( )->DC.CursorPos.y - 5.0f; // Поднимаем на 5 пикселей выше
	
	// Проверяем, что комбо бокс имеет разумные размеры
	if ( combo_width <= 0 || height <= 0 )
		return false;
	
	const ImRect frame_bb( ImVec2( combo_x, combo_y ), 
	                      ImVec2( combo_x + combo_width, combo_y + height ) );
	const ImRect total_bb( ImVec2( combo_x, combo_y + name_text_size.y + 5.0f ), frame_bb.Max );

	ImGui::ItemSize( frame_bb, height );

	if ( !ImGui::ItemAdd( frame_bb, ImGui::GetID( name ) ) )
		return false;

	bool hovered, held;
	bool pressed    = ImGui::ButtonBehavior( total_bb, ImGui::GetID( name ), &hovered, &held );
	bool popup_open = ImGui::IsPopupOpen( ImGui::GetID( name ), ImGuiPopupFlags_::ImGuiPopupFlags_None );

	ImGui::GetWindowDrawList( )->PushClipRect( total_bb.Min, total_bb.Max );
	
	// Фон комбо бокса - цвет 16, 16, 16
	ImGui::GetWindowDrawList( )->AddRectFilled( total_bb.Min, total_bb.Max, ImColor( 16, 16, 16, int( 255 * g_menu.m_animation_progress ) ), rounding,
	                                            ImDrawFlags_::ImDrawFlags_RoundCornersAll );

	// Обводка: акцент цвет при наведении/открыт, 33,33,33 когда не наведен
	if ( popup_open || hovered )
		ImGui::GetWindowDrawList( )->AddRect(
			total_bb.Min, total_bb.Max,
			ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) ), rounding,
			ImDrawFlags_::ImDrawFlags_RoundCornersAll, thickness );
	else
		ImGui::GetWindowDrawList( )->AddRect(
			total_bb.Min, total_bb.Max,
			ImColor( 33, 33, 33, int( 255 * g_menu.m_animation_progress ) ), rounding,
			ImDrawFlags_::ImDrawFlags_RoundCornersAll, thickness );

	// Текст: 150,150,150 когда не наведен, 255,255,255 при наведении
	ImColor text_color = hovered ? ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ) : 
	                               ImColor( 150, 150, 150, int( 255 * g_menu.m_animation_progress ) );
	
	// Позиция текста внутри комбо бокса
	ImGui::GetWindowDrawList( )->AddText( total_bb.Min + ImVec2( 5.0f, 3.0f ), text_color, preview_value );
	ImGui::GetWindowDrawList( )->PopClipRect( );

	if ( ( pressed || g.NavActivateId == ImGui::GetID( name ) ) && !popup_open ) {
		if ( ImGui::GetCurrentWindow( )->DC.NavLayerCurrent == 0 )
			ImGui::GetCurrentWindow( )->NavLastIds[ 0 ] = ImGui::GetID( name );

		ImGui::OpenPopupEx( ImGui::GetID( name ), ImGuiPopupFlags_::ImGuiPopupFlags_None );
		popup_open = true;
	}

	// Если popup не открыт, выходим, больше ничего не делая.
	if ( !popup_open )
		return false;

	ImGui::SetNextWindowPos( total_bb.Min + ImVec2( 0, ( height / 2.0f ) + spacing + 8.0f ) );

	// --- Анимация плавного открытия ---
	// �спользуем статическую переменную для хранения времени начала анимации.
	static float popup_anim_start = -1.0f;
	if ( popup_open && popup_anim_start < 0.0f )
		popup_anim_start = ImGui::GetTime( );
	if ( !popup_open )
		popup_anim_start = -1.0f;

	float anim_duration = 0.25f; // продолжительность анимации 250 мс
	float anim_progress = 1.0f;
	if ( popup_open ) {
		float elapsed = ImGui::GetTime( ) - popup_anim_start;
		anim_progress = elapsed < anim_duration ? elapsed / anim_duration : 1.0f;
	}

	// Расчёт полной высоты popup-а на основе количества элементов.
	float full_popup_height = popup_CalcMaxPopupHeightFromItemCount( items_count );
	// Ограничиваем максимальную высоту до 300.f, чтобы появился scrollbar.
	float constrained_height = full_popup_height > 300.f ? 300.f : full_popup_height;
	// Анимированное значение высоты - плавное увеличение от 0 до итоговой ограниченной высоты.
	float animated_height = constrained_height * anim_progress;

	// Устанавливаем ограничения размера окна с учётом анимации и максимальной высоты.
	ImGui::SetNextWindowSizeConstraints( ImVec2( combo_width, animated_height ), ImVec2( FLT_MAX, 300.f ) );

	// Флаги окна popup-а. Включаем скролл, когда список не помещается (много элементов).
	bool use_scroll = ( full_popup_height > 300.f );
	if ( use_scroll )
		ImGui::SetNextWindowSize( ImVec2( combo_width, 300.f ) );
	ImGuiWindowFlags flags = ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar |
	                         ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
	                         ImGuiWindowFlags_NoMove;
	if ( !use_scroll ) {
		
		
	}

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( padding, padding ) );

	bool call_once = ImGui::BeginPopup( name, flags );

	if ( call_once ) {
		ImGui::PushClipRect( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ), false );
		// Фон popup: 13, 13, 13 с обводкой 10, 10, 10 (используем закругления как в обычном комбобоксе)
		ImGui::GetWindowDrawList( )->AddRectFilled( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ),
		                                            ImColor( 13, 13, 13, int( 255 * g_menu.m_animation_progress ) ), 4.0f,
		                                            ImDrawFlags_RoundCornersAll );
		// Обводка popup
		ImGui::GetWindowDrawList( )->AddRect( ImGui::GetWindowPos( ), ImGui::GetWindowPos( ) + ImGui::GetWindowSize( ),
		                                      ImColor( 10, 10, 10, int( 255 * g_menu.m_animation_progress ) ), 4.0f,
		                                      ImDrawFlags_RoundCornersAll, thickness );
		ImGui::PopClipRect( );
		
		// Опускаем popup на 3px вниз (было -5, стало -2)
		ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 2.0f );
	} else {
		ImGui::EndPopup( );
		return false;
	}

	ImGui::PopStyleVar( );

	return true;
}


void popup_combo(const char* label, int& selected_index, const char* items[], int items_count,ImVec2 sz = ImVec2(180,39))
{
	if ( items_count <= 0 )
		return;

	selected_index = std::clamp( selected_index, 0, items_count - 1 );

	// Отображаем название слева серым цветом с отступом 6px вправо
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 6.0f );
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(150.0f/255.0f, 150.0f/255.0f, 150.0f/255.0f, 1.0f));
	ImGui::Text( label );
	ImGui::PopStyleColor();
	
	// Перемещаемся на ту же строку и позиционируемся после текста + 3px
	ImGui::SameLine();
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 3.0f );
	
	// Поднимаем комбо бокс на 15 пикселей выше
	ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 15.0f );
	
	// Вычисляем ширину комбо бокса: от текущей позиции до конца popup - 12px
	const float combo_width = ImGui::GetContentRegionAvail().x - 12.0f;

	{
		const ImVec2 frame_min = ImGui::GetCurrentWindow( )->DC.CursorPos + ImVec2( 4.0f, -5.0f );
		const ImVec2 frame_max = frame_min + ImVec2( combo_width, sz.y );
		const bool popup_already_open = ImGui::IsPopupOpen( ImGui::GetID( label ), ImGuiPopupFlags_None );
		if ( !popup_already_open && ImGui::IsMouseHoveringRect( frame_min, frame_max ) ) {
			const float wheel = ImGui::GetIO( ).MouseWheel;
			if ( wheel != 0.0f ) {
				const int delta = wheel > 0.0f ? -1 : 1;
				selected_index = ( selected_index + delta + items_count ) % items_count;
				ImGui::GetIO( ).MouseWheel = 0.0f;
			}
		}
	}
	
	// later do a custom text selector
	if ( popup_header_box( label, items[ selected_index ], items_count, ImVec2( combo_width, sz.y ) ) ) {
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
			
			// Рисуем hover эффект ПОД текстом, начинается левее для покрытия точки
			if ( item_hovered ) {
				ImVec2 hover_start = item_start_pos + ImVec2( -15.0f, 0.0f ); // Начинается левее
				ImVec2 hover_end = item_start_pos + item_size - ImVec2( 5.0f, 0.0f );
				ImGui::GetWindowDrawList()->AddRectFilled(
					hover_start,
					hover_end,
					ImColor( 21, 21, 21, int( 255 * g_menu.m_animation_progress ) ),
					0.0f
				);
			}
			
			// Добавляем отступ для текста
			ImGui::SetCursorPosY( ImGui::GetCursorPosY() + POPUP_COMBO_TEXT_OFFSET_Y );
			
			ImGui::PushStyleColor( ImGuiCol_Text,
			                       ( selected_index == i )
			                           ? ImVec4( GET_VARIABLE( g_variables.m_accent, c_color ).get_vec4( float( 1.f * g_menu.m_animation_progress ) ) )
								   : ImVec4( 255, 255, 255, float( 1.f * g_menu.m_animation_progress ) ) );
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));

			bool selected = (selected_index == i);
			if (ImGui::Selectable(items[i], selected)) {
				selected_index = i;
			}
			
			// Рисуем закругленную точку для активного элемента (левее на 21px)
			if ( selected ) {
				ImVec2 dot_pos = item_start_pos + ImVec2( -11.0f, item_size.y / 2.0f - 2.0f ); // Позиция точки без отступа
				ImGui::GetWindowDrawList()->AddCircleFilled(
					dot_pos + ImVec2( 2.0f, 2.0f ), // Центр круга
					3.0f, // Радиус
					ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) )
				);
			}

			ImGui::PopStyleColor(4);
		}
		ImGui::EndPopup();
	}
}

void popup_multi_combo( const char* label, std::vector< bool >& combos, const char* items[], int items_count, ImVec2 sz = ImVec2( 180, 39 ) )
{
	std::vector< std::string > selected;

	for ( int i = 0; i < items_count; i++ )
		if ( combos[ i ] )
			selected.push_back( items[ i ] );

	std::string preview = selected.empty( ) ? "none" : selected[ 0 ];

	for ( int i = 1; i < selected.size( ); i++ )
		i >= 2 ? preview = "..." : preview += ", " + selected[ i ];

	// Отображаем название слева серым цветом с отступом 6px вправо
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 6.0f );
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(150.0f/255.0f, 150.0f/255.0f, 150.0f/255.0f, 1.0f));
	ImGui::Text( label );
	ImGui::PopStyleColor();
	
	// Перемещаемся на ту же строку и позиционируемся после текста + 3px
	ImGui::SameLine();
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 3.0f );
	
	// Поднимаем комбо бокс на 15 пикселей выше
	ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 15.0f );
	
	// Вычисляем ширину комбо бокса: от текущей позиции до конца popup - 12px
	const float combo_width = ImGui::GetContentRegionAvail().x - 12.0f;

	// later do a custom text selector
	if ( popup_header_box( label, preview.c_str( ), items_count, ImVec2( combo_width, sz.y ) ) ) {
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
			
			// Рисуем hover эффект ПОД текстом, начинается левее для покрытия точки
			if ( item_hovered ) {
				ImVec2 hover_start = item_start_pos + ImVec2( -15.0f, 0.0f ); // Начинается левее
				ImVec2 hover_end = item_start_pos + item_size - ImVec2( 5.0f, 0.0f );
				ImGui::GetWindowDrawList()->AddRectFilled(
					hover_start,
					hover_end,
					ImColor( 21, 21, 21, int( 255 * g_menu.m_animation_progress ) ),
					0.0f
				);
			}
			
			// Добавляем отступ для текста
			ImGui::SetCursorPosY( ImGui::GetCursorPosY() + POPUP_COMBO_TEXT_OFFSET_Y );
			
			ImGui::PushStyleColor( ImGuiCol_Text,
			                       combos[ i ]
			                           ? ImVec4( GET_VARIABLE( g_variables.m_accent, c_color ).get_vec4( float( 255 * g_menu.m_animation_progress ) ) )
			                                                  : ImVec4( 255, 255, 255, float( 1.f * g_menu.m_animation_progress ) ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderActive, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_Header, ImVec4( 0, 0, 0, 0 ) );

			// Use a temporary variable to handle the proxy object issue
			bool temp = combos[ i ];
			if ( ImGui::Selectable( items[ i ], &temp, ImGuiSelectableFlags_DontClosePopups ) )
				combos[ i ] = temp;
			
			// Рисуем закругленную точку для активного элемента (левее на 21px)
			if ( combos[ i ] ) {
				ImVec2 dot_pos = item_start_pos + ImVec2( -11.0f, item_size.y / 2.0f - 2.0f ); // Позиция точки без отступа
				ImGui::GetWindowDrawList()->AddCircleFilled(
					dot_pos + ImVec2( 2.0f, 2.0f ), // Центр круга
					2.0f, // Радиус
					ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) )
				);
			}

			ImGui::PopStyleColor( 4 );
		}
		ImGui::EndPopup( );
	}
}

void popup_multi_combo( const char* label, bool combos[], const char* items[], int items_count, ImVec2 sz = ImVec2( 180, 39 ) )
{
	std::vector< std::string > selected;

	for ( int i = 0; i < items_count; i++ )
		if ( combos[ i ] )
			selected.push_back( items[ i ] );

	std::string preview = selected.empty( ) ? "none" : selected[ 0 ];

	for ( int i = 1; i < selected.size( ); i++ )
		i >= 2 ? preview = "..." : preview += ", " + selected[ i ];

	// Отображаем название слева серым цветом с отступом 6px вправо
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 6.0f );
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(150.0f/255.0f, 150.0f/255.0f, 150.0f/255.0f, 1.0f));
	ImGui::Text( label );
	ImGui::PopStyleColor();
	
	// Перемещаемся на ту же строку и позиционируемся после текста + 3px
	ImGui::SameLine();
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 3.0f );
	
	// Поднимаем комбо бокс на 15 пикселей выше
	ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 15.0f );
	
	// Вычисляем ширину комбо бокса: от текущей позиции до конца popup - 12px
	const float combo_width = ImGui::GetContentRegionAvail().x - 12.0f;

	// later do a custom text selector
	if ( popup_header_box( label, preview.c_str( ), items_count, ImVec2( combo_width, sz.y ) ) ) {
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
			
			// Рисуем hover эффект ПОД текстом, начинается левее для покрытия точки
			if ( item_hovered ) {
				ImVec2 hover_start = item_start_pos + ImVec2( -15.0f, 0.0f ); // Начинается левее
				ImVec2 hover_end = item_start_pos + item_size - ImVec2( 5.0f, 0.0f );
				ImGui::GetWindowDrawList()->AddRectFilled(
					hover_start,
					hover_end,
					ImColor( 21, 21, 21, int( 255 * g_menu.m_animation_progress ) ),
					0.0f
				);
			}
			
			// Добавляем отступ для текста
			ImGui::SetCursorPosY( ImGui::GetCursorPosY() + POPUP_COMBO_TEXT_OFFSET_Y );
			
			ImGui::PushStyleColor( ImGuiCol_Text,
			                       combos[ i ] ? ImVec4( 230 / 255.f, 210 / 255.f, 255 / 255.f, 255 / 255.f ) : ImVec4( 255, 255, 255, 255 ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderActive, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4( 0, 0, 0, 0 ) );
			ImGui::PushStyleColor( ImGuiCol_Header, ImVec4( 0, 0, 0, 0 ) );

			// Use a temporary variable to handle the proxy object issue
			bool temp = combos[ i ];
			if ( ImGui::Selectable( items[ i ], &temp, ImGuiSelectableFlags_DontClosePopups ) )
				combos[ i ] = temp;
			
			// Рисуем закругленную точку для активного элемента (левее на 21px)
			if ( combos[ i ] ) {
				ImVec2 dot_pos = item_start_pos + ImVec2( -11.0f, item_size.y / 2.0f - 2.0f ); // Позиция точки без отступа
				ImGui::GetWindowDrawList()->AddCircleFilled(
					dot_pos + ImVec2( 2.0f, 2.0f ), // Центр круга
					2.0f, // Радиус
					ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) )
				);
			}

			ImGui::PopStyleColor( 4 );
		}
		ImGui::EndPopup( );
	}
}
