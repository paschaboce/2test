#pragma once
#include "../../dependencies/imgui/imgui.h"
float LerpFloat( float current, float target, float smoothFactor, float dt );
namespace n_indicators
{
	struct impl_t {
		void on_paint_traverse( );
		void on_end_scene( );
		struct indicator_data_t {
			void reset( )
			{
				this->m_last_velocity           = 0;
				this->m_tick_prev_velocity      = 0;
				this->m_take_off_velocity       = 0;
				this->m_take_off_time_velocity  = 0.f;
				this->m_last_on_ground_velocity = false;
				this->m_was_in_air = false;
				this->m_frames_in_air = 0;
				this->m_ground_time = 0.f;
				this->m_last_stamina           = 0.f;
				this->m_tick_prev_stamina      = 0.f;
				this->m_take_off_stamina       = 0.f;
				this->m_take_off_time_stamina  = 0.f;
				this->m_last_on_ground_stamina = false;
			}
			int m_last_velocity            = 0;
			int m_tick_prev_velocity       = 0;
			int m_take_off_velocity        = 0;
			float m_take_off_time_velocity = 0.f;
			bool m_last_on_ground_velocity = false;
			bool m_was_in_air = false;
			int m_frames_in_air = 0;
			float m_ground_time = 0.f;
			static constexpr float GROUND_TIME_THRESHOLD = 0.1f;
			float m_last_stamina          = 0.f;
			float m_tick_prev_stamina     = 0.f;
			float m_take_off_stamina      = 0.f;
			float m_take_off_time_stamina = 0.f;
			bool m_last_on_ground_stamina = false;
			int m_last_velocity_before_jump = 0;
			bool m_was_on_ground_prev = false;
		} m_indicator_data;
	private:
		void keybind_indicators( );
		void keybind_indicators_hor( );
		void velocity( const bool on_ground );
		void stamina( const bool on_ground );
	};
}
inline n_indicators::impl_t g_indicators{ };