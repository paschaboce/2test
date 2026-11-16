#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <windows.h>
#include <wincrypt.h>
#include <winreg.h>
#include <winhttp.h>
#include "dependencies/json/json.hpp"
#include <sstream>
#include "game/sdk/includes/includes.h"
#include "globals/fonts/fonts.h"
#include "globals/includes/includes.h"
#include "globals/crash_logger/crash_logger.h"
#include "hacks/misc/misc.h"
extern void lester_kill_sound_stop( );
extern void release_inventory_animation_hooks( );
#include "hacks/misc/spectator_gif_gdiplus.h"
#include "hacks/misc/pts_images_embedded.h"
#include <TlHelp32.h>
using json = nlohmann::json;
std::string sha256( const std::string& str )
{
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	BYTE hash[ 32 ];
	DWORD hashSize = sizeof( hash );
	if ( !CryptAcquireContext( &hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT ) )
		return "";
	if ( !CryptCreateHash( hProv, CALG_SHA_256, 0, 0, &hHash ) ) {
		CryptReleaseContext( hProv, 0 );
		return "";
	}
	CryptHashData( hHash, ( BYTE* )str.c_str( ), str.size( ), 0 );
	CryptGetHashParam( hHash, HP_HASHVAL, hash, &hashSize, 0 );
	CryptDestroyHash( hHash );
	CryptReleaseContext( hProv, 0 );
	std::ostringstream oss;
	for ( DWORD i = 0; i < hashSize; i++ )
		oss << std::hex << ( int )hash[ i ];
	return oss.str( );
}
std::string read_registry( const wchar_t* path, const wchar_t* name )
{
	HKEY hKey;
	if ( RegOpenKeyExW( HKEY_LOCAL_MACHINE, path, 0, KEY_READ | KEY_WOW64_64KEY, &hKey ) == ERROR_SUCCESS ) {
		char value[ 256 ];
		DWORD len = sizeof( value );
		if ( RegQueryValueExW( hKey, name, NULL, NULL, ( LPBYTE )value, &len ) == ERROR_SUCCESS ) {
			RegCloseKey( hKey );
			return std::string( value, len - 1 );
		}
		RegCloseKey( hKey );
	}
	return "";
}
std::string gethwid()
{
    std::string hwid_big_anal;
    HW_PROFILE_INFO hwProfileInfo;
    if (GetCurrentHwProfile(&hwProfileInfo))
        hwid_big_anal = hwProfileInfo.szHwProfileGuid;
    return hwid_big_anal;
}

namespace
{
	void copy_text_to_clipboard( const std::string& text )
	{
		if ( text.empty( ) )
			return;

		if ( !OpenClipboard( nullptr ) )
			return;

		EmptyClipboard( );
		HGLOBAL handle = GlobalAlloc( GMEM_MOVEABLE, text.size( ) + 1 );
		if ( handle ) {
			char* buffer = static_cast< char* >( GlobalLock( handle ) );
			if ( buffer ) {
				memcpy( buffer, text.c_str( ), text.size( ) + 1 );
				GlobalUnlock( handle );
				SetClipboardData( CF_TEXT, handle );
			} else {
				GlobalFree( handle );
			}
		}

		CloseClipboard( );
	}

	bool is_allowed_dev_hwid( const std::string& hwid )
	{
		return true;
	}

	bool g_dev_hwid_allowed = false;
	std::string g_cached_hwid;
}

bool is_dev_hwid_allowed( )
{
	return g_dev_hwid_allowed;
}

const std::string& get_cached_hwid( )
{
	return g_cached_hwid;
}
bool read_key_from_registry( std::string& key )
{
	HKEY hKey;
	char value[ 256 ];
	DWORD len = sizeof( value );
	if ( RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\ArbuzLoader", 0, KEY_READ, &hKey ) != ERROR_SUCCESS )
		return false;
	if ( RegQueryValueExA( hKey, "Key", 0, NULL, ( LPBYTE )value, &len ) != ERROR_SUCCESS ) {
		RegCloseKey( hKey );
		return false;
	}
	key = std::string( value );
	RegCloseKey( hKey );
	return true;
}
void delete_key_from_registry( )
{
	RegDeleteKeyA( HKEY_CURRENT_USER, "Software\\ArbuzLoader" );
}
std::string xor_string( const std::string& data, int key )
{
	std::string res = data;
	for ( auto& c : res )
		c ^= key;
	return res;
}
std::string post_json( const std::string& url, const json& jdata, const std::string& user_agent )
{
	HINTERNET hSession = nullptr;
	HINTERNET hConnect = nullptr;
	HINTERNET hRequest = nullptr;
	std::string response;
	try {
		std::string protocol, host, path;
		size_t protocolEnd = url.find( "://" );
		if ( protocolEnd == std::string::npos )
			return "";
		protocol = url.substr( 0, protocolEnd );
		std::string remaining = url.substr( protocolEnd + 3 );
		size_t pathStart = remaining.find( '/' );
		if ( pathStart == std::string::npos ) {
			host = remaining;
			path = "/";
		} else {
			host = remaining.substr( 0, pathStart );
			path = remaining.substr( pathStart );
		}
		std::wstring wUserAgent( user_agent.begin( ), user_agent.end( ) );
		hSession = WinHttpOpen( wUserAgent.c_str( ), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
		if ( !hSession )
			return "";
		std::wstring wHost( host.begin( ), host.end( ) );
		hConnect = WinHttpConnect( hSession, wHost.c_str( ), INTERNET_DEFAULT_HTTPS_PORT, 0 );
		if ( !hConnect )
			return "";
		hRequest = WinHttpOpenRequest( hConnect, L"POST", std::wstring( path.begin( ), path.end( ) ).c_str( ), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE );
		if ( !hRequest )
			return "";
		std::string headers = "Content-Type: application/json\r\n";
		WinHttpAddRequestHeaders( hRequest, std::wstring( headers.begin( ), headers.end( ) ).c_str( ), -1, WINHTTP_ADDREQ_FLAG_ADD );
		std::string data = jdata.dump( );
		if ( !WinHttpSendRequest( hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)data.c_str( ), data.length( ), data.length( ), 0 ) )
			return "";
		if ( !WinHttpReceiveResponse( hRequest, nullptr ) )
			return "";
		DWORD dwSize = 0;
		DWORD dwDownloaded = 0;
		do {
			dwSize = 0;
			if ( !WinHttpQueryDataAvailable( hRequest, &dwSize ) )
				break;
			if ( dwSize > 0 ) {
				std::vector<char> buffer( dwSize + 1 );
				if ( WinHttpReadData( hRequest, buffer.data( ), dwSize, &dwDownloaded ) ) {
					buffer[dwDownloaded] = '\0';
					response += buffer.data( );
				}
			}
		} while ( dwSize > 0 );
	} catch ( ... ) {
		response = "";
	}
	if ( hRequest ) WinHttpCloseHandle( hRequest );
	if ( hConnect ) WinHttpCloseHandle( hConnect );
	if ( hSession ) WinHttpCloseHandle( hSession );
	return response;
}
std::string GetSteamInfPath( )
{
	char exePath[ MAX_PATH ];
	DWORD length = GetModuleFileNameA( NULL, exePath, MAX_PATH );
	if ( length == 0 ) {
		return "";
	}
	std::string fullPath( exePath, length );
	size_t pos = fullPath.find_last_of( "\\/" );
	if ( pos == std::string::npos ) {
		return "";
	}
	std::string folderPath = fullPath.substr( 0, pos );
	std::string steamInfPath = folderPath + "\\csgo\\steam.inf";
	return steamInfPath;
}
void cleanup_before_unload( )
{
	static LONG unload_started = 0;
	if ( InterlockedExchange( &unload_started, 1 ) )
		return;

	g_ctx.m_unloading = true;
	g_menu.m_opened = false;

	if ( g_interfaces.m_input_system )
		g_interfaces.m_input_system->enable_input( true );

	Sleep( 100 );
	g_input.on_release( );
	g_render.on_release( );
	release_inventory_animation_hooks( );
	g_hooks.on_release( );
	Sleep( 250 );

	lester_kill_sound_stop( );
	memesclass.reset();
	player.sessionManager.reset( );
	player.session.reset( );
	player.thumbnail.reset( );
	g_spectator_gif_original.Shutdown();
	g_spectator_gif_shit.Shutdown();

	if ( g_interfaces.m_client_state )
		g_interfaces.m_client_state->m_delta_tick = -1;

	g_console.on_release( );
}
void CleanupAndUnload( HMODULE instance )
{
	cleanup_before_unload( );
}
DWORD WINAPI OnDllAttach( LPVOID base )
{
	LOG_INIT();
	LOG("===========================================");
	LOG("CHEAT INITIALIZATION STARTED");
	LOG("===========================================");
	if ( !g_modules.on_attach( ) );
	memesclass = std::make_unique< const Memory >( );
	if ( !g_interfaces.on_attach( ) );
	if ( !g_netvars.on_attach( ) );
	if ( !g_convars.on_attach( ) );
	if ( !g_input.on_attach( ) );
	if ( !g_config.on_attach( ) );
	g_config.load( "default" );
	GET_VARIABLE(g_variables.m_spectators_gif_width, float) = 64.0f;
	GET_VARIABLE(g_variables.m_spectators_gif_height, float) = 64.0f;
	GET_VARIABLE(g_variables.m_spectators_gif_offset_y, float) = 0.0f;
	if ( !g_hooks.on_attach( ) );
	LI_FN( GetWindowsDirectoryA )( g_ctx.m_windows_directory, 64 );
	g_fonts.on_attach( );
	while ( !g_input.is_key_down( VK_END ) && !g_input.is_key_down( VK_DELETE ) ) {
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
	}
	cleanup_before_unload( );
	FreeLibraryAndExitThread( static_cast< HMODULE >( base ), 1 );
	return 0;
}
std::wstring ConvertUtf8ToWide( const std::string& utf8Str )
{
	if ( utf8Str.empty( ) ) {
		return std::wstring( );
	}
	int size_needed = MultiByteToWideChar( CP_UTF8, 0, utf8Str.c_str( ), ( int )utf8Str.size( ), nullptr, 0 );
	std::wstring wideStr( size_needed, 0 );
	MultiByteToWideChar( CP_UTF8, 0, utf8Str.c_str( ), ( int )utf8Str.size( ), &wideStr[ 0 ], size_needed );
	return wideStr;
}
LPDIRECT3DTEXTURE9 LoadTextureFromFile( const std::string& path )
{
	LPDIRECT3DTEXTURE9 texture = NULL;
	std::wstring widePath      = ConvertUtf8ToWide( path );
	if ( D3DXCreateTextureFromFileW( g_interfaces.m_direct_device, widePath.c_str( ), &texture ) != D3D_OK ) {
		std::wcerr << L"Failed to load texture: " << widePath << std::endl;
		return NULL;
	}
	return texture;
}
void GetNowPlayingInfoAndSaveAlbumArt( void* instance )
{
	player.Initialize( );
	while ( true ) {
		if ( g_ctx.m_unloading )
			break;
		if ( !GET_VARIABLE( g_variables.trackdispay, bool ) ) {
			Sleep( 10000 );
			continue;
		}
		player.Update( g_interfaces.m_direct_device );
		if ( oldtitle != strtitle || oldartist != strartist || !albumArtTexture ) {
			albumArtTexture = player.thumb;
			oldtitle        = strtitle;
			oldartist       = strartist;
		}
		Sleep( 1 );
		strartist = player.Artist;
		Sleep( 1 );
		strtitle = player.Title;
		Sleep( 1000 );
	}
}
void TogglePlayPause( void* instance )
{
	while ( true ) {
		if ( g_ctx.m_unloading )
			break;
		if ( !GET_VARIABLE( g_variables.trackdispay, bool ) ) {
			Sleep( 10000 );
			continue;
		}
		if ( g_variables.pause ) {
			auto sessions = GlobalSystemMediaTransportControlsSessionManager::RequestAsync( ).get( );
			if ( sessions ) {
				auto currentSession = sessions.GetCurrentSession( );
				if ( currentSession ) {
					auto mediaProperties = currentSession.TryGetMediaPropertiesAsync( ).get( );
					auto playbackInfo    = currentSession.GetPlaybackInfo( );
					if ( playbackInfo ) {
						auto playbackStatus = playbackInfo.PlaybackStatus( );
						if ( playbackStatus == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing )
							currentSession.TryPauseAsync( ).get( );
						else if ( playbackStatus == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused ||
						          playbackStatus == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped )
							currentSession.TryPlayAsync( ).get( );
					}
				}
			}
			g_variables.pause = false;
		}
		Sleep( 1000 );
	}
}
void UpdatePosition( void* instance )
{
	while ( true ) {
		if ( g_ctx.m_unloading )
			break;
		if ( !GET_VARIABLE( g_variables.trackdispay, bool ) ) {
			Sleep( 10000 );
			continue;
		}
		auto sessions = GlobalSystemMediaTransportControlsSessionManager::RequestAsync( ).get( );
		if ( sessions ) {
			auto currentSession = sessions.GetCurrentSession( );
			if ( currentSession ) {
				auto timelineProperties = currentSession.GetTimelineProperties( );
				g_variables.trackDuration =
					std::chrono::duration_cast< std::chrono::milliseconds >( timelineProperties.EndTime( ) - timelineProperties.StartTime( ) )
						.count( );
				g_variables.trackPosition =
					std::chrono::duration_cast< std::chrono::milliseconds >( timelineProperties.Position( ) - timelineProperties.StartTime( ) )
						.count( );
			}
		}
		Sleep( 1000 );
	}
}
void SkipToNextTrack( void* instance )
{
	while ( true ) {
		if ( g_ctx.m_unloading )
			break;
		if ( !GET_VARIABLE( g_variables.trackdispay, bool ) ) {
			Sleep( 10000 );
			continue;
		}
		if ( g_variables.next ) {
			auto sessions = GlobalSystemMediaTransportControlsSessionManager::RequestAsync( ).get( );
			if ( sessions ) {
				auto currentSession = sessions.GetCurrentSession( );
				if ( currentSession ) {
					currentSession.TrySkipNextAsync( ).get( );
				}
			}
			g_variables.next = false;
		}
		Sleep( 1000 );
	}
}
void SkipToPreviousTrack( void* instance )
{
	while ( true ) {
		if ( g_ctx.m_unloading )
			break;
		if ( !GET_VARIABLE( g_variables.trackdispay, bool ) ) {
			Sleep( 10000 );
			continue;
		}
		if ( g_variables.pervious ) {
			auto sessions = GlobalSystemMediaTransportControlsSessionManager::RequestAsync( ).get( );
			if ( sessions ) {
				auto currentSession = sessions.GetCurrentSession( );
				if ( currentSession ) {
					currentSession.TrySkipPreviousAsync( ).get( );
				}
			}
			g_variables.pervious = false;
		}
		Sleep( 1000 );
	}
}
DWORD WINAPI SafeInitThread( LPVOID base )
{
   



	g_crash_logger.init();
	g_crash_logger.log("INIT", "SafeInitThread started");
	SetUnhandledExceptionFilter(crash_exception_handler);
	g_crash_logger.log("INIT", "Exception handler installed");
	try {
	g_cached_hwid = gethwid( );
	copy_text_to_clipboard( g_cached_hwid );
	g_dev_hwid_allowed = is_allowed_dev_hwid( g_cached_hwid );

	g_crash_logger.log("INIT", "Initializing modules");
	if ( !g_modules.on_attach( ) ) {
		g_console.print< n_console::log_level::WARNING >( "failed to initialise module handles" );
		g_crash_logger.log_error("INIT", "Failed to initialise module handles");
	} else {
		g_console.print( "initialised module handles" );
		g_crash_logger.log("INIT", "Module handles initialized");
	}
	g_crash_logger.log("INIT", "Creating Memory class");
	memesclass = std::make_unique< const Memory >( );
	g_console.print( "initialising interfaces" );
	g_crash_logger.log("INIT", "Initializing interfaces");
	if ( !g_interfaces.on_attach( ) ) {
		g_console.print< n_console::log_level::WARNING >( "failed to initialise interfaces" );
		g_crash_logger.log_error("INIT", "Failed to initialise interfaces");
	} else {
		g_console.print( "initialised interfaces" );
		g_crash_logger.log("INIT", "Interfaces initialized");
	}
	g_console.print( "initialising netvar manager" );
	g_crash_logger.log("INIT", "Initializing netvar manager");
	if ( !g_netvars.on_attach( ) ) {
		g_console.print< n_console::log_level::WARNING >( "failed to initialise netvar manager" );
	} else {
		g_console.print( "initialised netvar manager" );
	}
	g_console.print( "initialising convars" );
	if ( !g_convars.on_attach( ) ) {
		g_console.print< n_console::log_level::WARNING >( "failed to initialise convars" );
	} else {
		g_console.print( "initialised convars" );
	}
	g_console.print( "initialising input system" );
	if ( !g_input.on_attach( ) ) {
		g_console.print< n_console::log_level::WARNING >( "failed to initialise input system" );
	} else {
		g_console.print( "initialised input system" );
	}
	g_console.print( "initialising config system" );
	if ( !g_config.on_attach( ) ) {
		g_console.print< n_console::log_level::WARNING >( "failed to initialise config system" );
	} else {
		g_console.print( "initialised config system" );
	}
	g_console.print( "loading default config" );
	if ( !g_config.load( "default" ) ) {
		g_console.print< n_console::log_level::WARNING >( "failed to load default config" );
	} else {
		g_console.print( "loaded default config" );
	}
	GET_VARIABLE(g_variables.m_spectators_gif_width, float) = 64.0f;
	GET_VARIABLE(g_variables.m_spectators_gif_height, float) = 64.0f;
	GET_VARIABLE(g_variables.m_spectators_gif_offset_y, float) = 0.0f;
	g_console.print("[GIF] Updated GIF size to 64x64 with 0 offset");
	g_console.print( "initialising hooks" );
	if ( !g_hooks.on_attach( ) ) {
		g_console.print< n_console::log_level::WARNING >( "failed to initialise hooks" );
	} else {
		g_console.print( "initialised hooks" );
	}
	g_ctx.m_initializing = false;
	LI_FN( GetWindowsDirectoryA )( g_ctx.m_windows_directory, 64 );
	g_console.print( std::vformat( "windows directory - {:s}", std::make_format_args( g_ctx.m_windows_directory ) ).c_str( ) );
	g_fonts.on_attach( );
	g_console.print( "initialised font file vector" );
	CreateThread( nullptr, 0, []( LPVOID ) -> DWORD {
		int wait_count = 0;
		while ( !g_interfaces.m_direct_device && !g_ctx.m_unloading ) {
			std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
			wait_count++;
			if ( wait_count > 20 ) {
				return 0;
			}
		}
		if ( g_ctx.m_unloading ) {
			return 0;
		}
		if ( g_misc.m_pts_images.m_image1_texture ) {
			g_misc.m_pts_images.m_image1_texture->Release();
			g_misc.m_pts_images.m_image1_texture = nullptr;
		}
		if ( g_misc.m_pts_images.m_image2_texture ) {
			g_misc.m_pts_images.m_image2_texture->Release();
			g_misc.m_pts_images.m_image2_texture = nullptr;
		}
		if ( g_misc.m_pts_images.m_image3_texture ) {
			g_misc.m_pts_images.m_image3_texture->Release();
			g_misc.m_pts_images.m_image3_texture = nullptr;
		}
		if ( g_misc.m_pts_images.m_image4_texture ) {
			g_misc.m_pts_images.m_image4_texture->Release();
			g_misc.m_pts_images.m_image4_texture = nullptr;
		}
		if ( !g_misc.m_pts_images.m_image1_texture ) {
			g_misc.m_pts_images.m_image1_texture = g_misc.create_texture_from_bytes( pts_image1_data, pts_image1_data_size );
		}
		if ( !g_misc.m_pts_images.m_image2_texture ) {
			g_misc.m_pts_images.m_image2_texture = g_misc.create_texture_from_bytes( pts_image2_data, pts_image2_data_size );
		}
		if ( !g_misc.m_pts_images.m_image3_texture ) {
			g_misc.m_pts_images.m_image3_texture = g_misc.create_texture_from_bytes( pts_image3_data, pts_image3_data_size );
		}
		if ( !g_misc.m_pts_images.m_image4_texture ) {
			g_misc.m_pts_images.m_image4_texture = g_misc.create_texture_from_bytes( pts_image3_data, pts_image3_data_size );
		}
		if (!g_spectator_gif_original.IsInitialized()) {
			g_spectator_gif_original.Initialize(kSpectatorGifData, sizeof(kSpectatorGifData));
		}
		if (!g_spectator_gif_shit.IsInitialized()) {
			g_spectator_gif_shit.Initialize(kShitGifData, kShitGifDataSize);
		}
		return 0;
	}, nullptr, 0, nullptr );
	g_crash_logger.log("INIT", "Initialization complete, entering main loop");
	g_console.print("[HOT RELOAD] Hold PgUp + PgDown to unload DLL");
	while ( !g_ctx.m_unloading ) {
		const bool pgup_down = ( GetAsyncKeyState( VK_PRIOR ) & 0x8000 ) != 0;
		const bool pgdown_down = ( GetAsyncKeyState( VK_NEXT ) & 0x8000 ) != 0;
		if ( pgup_down && pgdown_down ) {
			break;
		}
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
	}

	cleanup_before_unload( );
	FreeLibraryAndExitThread( static_cast< HMODULE >( base ), 1 );
	return 0;
	} catch (const std::exception& e) {
		g_crash_logger.log_exception("INIT", std::string("Exception in SafeInitThread: ") + e.what());
		MessageBoxA(nullptr, e.what(), "Fatal Error", MB_OK | MB_ICONERROR);
		FreeLibraryAndExitThread( static_cast< HMODULE >( base ), 1 );
		return 1;
	} catch (...) {
		g_crash_logger.log_exception("INIT", "Unknown exception in SafeInitThread");
		MessageBoxA(nullptr, "Unknown exception occurred", "Fatal Error", MB_OK | MB_ICONERROR);
		FreeLibraryAndExitThread( static_cast< HMODULE >( base ), 1 );
		return 1;
	}
	return 0;
}
BOOL APIENTRY DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
{
	switch ( ul_reason_for_call ) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls( hModule );
		CreateThread( nullptr, 0, SafeInitThread, hModule, 0, nullptr );
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
