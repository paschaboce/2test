#pragma once
#pragma comment( lib, "psapi.lib" )
#include <windows.h>
#include <string>
namespace n_reshade
{
	struct impl_t {
		bool is_reshade_loaded( ) const;
		bool is_reshade_menu_open( ) const;
		void set_reshade_menu_state( bool state );
		bool should_skip_rendering( ) const;
		bool is_reshade_call( ) const;
		std::string get_module_name( void* address ) const;
		void force_disable_rendering( bool disable );
		bool is_rendering_disabled( ) const;
		bool m_reshade_menu_open = false;
		bool m_force_disable_rendering = false;
		mutable void* m_reshade_address = nullptr;
		mutable bool m_reshade_detected = false;
	};
}
inline n_reshade::impl_t g_reshade{ };