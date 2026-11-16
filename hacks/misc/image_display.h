#pragma once
#include "../../globals/includes/includes.h"
#include <d3d9.h>
namespace n_image_display
{
    struct impl_t {
        void initialize_textures();
        void release_textures();
        void render_images();
        void render_pts_image(int pts_value, float pts_x, float pts_y);
        IDirect3DTexture9* create_texture_from_bytes(const unsigned char* data, size_t data_size);
        bool are_textures_initialized() const;
        void update_pts_animation();
        void start_pts_animation();
        void render_png_from_bytes(const unsigned char* png_data, size_t data_size, float x, float y, float width, float height);
    };
}
inline n_image_display::impl_t g_image_display{ };