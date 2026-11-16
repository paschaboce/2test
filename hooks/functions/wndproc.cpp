#include "../hooks.h"

#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../../globals/reshade/reshade.h"
#include "../../hacks/movement_recorder/movement_recorder.h"

long __stdcall n_detoured_functions::wndproc( HWND window, unsigned int message, unsigned int wide_param, long long_param )
{
	if ( g_ctx.m_unloading )
		return CallWindowProcW( g_input.m_old_wnd_proc, window, message, wide_param, long_param );

	g_input.on_wndproc( message, wide_param, long_param );

	// Проверяем клавишу ReShade (HOME)
	if ( message == WM_KEYDOWN && wide_param == VK_DELETE ) {
		// Устанавливаем состояние меню ReShade
		g_reshade.set_reshade_menu_state( true );
		// Пропускаем обработку клавиши для ReShade
		return CallWindowProcW( g_input.m_old_wnd_proc, window, message, wide_param, long_param );
	}

	// Проверяем, не закрыто ли меню ReShade
	if ( message == WM_KEYUP && wide_param == VK_DELETE ) {
		g_reshade.set_reshade_menu_state( false );
	}

	if ( g_input.is_key_released( VK_INSERT ) )
		g_menu.m_opened = !g_menu.m_opened;

	// F5 - принудительно отключить/включить рендеринг для совместимости с ReShade
	if ( g_input.is_key_released( VK_F5 ) ) {
		static bool force_disabled = false;
		force_disabled             = !force_disabled;
		g_reshade.force_disable_rendering( force_disabled );
		g_console.print( force_disabled ? "Rendering disabled for ReShade compatibility" : "Rendering enabled" );
	}

	// Добавляем проверку на валидность интерфейса
	if ( g_interfaces.m_input_system ) {
		g_interfaces.m_input_system->enable_input( !( g_menu.m_opened || g_movement_recorder.m_editor_open ) );
	}

	if ( g_render.m_initialised && ( g_menu.m_opened || g_movement_recorder.m_editor_open ) && ImGui_ImplWin32_WndProcHandler( window, message, wide_param, long_param ) )
		return 1L;

	return CallWindowProcW( g_input.m_old_wnd_proc, window, message, wide_param, long_param );
}