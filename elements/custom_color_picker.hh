#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/imgui_internal.h"
#include "../hacks/animations/animations.h"
#include "../game/sdk/classes/c_color.h"
#include <string>
#include <cmath>

// Кастомная функция для создания цветового пикера без использования ColorPicker4
bool custom_color_picker_window( const char* name, c_color* color, const ImVec2& window_size = ImVec2( 205.0f, 330.0f ) )
{
    // Устанавливаем размер окна ДО BeginPopup
    ImGui::SetNextWindowSize( window_size, ImGuiCond_Always );
    
    if ( !ImGui::BeginPopup( name, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground ) )
        return false;

    bool color_changed = false;
    
    const auto draw_list = ImGui::GetWindowDrawList( );
    const ImVec2 position = ImGui::GetWindowPos( );
    const ImVec2 size = ImGui::GetWindowSize( );
    
    // Фон popup в стиле меню с закруглением
    draw_list->AddRectFilled( position, position + size, ImColor( 13, 13, 13, 255 ), 8.0f, ImDrawFlags_RoundCornersAll );
    
    // Обводка вокруг popup в стиле меню
    draw_list->AddRect( position, position + size, ImColor( 33, 33, 33, 255 ), 8.0f, ImDrawFlags_RoundCornersAll, 1.0f );
    
    // Устанавливаем отступы 5px от краев
    ImGui::SetCursorPos( ImVec2( 5.0f, 5.0f ) );
    
    // Получаем текущий цвет
    ImVec4 color_vec4 = color->get_vec4( 1.0f );
    
    // Создаем область для цветового пикера
    ImVec2 picker_area_size = ImVec2( window_size.x - 10.0f, 60.0f );
    ImVec2 picker_area_pos = ImVec2( 5.0f, 5.0f );
    ImRect picker_area_bb( picker_area_pos, picker_area_pos + picker_area_size );
    
    // Фон для области цветового пикера
    draw_list->AddRectFilled( picker_area_bb.Min, picker_area_bb.Max, ImColor( 20, 20, 20, 255 ), 6.0f, ImDrawFlags_RoundCornersAll );
    draw_list->AddRect( picker_area_bb.Min, picker_area_bb.Max, ImColor( 40, 40, 40, 255 ), 6.0f, ImDrawFlags_RoundCornersAll, 1.0f );
    
    // Применяем кастомные стили ImGui для ColorPicker4
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiStyle backup_style = style;
    
    // Стилизуем ColorPicker4 в стиле меню
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f); // Темный фон
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f); // Hover фон
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f); // Active фон
    style.Colors[ImGuiCol_Border] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f); // Обводка
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // Убираем тень
    style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); // Светлый текст
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Отключенный текст
    style.Colors[ImGuiCol_Button] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f); // Кнопки
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f); // Hover кнопки
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f); // Active кнопки
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f); // Ползунок
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f); // Active ползунок
    
    // Дополнительные стили для ColorPicker4
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f); // Фон окна
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f); // Фон дочерних элементов
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f); // Фон popup
    style.Colors[ImGuiCol_Header] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f); // Заголовки
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f); // Hover заголовки
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f); // Active заголовки
    style.Colors[ImGuiCol_Separator] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f); // Разделители
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f); // Hover разделители
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f); // Active разделители
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f); // Ручки изменения размера
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f); // Hover ручки
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f); // Active ручки
    style.Colors[ImGuiCol_Tab] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f); // Вкладки
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f); // Hover вкладки
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f); // Active вкладки
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f); // Неактивные вкладки
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f); // Неактивные активные вкладки
    
    // Закругления в стиле меню
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowRounding = 8.0f;
    style.ChildRounding = 4.0f;
    style.PopupRounding = 8.0f;
    
    // Используем стандартный ColorPicker4 с кастомными стилями
    ImGui::SetCursorPos( ImVec2( 10.0f, 10.0f ) );
    if ( ImGui::ColorPicker4( ( std::string( "##custom_color_picker_" ) + name ).c_str( ), &color_vec4.x, 
                             ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview | 
                             ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview ) ) {
        // Обновляем цвет из ImVec4 обратно в c_color
        *color = c_color( color_vec4.x, color_vec4.y, color_vec4.z, color_vec4.w );
        color_changed = true;
    }
    
    // Восстанавливаем оригинальные стили
    style = backup_style;
    
    // Добавляем мини кнопки для быстрого выбора цветов (под альфа ползунком)
    ImVec2 preset_start_pos = ImVec2( 10.0f, 95.0f );
    
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
        ImVec2 button_pos = ImVec2( 
            preset_start_pos.x + ( i % 4 ) * ( mini_button_size.x + mini_button_spacing ), 
            preset_start_pos.y + ( i / 4 ) * ( mini_button_size.y + mini_button_spacing ) 
        );
        
        ImRect button_bb( button_pos, button_pos + mini_button_size );
        
        // Проверяем наведение и клик
        bool hovered = ImGui::IsMouseHoveringRect( button_bb.Min, button_bb.Max );
        bool clicked = hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left );
        
        // Цвет кнопки в стиле меню
        ImColor button_color = hovered ? ImColor( 25, 25, 25, 255 ) : ImColor( 16, 16, 16, 255 );
        ImColor outline_color = hovered ? ImColor( 50, 50, 50, 255 ) : ImColor( 33, 33, 33, 255 );
        
        // Рисуем кнопку с закруглениями в стиле меню
        draw_list->AddRectFilled( button_bb.Min, button_bb.Max, button_color, 4.0f, ImDrawFlags_RoundCornersAll );
        draw_list->AddRect( button_bb.Min, button_bb.Max, outline_color, 4.0f, ImDrawFlags_RoundCornersAll, 1.0f );
        
        // Рисуем цвет внутри кнопки
        ImRect color_bb( button_bb.Min + ImVec2( 1.0f, 1.0f ), button_bb.Max - ImVec2( 1.0f, 1.0f ) );
        draw_list->AddRectFilled( color_bb.Min, color_bb.Max, preset_colors[ i ].get_u32( 255 ), 3.0f, ImDrawFlags_RoundCornersAll );
        
        // Обработка клика
        if ( clicked ) {
            *color = preset_colors[ i ];
            color_changed = true;
        }
    }
    
    // ===== RGB INPUT FIELDS =====
    float rgb_input_start_y = 145.0f;
    {
        // Статические буферы для RGB ввода
        static std::unordered_map< ImGuiID, int[3] > rgb_values;
        static std::unordered_map< ImGuiID, bool > rgb_initialized;
        ImGuiID rgb_id = ImGui::GetID( ( std::string( "##rgb_input_" ) + name ).c_str( ) );
        
        if ( rgb_initialized.find( rgb_id ) == rgb_initialized.end( ) ) {
            rgb_initialized[ rgb_id ] = false;
        }
        
        // Инициализируем RGB значения из текущего цвета
        if ( !rgb_initialized[ rgb_id ] ) {
            rgb_values[ rgb_id ][ 0 ] = static_cast<int>( color_vec4.x * 255.0f + 0.5f );
            rgb_values[ rgb_id ][ 1 ] = static_cast<int>( color_vec4.y * 255.0f + 0.5f );
            rgb_values[ rgb_id ][ 2 ] = static_cast<int>( color_vec4.z * 255.0f + 0.5f );
            rgb_initialized[ rgb_id ] = true;
        }
        
        // Если цвет изменился из пикера, обновляем RGB
        if ( color_changed ) {
            rgb_values[ rgb_id ][ 0 ] = static_cast<int>( color_vec4.x * 255.0f + 0.5f );
            rgb_values[ rgb_id ][ 1 ] = static_cast<int>( color_vec4.y * 255.0f + 0.5f );
            rgb_values[ rgb_id ][ 2 ] = static_cast<int>( color_vec4.z * 255.0f + 0.5f );
        }
        
        // Стили для RGB полей
        ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.06f, 0.06f, 0.06f, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImVec4( 0.1f, 0.1f, 0.1f, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImVec4( 0.12f, 0.12f, 0.12f, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.9f, 0.9f, 0.9f, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 0.13f, 0.13f, 0.13f, 1.0f ) );
        ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 3.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4.0f, 2.0f ) );
        
        const char* rgb_labels[] = { "R", "G", "B" };
        const float label_width = 12.0f;
        const float input_width = 50.0f;
        const float spacing = 5.0f;
        
        for ( int i = 0; i < 3; i++ ) {
            float x_offset = 10.0f + i * ( label_width + input_width + spacing );
            
            // Рисуем label (R/G/B)
            ImVec2 label_pos = ImVec2( x_offset, rgb_input_start_y + 2.0f );
            draw_list->AddText( label_pos, ImColor( 150, 150, 150, 255 ), rgb_labels[ i ] );
            
            // Input field
            ImGui::SetCursorPos( ImVec2( x_offset + label_width, rgb_input_start_y ) );
            ImGui::PushItemWidth( input_width );
            
            if ( ImGui::InputInt( ( std::string( "##rgb_" ) + std::to_string( i ) + "_" + name ).c_str( ), 
                                  &rgb_values[ rgb_id ][ i ], 0, 0, ImGuiInputTextFlags_EnterReturnsTrue ) ) {
                // Ограничиваем значения 0-255
                rgb_values[ rgb_id ][ i ] = std::clamp( rgb_values[ rgb_id ][ i ], 0, 255 );
                *color = c_color( rgb_values[ rgb_id ][ 0 ], rgb_values[ rgb_id ][ 1 ], rgb_values[ rgb_id ][ 2 ], 
                                  static_cast<int>( color_vec4.w * 255.0f + 0.5f ) );
                color_changed = true;
            }
            
            // Также применяем при потере фокуса
            if ( ImGui::IsItemDeactivatedAfterEdit( ) ) {
                rgb_values[ rgb_id ][ i ] = std::clamp( rgb_values[ rgb_id ][ i ], 0, 255 );
                *color = c_color( rgb_values[ rgb_id ][ 0 ], rgb_values[ rgb_id ][ 1 ], rgb_values[ rgb_id ][ 2 ], 
                                  static_cast<int>( color_vec4.w * 255.0f + 0.5f ) );
                color_changed = true;
            }
            
            ImGui::PopItemWidth( );
        }
        
        ImGui::PopStyleVar( 2 );
        ImGui::PopStyleColor( 5 );
    }
    
    // ===== HEX CODE INPUT =====
    float hex_input_start_y = 170.0f; // Сдвинули вниз под RGB поля
    {
        // Статический буфер для hex ввода (per-popup via ID)
        static std::unordered_map< ImGuiID, char[10] > hex_buffers;
        static std::unordered_map< ImGuiID, bool > hex_initialized;
        ImGuiID hex_id = ImGui::GetID( ( std::string( "##hex_input_" ) + name ).c_str( ) );
        
        if ( hex_initialized.find( hex_id ) == hex_initialized.end( ) ) {
            hex_initialized[ hex_id ] = false;
        }
        
        // Обновляем hex буфер из текущего цвета
        if ( !hex_initialized[ hex_id ] ) {
            int r_val = static_cast<int>( color_vec4.x * 255.0f + 0.5f );
            int g_val = static_cast<int>( color_vec4.y * 255.0f + 0.5f );
            int b_val = static_cast<int>( color_vec4.z * 255.0f + 0.5f );
            snprintf( hex_buffers[ hex_id ], sizeof( hex_buffers[ hex_id ] ), "%02X%02X%02X", r_val, g_val, b_val );
            hex_initialized[ hex_id ] = true;
        }
        
        // Если цвет изменился из пикера, обновляем hex
        if ( color_changed ) {
            int r_val = static_cast<int>( color_vec4.x * 255.0f + 0.5f );
            int g_val = static_cast<int>( color_vec4.y * 255.0f + 0.5f );
            int b_val = static_cast<int>( color_vec4.z * 255.0f + 0.5f );
            snprintf( hex_buffers[ hex_id ], sizeof( hex_buffers[ hex_id ] ), "%02X%02X%02X", r_val, g_val, b_val );
        }
        
        // Рисуем label "#"
        ImVec2 hash_pos = ImVec2( 10.0f, hex_input_start_y + 2.0f );
        draw_list->AddText( hash_pos, ImColor( 150, 150, 150, 255 ), "#" );
        
        // Hex input field
        ImGui::SetCursorPos( ImVec2( 22.0f, hex_input_start_y ) );
        ImGui::PushItemWidth( 75.0f );
        ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.06f, 0.06f, 0.06f, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImVec4( 0.1f, 0.1f, 0.1f, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImVec4( 0.12f, 0.12f, 0.12f, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.9f, 0.9f, 0.9f, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 0.13f, 0.13f, 0.13f, 1.0f ) );
        ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 3.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4.0f, 2.0f ) );
        
        if ( ImGui::InputText( ( std::string( "##hex_" ) + name ).c_str( ), hex_buffers[ hex_id ], 7,
                               ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_EnterReturnsTrue ) ) {
            // Парсим hex и применяем цвет
            unsigned int hex_val = 0;
            if ( sscanf_s( hex_buffers[ hex_id ], "%X", &hex_val ) == 1 ) {
                int r_hex = ( hex_val >> 16 ) & 0xFF;
                int g_hex = ( hex_val >> 8 ) & 0xFF;
                int b_hex = hex_val & 0xFF;
                *color = c_color( r_hex, g_hex, b_hex, static_cast<int>( color_vec4.w * 255.0f + 0.5f ) );
                color_changed = true;
            }
        }
        
        // Также применяем при потере фокуса (не только Enter)
        if ( ImGui::IsItemDeactivatedAfterEdit( ) ) {
            unsigned int hex_val = 0;
            if ( sscanf_s( hex_buffers[ hex_id ], "%X", &hex_val ) == 1 ) {
                int r_hex = ( hex_val >> 16 ) & 0xFF;
                int g_hex = ( hex_val >> 8 ) & 0xFF;
                int b_hex = hex_val & 0xFF;
                *color = c_color( r_hex, g_hex, b_hex, static_cast<int>( color_vec4.w * 255.0f + 0.5f ) );
                color_changed = true;
            }
        }
        
        ImGui::PopStyleVar( 2 );
        ImGui::PopStyleColor( 5 );
        ImGui::PopItemWidth( );
        
        // Цветной превью квадратик справа от hex input
        ImVec2 preview_pos = ImVec2( 105.0f, hex_input_start_y + 1.0f );
        ImVec2 preview_size = ImVec2( 16.0f, 16.0f );
        draw_list->AddRectFilled( preview_pos, preview_pos + preview_size, color->get_u32( 1.0f ), 3.0f, ImDrawFlags_RoundCornersAll );
        draw_list->AddRect( preview_pos, preview_pos + preview_size, ImColor( 33, 33, 33, 255 ), 3.0f, ImDrawFlags_RoundCornersAll, 1.0f );
    }
    
    // Добавляем кнопки Copy / Paste / Restore внизу с новым дизайном меню
    float copy_paste_start_y = 195.0f; // Позиция под hex input (сдвинули вниз)
    
    // Создаем функцию для рисования кнопки в стиле меню
    auto draw_menu_button = [&]( const char* button_name, ImVec2 pos, ImVec2 size ) -> bool {
        ImRect button_bb( pos, pos + size );
        bool hovered = ImGui::IsMouseHoveringRect( button_bb.Min, button_bb.Max );
        bool clicked = hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left );
        
        // Анимация наведения (упрощенная версия)
        static std::unordered_map< ImGuiID, float > button_hover_anim;
        ImGuiID button_id = ImGui::GetID( button_name );
        
        if ( button_hover_anim.find( button_id ) == button_hover_anim.end( ) )
            button_hover_anim[ button_id ] = 0.0f;
        
        float dt = ImGui::GetIO( ).DeltaTime;
        float hover_target = hovered ? 1.0f : 0.0f;
        button_hover_anim[ button_id ] = smoothLerp( button_hover_anim[ button_id ], hover_target, dt * 8.0f );
        
        // Цвета в стиле меню
        ImColor base_color( 16, 16, 16, 255 );
        ImColor hover_color( 25, 25, 25, 255 );
        ImColor accent_color( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 255 ) );
        
        // Смешиваем цвета с учетом анимации
        ImColor final_color = smoothLerpColor( base_color, hover_color, button_hover_anim[ button_id ] );
        
        // Обводка в стиле меню
        ImColor base_outline_color( 33, 33, 33, 255 );
        ImColor hover_outline_color( 50, 50, 50, 255 );
        ImColor accent_outline_color( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( 255 ) );
        
        ImColor final_outline_color = smoothLerpColor( base_outline_color, hover_outline_color, button_hover_anim[ button_id ] );
        
        // Толщина обводки с анимацией
        float outline_thickness = 1.0f + button_hover_anim[ button_id ] * 0.5f;
        
        // Рисуем кнопку с менее закругленными углами
        draw_list->AddRectFilled( button_bb.Min, button_bb.Max, final_color, 1.0f, ImDrawFlags_RoundCornersAll );
        draw_list->AddRect( button_bb.Min, button_bb.Max, final_outline_color, 1.0f, ImDrawFlags_RoundCornersAll, outline_thickness );
        
        // Текст на кнопке
        ImVec2 text_size = ImGui::CalcTextSize( button_name );
        ImVec2 text_pos = ImVec2( 
            button_bb.Min.x + ( size.x - text_size.x ) * 0.5f,
            button_bb.Min.y + ( size.y - text_size.y ) * 0.5f
        );
        
        // Цвет текста в стиле меню
        ImColor base_text_color( 150, 150, 150, 255 );
        ImColor hover_text_color( 255, 255, 255, 255 );
        ImColor final_text_color = smoothLerpColor( base_text_color, hover_text_color, button_hover_anim[ button_id ] );
        
        draw_list->AddText( text_pos, final_text_color, button_name );
        
        return clicked;
    };
    
    // Кнопка Copy
    ImVec2 copy_button_size = ImVec2( 58.0f, 20.0f );
    ImVec2 copy_button_pos = ImVec2( 10.0f, copy_paste_start_y );
    bool copy_clicked = draw_menu_button( "Copy", copy_button_pos, copy_button_size );
    
    // Кнопка Paste (соседняя)
    ImVec2 paste_button_size = ImVec2( 58.0f, 20.0f );
    ImVec2 paste_button_pos = ImVec2( 78.0f, copy_paste_start_y );
    bool paste_clicked = draw_menu_button( "Paste", paste_button_pos, paste_button_size );

    // Кнопка Restore to default
    ImVec2 restore_button_size = ImVec2( 109.0f, 20.0f );
    ImVec2 restore_button_pos = ImVec2( 146.0f, copy_paste_start_y );
    bool restore_clicked = draw_menu_button( "Restore", restore_button_pos, restore_button_size );
    
    // Статическая переменная для хранения скопированного цвета
    static c_color clipboard_color = c_color( 255, 255, 255, 255 );
    static bool clipboard_has_color = false;
    
    // Обработка кликов на кнопки Copy и Paste
    if ( copy_clicked && color ) {
        clipboard_color = *color;
        clipboard_has_color = true;
    }
    
    if ( paste_clicked && color && clipboard_has_color ) {
        *color = clipboard_color;
        color_changed = true;
    }

    if ( restore_clicked && color ) {
        *color = c_color( 252, 194, 255, 255 );
        color_changed = true;
    }
    
    ImGui::EndPopup( );
    return color_changed;
}
