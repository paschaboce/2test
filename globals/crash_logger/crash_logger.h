#pragma once
// ebanat_crash.log отключён — слишком много мусора в рантайме.
// Макросы превращены в no-op заглушки, чтобы все call-сайты продолжали
// компилироваться без изменений. Файл на диске больше не создаётся.
//
// Если когда-нибудь понадобится реальный crash logger — верни старую
// реализацию из git-истории этого файла.
#include <windows.h>
#include <string>

struct crash_logger_t_stub {
	// No-op заглушки. Сигнатуры совпадают с прежним crash_logger_t, чтобы
	// прямые вызовы (g_crash_logger.log(...), .init(), .log_error(...),
	// .log_exception(...), .get_create_move_calls(), .increment_create_move())
	// продолжали компилироваться без изменений в call-сайтах.
	void init( ) noexcept { }
	void log( const std::string& /*module*/, const std::string& /*message*/ ) noexcept { }
	void log_error( const std::string& /*module*/, const std::string& /*error*/ ) noexcept { }
	void log_exception( const std::string& /*module*/, const std::string& /*exception*/ ) noexcept { }
	void log_crash( EXCEPTION_POINTERS* /*exception_info*/ ) noexcept { }
	void increment_create_move( ) noexcept { }
	int  get_create_move_calls( ) const noexcept { return 0; }
};
inline crash_logger_t_stub g_crash_logger;

inline LONG WINAPI crash_exception_handler( EXCEPTION_POINTERS* /*exception_info*/ )
{
	return EXCEPTION_CONTINUE_SEARCH;
}

#define LOG_INIT()              ( ( void )0 )
#define LOG( msg )              ( ( void )0 )
#define LOG_CAT( category, msg ) ( ( void )0 )
#define LOG_ERROR( msg )        ( ( void )0 )
#define LOG_WARNING( msg )      ( ( void )0 )
#define LOG_INFO( msg )         ( ( void )0 )
#define LOG_CREATE_MOVE()       ( ( void )0 )