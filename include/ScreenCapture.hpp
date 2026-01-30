#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <vector>
#include <memory>

using Microsoft::WRL::ComPtr;

/**
 * ScreenCapture manages the Windows Desktop Duplication API
 * to efficiently capture screen frames.
 */
class ScreenCapture {
public:
    ScreenCapture();
    ~ScreenCapture();

    bool Initialize();
    bool CaptureFrame(std::vector<uint8_t>& outBuffer, int& width, int& height);
    void SetRegion(RECT r) { m_captureRect = r; }
    void Cleanup();

private:
    ComPtr<ID3D11Device> m_d3dDevice;
    ComPtr<ID3D11DeviceContext> m_d3dContext;
    ComPtr<IDXGIOutputDuplication> m_deskDupl;
    
    DXGI_OUTPUT_DESC m_outputDesc;
    bool m_initialized = false;

    bool SetupDevice();
    bool SetupDuplication();

    RECT m_captureRect = {0}; // 0 = Fullscreen

    ComPtr<ID3D11Texture2D> m_stagingTexture;
    D3D11_TEXTURE2D_DESC m_stagingDesc = { (UINT)0 };
};
