#pragma once
#include "../instalisation fonts/fonts2.h"

bool input_text(const char* name, const char* value, char* buf, size_t buf_size, float width, float height, ImGuiInputTextFlags flags) {
    // Кастомный визуал для поля ввода
    ImVec2 input_pos = ImGui::GetCursorPos();
    ImVec2 input_min = ImGui::GetWindowPos() + input_pos;
    ImVec2 input_max = input_min + ImVec2(width, height);
    
    // Заливка поля ввода цветом (13, 13, 13)
    ImGui::GetWindowDrawList()->AddRectFilled(input_min, input_max, ImColor(13, 13, 13, 255), 4.0f, ImDrawFlags_RoundCornersAll);
    
    // Обводка 1 пиксель цветом (33, 33, 33)
    ImGui::GetWindowDrawList()->AddRect(input_min, input_max, ImColor(33, 33, 33, 255), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);
    
    // Используем стандартную функцию ImGui с кастомным стилем
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0)); // Прозрачный фон
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0)); // Прозрачная обводка
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    
    bool result = ImGui::InputTextEx(name, value, buf, buf_size, ImVec2(width, height), flags, 0, 0);
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    
    return result;
}

void text_with_keybind(const char* text) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
    
    // Используем PushStyleColor для изменения цвета текста
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(150.0f/255.0f, 150.0f/255.0f, 150.0f/255.0f, 1.0f));
    ImGui::Text(text);
    ImGui::PopStyleColor();
}
