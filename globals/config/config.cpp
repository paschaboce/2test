#define _SILENCE_CXX20_CISO646_REMOVED_WARNING
#include <cmath>
#include <fstream>
#include <shlobj.h>
#include "../../dependencies/json/json.hpp"
#include "../../game/sdk/classes/c_color.h"
#include "../../utilities/console/console.h"
#include "../macros/macros.h"
#include "variables.h"
#include "config.h"
#include "../../hacks\movement\movement.h"
#include "../../hacks\aimbot\aimbot.h"

namespace {
	nlohmann::json serialize_vec3( const c_vector& vec )
	{
		return nlohmann::json::array( { vec.m_x, vec.m_y, vec.m_z } );
	}

	c_vector deserialize_vec3( const nlohmann::json& value )
	{
		if ( !value.is_array( ) || value.size( ) < 3 )
			return { };
		return c_vector( value[ 0 ].get< float >( ), value[ 1 ].get< float >( ), value[ 2 ].get< float >( ) );
	}

	nlohmann::json serialize_point_entry( const points_check_t& point )
	{
		nlohmann::json entry = { };
		entry[ "pos" ] = serialize_vec3( point.pos );
		entry[ "map" ] = point.map;
		entry[ "jump" ] = point.jump;
		entry[ "minijump" ] = point.minijump;
		entry[ "longjump" ] = point.longjump;
		entry[ "jumpbug" ] = point.jumpbug;
		entry[ "crouch_hop" ] = point.crouch_hop;
		entry[ "mini_crouch_hop" ] = point.mini_crouch_hop;
		entry[ "c_jump" ] = point.c_jump;
		entry[ "c_minijump" ] = point.c_minijump;
		entry[ "c_longjump" ] = point.c_longjump;
		entry[ "c_jumpbug" ] = point.c_jumpbug;
		entry[ "c_crouch_hop" ] = point.c_crouch_hop;
		entry[ "c_mini_crouch_hop" ] = point.c_mini_crouch_hop;
		entry[ "high_crouch_jump" ] = point.high_crouch_jump;
		entry[ "c_high_crouch_jump" ] = point.c_high_crouch_jump;
		entry[ "active" ] = point.active;
		entry[ "radius" ] = point.radius;
		entry[ "delta_strafe" ] = point.delta_strafe;
		entry[ "selected_preset" ] = point.selected_preset;
		entry[ "advanced_enabled" ] = point.advanced_enabled;
		entry[ "selected_advanced" ] = point.selected_advanced;
		entry[ "first_jump_radius" ] = point.first_jump_radius;
		entry[ "use_custom_first_jump_radius" ] = point.use_custom_first_jump_radius;
		entry[ "enable_restricted_binds" ] = point.enable_restricted_binds;
		entry[ "selected_restricted_bind" ] = point.selected_restricted_bind;
		entry[ "restricted_binds_radius" ] = point.restricted_binds_radius;
		entry[ "is_free_point" ] = point.is_free_point;
		entry[ "point_type" ] = point.point_type;
		entry[ "show_3d_radius" ] = point.show_3d_radius;
		return entry;
	}

	void deserialize_point_entry( const nlohmann::json& entry, points_check_t& point )
	{
		point.pos = deserialize_vec3( entry.value( "pos", nlohmann::json::array( ) ) );
		point.map = entry.value( "map", std::string( ) );
		point.jump = entry.value( "jump", true );
		point.minijump = entry.value( "minijump", true );
		point.longjump = entry.value( "longjump", true );
		point.jumpbug = entry.value( "jumpbug", true );
		point.crouch_hop = entry.value( "crouch_hop", true );
		point.mini_crouch_hop = entry.value( "mini_crouch_hop", true );
		point.c_jump = entry.value( "c_jump", true );
		point.c_minijump = entry.value( "c_minijump", true );
		point.c_longjump = entry.value( "c_longjump", true );
		point.c_jumpbug = entry.value( "c_jumpbug", true );
		point.c_crouch_hop = entry.value( "c_crouch_hop", true );
		point.c_mini_crouch_hop = entry.value( "c_mini_crouch_hop", true );
		point.high_crouch_jump = entry.value( "high_crouch_jump", true );
		point.c_high_crouch_jump = entry.value( "c_high_crouch_jump", true );
		point.active = entry.value( "active", true );
		point.radius = entry.value( "radius", 300.f );
		point.delta_strafe = entry.value( "delta_strafe", 0.0f );
		point.selected_preset = entry.value( "selected_preset", 0 );
		point.advanced_enabled = entry.value( "advanced_enabled", false );
		point.selected_advanced = entry.value( "selected_advanced", 0 );
		point.first_jump_radius = entry.value( "first_jump_radius", 250.f );
		point.use_custom_first_jump_radius = entry.value( "use_custom_first_jump_radius", false );
		point.enable_restricted_binds = entry.value( "enable_restricted_binds", false );
		point.selected_restricted_bind = entry.value( "selected_restricted_bind", 0 );
		point.restricted_binds_radius = entry.value( "restricted_binds_radius", 200.f );
		point.is_free_point = entry.value( "is_free_point", false );
		point.point_type = entry.value( "point_type", 0 );
		point.show_3d_radius = entry.value( "show_3d_radius", false );
	}

	nlohmann::json serialize_segments( )
	{
		nlohmann::json result = nlohmann::json::array( );
		for ( const auto& segment : g_movement.m_pixelsurf_data.m_trajectory_segments ) {
			nlohmann::json segment_json = { };
			segment_json[ "map_name" ] = segment.map_name;
			segment_json[ "points" ] = nlohmann::json::array( );
			for ( const auto& point : segment.points )
				segment_json[ "points" ].push_back( serialize_vec3( point ) );
			result.push_back( segment_json );
		}
		return result;
	}

	void deserialize_segments( const nlohmann::json& data )
	{
		g_movement.m_pixelsurf_data.m_trajectory_segments.clear( );
		g_movement.m_pixelsurf_data.m_trajectory_points.clear( );
		if ( !data.is_array( ) )
			return;
		for ( const auto& segment_json : data ) {
			n_movement::impl_t::trajectory_segment_t segment;
			segment.map_name = segment_json.value( "map_name", std::string( ) );
			if ( const auto points_it = segment_json.find( "points" ); points_it != segment_json.end( ) && points_it->is_array( ) ) {
				for ( const auto& point_json : *points_it ) {
					const c_vector point = deserialize_vec3( point_json );
					segment.points.push_back( point );
					g_movement.m_pixelsurf_data.m_trajectory_points.push_back( point );
				}
			}
			g_movement.m_pixelsurf_data.m_trajectory_segments.push_back( segment );
		}
	}

	nlohmann::json serialize_aimbot_settings( )
	{
		nlohmann::json result = nlohmann::json::array( );
		for ( const auto& settings : g_aimbot.m_aimbot_settings ) {
			nlohmann::json item = { };
			item[ "m_enable" ] = settings.m_enable;
			item[ "m_silent" ] = settings.m_silent;
			item[ "m_visibility_check" ] = settings.m_visibility_check;
			item[ "m_friendly_fire" ] = settings.m_friendly_fire;
			item[ "m_recoil_control" ] = settings.m_recoil_control;
			item[ "m_aim_to_backtrack" ] = settings.m_aim_to_backtrack;
			item[ "m_hitboxes" ] = nlohmann::json::array( );
			for ( size_t i = 0; i < n_aimbot::impl_t::hitboxex_form_t::max_hitboxes; ++i )
				item[ "m_hitboxes" ].push_back( settings.m_hitboxes[ i ] );
			item[ "m_fov" ] = settings.m_fov;
			item[ "m_smooth" ] = settings.m_smooth;
			item[ "m_rcs_amount" ] = settings.m_rcs_amount;
			item[ "m_autowall_min_damage" ] = settings.m_autowall_min_damage;
			item[ "m_autowall_lethal_only" ] = settings.m_autowall_lethal_only;
			result.push_back( item );
		}
		return result;
	}

	void deserialize_aimbot_settings( const nlohmann::json& data )
	{
		if ( !data.is_array( ) )
			return;
		for ( size_t i = 0; i < g_aimbot.m_aimbot_settings.size( ) && i < data.size( ); ++i ) {
			const auto& json_settings = data[ i ];
			auto& settings = g_aimbot.m_aimbot_settings[ i ];
			settings.m_enable = json_settings.value( "m_enable", false );
			settings.m_silent = json_settings.value( "m_silent", false );
			settings.m_visibility_check = json_settings.value( "m_visibility_check", false );
			settings.m_friendly_fire = json_settings.value( "m_friendly_fire", false );
			settings.m_recoil_control = json_settings.value( "m_recoil_control", false );
			settings.m_aim_to_backtrack = json_settings.value( "m_aim_to_backtrack", false );
			if ( const auto hitboxes_it = json_settings.find( "m_hitboxes" ); hitboxes_it != json_settings.end( ) && hitboxes_it->is_array( ) ) {
				for ( size_t j = 0; j < hitboxes_it->size( ) && j < n_aimbot::impl_t::hitboxex_form_t::max_hitboxes; ++j )
					settings.m_hitboxes[ j ] = ( *hitboxes_it )[ j ].get< bool >( );
			}
			settings.m_fov = json_settings.value( "m_fov", 0.f );
			settings.m_smooth = json_settings.value( "m_smooth", 1.f );
			settings.m_rcs_amount = json_settings.value( "m_rcs_amount", 50 );
			settings.m_autowall_min_damage = json_settings.value( "m_autowall_min_damage", 1 );
			settings.m_autowall_lethal_only = json_settings.value( "m_autowall_lethal_only", false );
		}
	}
}

bool n_config::impl_t::on_attach( )
{
	if ( !std::filesystem::is_directory( this->m_path ) ) {
		std::filesystem::remove( this->m_path );
		if ( !std::filesystem::create_directories( this->m_path ) )
			return false;
	}
	this->refresh( );
	return true;
}
bool n_config::impl_t::save( std::string_view file_name )
{
	std::filesystem::path file_path( file_name );
	if ( file_path.extension( ) != ( ".cfg" ) )
		file_path.replace_extension( ( ".cfg" ) );
	const std::string file = std::filesystem::path( this->m_path / file_path ).string( );
	nlohmann::json variables = nlohmann::json::array( );
	try {
		for ( auto& variable : this->m_variables ) {
			nlohmann::json entry = { };
			entry[ ( "name-id" ) ] = variable.m_name_hash;
			entry[ ( "type-id" ) ] = variable.m_type_hash;
			switch ( variable.m_type_hash ) {
			case HASH_BT( "int" ): {
				entry[ ( "value" ) ] = variable.get< int >( );
				break;
			}
			case HASH_BT( "float" ): {
				entry[ ( "value" ) ] = variable.get< float >( );
				break;
			}
			case HASH_BT( "bool" ): {
				entry[ ( "value" ) ] = variable.get< bool >( );
				break;
			}
			case HASH_BT( "std::string" ): {
				entry[ ( "value" ) ] = variable.get< std::string >( );
				break;
			}
			case HASH_BT( "c_color" ): {
				const auto& color = variable.get< c_color >( );
				nlohmann::json sub = { };
				sub.push_back( color.get< e_color_type::color_type_r >( ) );
				sub.push_back( color.get< e_color_type::color_type_g >( ) );
				sub.push_back( color.get< e_color_type::color_type_b >( ) );
				sub.push_back( color.get< e_color_type::color_type_a >( ) );
				entry[ ( "value" ) ] = sub.dump( );
				break;
			}
			case HASH_BT( "key_bind_t" ): {
				const auto& bind = variable.get< key_bind_t >( );
				nlohmann::json sub = { };
				sub.push_back( bind.m_key );
				sub.push_back( bind.m_key_style );
				entry[ ( "value" ) ] = sub.dump( );
				break;
			}
			case HASH_BT( "font_setting_t" ): {
				const auto& bind = variable.get< font_setting_t >( );
				nlohmann::json sub = { };
				sub.push_back( bind.m_name );
				sub.push_back( bind.m_size );
				entry[ ( "value" ) ] = sub.dump( );
				break;
			}
			case HASH_BT( "std::vector<bool>" ): {
				const auto& booleans = variable.get< std::vector< bool > >( );
				nlohmann::json sub = { };
				for ( const auto&& value : booleans )
					sub.push_back( static_cast< bool >( value ) );
				entry[ ( "value" ) ] = sub.dump( );
				break;
			}
			case HASH_BT( "std::vector<int>" ): {
				const auto& integers = variable.get< std::vector< int > >( );
				nlohmann::json sub = { };
				for ( const auto& value : integers )
					sub.push_back( value );
				entry[ ( "value" ) ] = sub.dump( );
				break;
			}
			case HASH_BT( "std::vector<float>" ): {
				const auto& floats = variable.get< std::vector< float > >( );
				nlohmann::json sub = { };
				for ( const auto& value : floats )
					sub.push_back( value );
				entry[ ( "value" ) ] = sub.dump( );
				break;
			}
			case HASH_BT( "std::vector<std::string>" ): {
				const auto& strings = variable.get< std::vector< std::string > >( );
				nlohmann::json sub = { };
				for ( const auto& value : strings )
					sub.push_back( value );
				entry[ ( "value" ) ] = sub.dump( );
				break;
			}
			default:
				break;
			}
			variables.push_back( entry );
		}
	} catch ( const nlohmann::detail::exception& ex ) {
		g_console.print( std::format( "failed to save {}", ex.what( ) ).c_str( ) );
		return false;
	}
	nlohmann::json config = { };
	config[ "variables" ] = variables;
	config[ "extra" ][ "points" ] = nlohmann::json::array( );
	for ( const auto& point : m_points_check )
		config[ "extra" ][ "points" ].push_back( serialize_point_entry( point ) );
	config[ "extra" ][ "bounce_points" ] = nlohmann::json::array( );
	for ( const auto& point : m_bounce_points_check )
		config[ "extra" ][ "bounce_points" ].push_back( serialize_point_entry( point ) );
	config[ "extra" ][ "pixelsurf_segments" ] = serialize_segments( );
	config[ "extra" ][ "aimbot" ] = serialize_aimbot_settings( );
	std::ofstream output_file( file, std::ios::out | std::ios::trunc );
	if ( !output_file.good( ) )
		return false;
	try {
		output_file << config.dump( 4 );
		output_file.close( );
	} catch ( std::ofstream::failure& ex ) {
			g_console.print( std::format( "failed to save {}", ex.what( ) ).c_str( ) );
		return false;
	}
	return true;
}
bool n_config::impl_t::load( std::string_view file_name )
{
	const std::string file = std::filesystem::path( this->m_path / file_name ).string( );
	nlohmann::json config  = { };
	std::ifstream input_file( file, std::ios::in );
	if ( !input_file.good( ) )
		return false;
	try {
		config = nlohmann::json::parse( input_file, nullptr, false );
		if ( config.is_discarded( ) )
			return false;
		input_file.close( );
	} catch ( std::ifstream::failure& ex ) {
		g_console.print( std::format( "failed to save {}", ex.what( ) ).c_str( ) );
		;
		return false;
	}
	const nlohmann::json* variables = &config;
	if ( config.is_object( ) ) {
		auto variables_it = config.find( "variables" );
		if ( variables_it == config.end( ) || !variables_it->is_array( ) )
			return false;
		variables = &*variables_it;
	}
	try {
		for ( const auto& variable : *variables ) {
			const unsigned int index = this->get_variable_index( variable[ ( "name-id" ) ].get< unsigned int >( ) );
			if ( index == INVALID_VARIABLE )
				continue;
			auto& entry = this->m_variables[ index ];
			const unsigned int saved_type = variable[ ( "type-id" ) ].get< unsigned int >( );
			if ( variable[ ( "name-id" ) ].get< unsigned int >( ) == HASH_BT( "m_menu_dpi_scale" ) ) {
				int dpi_value = 100;
				if ( saved_type == HASH_BT( "float" ) ) {
					const float old_value = variable[ ( "value" ) ].get< float >( );
					dpi_value = static_cast< int >( std::round( old_value <= 2.0f ? old_value * 100.0f : old_value ) );
				} else if ( saved_type == HASH_BT( "int" ) ) {
					dpi_value = variable[ ( "value" ) ].get< int >( );
				}
				entry.set< int >( dpi_value < 75 ? 75 : dpi_value > 200 ? 200 : dpi_value );
				continue;
			}
			switch ( saved_type ) {
			case HASH_BT( "bool" ): {
				entry.set< bool >( variable[ ( "value" ) ].get< bool >( ) );
				break;
			}
			case HASH_BT( "float" ): {
				entry.set< float >( variable[ ( "value" ) ].get< float >( ) );
				break;
			}
			case HASH_BT( "int" ): {
				entry.set< int >( variable[ ( "value" ) ].get< int >( ) );
				break;
			}
			case HASH_BT( "std::string" ): {
				entry.set< std::string >( variable[ ( "value" ) ].get< std::string >( ) );
				break;
			}
			case HASH_BT( "c_color" ): {
				const nlohmann::json vector = nlohmann::json::parse( variable[ ( "value" ) ].get< std::string >( ) );
				entry.set< c_color >( c_color( vector[ 0 ].get< std::uint8_t >( ), vector[ 1 ].get< std::uint8_t >( ),
				                               vector[ 2 ].get< std::uint8_t >( ), vector[ 3 ].get< std::uint8_t >( ) ) );
				break;
			}
			case HASH_BT( "key_bind_t" ): {
				const nlohmann::json vector = nlohmann::json::parse( variable[ ( "value" ) ].get< std::string >( ) );
				entry.set< key_bind_t >( key_bind_t( vector[ 0 ].get< int >( ), vector[ 1 ].get< int >( ) ) );
				break;
			}
			case HASH_BT( "font_setting_t" ): {
				const nlohmann::json vector = nlohmann::json::parse( variable[ ( "value" ) ].get< std::string >( ) );
				entry.set< font_setting_t >( font_setting_t( vector[ 0 ].get< std::string >( ), vector[ 1 ].get< int >( ) ) );
				break;
			}
			case HASH_BT( "std::vector<bool>" ): {
				const nlohmann::json vector = nlohmann::json::parse( variable[ ( "value" ) ].get< std::string >( ) );
				auto& booleans              = entry.get< std::vector< bool > >( );
				for ( std::size_t i = 0U; i < vector.size( ); i++ ) {
					if ( i < booleans.size( ) )
						booleans[ i ] = vector[ i ].get< bool >( );
				}
				break;
			}
			case HASH_BT( "std::vector<int>" ): {
				const nlohmann::json vector = nlohmann::json::parse( variable[ ( "value" ) ].get< std::string >( ) );
				auto& integers              = entry.get< std::vector< int > >( );
				for ( std::size_t i = 0U; i < vector.size( ); i++ ) {
					if ( i < integers.size( ) )
						integers[ i ] = vector[ i ].get< int >( );
				}
				break;
			}
			case HASH_BT( "std::vector<float>" ): {
				const nlohmann::json vector = nlohmann::json::parse( variable[ ( "value" ) ].get< std::string >( ) );
				auto& floats                = entry.get< std::vector< float > >( );
				for ( std::size_t i = 0U; i < vector.size( ); i++ ) {
					if ( i < floats.size( ) )
						floats[ i ] = vector[ i ].get< float >( );
				}
				break;
			}
			case HASH_BT( "std::vector<std::string>" ): {
				const nlohmann::json vector = nlohmann::json::parse( variable[ ( "value" ) ].get< std::string >( ) );
				auto& strings               = entry.get< std::vector< std::string > >( );
				for ( std::size_t i = 0U; i < vector.size( ); i++ ) {
					if ( i < strings.size( ) )
						strings[ i ] = vector[ i ].get< std::string >( );
				}
				break;
			}
			default:
				break;
			}
		}
	} catch ( const nlohmann::detail::exception& ex ) {
		g_console.print( std::format( "josn {}", ex.what( ) ).c_str( ) );
		return false;
	}
	if ( config.is_object( ) ) {
		if ( const auto extra_it = config.find( "extra" ); extra_it != config.end( ) && extra_it->is_object( ) ) {
			m_points_check.clear( );
			if ( const auto points_it = extra_it->find( "points" ); points_it != extra_it->end( ) && points_it->is_array( ) ) {
				for ( const auto& point_json : *points_it ) {
					points_check_t point;
					deserialize_point_entry( point_json, point );
					m_points_check.push_back( point );
				}
			}
			m_bounce_points_check.clear( );
			if ( const auto bounce_it = extra_it->find( "bounce_points" ); bounce_it != extra_it->end( ) && bounce_it->is_array( ) ) {
				for ( const auto& point_json : *bounce_it ) {
					points_check_t point;
					deserialize_point_entry( point_json, point );
					m_bounce_points_check.push_back( point );
				}
			}
			if ( const auto segments_it = extra_it->find( "pixelsurf_segments" ); segments_it != extra_it->end( ) )
				deserialize_segments( *segments_it );
			if ( const auto aimbot_it = extra_it->find( "aimbot" ); aimbot_it != extra_it->end( ) )
				deserialize_aimbot_settings( *aimbot_it );
		}
	}
	return true;
}
void n_config::impl_t::remove( const std::size_t index )
{
	const std::string& file_name = this->m_file_names[ index ];
	const std::string file       = std::filesystem::path( this->m_path / file_name ).string( );
	if ( std::filesystem::remove( file ) )
		this->m_file_names.erase( this->m_file_names.cbegin( ) + static_cast< std::ptrdiff_t >( index ) );
}
void n_config::impl_t::refresh( )
{
	this->m_file_names.clear( );
	for ( const auto& it : std::filesystem::directory_iterator( this->m_path ) )
		if ( it.path( ).filename( ).extension( ) == ( ".cfg" ) )
			this->m_file_names.push_back( it.path( ).filename( ).string( ) );
}
std::size_t n_config::impl_t::get_variable_index( const unsigned int name_hash )
{
	for ( unsigned int i = 0U; i < this->m_variables.size( ); i++ ) {
		if ( this->m_variables[ i ].m_name_hash == name_hash )
			return i;
	}
	return INVALID_VARIABLE;
}
