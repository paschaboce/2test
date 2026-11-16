#include "../hooks.h"
#include "../../game/sdk/includes/includes.h"
#include "../../hacks/misc/chat_filter.h"
#include "../../hacks/misc/anti_server_chat.h"
#include "../../globals/includes/includes.h"

namespace {
	std::string sanitize_chat_text( const char* text )
	{
		if ( !text )
			return { };
		std::string out( text );
		out.erase( std::remove_if( out.begin( ), out.end( ), []( unsigned char ch ) {
			return ch < 32 && ch != ' ';
		} ), out.end( ) );
		for ( char& c : out ) {
			unsigned char uc = static_cast< unsigned char >( c );
			if ( uc >= 'A' && uc <= 'Z' )
				c = static_cast< char >( uc + 32 );
		}
		return out;
	}
}

void __cdecl n_detoured_functions::chat_printf( void* thisptr, int filter, int unused, const char* fmt, ... )
{
	static auto original = g_hooks.m_chat_printf.get_original< decltype( &n_detoured_functions::chat_printf ) >( );
	if ( !fmt || fmt[0] == '\0' )
		return original( thisptr, filter, unused, fmt );

	char buffer[ 2048 ];
	va_list args;
	va_start( args, fmt );
	vsnprintf_s( buffer, sizeof( buffer ), _TRUNCATE, fmt, args );
	va_end( args );

	const std::string sanitized_fmt = sanitize_chat_text( fmt );
	const std::string sanitized_buffer = sanitize_chat_text( buffer );

	// Always hide sv_cheats changes
	if ( sanitized_buffer.find( "sv_cheats" ) != std::string::npos )
		return;

	const bool hard_blocked = sanitized_fmt.find( "ps assisted" ) != std::string::npos || sanitized_buffer.find( "ps assisted" ) != std::string::npos;
	const bool own_message = sanitized_fmt.find( "kami +" ) != std::string::npos || sanitized_buffer.find( "kami +" ) != std::string::npos;
	if ( GET_VARIABLE( g_variables.m_anti_server_chat, bool ) && hard_blocked )
		return;

	if ( GET_VARIABLE( g_variables.m_anti_server_chat, bool ) && !own_message ) {
		if ( filter == 0 )
			return;

		if ( anti_server_chat::is_server_message( buffer ) )
			return;
	}

	std::string processed = GET_VARIABLE( g_variables.m_anti_server_chat, bool ) && !own_message ? chat_filter::process_chat_message( buffer ) : std::string( buffer );
	if ( processed.empty( ) )
		return;

	return original( thisptr, filter, unused, processed.c_str( ) );
}
