#pragma once
#include <string>
#include <Windows.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/imgui_freetype.h"
#include "../dependencies/imgui/imgui_internal.h"
namespace fonts_for_gui {
	inline ImFont* regular = nullptr;
	inline ImFont* bolder = nullptr;
	inline ImFont* regular_11 = nullptr;
	inline ImFont* bolder_11  = nullptr;
	inline ImFont* title = nullptr;
	inline ImFont* icons = nullptr;
	inline ImFont* logo  = nullptr;
	inline ImFont* pts_font = nullptr; 
	inline ImFont* pts_semibold_italic = nullptr; 
	inline ImFont* pts_rubik_light_italic = nullptr; 
	inline ImFont* rubik_regular = nullptr; 
	inline ImFont* rubik_italic = nullptr; 
	inline ImFont* barlow_regular_italic = nullptr; 
	inline ImFont* watermark_font = nullptr; 
	inline ImFont* tas_orbiter_font = nullptr; 
	inline ImFont* tas_orbiter_small = nullptr; 
	inline ImFont* tas_orbiter_menu = nullptr; 
	inline ImFont* fontello_font = nullptr; 
	inline ImFont* fontello_recorder_icon = nullptr; 
	inline ImFont* fontello_convar_icon = nullptr;   
	inline ImFont* fontello_route_icon = nullptr;   
	inline ImFont* fontello_fun_icon = nullptr;     
	inline ImFont* fontello_sidebar_icon = nullptr;
	inline ImFont* keyboard_icon_font = nullptr; 
	inline ImFont* verdanarus_esp_font = nullptr; 
	inline ImFont* doctrine_velocity = nullptr; 
	inline ImFont* tasorbiter_indicators = nullptr; 
	inline ImFont* tasorbiter_indicators_small = nullptr; 
	inline ImFont* tasorbiter_addwindow = nullptr; 
	inline ImFont* doctrine_addwindow = nullptr; 
	inline ImFont* doctrine_brackets = nullptr; 
	inline ImFont* noto_sans_syriac = nullptr; 
	inline ImFont* silkscreen_font = nullptr; 
	inline ImFont* rubikcustom_pts_font = nullptr; 
	inline ImFont* fonteditor_pts_font = nullptr;
	void instalisation( ImGuiIO& io );
} 