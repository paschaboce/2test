#pragma once

#include "../skinchanger/skinchanger.h"

namespace n_knife_changer
{
	struct impl_t {
		// called every FrameStageNotify; applies model overrides for T/CT knives
		void on_frame_stage_notify( int stage );

		// force server-side netvar resync so the swap takes effect immediately
		void request_full_update( );

		// returns the knife definition index currently configured for the given team,
		// or 0 / default (42 CT, 59 T) if the user selected "Default".
		int get_override_definition_for_team( int team ) const;

		// true if feature is enabled AND at least one side has a non-default knife
		bool has_any_override( ) const;
	};
}

inline n_knife_changer::impl_t g_knife_changer{ };
