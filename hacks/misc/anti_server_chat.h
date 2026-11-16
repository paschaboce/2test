#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace anti_server_chat
{
	inline bool is_server_message( const char* message )
	{
		if ( !message || message[ 0 ] == '\0' )
			return false;

		std::string msg( message );

		// strip color codes (\x01 - \x10)
		std::string clean;
		clean.reserve( msg.size( ) );
		for ( char c : msg ) {
			if ( static_cast< unsigned char >( c ) >= 0x01 && static_cast< unsigned char >( c ) <= 0x10 )
				continue;
			clean += c;
		}

		auto to_lower = []( std::string s ) -> std::string {
			std::transform( s.begin( ), s.end( ), s.begin( ), ::tolower );
			return s;
		};

		std::string lower = to_lower( clean );

		static const std::vector< std::string > server_prefixes = {
			"[sm]", "[sourcemod]", "[admin]", "[server]",
			"[plugin]", "[vip]", "[store]", "[shop]",
			"[ws]", "[knife]", "[gloves]", "[skins]",
			"[timer]", "[cksurf]", "[surftimer]", "[gokz]",
			"[kztimer]", "[rtv]", "[nominate]", "[mapchooser]",
			"[map]", "[rank]", "[lvl]", "[level]",
			"[warning]", "[info]", "[notice]", "[system]",
			"[discord]", "[ts]", "[teamspeak]", "[donate]",
			"[report]", "[calladmin]", "[anti-cheat]", "[ac]",
			"[motd]", "[rules]", "[welcome]", "[ad]",
			"[advertisement]", "[afk]", "[chat]", "[mute]",
			"[ban]", "[kick]", "[tp]", "[checkpoint]",
			"[cp]", "[bonus]", "[stage]", "[zone]",
			"[css]", "[csgo]", "[spec]", "[spectator]",
			"[gameme]", "[hlstatsx]", "[stats]",
			"[knife round]", "[warmup]", "[retake]", "[retakes]",
			"[executes]", "[1v1]", "[arena]", "[duel]",
			"[quiz]", "[trivia]", "[event]", "[giveaway]",
			"[skin]", "[model]", "[playermodel]",
			"[bhop]", "[auto]", "[scroll]",
			"[jail]", "[jb]", "[warden]", "[guard]",
			"[ttt]", "[traitor]", "[detective]", "[innocent]",
			"[deathrun]", "[dr]", "[zombie]", "[zm]",
			"[mg]", "[minigames]", "[hns]", "[prop hunt]",
			"[surf]", "[kz]",
			"[radio]", "[music]",
			"[tag]", "[tags]", "[clantag]",
			"[cmd]", "[command]", "[commands]",
			"[vote]", "[votes]", "[voting]",
			"[connect]", "[disconnect]",
			"[reserve]", "[slot]", "[premium]",
			"* ", "# ",
		};

		for ( const auto& prefix : server_prefixes ) {
			if ( lower.find( prefix ) == 0 )
				return true;
		}

		static const std::vector< std::string > server_patterns = {
			"y6",
			"ps assisted",
			"has joined the server",
			"has left the server",
			"connected to the server",
			"disconnected from the server",
			"type !",
			"type /",
			"write !",
			"write /",
			"say !",
			"say /",
			"changed map to",
			"next map:",
			"timeleft:",
			"time left:",
			"current map:",
			"players online:",
			"visit our",
			"join our discord",
			"www.",
			".com",
			".net",
			".org",
			".ru",
			".gg",
		};

		for ( const auto& pattern : server_patterns ) {
			if ( lower.find( pattern ) != std::string::npos )
				return true;
		}

		return false;
	}
}
