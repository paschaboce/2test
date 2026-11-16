#include "source_move.h"
#include "../../globals/interfaces/interfaces.h"
#include <cmath>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Rebuilt Source engine CGameMovement — air path only.
// Reference: Source SDK 2013  gamemovement.cpp / gamemovement.h
// ─────────────────────────────────────────────────────────────────────────────

namespace source_move
{

// ── helpers ──────────────────────────────────────────────────────────────────

static void trace_player_bbox( const c_vector& start, const c_vector& end,
                               const c_vector& mins, const c_vector& maxs,
                               trace_t& result )
{
	c_trace_filter_world filter;
	ray_t ray( start, end, mins, maxs );
	g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid_brushonly, &filter, &result );
}

// ── ClipVelocity ─────────────────────────────────────────────────────────────
// Source SDK: CGameMovement::ClipVelocity
// Returns 1 if clipped, 0x2 if floor, 0 otherwise.
int clip_velocity( const c_vector& in, const c_vector& normal, c_vector& out, float overbounce )
{
	const float backoff = in.dot_product( normal ) * overbounce;

	out.m_x = in.m_x - normal.m_x * backoff;
	out.m_y = in.m_y - normal.m_y * backoff;
	out.m_z = in.m_z - normal.m_z * backoff;

	// Source engine: snap tiny values to zero to prevent float drift
	for ( int i = 0; i < 3; i++ )
	{
		if ( std::fabsf( out[i] ) < SURF_EPSILON )
			out[i] = 0.f;
	}

	int blocked = 0;
	if ( normal.m_z > 0.f )
		blocked |= 0x1;    // floor
	if ( std::fabsf( normal.m_z ) < 0.01f )
		blocked |= 0x2;    // step / wall

	return blocked;
}

// ── TryPlayerMove ────────────────────────────────────────────────────────────
// Source SDK: CGameMovement::TryPlayerMove
// Traces the player hull along velocity, clips against surfaces up to
// MAX_BUMPS times.  Modifies data.origin and data.velocity.
int try_player_move( move_data_t& data )
{
	int   blocked     = 0;
	int   num_planes  = 0;
	float time_left   = data.tick_interval;
	float allFraction = 0.f;

	c_vector planes[MAX_CLIP_PLANES];
	c_vector original_velocity = data.velocity;
	c_vector primal_velocity   = data.velocity;
	c_vector new_velocity{};

	data.clipped          = false;
	data.clip_count       = 0;
	data.last_clip_normal = c_vector{};

	for ( int bump = 0; bump < MAX_BUMPS; bump++ )
	{
		if ( data.velocity.length_squared() < 0.0001f )
			break;

		c_vector end = data.origin + data.velocity * time_left;

		trace_t trace;
		trace_player_bbox( data.origin, end, data.mins, data.maxs, trace );

		allFraction += trace.m_fraction;

		// stuck in solid
		if ( trace.m_all_solid )
		{
			data.velocity = c_vector{};
			return 4;
		}

		// moved some distance
		if ( trace.m_fraction > 0.f )
		{
			// Source: "actually covered some distance"
			data.origin = trace.m_end;
			original_velocity = data.velocity;
			num_planes = 0;
		}

		// moved entire distance
		if ( trace.m_fraction == 1.f )
			break;

		// hit a surface — record it
		data.clipped = true;
		data.clip_count++;
		data.last_clip_normal = trace.m_plane.m_normal;

		// Check if it's a floor
		if ( trace.m_plane.m_normal.m_z > 0.7f )
		{
			blocked |= 0x1;  // floor
			data.on_ground = true;
		}
		if ( std::fabsf( trace.m_plane.m_normal.m_z ) < 0.01f )
			blocked |= 0x2;  // wall

		// reduce remaining time
		time_left -= time_left * trace.m_fraction;

		// add plane if we haven't seen too many
		if ( num_planes >= MAX_CLIP_PLANES )
		{
			data.velocity = c_vector{};
			break;
		}
		planes[num_planes] = trace.m_plane.m_normal;
		num_planes++;

		// ── clip velocity against all accumulated planes ──
		// (Source engine multi-plane clipping logic)
		if ( num_planes == 1 )
		{
			// single plane: simple clip
			if ( planes[0].m_z > 0.7f )
			{
				// floor — clip and keep going
				clip_velocity( original_velocity, planes[0], new_velocity, OVERCLIP );
				data.velocity = new_velocity;
				original_velocity = new_velocity;
			}
			else
			{
				// wall — clip with overbounce 1.0
				clip_velocity( original_velocity, planes[0], new_velocity, OVERCLIP );
				data.velocity = new_velocity;
				original_velocity = new_velocity;
			}
		}
		else
		{
			// two or more planes — find a velocity that doesn't re-enter any plane
			int i, j;
			for ( i = 0; i < num_planes; i++ )
			{
				clip_velocity( original_velocity, planes[i], data.velocity, OVERCLIP );

				// check this clipped velocity doesn't go into another plane
				for ( j = 0; j < num_planes; j++ )
				{
					if ( j == i )
						continue;
					if ( data.velocity.dot_product( planes[j] ) < 0.f )
						break;  // goes into plane j
				}
				if ( j == num_planes )
					break;  // didn't go into any other plane
			}

			if ( i == num_planes )
			{
				// couldn't find a non-conflicting clip — stuck in a corner
				if ( num_planes != 2 )
				{
					data.velocity = c_vector{};
					break;
				}

				// crease: project velocity along the intersection of the two planes
				c_vector dir = planes[0].cross_product( planes[1] );
				dir.normalize_in_place();

				float d = dir.dot_product( data.velocity );
				data.velocity = dir * d;
			}

			// if the new velocity goes back against the original direction, stop
			if ( data.velocity.dot_product( primal_velocity ) <= 0.f )
			{
				data.velocity = c_vector{};
				break;
			}
		}
	}

	return blocked;
}

// ── StartGravity ─────────────────────────────────────────────────────────────
// Source SDK: CGameMovement::StartGravity
// Apply half of the gravity for this tick before movement
void start_gravity( move_data_t& data )
{
	float ent_gravity = 1.0f;  // player gravity scale, default 1.0
	data.velocity.m_z -= ( data.sv_gravity * ent_gravity * 0.5f * data.tick_interval );
	data.velocity.m_z += data.base_velocity.m_z * data.tick_interval;

	data.base_velocity.m_z = 0.f;

	// clamp to terminal velocity
	const float term_vel = data.sv_gravity * data.tick_interval;
	if ( data.velocity.m_z < -term_vel * 10.f )
		data.velocity.m_z = -term_vel * 10.f;
}

// ── FinishGravity ────────────────────────────────────────────────────────────
// Source SDK: CGameMovement::FinishGravity
// Apply second half of gravity after movement
void finish_gravity( move_data_t& data )
{
	float ent_gravity = 1.0f;
	data.velocity.m_z -= ( data.sv_gravity * ent_gravity * 0.5f * data.tick_interval );

	const float term_vel = data.sv_gravity * data.tick_interval;
	if ( data.velocity.m_z < -term_vel * 10.f )
		data.velocity.m_z = -term_vel * 10.f;
}

// ── AirAccelerate ────────────────────────────────────────────────────────────
// Source SDK: CGameMovement::AirAccelerate
void air_accelerate( move_data_t& data, const c_vector& wishdir, float wishspeed )
{
	// Source engine: cap wishspeed to 30 in air
	float wish_spd = wishspeed;
	if ( wish_spd > AIR_SPEED_CAP )
		wish_spd = AIR_SPEED_CAP;

	// current speed along wish direction (only XY matters but Z is included in Source)
	float current_speed = data.velocity.dot_product( wishdir );

	// how much speed to add
	float add_speed = wish_spd - current_speed;
	if ( add_speed <= 0.f )
		return;

	// acceleration
	float accel_speed = data.sv_airaccelerate * wishspeed * data.tick_interval * data.surface_friction;

	// cap it
	if ( accel_speed > add_speed )
		accel_speed = add_speed;

	data.velocity.m_x += accel_speed * wishdir.m_x;
	data.velocity.m_y += accel_speed * wishdir.m_y;
	data.velocity.m_z += accel_speed * wishdir.m_z;
}

// ── ComputeWishVelocity ─────────────────────────────────────────────────────
// Derives wishdir + wishspeed from view angles + forward_move / side_move.
// Source SDK: CGameMovement::AirMove (first half)
void compute_wish_velocity( const move_data_t& data, c_vector& out_wishdir, float& out_wishspeed )
{
	// build forward and right vectors from viewangles (Z component zeroed)
	const float yaw_rad = deg2rad( data.view_angles.m_y );
	const float cy = std::cosf( yaw_rad );
	const float sy = std::sinf( yaw_rad );

	c_vector forward{ cy, sy, 0.f };
	c_vector right{ sy, -cy, 0.f };

	// wish velocity = forward * fmove + right * smove
	c_vector wishvel;
	wishvel.m_x = forward.m_x * data.forward_move + right.m_x * data.side_move;
	wishvel.m_y = forward.m_y * data.forward_move + right.m_y * data.side_move;
	wishvel.m_z = 0.f;

	// normalize to get wishdir + wishspeed
	out_wishspeed = wishvel.normalize_in_place();
	out_wishdir   = wishvel;

	// clamp wishspeed to maxspeed
	if ( out_wishspeed > data.sv_maxspeed )
		out_wishspeed = data.sv_maxspeed;
}

// ── SimulateAirTick ──────────────────────────────────────────────────────────
// Full CGameMovement::AirMove for one tick.  Pure function.
void simulate_air_tick( move_data_t& data )
{
	data.on_ground = false;

	// 1. Compute wish velocity from cmd inputs
	c_vector wishdir;
	float wishspeed;
	compute_wish_velocity( data, wishdir, wishspeed );

	// 2. Half gravity (before move)
	start_gravity( data );

	// 3. Air acceleration (XY only effectively, since wishdir.z = 0)
	air_accelerate( data, wishdir, wishspeed );

	// 4. Add base velocity
	data.velocity += data.base_velocity;

	// 5. Move & clip
	try_player_move( data );

	// 6. Remove base velocity
	data.velocity -= data.base_velocity;

	// 7. Second half of gravity
	finish_gravity( data );
}

} // namespace source_move
