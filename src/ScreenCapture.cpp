#include "ScreenCapture.hpp"
#include <iostream>

ScreenCapture::ScreenCapture() : m_initialized(false) {}

ScreenCapture::~ScreenCapture() {
    Cleanup();
}

bool ScreenCapture::Initialize() {
    if (!SetupDevice()) return false;
    if (!SetupDuplication()) return false;
    
    m_initialized = true;
    return true;
}

bool ScreenCapture::SetupDevice() {
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_d3dDevice,
        nullptr,
        &m_d3dContext
    );

    if (FAILED(hr)) {
        std::cerr << "Failed to create D3D11 Device: " << std::hex << hr << std::endl;
        return false;
    }

    return true;
}

bool ScreenCapture::SetupDuplication() {
    ComPtr<IDXGIDevice> dxgiDevice;
    m_d3dDevice.As(&dxgiDevice);

    ComPtr<IDXGIAdapter> dxgiAdapter;
    dxgiDevice->GetParent(__uuidof(IDXGIAdapter), &dxgiAdapter);

    ComPtr<IDXGIOutput> dxgiOutput;
    dxgiAdapter->EnumOutputs(0, &dxgiOutput);

    ComPtr<IDXGIOutput1> dxgiOutput1;
    dxgiOutput.As(&dxgiOutput1);

    dxgiOutput->GetDesc(&m_outputDesc);

    HRESULT hr = dxgiOutput1->DuplicateOutput(m_d3dDevice.Get(), &m_deskDupl);
    if (FAILED(hr)) {
        std::cerr << "Failed to duplicate output: " << std::hex << hr << std::endl;
        return false;
    }

    return true;
}

bool ScreenCapture::CaptureFrame(std::vector<uint8_t>& outBuffer, int& width, int& height) {
    if (!m_initialized) return false;

    IDXGIResource* desktopResource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    
    // Acquiring the next frame
    HRESULT hr = m_deskDupl->AcquireNextFrame(100, &frameInfo, &desktopResource);
    if (FAILED(hr)) return false;

    ComPtr<ID3D11Texture2D> desktopTexture;
    desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), &desktopTexture);
    desktopResource->Release();

    D3D11_TEXTURE2D_DESC desc;
    desktopTexture->GetDesc(&desc);
    
    width = desc.Width;
    height = desc.Height;

    // Reuse or recreate staging texture only if size changes
    if (!m_stagingTexture || m_stagingDesc.Width != desc.Width || m_stagingDesc.Height != desc.Height) {
        m_stagingDesc = desc;
        m_stagingDesc.Usage = D3D11_USAGE_STAGING;
        m_stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        m_stagingDesc.BindFlags = 0;
        m_stagingDesc.MiscFlags = 0;
        m_d3dDevice->CreateTexture2D(&m_stagingDesc, nullptr, &m_stagingTexture);
    }

    m_d3dContext->CopyResource(m_stagingTexture.Get(), desktopTexture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = m_d3dContext->Map(m_stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    
    if (SUCCEEDED(hr)) {
        // Copy the BGRA data to our buffer, respecting the capture region
        
        // Default to full screen if rect is empty
        int capX = 0;
        int capY = 0;
        int capW = desc.Width;
        int capH = desc.Height;

        if (m_captureRect.right > 0 && m_captureRect.bottom > 0) {
            capX = m_captureRect.left;
            capY = m_captureRect.top;
            capW = m_captureRect.right - m_captureRect.left;
            capH = m_captureRect.bottom - m_captureRect.top;

            // Clamp to screen bounds
            if (capX < 0) capX = 0;
            if (capY < 0) capY = 0;
            if (capX + capW > (int)desc.Width) capW = (int)desc.Width - capX;
            if (capY + capH > (int)desc.Height) capH = (int)desc.Height - capY;
        }

        // Update output dimensions
        width = capW;
        height = capH;
        
        // Align to 2 for encoding safety
        if (width % 2 != 0) width--;
        if (height % 2 != 0) height--;

        size_t pixelSize = 4;
        size_t rowPitch = mapped.RowPitch;
        size_t outRowPitch = width * pixelSize;
        size_t size = outRowPitch * height;
        
        outBuffer.resize(size);

        uint8_t* srcPtr = (uint8_t*)mapped.pData + (capY * rowPitch) + (capX * pixelSize);
        uint8_t* dstPtr = outBuffer.data();

        for (int h = 0; h < height; ++h) {
            memcpy(dstPtr, srcPtr, outRowPitch);
            srcPtr += rowPitch;
            dstPtr += outRowPitch;
        }
        
        m_d3dContext->Unmap(m_stagingTexture.Get(), 0);
    }

    m_deskDupl->ReleaseFrame();
    return SUCCEEDED(hr);
}

void ScreenCapture::Cleanup() {
    m_deskDupl.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();
    m_initialized = false;
}
