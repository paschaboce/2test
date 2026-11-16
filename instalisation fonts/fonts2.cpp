#include "fonts2.h"
#include "../dependencies/fonts/anime.h"
#include "../dependencies/fonts/lobotomy.h"
#include "../dependencies/fonts/OpenSans_Regular_font.h"
#include "../dependencies/fonts/OpenSans_Italic_font.h"
#include "../dependencies/fonts/OpenSans_SemiBoldItalic_font.h"
#include "../dependencies/fonts/Rubik_Regular_font.h"
#include "../dependencies/fonts/Rubik_Italic_font.h"
#include "../dependencies/fonts/Rubik_Medium_font.h"
#include "../dependencies/fonts/Barlow_Regular_font.h"
#include "../dependencies/fonts/Silkscreen_font.h"
#include "../dependencies/fonts/RubikCustom_font.h"
#include "../dependencies/fonts/Fonteditor_font.h"
#include "../dependencies/fonts/Kamibebra_font.h"
#include "../dependencies/fonts/fontello_font.h"
#include "../dependencies/fonts/fontello_recorder_icon.h"
#include "../dependencies/fonts/fontello_convar_icon.h"
#include "../dependencies/fonts/fontello_route_icon.h"
#include "../dependencies/fonts/fontello_fun_icon.h"
#include "../dependencies/fonts/fontello_sidebar_icon.h"
#include "../dependencies/fonts/keybord_icon_font.h"
#include "../dependencies/fonts/VERDANARUS_font.h"
#include "../dependencies/fonts/GoogleSans_Regular_font.h"
#include "../dependencies/fonts/Doctrine_W00_Regular_font.h"
#include "../dependencies/imgui/helpers/fonts.h"
std::string get_fonts_folder() {
	char windowsPath[MAX_PATH];
	if (GetWindowsDirectoryA(windowsPath, MAX_PATH)) 
		return std::string(windowsPath) + "\\Fonts";
	return "C:\\Windows\\Fonts"; 
}
void fonts_for_gui::instalisation( ImGuiIO& io )
{
#ifdef _DEBUG
	regular = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_compressed_data, verdana_compressed_size, 13.f );
	bolder  = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_bold_compressed_data, verdana_bold_compressed_size, 13.f );
	title   = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_bold_compressed_data, verdana_bold_compressed_size, 20.f );
	icons   = io.Fonts->AddFontFromMemoryCompressedTTF( icon_font_compressed_data, icon_font_compressed_size, 13.f );
	logo    = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_bold_compressed_data, verdana_bold_compressed_size, 34.f );
	regular_11 = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_compressed_data, verdana_compressed_size, 13.f );
	bolder_11  = io.Fonts->AddFontFromMemoryCompressedTTF( verdana_bold_compressed_data, verdana_bold_compressed_size, 13.f );
	auto rubik_esp_debug_config               = ImFontConfig( );
	rubik_esp_debug_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	rubik_esp_debug_config.FontDataSize       = Rubik_Regular_compressed_size;
	rubik_esp_debug_config.SizePixels         = 12.0f;
	rubik_esp_debug_config.PixelSnapH         = true;
	rubik_esp_debug_config.OversampleH        = 5;
	rubik_esp_debug_config.OversampleV        = 1;
	rubik_esp_debug_config.RasterizerMultiply = 1.0f;
	rubik_esp_debug_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	verdanarus_esp_font = io.Fonts->AddFontFromMemoryTTF( rubik_esp_debug_config.FontData, rubik_esp_debug_config.FontDataSize, rubik_esp_debug_config.SizePixels, &rubik_esp_debug_config,
	                                                     io.Fonts->GetGlyphRangesCyrillic( ) );
	fontello_font = io.Fonts->AddFontFromMemoryCompressedTTF( fontello_compressed_data, fontello_compressed_size, 14.f );
	fontello_recorder_icon = io.Fonts->AddFontFromMemoryCompressedTTF( fontello_recorder_icon_compressed_data, fontello_recorder_icon_compressed_size, 14.f );
	{
		static const ImWchar range_convar[] = { 0x6E, 0x6E, 0 };  
		static const ImWchar range_route[] = { 0x63, 0x63, 0 };    
		static const ImWchar range_fun[] = { 0x6D, 0x6D, 0 };     
		ImFontConfig cfg;
		cfg.SizePixels = 14.f;
		cfg.PixelSnapH = true;
		cfg.OversampleH = 5;
		cfg.OversampleV = 1;
		cfg.FontData = const_cast<void*>( static_cast<const void*>( fontello_convar_icon_compressed_data ) );
		cfg.FontDataSize = (int)fontello_convar_icon_compressed_size;
		fontello_convar_icon = io.Fonts->AddFontFromMemoryTTF( cfg.FontData, cfg.FontDataSize, cfg.SizePixels, &cfg, range_convar );
		cfg.FontData = const_cast<void*>( static_cast<const void*>( fontello_route_icon_compressed_data ) );
		cfg.FontDataSize = (int)fontello_route_icon_compressed_size;
		fontello_route_icon = io.Fonts->AddFontFromMemoryTTF( cfg.FontData, cfg.FontDataSize, cfg.SizePixels, &cfg, range_route );
		cfg.FontData = const_cast<void*>( static_cast<const void*>( fontello_fun_icon_compressed_data ) );
		cfg.FontDataSize = (int)fontello_fun_icon_compressed_size;
		fontello_fun_icon = io.Fonts->AddFontFromMemoryTTF( cfg.FontData, cfg.FontDataSize, cfg.SizePixels, &cfg, range_fun );
		cfg.FontData = const_cast<void*>( static_cast<const void*>( fontello_sidebar_icon_compressed_data ) );
		cfg.FontDataSize = (int)fontello_sidebar_icon_compressed_size;
		static const ImWchar range_sidebar_dbg[] = {
			( ImWchar )0xE52F, ( ImWchar )0xE52F, ( ImWchar )0xF093, ( ImWchar )0xF093,
			( ImWchar )0xF0C0, ( ImWchar )0xF0C0, ( ImWchar )0xF1DE, ( ImWchar )0xF1DE, 0
		};
		fontello_sidebar_icon = io.Fonts->AddFontFromMemoryTTF( cfg.FontData, cfg.FontDataSize, cfg.SizePixels, &cfg, range_sidebar_dbg );
	}
	#else
	ImGuiStyle& style = ImGui::GetStyle( );
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;
	auto title_icon_config               = ImFontConfig( );
	title_icon_config.FontData           = ANIME;
	title_icon_config.FontDataSize       = sizeof( ANIME );
	title_icon_config.SizePixels         = 8.0f;
	title_icon_config.RasterizerMultiply = 1.0f;
	title_icon_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	auto tab_icons_config               = ImFontConfig( );
	tab_icons_config.FontData           = lobotomy;
	tab_icons_config.FontDataSize       = sizeof( lobotomy );
	tab_icons_config.SizePixels         = 14.0f;
	tab_icons_config.RasterizerMultiply = 1.0f;
	tab_icons_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	auto regular_config               = ImFontConfig( );
	regular_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	regular_config.FontDataSize       = Rubik_Regular_compressed_size;
	regular_config.SizePixels         = 13.0f;
	regular_config.PixelSnapH         = true;
	regular_config.OversampleH        = 5;
	regular_config.OversampleV        = 1;
	regular_config.RasterizerMultiply = 1.0f;
	regular_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap | 
	                                    ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_LightHinting;
	auto bold_config             = ImFontConfig( );
	bold_config.FontData         = const_cast<void*>(static_cast<const void*>(Rubik_Medium_compressed_data));
	bold_config.FontDataSize     = Rubik_Medium_compressed_size;
	bold_config.SizePixels       = 13.0f;
	bold_config.PixelSnapH       = true;
	bold_config.OversampleH      = 5;
	bold_config.OversampleV      = 1;
	bold_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap | 
	                               ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_LightHinting;
	auto regular_config_11               = ImFontConfig( );
	regular_config_11.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	regular_config_11.FontDataSize       = Rubik_Regular_compressed_size;
	regular_config_11.SizePixels         = 11.0f;
	regular_config_11.PixelSnapH         = true;
	regular_config_11.OversampleH        = 5;
	regular_config_11.OversampleV        = 1;
	regular_config_11.RasterizerMultiply = 1.0f;
	regular_config_11.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap |
	                                     ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Monochrome |
	                                     ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_MonoHinting;
	auto bold_config_11            = ImFontConfig( );
	bold_config_11.FontData         = const_cast<void*>(static_cast<const void*>(Rubik_Medium_compressed_data));
	bold_config_11.FontDataSize     = Rubik_Medium_compressed_size;
	bold_config_11.SizePixels       = 11.0f;
	bold_config_11.PixelSnapH       = true;
	bold_config_11.OversampleH      = 5;
	bold_config_11.OversampleV      = 1;
	bold_config_11.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap |
	                                  ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Monochrome |
	                                  ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_MonoHinting;
	auto logo_conf          = ImFontConfig( );
	logo_conf.FontData         = const_cast<void*>(static_cast<const void*>(Rubik_Medium_compressed_data));
	logo_conf.FontDataSize     = Rubik_Medium_compressed_size;
	logo_conf.SizePixels       = 42.0f;
	logo_conf.PixelSnapH       = true;
	logo_conf.OversampleH      = 5;
	logo_conf.OversampleV      = 1;
	logo_conf.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	regular = io.Fonts->AddFontFromMemoryTTF( regular_config.FontData, regular_config.FontDataSize, regular_config.SizePixels, &regular_config,
	                                          io.Fonts->GetGlyphRangesCyrillic( ) );
	bolder  = io.Fonts->AddFontFromMemoryTTF( bold_config.FontData, bold_config.FontDataSize, bold_config.SizePixels, &bold_config,
	                                          io.Fonts->GetGlyphRangesCyrillic( ) );
	regular_11 = regular;
	bolder_11 = bolder;
	title   = io.Fonts->AddFontFromMemoryTTF( title_icon_config.FontData, title_icon_config.FontDataSize, title_icon_config.SizePixels,
	                                          &title_icon_config, io.Fonts->GetGlyphRangesCyrillic( ) );
	icons = io.Fonts->AddFontFromMemoryTTF( tab_icons_config.FontData, tab_icons_config.FontDataSize, tab_icons_config.SizePixels, &tab_icons_config,
	                                        io.Fonts->GetGlyphRangesCyrillic( ) );
	logo = io.Fonts->AddFontFromMemoryTTF( logo_conf.FontData, logo_conf.FontDataSize, logo_conf.SizePixels, &logo_conf,
	                                       io.Fonts->GetGlyphRangesCyrillic( ) );
	auto pts_config               = ImFontConfig( );
	pts_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Italic_compressed_data));
	pts_config.FontDataSize       = Rubik_Italic_compressed_size;
	pts_config.SizePixels         = 18.0f;
	pts_config.PixelSnapH         = true;
	pts_config.OversampleH        = 5;
	pts_config.OversampleV        = 1;
	pts_config.RasterizerMultiply = 1.0f;
	pts_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	pts_font = io.Fonts->AddFontFromMemoryTTF( pts_config.FontData, pts_config.FontDataSize, pts_config.SizePixels, &pts_config,
	                                          io.Fonts->GetGlyphRangesCyrillic( ) );
	auto pts_semibold_italic_config               = ImFontConfig( );
	pts_semibold_italic_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Italic_compressed_data));
	pts_semibold_italic_config.FontDataSize       = Rubik_Italic_compressed_size;
	pts_semibold_italic_config.SizePixels         = 18.0f;
	pts_semibold_italic_config.PixelSnapH         = true;
	pts_semibold_italic_config.OversampleH        = 5;
	pts_semibold_italic_config.OversampleV        = 1;
	pts_semibold_italic_config.RasterizerMultiply = 1.0f;
	pts_semibold_italic_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap | ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bold;
	pts_semibold_italic = io.Fonts->AddFontFromMemoryTTF( pts_semibold_italic_config.FontData, pts_semibold_italic_config.FontDataSize, pts_semibold_italic_config.SizePixels, &pts_semibold_italic_config,
	                                                     io.Fonts->GetGlyphRangesCyrillic( ) );
	auto watermark_config               = ImFontConfig( );
	watermark_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	watermark_config.FontDataSize       = Rubik_Regular_compressed_size;
	watermark_config.SizePixels         = 13.0f;
	watermark_config.PixelSnapH         = true;
	watermark_config.OversampleH        = 5;
	watermark_config.OversampleV        = 1;
	watermark_config.RasterizerMultiply = 1.0f;
	watermark_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	watermark_font = io.Fonts->AddFontFromMemoryTTF( watermark_config.FontData, watermark_config.FontDataSize, watermark_config.SizePixels, &watermark_config,
	                                                io.Fonts->GetGlyphRangesCyrillic( ) );
	auto tas_orbiter_config               = ImFontConfig( );
	tas_orbiter_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	tas_orbiter_config.FontDataSize       = Rubik_Regular_compressed_size;
	tas_orbiter_config.SizePixels         = 12.0f;
	tas_orbiter_config.PixelSnapH         = true;
	tas_orbiter_config.OversampleH        = 5;
	tas_orbiter_config.OversampleV        = 1;
	tas_orbiter_config.RasterizerMultiply = 1.0f;
	tas_orbiter_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	tas_orbiter_font = io.Fonts->AddFontFromMemoryTTF( tas_orbiter_config.FontData, tas_orbiter_config.FontDataSize, tas_orbiter_config.SizePixels, &tas_orbiter_config,
	                                                   io.Fonts->GetGlyphRangesCyrillic( ) );
	auto tas_orbiter_small_config               = ImFontConfig( );
	tas_orbiter_small_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	tas_orbiter_small_config.FontDataSize       = Rubik_Regular_compressed_size;
	tas_orbiter_small_config.SizePixels         = 10.0f;
	tas_orbiter_small_config.PixelSnapH         = true;
	tas_orbiter_small_config.OversampleH        = 5;
	tas_orbiter_small_config.OversampleV        = 1;
	tas_orbiter_small_config.RasterizerMultiply = 1.0f;
	tas_orbiter_small_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	tas_orbiter_small = io.Fonts->AddFontFromMemoryTTF( tas_orbiter_small_config.FontData, tas_orbiter_small_config.FontDataSize, tas_orbiter_small_config.SizePixels, &tas_orbiter_small_config,
	                                                    io.Fonts->GetGlyphRangesCyrillic( ) );
	auto tas_orbiter_menu_config               = ImFontConfig( );
	tas_orbiter_menu_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	tas_orbiter_menu_config.FontDataSize       = Rubik_Regular_compressed_size;
	tas_orbiter_menu_config.SizePixels         = 13.0f;
	tas_orbiter_menu_config.PixelSnapH         = true;
	tas_orbiter_menu_config.OversampleH        = 5;
	tas_orbiter_menu_config.OversampleV        = 1;
	tas_orbiter_menu_config.RasterizerMultiply = 1.0f;
	tas_orbiter_menu_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	tas_orbiter_menu = io.Fonts->AddFontFromMemoryTTF( tas_orbiter_menu_config.FontData, tas_orbiter_menu_config.FontDataSize, tas_orbiter_menu_config.SizePixels, &tas_orbiter_menu_config,
	                                                   io.Fonts->GetGlyphRangesCyrillic( ) );
	auto fontello_config               = ImFontConfig( );
	fontello_config.FontData           = const_cast<void*>(static_cast<const void*>(fontello_compressed_data));
	fontello_config.FontDataSize       = sizeof(fontello_compressed_data);
	fontello_config.SizePixels         = 14.0f; 
	fontello_config.PixelSnapH         = true;
	fontello_config.OversampleH        = 5;
	fontello_config.OversampleV        = 1;
	fontello_config.RasterizerMultiply = 1.0f;
	fontello_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	fontello_font = io.Fonts->AddFontFromMemoryTTF( fontello_config.FontData, fontello_config.FontDataSize, fontello_config.SizePixels, &fontello_config,
	                                                io.Fonts->GetGlyphRangesCyrillic( ) );
	auto fontello_recorder_config               = ImFontConfig( );
	fontello_recorder_config.FontData           = const_cast<void*>(static_cast<const void*>(fontello_recorder_icon_compressed_data));
	fontello_recorder_config.FontDataSize       = sizeof(fontello_recorder_icon_compressed_data);
	fontello_recorder_config.SizePixels         = 14.0f;
	fontello_recorder_config.PixelSnapH         = true;
	fontello_recorder_config.OversampleH        = 5;
	fontello_recorder_config.OversampleV        = 1;
	fontello_recorder_config.RasterizerMultiply = 1.0f;
	fontello_recorder_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	fontello_recorder_icon = io.Fonts->AddFontFromMemoryTTF( fontello_recorder_config.FontData, fontello_recorder_config.FontDataSize, fontello_recorder_config.SizePixels, &fontello_recorder_config,
	                                                         io.Fonts->GetGlyphRangesCyrillic( ) );
	static const ImWchar range_convar[] = { 0x6E, 0x6E, 0 };
	static const ImWchar range_route[] = { 0x63, 0x63, 0 };
	static const ImWchar range_fun[] = { 0x6D, 0x6D, 0 };
	auto fontello_convar_config = ImFontConfig( );
	fontello_convar_config.FontData = const_cast<void*>( static_cast<const void*>( fontello_convar_icon_compressed_data ) );
	fontello_convar_config.FontDataSize = fontello_convar_icon_compressed_size;
	fontello_convar_config.SizePixels = 14.0f;
	fontello_convar_config.PixelSnapH = true;
	fontello_convar_config.OversampleH = 5;
	fontello_convar_config.OversampleV = 1;
	fontello_convar_config.RasterizerMultiply = 1.0f;
	fontello_convar_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	fontello_convar_icon = io.Fonts->AddFontFromMemoryTTF( fontello_convar_config.FontData, fontello_convar_config.FontDataSize, fontello_convar_config.SizePixels, &fontello_convar_config, range_convar );
	auto fontello_route_config = ImFontConfig( );
	fontello_route_config.FontData = const_cast<void*>( static_cast<const void*>( fontello_route_icon_compressed_data ) );
	fontello_route_config.FontDataSize = fontello_route_icon_compressed_size;
	fontello_route_config.SizePixels = 14.0f;
	fontello_route_config.PixelSnapH = true;
	fontello_route_config.OversampleH = 5;
	fontello_route_config.OversampleV = 1;
	fontello_route_config.RasterizerMultiply = 1.0f;
	fontello_route_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	fontello_route_icon = io.Fonts->AddFontFromMemoryTTF( fontello_route_config.FontData, fontello_route_config.FontDataSize, fontello_route_config.SizePixels, &fontello_route_config, range_route );
	auto fontello_fun_config = ImFontConfig( );
	fontello_fun_config.FontData = const_cast<void*>( static_cast<const void*>( fontello_fun_icon_compressed_data ) );
	fontello_fun_config.FontDataSize = fontello_fun_icon_compressed_size;
	fontello_fun_config.SizePixels = 14.0f;
	fontello_fun_config.PixelSnapH = true;
	fontello_fun_config.OversampleH = 5;
	fontello_fun_config.OversampleV = 1;
	fontello_fun_config.RasterizerMultiply = 1.0f;
	fontello_fun_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	fontello_fun_icon = io.Fonts->AddFontFromMemoryTTF( fontello_fun_config.FontData, fontello_fun_config.FontDataSize, fontello_fun_config.SizePixels, &fontello_fun_config, range_fun );
	auto fontello_sidebar_config = ImFontConfig( );
	fontello_sidebar_config.FontData = const_cast<void*>( static_cast<const void*>( fontello_sidebar_icon_compressed_data ) );
	fontello_sidebar_config.FontDataSize = fontello_sidebar_icon_compressed_size;
	fontello_sidebar_config.SizePixels = 14.0f;
	fontello_sidebar_config.PixelSnapH = true;
	fontello_sidebar_config.OversampleH = 5;
	fontello_sidebar_config.OversampleV = 1;
	fontello_sidebar_config.RasterizerMultiply = 1.0f;
	fontello_sidebar_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	static const ImWchar range_sidebar[] = {
		( ImWchar )0xE52F, ( ImWchar )0xE52F, ( ImWchar )0xF093, ( ImWchar )0xF093,
		( ImWchar )0xF0C0, ( ImWchar )0xF0C0, ( ImWchar )0xF1DE, ( ImWchar )0xF1DE, 0
	};
	fontello_sidebar_icon = io.Fonts->AddFontFromMemoryTTF( fontello_sidebar_config.FontData, fontello_sidebar_config.FontDataSize, fontello_sidebar_config.SizePixels, &fontello_sidebar_config, range_sidebar );
	auto keyboard_icon_config               = ImFontConfig( );
	keyboard_icon_config.FontData           = const_cast<void*>(static_cast<const void*>(keyboard_icon_compressed_data));
	keyboard_icon_config.FontDataSize       = keyboard_icon_compressed_size;
	keyboard_icon_config.SizePixels         = 12.0f; 
	keyboard_icon_config.PixelSnapH         = true;
	keyboard_icon_config.OversampleH        = 5;
	keyboard_icon_config.OversampleV        = 1;
	keyboard_icon_config.RasterizerMultiply = 1.0f;
	keyboard_icon_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	keyboard_icon_font = io.Fonts->AddFontFromMemoryTTF( keyboard_icon_config.FontData, keyboard_icon_config.FontDataSize, keyboard_icon_config.SizePixels, &keyboard_icon_config,
	                                                     io.Fonts->GetGlyphRangesCyrillic( ) );
	auto verdanarus_esp_config               = ImFontConfig( );
	verdanarus_esp_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	verdanarus_esp_config.FontDataSize       = Rubik_Regular_compressed_size;
	verdanarus_esp_config.SizePixels         = 12.0f;
	verdanarus_esp_config.PixelSnapH         = true;
	verdanarus_esp_config.OversampleH        = 5;
	verdanarus_esp_config.OversampleV        = 1;
	verdanarus_esp_config.RasterizerMultiply = 1.0f;
	verdanarus_esp_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	verdanarus_esp_font = io.Fonts->AddFontFromMemoryTTF( verdanarus_esp_config.FontData, verdanarus_esp_config.FontDataSize, verdanarus_esp_config.SizePixels, &verdanarus_esp_config,
	                                                     io.Fonts->GetGlyphRangesCyrillic( ) );
	auto doctrine_velocity_config = ImFontConfig( );
	doctrine_velocity_config.FontData = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	doctrine_velocity_config.FontDataSize = Rubik_Regular_compressed_size;
	doctrine_velocity_config.SizePixels = 22.0f;
	doctrine_velocity_config.PixelSnapH = true;
	doctrine_velocity_config.OversampleH = 5;
	doctrine_velocity_config.OversampleV = 1;
	doctrine_velocity_config.RasterizerMultiply = 1.0f;
	doctrine_velocity_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	doctrine_velocity = io.Fonts->AddFontFromMemoryTTF( doctrine_velocity_config.FontData, doctrine_velocity_config.FontDataSize, doctrine_velocity_config.SizePixels, &doctrine_velocity_config,
	                                                     io.Fonts->GetGlyphRangesCyrillic( ) );
	auto tasorbiter_indicators_config = ImFontConfig( );
	tasorbiter_indicators_config.FontData = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	tasorbiter_indicators_config.FontDataSize = Rubik_Regular_compressed_size;
	tasorbiter_indicators_config.SizePixels = 22.0f;
	tasorbiter_indicators_config.PixelSnapH = true;
	tasorbiter_indicators_config.OversampleH = 5;
	tasorbiter_indicators_config.OversampleV = 1;
	tasorbiter_indicators_config.RasterizerMultiply = 1.0f;
	tasorbiter_indicators_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	tasorbiter_indicators = io.Fonts->AddFontFromMemoryTTF( tasorbiter_indicators_config.FontData, tasorbiter_indicators_config.FontDataSize, tasorbiter_indicators_config.SizePixels, &tasorbiter_indicators_config,
	                                                       io.Fonts->GetGlyphRangesCyrillic( ) );
	auto tasorbiter_indicators_small_config = ImFontConfig( );
	tasorbiter_indicators_small_config.FontData = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	tasorbiter_indicators_small_config.FontDataSize = Rubik_Regular_compressed_size;
	tasorbiter_indicators_small_config.SizePixels = 19.0f;
	tasorbiter_indicators_small_config.PixelSnapH = true;
	tasorbiter_indicators_small_config.OversampleH = 5;
	tasorbiter_indicators_small_config.OversampleV = 1;
	tasorbiter_indicators_small_config.RasterizerMultiply = 1.0f;
	tasorbiter_indicators_small_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	tasorbiter_indicators_small = io.Fonts->AddFontFromMemoryTTF( tasorbiter_indicators_small_config.FontData, tasorbiter_indicators_small_config.FontDataSize, tasorbiter_indicators_small_config.SizePixels, &tasorbiter_indicators_small_config,
	                                                             io.Fonts->GetGlyphRangesCyrillic( ) );
	auto tasorbiter_addwindow_config = ImFontConfig( );
	tasorbiter_addwindow_config.FontData = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	tasorbiter_addwindow_config.FontDataSize = Rubik_Regular_compressed_size;
	tasorbiter_addwindow_config.SizePixels = 13.0f;
	tasorbiter_addwindow_config.PixelSnapH = true;
	tasorbiter_addwindow_config.OversampleH = 5;
	tasorbiter_addwindow_config.OversampleV = 1;
	tasorbiter_addwindow_config.RasterizerMultiply = 1.0f;
	tasorbiter_addwindow_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	tasorbiter_addwindow = io.Fonts->AddFontFromMemoryTTF( tasorbiter_addwindow_config.FontData, tasorbiter_addwindow_config.FontDataSize, tasorbiter_addwindow_config.SizePixels, &tasorbiter_addwindow_config,
	                                                      io.Fonts->GetGlyphRangesCyrillic( ) );
	auto doctrine_addwindow_config = ImFontConfig( );
	doctrine_addwindow_config.FontData = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	doctrine_addwindow_config.FontDataSize = Rubik_Regular_compressed_size;
	doctrine_addwindow_config.SizePixels = 12.0f;
	doctrine_addwindow_config.PixelSnapH = true;
	doctrine_addwindow_config.OversampleH = 5;
	doctrine_addwindow_config.OversampleV = 1;
	doctrine_addwindow_config.RasterizerMultiply = 1.0f;
	doctrine_addwindow_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	doctrine_addwindow = io.Fonts->AddFontFromMemoryTTF( doctrine_addwindow_config.FontData, doctrine_addwindow_config.FontDataSize, doctrine_addwindow_config.SizePixels, &doctrine_addwindow_config,
	                                                     io.Fonts->GetGlyphRangesCyrillic( ) );
	auto doctrine_brackets_config = ImFontConfig( );
	doctrine_brackets_config.FontData = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	doctrine_brackets_config.FontDataSize = Rubik_Regular_compressed_size;
	doctrine_brackets_config.SizePixels = 22.0f;
	doctrine_brackets_config.PixelSnapH = true;
	doctrine_brackets_config.OversampleH = 5;
	doctrine_brackets_config.OversampleV = 1;
	doctrine_brackets_config.RasterizerMultiply = 1.0f;
	doctrine_brackets_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	doctrine_brackets = io.Fonts->AddFontFromMemoryTTF( doctrine_brackets_config.FontData, doctrine_brackets_config.FontDataSize, doctrine_brackets_config.SizePixels, &doctrine_brackets_config,
	                                                    io.Fonts->GetGlyphRangesCyrillic( ) );
	noto_sans_syriac = nullptr; 
	auto pts_rubik_light_italic_config               = ImFontConfig( );
	pts_rubik_light_italic_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Italic_compressed_data));
	pts_rubik_light_italic_config.FontDataSize       = Rubik_Italic_compressed_size;
	pts_rubik_light_italic_config.SizePixels         = 18.0f; 
	pts_rubik_light_italic_config.PixelSnapH         = true;
	pts_rubik_light_italic_config.OversampleH        = 5;
	pts_rubik_light_italic_config.OversampleV        = 1;
	pts_rubik_light_italic_config.RasterizerMultiply = 1.0f;
	pts_rubik_light_italic_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	pts_rubik_light_italic = io.Fonts->AddFontFromMemoryTTF( pts_rubik_light_italic_config.FontData, pts_rubik_light_italic_config.FontDataSize, pts_rubik_light_italic_config.SizePixels, &pts_rubik_light_italic_config,
	                                                         io.Fonts->GetGlyphRangesCyrillic( ) );
	if (pts_rubik_light_italic == nullptr) {
		pts_rubik_light_italic = pts_semibold_italic;
	}
	auto rubik_regular_config               = ImFontConfig( );
	rubik_regular_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Regular_compressed_data));
	rubik_regular_config.FontDataSize       = Rubik_Regular_compressed_size;
	rubik_regular_config.SizePixels         = 22.0f; 
	rubik_regular_config.PixelSnapH         = true;
	rubik_regular_config.OversampleH        = 5;
	rubik_regular_config.OversampleV        = 1;
	rubik_regular_config.RasterizerMultiply = 1.0f;
	rubik_regular_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	rubik_regular = io.Fonts->AddFontFromMemoryTTF( rubik_regular_config.FontData, rubik_regular_config.FontDataSize, rubik_regular_config.SizePixels, &rubik_regular_config,
	                                                 io.Fonts->GetGlyphRangesCyrillic( ) );
	auto rubik_italic_config               = ImFontConfig( );
	rubik_italic_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Italic_compressed_data));
	rubik_italic_config.FontDataSize       = Rubik_Italic_compressed_size;
	rubik_italic_config.SizePixels         = 22.0f; 
	rubik_italic_config.PixelSnapH         = true;
	rubik_italic_config.OversampleH        = 5;
	rubik_italic_config.OversampleV        = 1;
	rubik_italic_config.RasterizerMultiply = 1.0f;
	rubik_italic_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	rubik_italic = io.Fonts->AddFontFromMemoryTTF( rubik_italic_config.FontData, rubik_italic_config.FontDataSize, rubik_italic_config.SizePixels, &rubik_italic_config,
	                                                io.Fonts->GetGlyphRangesCyrillic( ) );
	auto barlow_regular_italic_config               = ImFontConfig( );
	barlow_regular_italic_config.FontData           = const_cast<void*>(static_cast<const void*>(Barlow_Regular_compressed_data));
	barlow_regular_italic_config.FontDataSize       = Barlow_Regular_compressed_size;
	barlow_regular_italic_config.SizePixels         = 18.0f; 
	barlow_regular_italic_config.PixelSnapH         = true;
	barlow_regular_italic_config.OversampleH        = 5;
	barlow_regular_italic_config.OversampleV        = 1;
	barlow_regular_italic_config.RasterizerMultiply = 1.0f;
	barlow_regular_italic_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap | ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Oblique; 
	barlow_regular_italic = io.Fonts->AddFontFromMemoryTTF( barlow_regular_italic_config.FontData, barlow_regular_italic_config.FontDataSize, barlow_regular_italic_config.SizePixels, &barlow_regular_italic_config,
	                                                         io.Fonts->GetGlyphRangesCyrillic( ) );
	auto silkscreen_config               = ImFontConfig( );
	silkscreen_config.FontData           = const_cast<void*>(static_cast<const void*>(Silkscreen_compressed_data));
	silkscreen_config.FontDataSize       = Silkscreen_compressed_size;
	silkscreen_config.SizePixels         = 10.0f; 
	silkscreen_config.PixelSnapH         = true;
	silkscreen_config.OversampleH        = 3;
	silkscreen_config.OversampleV        = 3;
	silkscreen_config.RasterizerMultiply = 1.0f;
	silkscreen_config.GlyphExtraSpacing  = ImVec2( -1.0f, 0.0f ); 
	silkscreen_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	silkscreen_font = io.Fonts->AddFontFromMemoryTTF( silkscreen_config.FontData, silkscreen_config.FontDataSize, silkscreen_config.SizePixels, &silkscreen_config,
	                                                   io.Fonts->GetGlyphRangesCyrillic( ) );
	auto rubikcustom_config               = ImFontConfig( );
	rubikcustom_config.FontData           = const_cast<void*>(static_cast<const void*>(Rubik_Italic_compressed_data));
	rubikcustom_config.FontDataSize       = Rubik_Italic_compressed_size;
	rubikcustom_config.SizePixels         = 18.0f;
	rubikcustom_config.PixelSnapH         = true;
	rubikcustom_config.OversampleH        = 5;
	rubikcustom_config.OversampleV        = 1;
	rubikcustom_config.RasterizerMultiply = 1.0f;
	rubikcustom_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	rubikcustom_pts_font = io.Fonts->AddFontFromMemoryTTF( rubikcustom_config.FontData, rubikcustom_config.FontDataSize, rubikcustom_config.SizePixels, &rubikcustom_config,
	                                                        io.Fonts->GetGlyphRangesCyrillic( ) );
	auto rubikcustom_merge_config               = ImFontConfig( );
	rubikcustom_merge_config.FontData           = const_cast<void*>(static_cast<const void*>(RubikCustom_compressed_data));
	rubikcustom_merge_config.FontDataSize       = RubikCustom_compressed_size;
	rubikcustom_merge_config.SizePixels         = 18.0f;
	rubikcustom_merge_config.PixelSnapH         = true;
	rubikcustom_merge_config.OversampleH        = 5;
	rubikcustom_merge_config.OversampleV        = 1;
	rubikcustom_merge_config.RasterizerMultiply = 1.0f;
	rubikcustom_merge_config.FontBuilderFlags   = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bitmap;
	rubikcustom_merge_config.MergeMode          = true; 
	static const ImWchar ranges_seven[] = { 0x37, 0x37, 0 }; 
	io.Fonts->AddFontFromMemoryTTF( rubikcustom_merge_config.FontData, rubikcustom_merge_config.FontDataSize, rubikcustom_merge_config.SizePixels, &rubikcustom_merge_config,
	                                ranges_seven );
	
	// Rubik Italic PTS font (using rubikcustom_pts_font)
	fonteditor_pts_font = rubikcustom_pts_font;
	#endif
}