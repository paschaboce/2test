#pragma once
#include <string>
#include <vector>
#include <utility>

namespace chat_filter
{
	void initialize( );
	std::string process_chat_message( const char* message );

	inline std::vector< std::pair< std::string, std::string > > m_replacements;
}
