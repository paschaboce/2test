#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../hooks.h"
#undef max
int __fastcall n_detoured_functions::send_datagram( c_net_channel* net_channel, int edx, bf_write* datagram )
{
	static auto original = g_hooks.m_send_datagram.get_original< decltype( &n_detoured_functions::send_datagram ) >( );
	return original( net_channel, edx, datagram );
}