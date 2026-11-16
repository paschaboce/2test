#include "../../globals/includes/includes.h"
#include "../../globals/reshade/reshade.h"
bool is_reshade_active() {
    HMODULE reshade_modules[] = {
        GetModuleHandleA("d3d9.dll"),
        GetModuleHandleA("dxgi.dll"),
        GetModuleHandleA("d3d11.dll"),
        GetModuleHandleA("d3d12.dll"),
        GetModuleHandleA("opengl32.dll"),
        GetModuleHandleA("vulkan-1.dll")
    };
    for (auto module : reshade_modules) {
        if (module) {
            return true;
        }
    }
    return false;
}
void safe_render_with_reshade() {
    static bool reshade_detected = false;
    static bool first_check = true;
    if (first_check) {
        first_check = false;
        reshade_detected = is_reshade_active();
        if (reshade_detected) {
            g_console.print("ReShade detected - enabling compatibility mode");
        }
    }
    if (reshade_detected) {
        return;
    }
}