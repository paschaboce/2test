#pragma once

template< typename FuncType >
__forceinline static FuncType CallVFunction( void* ppClass, int index )
{
	int* pVTable  = *( int** )ppClass;
	int dwAddress = pVTable[ index ];
	return ( FuncType )( dwAddress );
}

#include <cstdarg>
#include <climits>
#include <string>
#include <stdio.h>
#define MAX_BUFFER_SIZE 1024

#define CHAT_COLOR_DEFAULT        "\x01"
#define CHAT_COLOR_RED            "\x02"
#define CHAT_COLOR_LIGHTPURPLE    "\x03"
#define CHAT_COLOR_GREEN          "\x04"
#define CHAT_COLOR_LIME           "\x05"
#define CHAT_COLOR_LIGHTGREEN     "\x06"
#define CHAT_COLOR_LIGHTRED       "\x07"
#define CHAT_COLOR_GRAY           "\x08"
#define CHAT_COLOR_LIGHTOLIVE     "\x09"
#define CHAT_COLOR_LIGHTSTEELBLUE "\x0A"
#define CHAT_COLOR_LIGHTBLUE      "\x0B"
#define CHAT_COLOR_BLUE           "\x0C"
#define CHAT_COLOR_PURPLE         "\x0D"
#define CHAT_COLOR_PINK           "\x0E"
#define CHAT_COLOR_LIGHTRED2      "\x0F"
#define CHAT_COLOR_OLIVE          "\x10"

template< typename color_t >
inline const char* get_closest_chat_color_code( const color_t& color )
{
	struct chat_color_candidate_t {
		const char* code;
		int r;
		int g;
		int b;
	};

	static constexpr chat_color_candidate_t candidates[] = {
		{ CHAT_COLOR_LIGHTPURPLE, 196, 181, 253 },
		{ CHAT_COLOR_GREEN, 64, 255, 64 },
		{ CHAT_COLOR_LIME, 128, 255, 0 },
		{ CHAT_COLOR_LIGHTGREEN, 153, 255, 153 },
		{ CHAT_COLOR_LIGHTRED, 255, 64, 64 },
		{ CHAT_COLOR_GRAY, 128, 128, 128 },
		{ CHAT_COLOR_LIGHTOLIVE, 192, 192, 64 },
		{ CHAT_COLOR_LIGHTSTEELBLUE, 176, 196, 222 },
		{ CHAT_COLOR_LIGHTBLUE, 96, 160, 255 },
		{ CHAT_COLOR_BLUE, 64, 96, 255 },
		{ CHAT_COLOR_PURPLE, 180, 96, 255 },
		{ CHAT_COLOR_PINK, 255, 105, 180 },
		{ CHAT_COLOR_LIGHTRED2, 255, 128, 128 },
		{ CHAT_COLOR_OLIVE, 128, 128, 0 },
	};

	const int target_r = static_cast< int >( color[ 0 ] );
	const int target_g = static_cast< int >( color[ 1 ] );
	const int target_b = static_cast< int >( color[ 2 ] );

	const chat_color_candidate_t* best_candidate = &candidates[ 0 ];
	int best_distance = INT_MAX;

	for ( const auto& candidate : candidates ) {
		const int dr = target_r - candidate.r;
		const int dg = target_g - candidate.g;
		const int db = target_b - candidate.b;
		const int distance = dr * dr + dg * dg + db * db;
		if ( distance < best_distance ) {
			best_distance = distance;
			best_candidate = &candidate;
		}
	}

	return best_candidate->code;
}

inline const char* get_kami_prefix_chat_color_code( )
{
	return CHAT_COLOR_GRAY;
}

template< typename color_t >
inline std::string make_kami_chat_message( const color_t& accent, const char* text )
{
	std::string message;
	message.reserve( 34 + ( text ? std::strlen( text ) : 0 ) );
	message += CHAT_COLOR_DEFAULT;
	message += " ";
	message += get_kami_prefix_chat_color_code( );
	message += "kami +";
	message += get_closest_chat_color_code( accent );
	message += "|";
	message += CHAT_COLOR_DEFAULT;
	message += " ";
	message += text ? text : "";
	return message;
}

class c_hudchat
{
public:
	void chatprintf( int iPlayerIndex, int iFilter, const char* format, ... )
	{
		static char buf[ MAX_BUFFER_SIZE ] = "";
		va_list va;
		va_start( va, format );
		vsnprintf_s( buf, MAX_BUFFER_SIZE, format, va );
		va_end( va );
		CallVFunction< void( __cdecl* )( void*, int, int, const char*, ... ) >( this, 27 )( this, iPlayerIndex, iFilter, buf );
	}
};
