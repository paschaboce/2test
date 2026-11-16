#include "chat_filter.h"
#include <algorithm>

namespace {
	std::string sanitize_for_match( std::string text )
	{
		text.erase( std::remove_if( text.begin( ), text.end( ), []( unsigned char ch ) {
			return ch < 32 && ch != ' ';
		} ), text.end( ) );
		std::transform( text.begin( ), text.end( ), text.begin( ), ::tolower );
		return text;
	}
}

void chat_filter::initialize( )
{
	m_replacements.clear( );
	m_replacements.emplace_back( "y6", "\x0Bmiku\x01" );
}

std::string chat_filter::process_chat_message( const char* message )
{
	if ( !message || message[ 0 ] == '\0' )
		return { };

	std::string result( message );
	const std::string sanitized = sanitize_for_match( result );
	if ( sanitized.find( "ps assisted" ) != std::string::npos )
		return { };

	for ( const auto& [ from, to ] : m_replacements ) {
		size_t pos = 0;
		while ( ( pos = result.find( from, pos ) ) != std::string::npos ) {
			result.replace( pos, from.length( ), to );
			pos += to.length( );
		}
	}

	return result;
}
