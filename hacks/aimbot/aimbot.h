#pragma once
#include "../lagcomp/lagcomp.h"
#include <array>
class c_base_entity;
namespace n_aimbot
{
	struct impl_t {
		void on_create_move_post( );
		enum hitboxex_form_t {
			hb_head,
			hb_neck,
			hb_body,
			hb_legs,
			max_hitboxes
		};
		struct aimbot_settings_t {
			bool  m_enable{ false };
			bool  m_silent{ false };
			bool  m_visibility_check{ false };
			bool  m_friendly_fire{ false };
			bool  m_recoil_control{ false };
			bool  m_aim_to_backtrack{ false };
			bool  m_hitboxes[ hitboxex_form_t::max_hitboxes ]{ };
			float m_fov{ 0.f };
			float m_smooth{ 1.f };
			// 1.txt: RCS amount как процент 0..100. 50% = полная компенсация
			// (`punch * (rcs_p / 50)` — стандарт). 100% = двойная (overcorrect).
			int   m_rcs_amount{ 50 };
			// 1.txt: autowall min damage + lethal-only (если попадание не
			// убьёт — игнорируем кандидата). Lethal-only имеет приоритет.
			int   m_autowall_min_damage{ 1 };
			bool  m_autowall_lethal_only{ false };
		};
		// 1.txt: 8 weapon-классов. Индексы совпадают с menu::weapon_selection
		// в 1.txt (pistol=0, heavy_pistol=1, shotgun=2, heavy=3, smg=4,
		// rifle=5, sniper=6, auto_sniper=7).
		enum weapon_class_t {
			wc_pistol,
			wc_heavy_pistol,
			wc_shotgun,
			wc_heavy,
			wc_smg,
			wc_rifle,
			wc_sniper,
			wc_auto_sniper,
			wc_max
		};
		std::array< aimbot_settings_t, weapon_class_t::wc_max > m_aimbot_settings{ };
		// 1.txt: menu::weapon_selection — индекс оружия в меню. При смене оружия
		// в игре (когда меню закрыто) автоматически переключается на текущий
		// weapon-class — UX из 1.txt.
		int m_menu_weapon_selection{ 0 };
		bool save( );
		bool load( );
		aimbot_settings_t get_aimbot_settings( );
		// 1.txt is_xxx() helpers по item_definition_index.
		static bool is_knife( c_base_entity* weapon );
		static bool is_pistol( c_base_entity* weapon );
		static bool is_heavy_pistol( c_base_entity* weapon );
		static bool is_shotgun( c_base_entity* weapon );
		static bool is_heavy( c_base_entity* weapon );
		static bool is_smg( c_base_entity* weapon );
		static bool is_rifle( c_base_entity* weapon );
		static bool is_sniper( c_base_entity* weapon );
		static bool is_auto_sniper( c_base_entity* weapon );
	private:
		c_base_entity* find_closest_player( );
		bool can_aimbot( );
	};
}
inline n_aimbot::impl_t g_aimbot{ };