#pragma once
#include "../../game/sdk/classes/c_vector.h"
#include "../../game/sdk/classes/c_angle.h"
#include "../../game/sdk/classes/c_engine_trace.h"

// ─────────────────────────────────────────────────────────────────────────────
// Rebuilt Source engine movement simulation (CGameMovement).
// All functions are pure — they operate only on the passed-in state struct and
// never touch game entity memory.  The only engine call is trace_ray(), which
// is read-only.
// ─────────────────────────────────────────────────────────────────────────────
namespace source_move
{
	// ── constants matching Source 2013 / CSGO ──
	constexpr int   MAX_CLIP_PLANES      = 5;
	constexpr int   MAX_BUMPS            = 4;
	constexpr float OVERCLIP             = 1.000f;
	constexpr float SURF_EPSILON         = 0.03125f;          // 1/32
	constexpr float AIR_SPEED_CAP        = 30.0f;             // hard-coded in AirAccelerate
	constexpr float COORD_FRAME_MAX      = 16384.0f;
	constexpr float STOP_EPSILON         = 0.1f;

	// ── simulation I/O ──
	struct move_data_t
	{
		c_vector origin;
		c_vector velocity;
		c_vector base_velocity;         // trigger push etc.  Usually 0.
		c_vector mins;                  // OBB mins  (e.g. {-16,-16, 0})
		c_vector maxs;                  // OBB maxs  (e.g. { 16, 16, 64})
		float    surface_friction;      // player surface friction multiplier (default 1)

		// convars
		float    sv_gravity;            // 800
		float    sv_airaccelerate;      // 10
		float    sv_maxspeed;           // 250 / 320
		float    tick_interval;         // 1/64 = 0.015625

		// cmd input
		float    forward_move;
		float    side_move;
		c_angle  view_angles;
		bool     duck;                  // in_duck pressed

		// output flags
		bool     on_ground;             // set by TryPlayerMove if landed
		bool     clipped;               // true if any clip happened
		int      clip_count;            // how many surfaces were clipped
		c_vector last_clip_normal;      // normal of last clip plane
	};

	// ── public API ──

	// Simulate one full air-movement tick.  Fills `data` in-place.
	void simulate_air_tick( move_data_t& data );

	// Individual phases (exposed for unit-testing / analytical solvers)
	void start_gravity( move_data_t& data );
	void finish_gravity( move_data_t& data );
	void air_accelerate( move_data_t& data, const c_vector& wishdir, float wishspeed );
	int  clip_velocity( const c_vector& in, const c_vector& normal, c_vector& out, float overbounce );
	int  try_player_move( move_data_t& data );

	// Compute wishdir + wishspeed from cmd inputs + viewangles.
	void compute_wish_velocity( const move_data_t& data, c_vector& out_wishdir, float& out_wishspeed );
}
