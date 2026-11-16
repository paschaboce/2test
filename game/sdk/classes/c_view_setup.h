#include "c_angle.h"
#include "c_vector.h"
enum e_motion_blur_mode {
	motion_blur_disable = 1,
	motion_blur_game    = 2,
	motion_blur_sfm     = 3
};
class c_view_setup 
{
public:
	unsigned char _0x0000[ 16 ];
	int m_width;
	int m_width_old;
	int m_height;
	int m_height_old;
	int m_x;
	int m_x_old;
	int m_y;
	int m_y_old;
	unsigned char _0x0030[ 128 ];
	float m_fov;
	float m_fov_viewmodel;
	c_vector m_origin;
	c_angle m_angles;
	float m_znear;
	float m_zfar;
	float m_znear_viewmodel;
	float m_zfar_viewmodel;
	float m_aspect_ratio;
	float m_near_blur_depth;
	float m_near_focus_depth;
	float m_far_focus_depth;
	float m_far_blur_depth;
	float m_near_blur_radius;
	float m_far_blur_radius;
	int m_dof_quality;
	e_motion_blur_mode m_motion_blur_mode;
	float m_shutter_time;
	c_vector m_shutter_open_position;
	c_angle m_shutter_open_angles;
	c_vector m_shutter_close_position;
	c_angle m_shutter_close_angles;
	float m_off_center_top;
	float m_off_center_bottom;
	float m_off_center_left;
	float m_off_center_right;
	int m_edge_blur;
};