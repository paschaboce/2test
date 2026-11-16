#include "image_display.h"
#include "../../globals/includes/includes.h"
#include "../../globals/render/render.h"
#include "../../dependencies/imgui/imgui.h"
#include "../../dependencies/imgui/imgui_impl_dx9.h"
#include <d3d9.h>
#include <d3dx9.h>
namespace n_image_display
{
    void impl_t::initialize_textures()
    {
        return;
    }
    void impl_t::release_textures()
    {
        return;
    }
    IDirect3DTexture9* impl_t::create_texture_from_bytes(const unsigned char* data, size_t data_size)
    {
        return nullptr;
    }
    bool impl_t::are_textures_initialized() const
    {
        return false;
    }
    void impl_t::render_images()
    {
        return;
    }
    void impl_t::render_pts_image(int pts_value, float pts_x, float pts_y)
    {
        return;
    }
    void impl_t::update_pts_animation()
    {
        return;
    }
    void impl_t::start_pts_animation()
    {
        return;
    }
    void impl_t::render_png_from_bytes(const unsigned char* png_data, size_t data_size, float x, float y, float width, float height)
    {
        return;
    }
}