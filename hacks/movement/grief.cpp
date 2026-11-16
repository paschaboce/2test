#include "grief.h"
#include "../prediction/prediction.h"
#include "../../globals/math/math.h"

n_grief::impl_t g_grief;

void n_grief::impl_t::blockbot( c_user_cmd* cmd ) 
{ 
	 if ( !GET_VARIABLE( g_variables.m_blockbot, bool ) ) 
	 	 return; 
	 
	 key_bind_t temp_key = GET_VARIABLE( g_variables.m_blockbot_key, key_bind_t ); 
	 temp_key.m_key_style = GET_VARIABLE( g_variables.m_blockbot_key_type, int ) + 1; 
	 
	 if ( !g_input.check_input( &temp_key ) ) 
	 	 return; 
	 
	 if ( !g_interfaces.m_engine_client->is_connected( ) || !g_interfaces.m_engine_client->is_in_game( ) ) 
	 	 return; 
	 
	 if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) ) 
	 	 return; 
	 
	 if ( g_ctx.m_local->get_move_type( ) == e_move_types::move_type_ladder || 
	      g_ctx.m_local->get_move_type( ) == e_move_types::move_type_noclip ) 
	 	 return; 
	 
	 float best_dist = std::numeric_limits< float >::max( ); 
	 int best_target = -1; 
	 
	 for ( int i = 1; i < g_interfaces.m_global_vars_base->m_max_clients; i++ ) { 
	 	 auto entity = reinterpret_cast< c_base_entity* >( g_interfaces.m_client_entity_list->get_client_entity( i ) ); 
	 	 if ( !entity || entity->is_dormant( ) || !entity->is_alive( ) || entity == g_ctx.m_local ) 
	 	 	 continue; 
	 	 
	 	 float dist = g_ctx.m_local->get_abs_origin( ).dist_to( entity->get_abs_origin( ) ); 
	 	 if ( dist < best_dist ) { 
	 	 	 best_dist = dist; 
	 	 	 best_target = i; 
	 	 } 
	 } 
	 
	 if ( best_target == -1 ) 
	 	 return; 
	 
	 auto entity = reinterpret_cast< c_base_entity* >( g_interfaces.m_client_entity_list->get_client_entity( best_target ) ); 
	 if ( !entity ) 
	 	 return; 
	 
	 const float speed = entity->get_velocity( ).length( ); 
	 const c_vector forward = entity->get_abs_origin( ) - g_ctx.m_local->get_abs_origin( ); 
	 
	 if ( entity->get_bone_position( 6 ).m_z < g_ctx.m_local->get_abs_origin( ).m_z && 
	      g_ctx.m_local->get_abs_origin( ).dist_to( entity->get_abs_origin( ) ) < 100.f ) { 
	 	 cmd->m_forward_move = ( ( sin( deg2rad( cmd->m_view_point.m_y ) ) * forward.m_y ) + 
	 	                         ( cos( deg2rad( cmd->m_view_point.m_y ) ) * forward.m_x ) ) * speed; 
	 	 cmd->m_side_move = ( ( cos( deg2rad( cmd->m_view_point.m_y ) ) * -forward.m_y ) + 
	 	                      ( sin( deg2rad( cmd->m_view_point.m_y ) ) * forward.m_x ) ) * speed; 
	 } else { 
	 	 float yaw_delta = ( atan2( forward.m_y, forward.m_x ) * 180.0f / 3.14159265359f ) - cmd->m_view_point.m_y; 
	 	 
	 	 if ( yaw_delta > 180.f ) 
	 	 	 yaw_delta -= 360.f; 
	 	 else if ( yaw_delta < -180.f ) 
	 	 	 yaw_delta += 360.f; 
	 	 
	 	 if ( yaw_delta > 0.25f ) 
	 	 	 cmd->m_side_move = -speed; 
	 	 else if ( yaw_delta < -0.25f ) 
	 	 	 cmd->m_side_move = speed; 
	 } 
} 

void n_grief::impl_t::door_spam( c_user_cmd* cmd ) 
{ 
    if ( !GET_VARIABLE( g_variables.m_door_spam, bool ) || !g_input.check_input( &GET_VARIABLE( g_variables.m_door_spam_key, key_bind_t ) ) ) 
        return; 
    
    static float time = 0.f; 
    
    if ( g_interfaces.m_global_vars_base->m_current_time > time ) { 
        cmd->m_buttons |= in_use; 
        time = g_interfaces.m_global_vars_base->m_current_time + 0.2f; 
    } 
    
    if ( fabs( time - g_interfaces.m_global_vars_base->m_current_time ) > 0.3f ) 
        time = g_interfaces.m_global_vars_base->m_current_time; 
}

void n_grief::impl_t::deagle_spinner( c_base_entity* local, void* viewmodel, long& sequence )
{
	if ( !GET_VARIABLE( g_variables.m_deagle_spinner, bool ) || !local || !viewmodel )
		return;

	auto active_weapon = g_interfaces.m_client_entity_list->get< c_base_entity >( local->get_active_weapon_handle( ) );
	if ( active_weapon && active_weapon->get_client_class( )->m_class_id == e_class_ids::c_deagle ) {
		if ( sequence == 7 ) {
			sequence = 8;
		}
	}
}
