#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../../hacks/inventory/inventory_changer.h"
#include "../hooks.h"

unsigned long __fastcall n_detoured_functions::find_mdl( void* ecx, void* edx, char* path )
{
	static auto original = g_hooks.m_find_mdl.get_original< decltype( &n_detoured_functions::find_mdl ) >( );

	if ( !path || !*path )
		return original( ecx, edx, path );

	return original( ecx, edx, path );
}
