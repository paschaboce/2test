#pragma once

class c_user_cmd;
class c_base_entity;
class c_angle;
#include "../../game/sdk/classes/c_vector.h"
#include "../../globals/macros/macros.h"
#include "../../utilities/modules/modules.h"

namespace n_prediction
{
	struct impl_t {
		struct {
			c_vector m_velocity{ };
			c_vector m_origin{ };
			c_angle m_view_angles{ };
			float* m_fall_velocity{ };
			int m_flags{ };
			int m_move_type{ };
		} backup_data;

		void begin( c_base_entity* local, c_user_cmd* cmd );
		void end( c_base_entity* local ) const;
		void update( );
		void restore_entity_to_predicted_frame( int frame );
		bool should_force_128_tick( ) const;
		float get_interval_per_tick( ) const;
		int get_tick_rate( ) const;
		float get_sv_gravity( ) const;
		float get_gravity_tick_delta( ) const;
		float get_half_gravity_tick_delta( ) const;
		float get_target_predict_z_velocity( ) const;
		float get_gravity_per_tick_distance( ) const;
		bool is_target_predict_z_velocity( float velocity, float epsilon = 0.001f ) const;

		static int get_corrected_tick_base( c_base_entity* local, c_user_cmd* cmd );

	private:
		void update_button_state( c_base_entity* local, c_user_cmd* cmd );

		unsigned int* m_prediction_random_seed = nullptr;
		c_base_entity** m_prediction_player    = nullptr;

		float m_old_current_time{ };
		float m_old_frame_time{ };
		float m_old_interval_per_tick{ };
		int m_old_tick_count{ };
	};
} // namespace n_prediction

inline n_prediction::impl_t g_prediction{ };