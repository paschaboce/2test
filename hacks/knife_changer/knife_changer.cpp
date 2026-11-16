#include "knife_changer.h"

#include "../../globals/includes/includes.h"
#include "../../game/sdk/includes/includes.h"

namespace {
	constexpr int WEAPON_KNIFE_CT = 42;
	constexpr int WEAPON_KNIFE_T  = 59;
	constexpr int ITEM_QUALITY_KNIFE = 3;

	bool is_knife_definition( int def_index )
	{
		return def_index == WEAPON_KNIFE_CT || def_index == WEAPON_KNIFE_T ||
		       ( def_index >= 500 && def_index < 525 );
	}

	int find_model_index_for_definition( int definition_index )
	{
		const auto it = g_skinchanger_weapon_info.find( definition_index );
		if ( it == g_skinchanger_weapon_info.end( ) || !it->second.model )
			return -1;
		return g_interfaces.m_model_info->get_model_index( it->second.model );
	}

	int resolve_knife_def_for_index( int list_index )
	{
		if ( list_index <= 0 || list_index >= static_cast< int >( g_skinchanger_knife_names.size( ) ) )
			return 0;
		return g_skinchanger_knife_names[ list_index ].definition_index;
	}
}

int n_knife_changer::impl_t::get_override_definition_for_team( int team ) const
{
	if ( !GET_VARIABLE( g_variables.m_knife_changer_enabled, bool ) )
		return 0;

	// team 2 = T, team 3 = CT
	const int list_index = ( team == 2 )
		? GET_VARIABLE( g_variables.m_knife_changer_t_index, int )
		: GET_VARIABLE( g_variables.m_knife_changer_ct_index, int );
	return resolve_knife_def_for_index( list_index );
}

bool n_knife_changer::impl_t::has_any_override( ) const
{
	if ( !GET_VARIABLE( g_variables.m_knife_changer_enabled, bool ) )
		return false;
	return resolve_knife_def_for_index( GET_VARIABLE( g_variables.m_knife_changer_t_index, int ) ) > 0 ||
	       resolve_knife_def_for_index( GET_VARIABLE( g_variables.m_knife_changer_ct_index, int ) ) > 0;
}

void n_knife_changer::impl_t::request_full_update( )
{
	// IMPORTANT: do NOT set m_delta_tick = -1 here. That forces the server to
	// resend a full snapshot of the world in one burst -> several hundred ms
	// freeze ("self-ddos"). m_bNeedFullUpdate alone is enough; the existing
	// inventory_changer frame hook picks it up safely.
	g_ctx.m_bNeedFullUpdate = true;
}

void n_knife_changer::impl_t::on_frame_stage_notify( int stage )
{
	if ( !GET_VARIABLE( g_variables.m_knife_changer_enabled, bool ) )
		return;

	if ( stage != net_update_postdataupdate_start )
		return;

	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return;

	const int local_team = g_ctx.m_local->get_team( );
	const int override_def = get_override_definition_for_team( local_team );
	if ( override_def <= 0 )
		return;

	player_info_t info{ };
	if ( !g_interfaces.m_engine_client->get_player_info( g_ctx.m_local->get_index( ), &info ) )
		return;

	const int override_model_index = find_model_index_for_definition( override_def );
	if ( override_model_index <= 0 )
		return;

	const bool anim_fix = GET_VARIABLE( g_variables.m_knife_changer_anim_fix, bool );

	// 1) Owned weapons — rewrite only knife entities and ONLY when the current
	// definition differs from our override. Doing this every tick caused the
	// "laggy/frozen weapon" feeling; stale-guard prevents that.
	//
	// anim_fix mode: do NOT touch m_iItemDefinitionIndex on the weapon entity.
	// Some knives (Butterfly, Karambit, Talon, Nomad) have unique
	// inspect / idle_special sequences. When a server-side skinchanger plugin
	// has already set the def and we rewrite it on top, the engine's current
	// m_nSequence references a slot that doesn't exist on the new model →
	// T-pose or wrong animation. In this mode we only swap visual models.
	auto* weapons = g_ctx.m_local->get_weapons_handle( );
	if ( weapons && !anim_fix ) {
		for ( int i = 0; weapons[ i ] != 0xFFFFFFFF; ++i ) {
			auto* weapon = static_cast< c_base_entity* >(
				g_interfaces.m_client_entity_list->get_client_entity_from_handle( weapons[ i ] ) );
			if ( !weapon )
				continue;

			auto& def_index = weapon->get_item_definition_index( );
			if ( !is_knife_definition( def_index ) )
				continue;
			if ( def_index == static_cast< short >( override_def ) )
				continue; // already patched — skip to avoid re-triggering engine paths every net tick

			// clone / fake ownership so server accepts the visual swap
			weapon->get_item_id_high( )   = -1;
			weapon->get_account_id( )     = static_cast< int >( info.m_xuid_low );
			weapon->get_item_id_low( )    = weapon->m_nItemID( );
			weapon->get_entity_quality( ) = ITEM_QUALITY_KNIFE;
			weapon->get_fall_back_wear( ) = 0.0001f;

			def_index            = static_cast< short >( override_def );
			weapon->m_nItemID( ) = static_cast< int16_t >( override_def );
			weapon->set_model_index( override_model_index );
			// NOTE: do NOT call pre_data_update(0) — the engine owns that call.
			// Invoking it here resets interpolation on the weapon every tick.
		}
	}

	// 2) View model (first person) + world model (third person in mirrors etc.)
	auto* view_model = static_cast< c_base_entity* >(
		g_interfaces.m_client_entity_list->get_client_entity_from_handle( g_ctx.m_local->get_view_model_handle( ) ) );
	if ( !view_model )
		return;

	auto* view_weapon = static_cast< c_base_entity* >(
		g_interfaces.m_client_entity_list->get_client_entity_from_handle( view_model->get_weapon_handle( ) ) );
	if ( !view_weapon )
		return;

	if ( !is_knife_definition( view_weapon->get_item_definition_index( ) ) )
		return;

	if ( view_model->get_model_index( ) != override_model_index )
		view_model->get_model_index( ) = override_model_index;

	if ( auto* world_model = static_cast< c_base_entity* >(
			g_interfaces.m_client_entity_list->get_client_entity_from_handle( view_weapon->m_hWeaponWorldModel( ) ) ) ) {
		const int world_target = override_model_index + 1;
		if ( world_model->get_model_index( ) != world_target )
			world_model->get_model_index( ) = world_target;
	}
}
