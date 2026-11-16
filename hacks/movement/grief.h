#pragma once
#include "../../globals/includes/includes.h"

namespace n_grief
{
	struct impl_t {
		void blockbot( c_user_cmd* cmd );
		void door_spam( c_user_cmd* cmd );
		void deagle_spinner( c_base_entity* local, void* viewmodel, long& sequence );
	};
}

extern n_grief::impl_t g_grief;
