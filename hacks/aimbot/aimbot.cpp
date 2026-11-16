#include "aimbot.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../auto_wall/auto_wall.h"
#include "../entity_cache/entity_cache.h"
#include "../prediction/prediction.h"
#include <cfloat>
#include <initializer_list>
#include "../chat_hud.h"
bool n_aimbot::impl_t::save( )
{
	return true;
}
bool n_aimbot::impl_t::load( )
{
	return true;
}

// 1.txt: проверка по item_definition_index из массива id-шников.
namespace {
	inline bool weapon_in( c_base_entity* w, std::initializer_list< short > ids )
	{
		if ( !w )
			return false;
		const short id = w->get_item_definition_index( );
		for ( auto v : ids )
			if ( v == id )
				return true;
		return false;
	}
}

bool n_aimbot::impl_t::is_knife( c_base_entity* w )
{
	return weapon_in( w, {
		weapon_knife, weapon_knife_gg, weapon_knife_t, weapon_knife_butterfly,
		weapon_knife_falchion, weapon_knife_flip, weapon_knife_ghost, weapon_knife_gut,
		weapon_knife_gypsy_jackknife, weapon_knife_karambit, weapon_knife_m9_bayonet,
		weapon_knife_push, weapon_knife_stiletto, weapon_knife_tactical, weapon_knife_ursus,
		weapon_knife_widowmaker, weapon_knife_bayonet, weapon_knife_css, weapon_knife_survival_bowie,
		weapon_knife_cord, weapon_knife_canis, weapon_knife_outdoor, weapon_knife_skeleton
	} );
}
bool n_aimbot::impl_t::is_pistol( c_base_entity* w )
{
	return weapon_in( w, { weapon_glock, weapon_elite, weapon_p250, weapon_tec9,
	                       weapon_cz75a, weapon_usp_silencer, weapon_hkp2000, weapon_fiveseven } );
}
bool n_aimbot::impl_t::is_heavy_pistol( c_base_entity* w )
{
	return weapon_in( w, { weapon_deagle, weapon_revolver } );
}
bool n_aimbot::impl_t::is_shotgun( c_base_entity* w )
{
	return weapon_in( w, { weapon_nova, weapon_xm1014, weapon_sawedoff, weapon_mag7 } );
}
bool n_aimbot::impl_t::is_heavy( c_base_entity* w )
{
	return weapon_in( w, { weapon_m249, weapon_negev } );
}
bool n_aimbot::impl_t::is_smg( c_base_entity* w )
{
	return weapon_in( w, { weapon_mac10, weapon_mp7, weapon_ump45, weapon_p90,
	                       weapon_bizon, weapon_mp9, weapon_mp5sd } );
}
bool n_aimbot::impl_t::is_rifle( c_base_entity* w )
{
	return weapon_in( w, { weapon_ak47, weapon_aug, weapon_famas, weapon_galilar,
	                       weapon_m4a1, weapon_m4a1_silencer, weapon_sg556 } );
}
bool n_aimbot::impl_t::is_sniper( c_base_entity* w )
{
	return weapon_in( w, { weapon_awp, weapon_ssg08 } );
}
bool n_aimbot::impl_t::is_auto_sniper( c_base_entity* w )
{
	return weapon_in( w, { weapon_g3sg1, weapon_scar20 } );
}
c_base_entity* n_aimbot::impl_t::find_closest_player( )
{
	float closest_fov             = 180.f;
	c_base_entity* closest_player = nullptr;
	const auto eye_position = g_ctx.m_local->get_eye_position( false );
	g_entity_cache.enumerate( e_enumeration_type::type_players, [ & ]( c_base_entity* entity ) {
		if ( !entity || !entity->is_alive( ) || entity->is_dormant( ) || ( !g_ctx.m_local->is_enemy( entity ) && !GET_VARIABLE( g_variables.m_aimbot_friendly_fire, bool ) ) || entity == g_ctx.m_local || g_menu.is_server_player_aimbot_whitelisted( entity->get_index( ) ) )
			return;
		const auto hitbox_position = entity->get_hitbox_position( hitbox_head );
		auto save_tick_count       = g_ctx.m_cmd->m_tick_count;
	   {
			fire_bullet_data_t data = { };
			if ( g_auto_wall.get_damage( hitbox_position, &data ) < 0.f )
				return;
		}
		auto angle = g_math.calculate_angle( eye_position, hitbox_position );
		angle -= ( g_ctx.m_local->get_punch( ) * g_convars[ HASH_BT( "weapon_recoil_scale" ) ]->get_float( ) );
		if ( float calculated_fov = g_math.calculate_fov( g_ctx.m_cmd->m_view_point, angle ); calculated_fov < closest_fov ) {
			closest_fov    = calculated_fov;
			closest_player = entity;
		}
	} );
	return closest_player;
}
n_aimbot::impl_t::aimbot_settings_t n_aimbot::impl_t::get_aimbot_settings( )
{
	// 1.txt: маппинг item_definition_index → weapon-class slot.
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return { };
	const auto weapon = g_interfaces.m_client_entity_list->get< c_base_entity >( g_ctx.m_local->get_active_weapon_handle( ) );
	if ( !weapon )
		return { };

	if ( is_pistol( weapon ) )       return m_aimbot_settings[ wc_pistol ];
	if ( is_heavy_pistol( weapon ) ) return m_aimbot_settings[ wc_heavy_pistol ];
	if ( is_shotgun( weapon ) )      return m_aimbot_settings[ wc_shotgun ];
	if ( is_heavy( weapon ) )        return m_aimbot_settings[ wc_heavy ];
	if ( is_smg( weapon ) )          return m_aimbot_settings[ wc_smg ];
	if ( is_rifle( weapon ) )        return m_aimbot_settings[ wc_rifle ];
	if ( is_sniper( weapon ) )       return m_aimbot_settings[ wc_sniper ];
	if ( is_auto_sniper( weapon ) )  return m_aimbot_settings[ wc_auto_sniper ];

	// 1.txt: если не один из 8 классов (knife/grenade/c4) — settings disabled.
	return { };
}
bool n_aimbot::impl_t::can_aimbot( )
{
	return g_ctx.m_cmd->m_tick_count != 0 && g_ctx.m_cmd->m_buttons & in_attack;
}
void n_aimbot::impl_t::on_create_move_post( )
{
	// Порт логики из 1.txt: panic-key, hold-key, RCS %, autowall thresholds.
	// Без naive 2-tick prediction (он перелетал и ломал попадания),
	// backtrack через historic bone matrices.

	// --- Auto-sync menu weapon-selection. 1.txt: при смене оружия в игре
	// (когда меню закрыто) UI автоматически переключается на текущий слот.
	if ( !g_menu.m_opened && g_ctx.m_local && g_ctx.m_local->is_alive( ) ) {
		if ( const auto active = g_interfaces.m_client_entity_list->get< c_base_entity >( g_ctx.m_local->get_active_weapon_handle( ) ) ) {
			if ( is_pistol( active ) )            m_menu_weapon_selection = wc_pistol;
			else if ( is_heavy_pistol( active ) ) m_menu_weapon_selection = wc_heavy_pistol;
			else if ( is_shotgun( active ) )      m_menu_weapon_selection = wc_shotgun;
			else if ( is_heavy( active ) )        m_menu_weapon_selection = wc_heavy;
			else if ( is_smg( active ) )          m_menu_weapon_selection = wc_smg;
			else if ( is_rifle( active ) )        m_menu_weapon_selection = wc_rifle;
			else if ( is_sniper( active ) )       m_menu_weapon_selection = wc_sniper;
			else if ( is_auto_sniper( active ) )  m_menu_weapon_selection = wc_auto_sniper;
		}
	}

	// --- Panic key (toggle с notify в чат). 1.txt: katai-флаг.
	{
		static bool aimbot_disabled_by_panic = false;
		static bool prev_panic_held          = false;

		const auto& panic_bind = GET_VARIABLE( g_variables.m_aimbot_panic_key, key_bind_t );
		if ( panic_bind.m_key != 0 ) {
			const bool panic_held = g_input.check_input( const_cast< key_bind_t* >( &panic_bind ) );
			if ( panic_held && !prev_panic_held ) {
				aimbot_disabled_by_panic = !aimbot_disabled_by_panic;
				if ( n_interfaces::chat_element ) {
					n_interfaces::chat_element->chatprintf(
						0, 0,
						make_kami_chat_message( GET_VARIABLE( g_variables.m_accent, c_color ), aimbot_disabled_by_panic ? "aimbot is off" : "aimbot is on" )
							.c_str( ) );
				}
			}
			prev_panic_held = panic_held;
		}

		if ( aimbot_disabled_by_panic )
			return;
	}

	const auto& aimbot_bind = GET_VARIABLE( g_variables.m_aimbot_key, key_bind_t );
	const bool aimbot_key_held = g_input.check_input( const_cast< key_bind_t* >( &aimbot_bind ) );
	static bool was_aimbot_key_held = false;

	if ( aimbot_bind.m_key != 0 ) {
		if ( aimbot_key_held && !was_aimbot_key_held ) {
			if ( GET_VARIABLE( g_variables.m_aimbot_nospread, bool ) )
				g_interfaces.m_engine_client->execute_client_cmd( "sm_nospread" );
			if ( GET_VARIABLE( g_variables.m_aimbot_friendly_fire, bool ) )
				g_interfaces.m_engine_client->execute_client_cmd( "sm_ff" );
		} else if ( !aimbot_key_held && was_aimbot_key_held ) {
			if ( GET_VARIABLE( g_variables.m_aimbot_nospread, bool ) )
				g_interfaces.m_engine_client->execute_client_cmd( "sm_nospread" );
			if ( GET_VARIABLE( g_variables.m_aimbot_friendly_fire, bool ) )
				g_interfaces.m_engine_client->execute_client_cmd( "sm_ff" );
		}
		was_aimbot_key_held = aimbot_key_held;
	}

	// --- Hold-key. 1.txt: `if (!GetAsyncKeyState(c::aimbot::aimbot_key)) return;`
	// Если bind пустой — check_input возвращает true, и аим работает как раньше (всегда при IN_ATTACK).
	if ( !aimbot_key_held )
		return;

	if ( !can_aimbot( ) )
		return;
	const auto weapon = g_interfaces.m_client_entity_list->get< c_base_entity >( g_ctx.m_local->get_active_weapon_handle( ) );
	if ( !weapon )
		return;
	const auto weapon_data = g_interfaces.m_weapon_system->get_weapon_data( weapon->get_item_definition_index( ) );
	if ( !weapon_data )
		return;
	if ( !weapon_data->is_gun( ) )
		return;
	if ( !( g_ctx.m_local->can_shoot( weapon ) ) )
		return;
	aimbot_settings_t settings = get_aimbot_settings( );
	if ( !settings.m_enable )
		return;

	const auto  eye_position = g_ctx.m_local->get_eye_position( false );
	const float recoil_scale = g_convars[ HASH_BT( "weapon_recoil_scale" ) ]->get_float( );
	const auto  raw_punch    = g_ctx.m_local->get_punch( );
	// 1.txt RCS: `aim_punch_angle * (rcs_p / 50)` — 50% = full compensation.
	const float rcs_factor   = settings.m_recoil_control ? ( static_cast< float >( settings.m_rcs_amount ) / 50.f ) : 0.f;
	const c_angle effective_punch = ( raw_punch * recoil_scale ) * rcs_factor;
	const bool   bt_enabled  = settings.m_aim_to_backtrack && GET_VARIABLE( g_variables.m_backtrack_enable, bool );
	auto can_see_target_point = [ & ]( c_base_entity* entity, const c_vector& point ) -> bool {
		if ( !entity )
			return false;

		trace_t trace{ };
		c_trace_filter filter( g_ctx.m_local );
		ray_t ray( eye_position, point );
		g_interfaces.m_engine_trace->trace_ray( ray, mask_shot | contents_grate, &filter, &trace );
		return trace.m_hit_entity == entity || trace.m_fraction > 0.97f;
	};

	auto autowall_can_hit_target = [ & ]( c_base_entity* entity, const c_vector& point, float& damage_out ) -> bool {
		damage_out = -1.0f;
		if ( !entity )
			return false;

		fire_bullet_data_t data{ };
		damage_out = g_auto_wall.get_damage( point, &data );
		return damage_out > 0.0f && data.m_enter_trace.m_hit_entity == entity;
	};

	// Хитбоксы с приоритетом (head → neck → chest → legs).
	std::array< int, 4 > hitbox_ids{ };
	int hitbox_count = 0;
	if ( settings.m_hitboxes[ hb_head ] ) hitbox_ids[ hitbox_count++ ] = hitbox_head;
	if ( settings.m_hitboxes[ hb_neck ] ) hitbox_ids[ hitbox_count++ ] = hitbox_neck;
	if ( settings.m_hitboxes[ hb_body ] ) hitbox_ids[ hitbox_count++ ] = hitbox_chest;
	if ( settings.m_hitboxes[ hb_legs ] ) hitbox_ids[ hitbox_count++ ] = hitbox_left_foot;
	if ( hitbox_count == 0 )
		hitbox_ids[ hitbox_count++ ] = hitbox_head;

	float    best_fov        = settings.m_fov;
	c_vector best_target_pos = c_vector( 0.f, 0.f, 0.f );
	int      best_cmd_tick   = -1;
	bool     found           = false;

	g_entity_cache.enumerate( e_enumeration_type::type_players, [ & ]( c_base_entity* entity ) {
		if ( !entity || !entity->is_alive( ) || entity->is_dormant( ) || ( !g_ctx.m_local->is_enemy( entity ) && !GET_VARIABLE( g_variables.m_aimbot_friendly_fire, bool ) ) || entity == g_ctx.m_local ||
		     g_menu.is_server_player_aimbot_whitelisted( entity->get_index( ) ) )
			return;
		// 1.txt: пропускаем игроков с gun-game/spawn immunity (DM/арена-моды).
		if ( entity->has_gun_game_immunity( ) )
			return;

		const int entity_index = entity->get_index( );

		for ( int hb_idx = 0; hb_idx < hitbox_count; ++hb_idx ) {
			const int hb_id = hitbox_ids[ hb_idx ];

			// --- Live-position проверка (как в 1.txt — без prediction).
			c_vector hitbox_position = entity->get_hitbox_position( hb_id );
			c_angle  angle           = g_math.calculate_angle( eye_position, hitbox_position );
			angle -= effective_punch;
			float fov = g_math.calculate_fov( g_ctx.m_cmd->m_view_point, angle );

			bool reachable = false;
			if ( fov <= settings.m_fov ) {
				if ( settings.m_visibility_check ) {
					reachable = can_see_target_point( entity, hitbox_position );
				} else {
					float damage = -1.0f;
					const bool damage_hits_target = autowall_can_hit_target( entity, hitbox_position, damage );
					// 1.txt autowall: lethal-only > min damage threshold > damage > 0.
					if ( settings.m_autowall_lethal_only )
						reachable = damage_hits_target && damage >= static_cast< float >( entity->get_health( ) );
					else
						reachable = damage_hits_target && damage >= static_cast< float >( settings.m_autowall_min_damage );
				}
			}

			int hit_bt_ticks = -1;

			// --- Backtrack pass: если live недоступен, перебираем historic
			// записи и берём historic hitbox-position через перегрузку
			// get_hitbox_position(id, matrix).
			if ( !reachable && bt_enabled && entity_index >= 0 && entity_index < 65 ) {
				auto& recs = Backtrack::records[ entity_index ];
				if ( !recs.empty( ) ) {
					float    bt_best_fov  = FLT_MAX;
					c_vector bt_best_pos  = c_vector( 0.f, 0.f, 0.f );
					int      bt_best_tick = -1;

					for ( auto& rec : recs ) {
						if ( !Backtrack::valid( rec.simulationTime ) )
							continue;

						// Historic hitbox position через записанную bone-матрицу.
						c_vector pos = entity->get_hitbox_position( hb_id, rec.matrix );

						c_angle a = g_math.calculate_angle( eye_position, pos );
						a -= effective_punch;
						const float f = g_math.calculate_fov( g_ctx.m_cmd->m_view_point, a );

						if ( f > settings.m_fov || f >= bt_best_fov )
							continue;

						bool bt_ok = false;
						if ( settings.m_visibility_check ) {
							bt_ok = can_see_target_point( entity, pos );
						} else {
							float bt_dmg = -1.0f;
							const bool bt_damage_hits_target = autowall_can_hit_target( entity, pos, bt_dmg );
							if ( settings.m_autowall_lethal_only )
								bt_ok = bt_damage_hits_target && bt_dmg >= static_cast< float >( entity->get_health( ) );
							else
								bt_ok = bt_damage_hits_target && bt_dmg >= static_cast< float >( settings.m_autowall_min_damage );
						}
						if ( !bt_ok )
							continue;

						bt_best_fov  = f;
						bt_best_pos  = pos;
						bt_best_tick = Backtrack::timeToTicks( rec.simulationTime + Backtrack::getLerp( ) );
					}

					if ( bt_best_tick != -1 ) {
						hitbox_position = bt_best_pos;
						fov             = bt_best_fov;
						hit_bt_ticks    = bt_best_tick;
						reachable       = true;
					}
				}
			}

			if ( !reachable )
				continue;

			// Min-FOV селекция (как в 1.txt — никаких score-multipliers).
			if ( fov < best_fov ) {
				best_fov        = fov;
				best_target_pos = hitbox_position;
				best_cmd_tick   = hit_bt_ticks;
				found           = true;
			}
		}
	} );

	if ( !found )
		return;

	// Финальный угол в цель + recoil-compensation.
	c_angle target_angle = g_math.calculate_angle( eye_position, best_target_pos );
	target_angle -= effective_punch;

	c_angle delta = target_angle - g_ctx.m_cmd->m_view_point;
	delta.normalize( );

	if ( settings.m_smooth > 1.f ) {
		delta.m_x /= settings.m_smooth;
		delta.m_y /= settings.m_smooth;
		delta.m_z /= settings.m_smooth;
	}

	if ( best_cmd_tick != -1 )
		g_ctx.m_cmd->m_tick_count = best_cmd_tick;

	g_ctx.m_cmd->m_view_point += delta;
	g_ctx.m_cmd->m_view_point.normalize( );

	if ( !settings.m_silent )
		g_interfaces.m_engine_client->set_view_angles( g_ctx.m_cmd->m_view_point );
}
