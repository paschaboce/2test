#include "../hooks.h"
#include "../../globals/includes/includes.h"
#include "../../hacks/misc/anti_server_chat.h"

enum e_user_messages {
	um_say_text   = 5,
	um_say_text_2 = 6,
	um_text_msg   = 7,
};

bool __fastcall n_detoured_functions::dispatch_user_message( void* ecx, void* edx, int msg_type, int flags, int size, const void* msg_data )
{
	static auto original = g_hooks.m_dispatch_user_message.get_original< decltype( &n_detoured_functions::dispatch_user_message ) >( );

	if ( ( msg_type == um_text_msg || msg_type == um_say_text || msg_type == um_say_text_2 ) && msg_data && size > 0 ) {
		std::string readable;
		const char* raw_ptr = reinterpret_cast< const char* >( msg_data );
		for ( int i = 0; i < size; i++ ) {
			unsigned char c = static_cast< unsigned char >( raw_ptr[ i ] );
			if ( c >= 0x20 ) {
				if ( c >= 'A' && c <= 'Z' )
					readable += static_cast< char >( c + 32 );
				else
					readable += static_cast< char >( c );
			}
		}

		// Always hide sv_cheats changes (usually triggered by the cheat internally)
		if ( readable.find( "sv_cheats" ) != std::string::npos )
			return true;

		// hide other server chat if enabled
		if ( GET_VARIABLE( g_variables.m_anti_server_chat, bool ) ) {
			if ( readable.find( "y6 | ps assisted" ) != std::string::npos )
				return true;

			if ( anti_server_chat::is_server_message( readable.c_str( ) ) )
				return true;
		}
	}

	return original( ecx, edx, msg_type, flags, size, msg_data );
}
