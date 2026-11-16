#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include "spectator_gif_data.h"
#include "spectator_gif_shit_data.h"
#include "../../globals/includes/includes.h"
#include "../../globals/config/variables.h"
#pragma comment(lib, "gdiplus.lib")
class SpectatorGifRenderer {
private:
    Gdiplus::Bitmap* gif_bitmap = nullptr;
    UINT frame_count = 0;
    UINT current_frame = 0;
    std::vector<UINT> frame_delays;
    DWORD last_frame_time = 0;
    bool initialized = false;
    ULONG_PTR gdiplus_token = 0;
    IDirect3DTexture9* cached_texture = nullptr;
    IDirect3DDevice9* cached_device = nullptr;
    UINT cached_frame = 0xFFFFFFFF;
public:
    bool Initialize() {
        return Initialize(kSpectatorGifData, sizeof(kSpectatorGifData));
    }
    bool Initialize(const unsigned char* gif_data, size_t gif_size) {
        if (initialized) return true;
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        if (Gdiplus::GdiplusStartup(&gdiplus_token, &gdiplusStartupInput, nullptr) != Gdiplus::Ok) {
            return false;
        }
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, gif_size);
        if (!hMem) {
            Gdiplus::GdiplusShutdown(gdiplus_token);
            return false;
        }
        void* pMem = GlobalLock(hMem);
        if (!pMem) {
            GlobalFree(hMem);
            Gdiplus::GdiplusShutdown(gdiplus_token);
            return false;
        }
        memcpy(pMem, gif_data, gif_size);
        GlobalUnlock(hMem);
        IStream* pStream = nullptr;
        if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) != S_OK) {
            GlobalFree(hMem);
            Gdiplus::GdiplusShutdown(gdiplus_token);
            return false;
        }
        gif_bitmap = Gdiplus::Bitmap::FromStream(pStream);
        pStream->Release();
        if (!gif_bitmap || gif_bitmap->GetLastStatus() != Gdiplus::Ok) {
            if (gif_bitmap) delete gif_bitmap;
            Gdiplus::GdiplusShutdown(gdiplus_token);
            return false;
        }
        UINT count = gif_bitmap->GetFrameDimensionsCount();
        if (count == 0) {
            delete gif_bitmap;
            Gdiplus::GdiplusShutdown(gdiplus_token);
            return false;
        }
        GUID* pDimensionIDs = new GUID[count];
        gif_bitmap->GetFrameDimensionsList(pDimensionIDs, count);
        frame_count = gif_bitmap->GetFrameCount(&pDimensionIDs[0]);
        delete[] pDimensionIDs;
        UINT size = gif_bitmap->GetPropertyItemSize(PropertyTagFrameDelay);
        if (size > 0) {
            Gdiplus::PropertyItem* pItem = (Gdiplus::PropertyItem*)malloc(size);
            if (pItem) {
                gif_bitmap->GetPropertyItem(PropertyTagFrameDelay, size, pItem);
                for (UINT i = 0; i < frame_count; ++i) {
                    UINT delay = ((UINT*)pItem->value)[i] * 10;
                    if (delay == 0) delay = 100;
                    frame_delays.push_back(delay);
                }
                free(pItem);
            }
        }
        if (frame_delays.empty()) {
            for (UINT i = 0; i < frame_count; ++i) {
                frame_delays.push_back(100);
            }
        }
        initialized = true;
        last_frame_time = GetTickCount();
        return true;
    }
    void Shutdown() {
        if (cached_texture) {
            cached_texture->Release();
            cached_texture = nullptr;
        }
        cached_device = nullptr;
        cached_frame = 0xFFFFFFFF;
        if (gif_bitmap) {
            delete gif_bitmap;
            gif_bitmap = nullptr;
        }
        if (gdiplus_token) {
            Gdiplus::GdiplusShutdown(gdiplus_token);
            gdiplus_token = 0;
        }
        initialized = false;
    }
    void Update() {
        if (!initialized || frame_count <= 1) return;
        DWORD current_time = GetTickCount();
        if (current_time - last_frame_time >= frame_delays[current_frame]) {
            current_frame = (current_frame + 1) % frame_count;
            GUID pageGuid = Gdiplus::FrameDimensionTime;
            gif_bitmap->SelectActiveFrame(&pageGuid, current_frame);
            last_frame_time = current_time;
        }
    }
    IDirect3DTexture9* GetCurrentFrameTexture(IDirect3DDevice9* device) {
        if (!initialized || !gif_bitmap || !device) return nullptr;
        Update();
        if (cached_device != device || !cached_texture) {
            if (cached_texture) {
                cached_texture->Release();
                cached_texture = nullptr;
            }
            UINT width = gif_bitmap->GetWidth();
            UINT height = gif_bitmap->GetHeight();
            if (width == 0 || height == 0) return nullptr;
            HRESULT hr = E_FAIL;
            hr = device->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &cached_texture, nullptr);
            if (FAILED(hr)) {
                hr = device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &cached_texture, nullptr);
            }
            if (FAILED(hr)) {
                hr = device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &cached_texture, nullptr);
            }
            if (FAILED(hr)) {
                return nullptr;
            }
            cached_device = device;
            cached_frame = 0xFFFFFFFF;
        }
        if (cached_frame != current_frame && cached_texture) {
            UINT width = gif_bitmap->GetWidth();
            UINT height = gif_bitmap->GetHeight();
            Gdiplus::BitmapData bitmapData;
            Gdiplus::Rect rect(0, 0, width, height);
            if (gif_bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) != Gdiplus::Ok) {
                return cached_texture;
            }
            D3DLOCKED_RECT locked_rect;
            if (SUCCEEDED(cached_texture->LockRect(0, &locked_rect, nullptr, 0))) {
                BYTE* src = (BYTE*)bitmapData.Scan0;
                BYTE* dst = (BYTE*)locked_rect.pBits;
                for (UINT y = 0; y < height; ++y) {
                    memcpy(dst, src, width * 4);
                    src += bitmapData.Stride;
                    dst += locked_rect.Pitch;
                }
                cached_texture->UnlockRect(0);
                cached_frame = current_frame;
            }
            gif_bitmap->UnlockBits(&bitmapData);
        }
        return cached_texture;
    }
    UINT GetWidth() const {
        return gif_bitmap ? gif_bitmap->GetWidth() : 0;
    }
    UINT GetHeight() const {
        return gif_bitmap ? gif_bitmap->GetHeight() : 0;
    }
    bool IsInitialized() const {
        return initialized;
    }
};
inline SpectatorGifRenderer g_spectator_gif_original;
inline SpectatorGifRenderer g_spectator_gif_shit;
inline SpectatorGifRenderer& GetCurrentGifRenderer() {
    int gif_type = GET_VARIABLE(g_variables.m_spectators_gif_type, int);
    return (gif_type == 0) ? g_spectator_gif_original : g_spectator_gif_shit;
}