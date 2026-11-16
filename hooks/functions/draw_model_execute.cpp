#include "../../game/sdk/includes/includes.h"
#include "../hooks.h"
#include "../../dependencies/imgui/imgui.h"
#include "../../globals/config/variables.h"
#include "../../globals/globals.h"
#include "../../globals/interfaces/interfaces.h"
#include "../../hacks/lagcomp/lagcomp.h"
#include "../../utilities/console/console.h"
#include <array>
#include <algorithm>
#include <fstream>
#include <iosfwd>
auto create_material = [ & ]( const char* material_name, const char* shader, const char* material ) {
	c_key_values* key_values = new c_key_values( shader );
	key_values->load_from_buffer( material_name, material );
	return g_interfaces.m_material_system->create_material( material_name, key_values );
};
c_material* return_material( const int mat_index )
{
	switch ( mat_index ) {
	case 0:
		return g_interfaces.m_material_system->find_material( "debug/debugdrawflat" );
	case 1:
		return g_interfaces.m_material_system->find_material( "debug/debugambientcube", TEXTURE_GROUP_MODEL );
	case 2:
		return create_material( "glow_material", "VertexLitGeneric", R"("VertexLitGeneric" 
        {
            "$additive" "1"
            "$envmap" "models/effects/cube_white"
            "$envmaptint" "[1 1 1]"
            "$envmapfresnel" "1"
            "$envmapfresnelminmaxexp" "[0 1 2]"
            "$alpha" "0.2"
        }
        )" );
	case 3:
		return g_interfaces.m_material_system->find_material( "debug/debugdrawflat" );
	case 4:
		return create_material( "crystal_material", "VertexLitGeneric", R"("VertexLitGeneric"
        {
            "$basetexture" "vgui/white"
            "$model" "1"
            "$flat" "0"
            "$nocull" "0"
            "$nofog" "1"
            "$ignorez" "0"
            "$znearer" "0"
            "$halflambert" "1"
            "$phong" "1"
            "$phongboost" "1.35"
            "$phongfresnelranges" "[0.15 0.55 1]"
            "$envmap" "env_cubemap"
            "$envmapfresnel" "1"
            "$envmapfresnelminmaxexp" "[0 1 1.75]"
            "$envmapcontrast" "1"
            "$selfillum" "1"
            "$selfillumfresnel" "1"
            "$selfillumfresnelminmaxexp" "[0.2 0.35 0.6]"
            "$alpha" "0.82"
        }
        )" );
	case 5:
		return create_material( "metallic_material", "UnlitGeneric", R"("UnlitGeneric" {
            "$basetexture" "vgui/white"
            "$ignorez"      "0"
            "$envmap"       "env_cubemap"
            "$nofog"        "1"
            "$model"        "1"
            "$nocull"       "0"
            "$selfillum"    "1"
            "$halflambert"  "1"
            "$znearer"      "0"
            "$flat"         "1"
            "$rimlight"     "1"
            "$rimlightexponent"  "2"
            "$rimlightboost"     ".2"    
            "$reflectivity" "[1 1 1]"
        }
        )" );
	case 6:
		return create_material( "energy_material", "VertexLitGeneric", R"("VertexLitGeneric"
        {
            "$basetexture"               "vgui/white"
            "$model"                     "1"
            "$additive"                  "1"
            "$nofog"                     "1"
            "$ignorez"                   "0"
            "$znearer"                   "0"
            "$nocull"                    "0"
            "$halflambert"               "1"
            "$envmap"                    "models/effects/cube_white"
            "$envmaptint"                "[1 1 1]"
            "$envmapfresnel"             "1"
            "$envmapfresnelminmaxexp"    "[0 5 2]"
            "$envmapcontrast"            "1"
            "$envmapsaturation"          "1"
            "$selfillum"                 "1"
            "$selfillumfresnel"          "1"
            "$selfillumfresnelminmaxexp" "[-0.5 1 2.5]"
            "$selfillumtint"             "[1 1 1]"
            "$phong"                     "1"
            "$phongboost"                "2"
            "$phongfresnelranges"        "[0.15 0.8 1]"
            "$rimlight"                  "1"
            "$rimlightexponent"          "1.5"
            "$rimlightboost"             "5"
            "$alpha"                     "0.55"
        }
        )" );
	case 7:
		return create_material( "osiris_animated_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$envmap" "editor/cube_vertigo"
            "$envmapcontrast" "1"
            "$basetexture" "dev/zone_warning"
            "$envmaptint" "[0.7 0.7 0.7]"
            "proxies" { "texturescroll" { "texturescrollvar" "$basetexturetransform" "texturescrollrate" "0.6" "texturescrollangle" "90" } }
        })" );
	case 8:
		return create_material( "osiris_platinum_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$basetexture" "models/player/ct_fbi/ct_fbi_glass"
            "$envmap" "env_cubemap"
            "$envmaptint" "[0.4 0.6 0.7]"
        })" );
	case 9:
		return create_material( "osiris_glass_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$basetexture" "detail/dt_metal1"
            "$additive" "1"
            "$envmap" "editor/cube_vertigo"
            "$color" "[0.05 0.05 0.05]"
        })" );
	case 10:
		return create_material( "osiris_chrome_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$envmap" "env_cubemap"
        })" );
	case 11:
		return create_material( "osiris_silver_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$basetexture" "white"
            "$bumpmap" "effects/flat_normal"
            "$envmap" "editor/cube_vertigo"
            "$envmapfresnel" ".6"
            "$phong" "1"
            "$phongboost" "2"
            "$phongexponent" "8"
            "$color2" "[0.05 0.05 0.05]"
            "$envmaptint" "[0.2 0.2 0.2]"
            "$phongfresnelranges" "[0.7 0.8 1]"
            "$phongtint" "[0.8 0.9 1]"
        })" );
	case 12:
		return create_material( "osiris_gold_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$basetexture" "white"
            "$bumpmap" "effects/flat_normal"
            "$envmap" "editor/cube_vertigo"
            "$envmapfresnel" ".6"
            "$phong" "1"
            "$phongboost" "6"
            "$phongexponent" "128"
            "$phongdisablehalflambert" "1"
            "$color2" "[0.18 0.15 0.06]"
            "$envmaptint" "[0.6 0.5 0.2]"
            "$phongfresnelranges" "[0.7 0.8 1]"
            "$phongtint" "[0.6 0.5 0.2]"
        })" );
	case 13:
		return create_material( "osiris_plastic_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$basetexture" "black"
            "$bumpmap" "models/inventory_items/trophy_majors/matte_metal_normal"
            "$additive" "1"
            "$envmap" "editor/cube_vertigo"
            "$envmapfresnel" "1"
            "$normalmapalphaenvmapmask" "1"
            "$phong" "1"
            "$phongboost" "20"
            "$phongexponent" "3000"
            "$phongdisablehalflambert" "1"
            "$phongfresnelranges" "[0.1 0.4 1]"
            "$phongtint" "[0.8 0.9 1]"
        })" );
	case 14:
		return create_material( "osiris_pearlescent_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$basetexture" "vgui/white_additive"
            "$ambientonly" "1"
            "$phong" "1"
            "$pearlescent" "3"
            "$basemapalphaphongmask" "1"
        })" );
	case 15:
		return create_material( "osiris_smoke_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$basetexture" "models/weapons/customization/paints/anodized_multi/smoke"
            "$nofog" "1"
            "$envmap" "env_cubemap"
            "$envmaptint" "[1 1 1]"
            "$phong" "1"
            "$basemapalphaphongmask" "1"
            "$rimlight" "1"
            "$selfillum" "1"
            "proxies" { "texturescroll" { "texturescrollvar" "$basetexturetransform" "texturescrollrate" "0.025" } }
        })" );
	case 16:
		return create_material( "osiris_smoke2_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$basetexture" "models/weapons/customization/paints/anodized_multi/smoke2"
            "$ambientonly" "1"
            "$phong" "1"
            "$pearlescent" "3"
            "$basemapalphaphongmask" "1"
            "proxies" { "texturescroll" { "texturescrollvar" "$basetexturetransform" "texturescrollrate" "0.025" } }
        })" );
	case 17:
		return create_material( "osiris_leaves_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$basetexture" "models/props_foliage/urban_tree03_branches"
            "$translucent" "1"
        })" );
	case 18:
		return create_material( "osiris_lines_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$basetexture" "models/inventory_items/music_kit/darude_01/mp3_detail"
            "$additive" "1"
            "proxies" { "texturescroll" { "texturescrollvar" "$basetexturetransform" "texturescrollrate" "0.25" "texturescrollangle" "90" } }
        })" );
	case 19:
		return create_material( "osiris_snowflakes_material", "VertexLitGeneric", R"("VertexLitGeneric" {
            "$basetexture" "dev/snowfield"
            "$additive" "1"
            "proxies" { "texturescroll" { "texturescrollvar" "$basetexturetransform" "texturescrollrate" "0.1" } }
        })" );
	default:
		return g_interfaces.m_material_system->find_material( "debug/debugdrawflat" );
	}
	g_console.print( std::format( "return_material failed at index {}", mat_index ).c_str( ) );
	return nullptr;
}
void __fastcall n_detoured_functions::draw_model_execute( void* ecx, void* edx, void* context, void* state, model_render_info_t& info,
                                                          matrix3x4_t* custom_bone_to_world )
{
	static auto original = g_hooks.m_draw_model_execute.get_original< decltype( &n_detoured_functions::draw_model_execute ) >( );
	static c_material* lag_material{ };
	static c_material* vis_material{ };
	static c_material* invis_material{ };
	static std::array< c_material*, 20 > cached_materials{ };
	auto get_cached_material = []( int material_index ) -> c_material* {
		material_index = std::clamp( material_index, 0, 19 );
		if ( !cached_materials[ material_index ] ) {
			cached_materials[ material_index ] = return_material( material_index );
			if ( cached_materials[ material_index ] )
				cached_materials[ material_index ]->increment_reference_count( );
		}
		return cached_materials[ material_index ];
	};
	auto draw_chams = [&]( int material_index, const c_color& color, bool ignore_z, bool wireframe, bool cover, matrix3x4_t* matrix ) -> bool {
		c_material* material = get_cached_material( material_index );
		if ( !material )
			return false;

		if ( cover )
			original( g_interfaces.m_model_render, edx, context, state, info, matrix ? matrix : custom_bone_to_world );
		material->color_modulate( color.base< color_type_r >( ), color.base< color_type_g >( ), color.base< color_type_b >( ) );
		material->alpha_modulate( color.base< color_type_a >( ) );
		material->set_material_var_flag( material_var_nofog, true );
		material->set_material_var_flag( material_var_ignorez, ignore_z );
		material->set_material_var_flag( material_var_znearer, ignore_z );
		material->set_material_var_flag( material_var_wireframe, wireframe || material_index == 3 );
		bool found = false;
		if ( const auto material_var = material->find_var( "$envmaptint", &found ); found ) {
			material_var->set_vector( color.base< e_color_type::color_type_r >( ), color.base< e_color_type::color_type_g >( ),
			                          color.base< e_color_type::color_type_b >( ) );
		}
		g_interfaces.m_model_render->forced_material_override( material );
		original( g_interfaces.m_model_render, edx, context, state, info, matrix ? matrix : custom_bone_to_world );
		g_interfaces.m_model_render->forced_material_override( nullptr );
		material->set_material_var_flag( material_var_wireframe, false );
		return true;
	};
	if ( static int lag_mat_index = -1; !lag_material || lag_mat_index != GET_VARIABLE( g_variables.m_player_lag_chams_material, int ) ) {
		lag_material = return_material( GET_VARIABLE( g_variables.m_player_lag_chams_material, int ) );
		lag_material->increment_reference_count( );
		lag_mat_index = GET_VARIABLE( g_variables.m_player_lag_chams_material, int );
	}
	if ( static int vis_mat_index = -1; !vis_material || vis_mat_index != GET_VARIABLE( g_variables.m_player_visible_chams_material, int ) ) {
		vis_material = return_material( GET_VARIABLE( g_variables.m_player_visible_chams_material, int ) );
		vis_material->increment_reference_count( );
		vis_mat_index = GET_VARIABLE( g_variables.m_player_visible_chams_material, int );
	}
	if ( static int invis_mat_index = -1; !invis_material || invis_mat_index != GET_VARIABLE( g_variables.m_player_invisible_chams_material, int ) ) {
		invis_material = return_material( GET_VARIABLE( g_variables.m_player_invisible_chams_material, int ) );
		invis_material->increment_reference_count( );
		invis_mat_index = GET_VARIABLE( g_variables.m_player_invisible_chams_material, int );
	}
	const auto backtrack_color = GET_VARIABLE( g_variables.m_player_lag_chams_color, c_color );
	const auto visible_color   = GET_VARIABLE( g_variables.m_player_visible_chams_color, c_color );
	const auto invisible_color = GET_VARIABLE( g_variables.m_player_invisible_chams_color, c_color );
	const bool should_draw_lag   = GET_VARIABLE( g_variables.m_player_lag_chams, bool );
	const bool should_draw_vis   = GET_VARIABLE( g_variables.m_player_visible_chams, bool );
	const bool should_draw_invis = GET_VARIABLE( g_variables.m_player_invisible_chams, bool );
	const bool should_draw_local = GET_VARIABLE( g_variables.m_player_local_chams, bool );
	const bool should_draw_ally = GET_VARIABLE( g_variables.m_player_ally_chams, bool );
	const bool should_draw_weapon = GET_VARIABLE( g_variables.m_weapon_chams, bool );
	const bool should_draw_hands = GET_VARIABLE( g_variables.m_hands_chams, bool );
	const bool should_draw_sleeves = GET_VARIABLE( g_variables.m_sleeves_chams, bool );
	const bool should_draw_ragdoll = GET_VARIABLE( g_variables.m_ragdoll_chams, bool );
	const bool should_draw_chams = GET_VARIABLE( g_variables.m_chams_enable, bool ) && (should_draw_lag || should_draw_vis || should_draw_invis || should_draw_local || should_draw_ally ||
	                               should_draw_weapon || should_draw_hands || should_draw_sleeves || should_draw_ragdoll);
	if ( !context || !custom_bone_to_world || !g_interfaces.m_engine_client->is_connected_safe( ) || !g_ctx.m_local || !should_draw_chams ||
	     g_ctx.m_is_glow_being_drawn || !info.model || strstr( info.model->m_name, "player/contactshadow" ) ||
	     g_interfaces.m_model_render->is_forced_material_override( ) )
		return original( ecx, edx, context, state, info, custom_bone_to_world );
	const std::string mdl = info.model->m_name;
	auto player           = g_interfaces.m_client_entity_list->get< c_base_entity >( info.entity_index );
	if ( player && player->is_valid_enemy( ) && mdl.find( "player" ) != std::string::npos && info.entity_index > 0 && info.entity_index < 64 ) {
		if ( g_menu.is_server_player_visuals_whitelisted( info.entity_index ) )
			return original( ecx, edx, context, state, info, custom_bone_to_world );
		bool applied_chams = false;
		auto record = &Backtrack::records[ info.entity_index ];
		static float distance = 0.f;
		switch ( GET_VARIABLE( g_variables.m_player_lag_chams_type, int ) ) {
		case 0:
			if ( !should_draw_lag )
				break;
			if ( record && record->size( ) && Backtrack::valid( record->back( ).simulationTime ) ) {
				applied_chams |= draw_chams( GET_VARIABLE( g_variables.m_player_lag_chams_material, int ), backtrack_color,
				                             GET_VARIABLE( g_variables.m_player_lag_chams_ignorez, bool ) || GET_VARIABLE( g_variables.m_player_lag_chams_xqz, bool ),
				                             GET_VARIABLE( g_variables.m_player_lag_chams_wireframe, bool ),
				                             GET_VARIABLE( g_variables.m_player_lag_chams_cover, bool ), record->back( ).matrix );
			}
			break;
		case 1:
			if ( !should_draw_lag )
				break;
			if ( record && record->size( ) && Backtrack::valid( record->front( ).simulationTime ) ) {
				distance = record->back( ).origin.dist_to( player->get_abs_origin( ) );
				if ( const auto record_list = record; distance < LAG_COMPENSATION_TELEPORTED_DISTANCE_SQR ) {
					for ( int i = 0; i < record->size( ) - 1; i++ ) {
						if ( !Backtrack::valid( record_list->at( i ).simulationTime ) )
							continue;
						ImAnimationHelper alpha_animation = ImAnimationHelper( HASH_RT( mdl.c_str( ) ), ImGui::GetIO( ).DeltaTime );
						alpha_animation.Update( 2.f, distance > 3.f ? 2.f : -2.f );
						if ( alpha_animation.AnimationData->second > 0.f ) {
							auto fading_color = backtrack_color;
							fading_color[ 3 ] *= alpha_animation.AnimationData->second;
							applied_chams |= draw_chams( GET_VARIABLE( g_variables.m_player_lag_chams_material, int ), fading_color,
							                             GET_VARIABLE( g_variables.m_player_lag_chams_ignorez, bool ) || GET_VARIABLE( g_variables.m_player_lag_chams_xqz, bool ),
							                             GET_VARIABLE( g_variables.m_player_lag_chams_wireframe, bool ),
							                             GET_VARIABLE( g_variables.m_player_lag_chams_cover, bool ), record_list->at( i ).matrix );
						}
					}
				}
				break;
			}
			break;
		}
		if ( should_draw_invis ) {
			applied_chams |= draw_chams( GET_VARIABLE( g_variables.m_player_invisible_chams_material, int ), invisible_color,
			                             true,
			                             GET_VARIABLE( g_variables.m_player_invisible_chams_wireframe, bool ),
			                             GET_VARIABLE( g_variables.m_player_invisible_chams_cover, bool ), custom_bone_to_world );
		}
		if ( should_draw_vis ) {
			applied_chams |= draw_chams( GET_VARIABLE( g_variables.m_player_visible_chams_material, int ), visible_color,
			                             GET_VARIABLE( g_variables.m_player_visible_chams_ignorez, bool ),
			                             GET_VARIABLE( g_variables.m_player_visible_chams_wireframe, bool ),
			                             GET_VARIABLE( g_variables.m_player_visible_chams_cover, bool ), custom_bone_to_world );
		}
		if ( applied_chams )
			return;
	}
	else if ( player && player == g_ctx.m_local && mdl.find( "player" ) != std::string::npos && should_draw_local ) {
		if ( draw_chams( GET_VARIABLE( g_variables.m_player_local_chams_material, int ), GET_VARIABLE( g_variables.m_player_local_chams_color, c_color ),
		                 GET_VARIABLE( g_variables.m_player_local_chams_ignorez, bool ), GET_VARIABLE( g_variables.m_player_local_chams_wireframe, bool ),
		                 GET_VARIABLE( g_variables.m_player_local_chams_cover, bool ), custom_bone_to_world ) )
			return;
	}
	else if ( player && player->is_player( ) && !g_ctx.m_local->is_enemy( player ) && player != g_ctx.m_local && mdl.find( "player" ) != std::string::npos &&
	          should_draw_ally ) {
		if ( draw_chams( GET_VARIABLE( g_variables.m_player_ally_chams_material, int ), GET_VARIABLE( g_variables.m_player_ally_chams_color, c_color ),
		                 GET_VARIABLE( g_variables.m_player_ally_chams_ignorez, bool ), GET_VARIABLE( g_variables.m_player_ally_chams_wireframe, bool ),
		                 GET_VARIABLE( g_variables.m_player_ally_chams_cover, bool ), custom_bone_to_world ) )
			return;
	}
	else if ( should_draw_ragdoll && mdl.find( "player" ) != std::string::npos && mdl.find( "ragdoll" ) != std::string::npos ) {
		if ( draw_chams( GET_VARIABLE( g_variables.m_ragdoll_chams_material, int ), GET_VARIABLE( g_variables.m_ragdoll_chams_color, c_color ),
		                 GET_VARIABLE( g_variables.m_ragdoll_chams_ignorez, bool ), GET_VARIABLE( g_variables.m_ragdoll_chams_wireframe, bool ),
		                 GET_VARIABLE( g_variables.m_ragdoll_chams_cover, bool ), custom_bone_to_world ) )
			return;
	}
	else if ( should_draw_hands && mdl.find( "arms" ) != std::string::npos && mdl.find( "sleeve" ) == std::string::npos ) {
		if ( draw_chams( GET_VARIABLE( g_variables.m_hands_chams_material, int ), GET_VARIABLE( g_variables.m_hands_chams_color, c_color ),
		                 GET_VARIABLE( g_variables.m_hands_chams_ignorez, bool ), GET_VARIABLE( g_variables.m_hands_chams_wireframe, bool ),
		                 GET_VARIABLE( g_variables.m_hands_chams_cover, bool ), custom_bone_to_world ) )
			return;
	}
	else if ( should_draw_sleeves && mdl.find( "sleeve" ) != std::string::npos ) {
		if ( draw_chams( GET_VARIABLE( g_variables.m_sleeves_chams_material, int ), GET_VARIABLE( g_variables.m_sleeves_chams_color, c_color ),
		                 GET_VARIABLE( g_variables.m_sleeves_chams_ignorez, bool ), GET_VARIABLE( g_variables.m_sleeves_chams_wireframe, bool ),
		                 GET_VARIABLE( g_variables.m_sleeves_chams_cover, bool ), custom_bone_to_world ) )
			return;
	}
	else if ( should_draw_weapon && ( mdl.find( "weapons/v_" ) != std::string::npos || mdl.find( "weapons/w_" ) != std::string::npos ) ) {
		if ( draw_chams( GET_VARIABLE( g_variables.m_weapon_chams_material, int ), GET_VARIABLE( g_variables.m_weapon_chams_color, c_color ),
		                 GET_VARIABLE( g_variables.m_weapon_chams_ignorez, bool ), GET_VARIABLE( g_variables.m_weapon_chams_wireframe, bool ),
		                 GET_VARIABLE( g_variables.m_weapon_chams_cover, bool ), custom_bone_to_world ) )
			return;
	}
	original( ecx, edx, context, state, info, custom_bone_to_world );
	g_interfaces.m_model_render->forced_material_override( nullptr );
}
