#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../hooks.h"
#include <intrin.h>
bool __fastcall n_detoured_functions::is_playing_demo( void* ecx, void* edx )
{
	static auto original = g_hooks.m_is_playing_demo.get_original< decltype( &n_detoured_functions::is_playing_demo ) >( );
	static const auto demo_or_hltv = reinterpret_cast< std::uintptr_t >( g_modules[ CLIENT_DLL ].find_pattern( "84 C0 75 09 38 05" ) );
	static const auto money = reinterpret_cast< std::uintptr_t >( g_modules[ CLIENT_DLL ].find_pattern( "84 C0 75 0C 5B" ) );
	if ( GET_VARIABLE( g_variables.m_reveal_money, bool ) && demo_or_hltv && money ) {
		const auto ret = reinterpret_cast< std::uintptr_t >( _ReturnAddress( ) );
		const auto caller = *reinterpret_cast< std::uintptr_t* >( ( reinterpret_cast< std::uintptr_t >( _AddressOfReturnAddress( ) ) - sizeof( std::uintptr_t ) ) + 8 );
		if ( ret == demo_or_hltv && caller == money )
			return true;
	}
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return original( ecx, edx );
	static const auto is_paused_extrapolate =
		reinterpret_cast< unsigned int >( g_modules[ CLIENT_DLL ].find_pattern( "0F B6 0D ? ? ? ? 84 C0 0F 44" ) );
	if ( const auto return_address = reinterpret_cast< unsigned int >( _ReturnAddress( ) ); return_address == is_paused_extrapolate )
		return true;
	return original( ecx, edx );
}

const demo_playback_parameters_t* __fastcall n_detoured_functions::get_demo_playback_parameters( void* ecx, void* edx )
{
	static auto original = g_hooks.m_get_demo_playback_parameters.get_original< decltype( &n_detoured_functions::get_demo_playback_parameters ) >( );
	const auto params = original( ecx, edx );
	if ( !params || !GET_VARIABLE( g_variables.m_reveal_suspect, bool ) )
		return params;

	static const auto demo_file_end_reached =
		reinterpret_cast< std::uintptr_t >( g_modules[ CLIENT_DLL ].find_pattern( "8B C8 85 C9 74 1F 80 79 10" ) );
	if ( demo_file_end_reached && reinterpret_cast< std::uintptr_t >( _ReturnAddress( ) ) == demo_file_end_reached )
		return params;

	static demo_playback_parameters_t custom_params;
	custom_params = *params;
	custom_params.anonymous_player_identity = false;
	return &custom_params;
}

float __fastcall n_detoured_functions::get_screen_aspect_ratio( void* ecx, void* edx, int width, int height )
{
	static auto original = g_hooks.m_get_screen_aspect_ratio.get_original< decltype( &n_detoured_functions::get_screen_aspect_ratio ) >( );
	const float aspect_ratio = GET_VARIABLE( g_variables.m_aspect_ratio, float );
	if ( aspect_ratio > 0.f )
		return aspect_ratio;

	return original( ecx, edx, width, height );
}
