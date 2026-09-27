#pragma once

#include "support/CaptureCorpus.hpp"
#include "support/PixelComparison.hpp"

#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace avemotion::testsupport {

using Microsoft::WRL::ComPtr;

[[noreturn]] inline void captureSurfaceFail(const std::string& message) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

inline void captureSurfaceRequire(bool condition, const std::string& message) {
    if (!condition) captureSurfaceFail(message);
}

inline void captureSurfaceRequireHr(HRESULT value, const std::string& operation) {
    if (FAILED(value)) {
        std::ostringstream stream;
        stream << operation << " failed with HRESULT 0x"
               << std::hex << std::uppercase
               << static_cast<std::uint32_t>(value);
        captureSurfaceFail(stream.str());
    }
}

class WarpCaptureSurface final {
public:
    WarpCaptureSurface() {
        createDevice();
    }

    void configure(const avemotion::testsupport::CaptureProfile& profile) {
        target_.Reset();
        texture_.Reset();
        width_ = static_cast<UINT>(profile.pixelWidth);
        height_ = static_cast<UINT>(profile.pixelHeight);
        dpiX_ = profile.dpiX;
        dpiY_ = profile.dpiY;

        D3D11_TEXTURE2D_DESC description{};
        description.Width = width_;
        description.Height = height_;
        description.MipLevels = 1U;
        description.ArraySize = 1U;
        description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        description.SampleDesc.Count = 1U;
        description.Usage = D3D11_USAGE_DEFAULT;
        description.BindFlags = D3D11_BIND_RENDER_TARGET
            | D3D11_BIND_SHADER_RESOURCE;
        captureSurfaceRequireHr(
            d3dDevice_->CreateTexture2D(&description, nullptr, &texture_),
            "ID3D11Device::CreateTexture2D(capture target)");

        ComPtr<IDXGISurface> surface;
        captureSurfaceRequireHr(texture_.As(&surface),
                  "ID3D11Texture2D::As(IDXGISurface)");
        const auto properties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_PREMULTIPLIED),
            dpiX_,
            dpiY_);
        captureSurfaceRequireHr(
            d2dContext_->CreateBitmapFromDxgiSurface(
                surface.Get(), &properties, &target_),
            "ID2D1DeviceContext::CreateBitmapFromDxgiSurface(capture)");
        d2dContext_->SetTarget(target_.Get());
        d2dContext_->SetDpi(dpiX_, dpiY_);
        d2dContext_->SetUnitMode(D2D1_UNIT_MODE_DIPS);
        d2dContext_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        d2dContext_->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
        d2dContext_->SetTransform(D2D1::Matrix3x2F::Identity());
    }

    [[nodiscard]] ID2D1DeviceContext* context() const noexcept {
        return d2dContext_.Get();
    }

    void begin() {
        captureSurfaceRequire(target_ != nullptr, "capture target is not configured");
        d2dContext_->SetTarget(target_.Get());
        d2dContext_->SetDpi(dpiX_, dpiY_);
        d2dContext_->SetUnitMode(D2D1_UNIT_MODE_DIPS);
        d2dContext_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        d2dContext_->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
        d2dContext_->SetTransform(D2D1::Matrix3x2F::Identity());
        d2dContext_->BeginDraw();
        d2dContext_->Clear(D2D1::ColorF(0.0F, 0.0F, 0.0F, 0.0F));
    }

    void end() {
        captureSurfaceRequireHr(d2dContext_->EndDraw(),
                  "ID2D1DeviceContext::EndDraw(capture)");
        d3dContext_->Flush();
    }

    [[nodiscard]] std::vector<PixelBGRA> readPixels() const {
        D3D11_TEXTURE2D_DESC description{};
        texture_->GetDesc(&description);
        description.Usage = D3D11_USAGE_STAGING;
        description.BindFlags = 0U;
        description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        description.MiscFlags = 0U;

        ComPtr<ID3D11Texture2D> staging;
        captureSurfaceRequireHr(
            d3dDevice_->CreateTexture2D(&description, nullptr, &staging),
            "ID3D11Device::CreateTexture2D(capture staging)");
        d3dContext_->CopyResource(staging.Get(), texture_.Get());

        D3D11_MAPPED_SUBRESOURCE mapped{};
        captureSurfaceRequireHr(
            d3dContext_->Map(
                staging.Get(), 0U, D3D11_MAP_READ, 0U, &mapped),
            "ID3D11DeviceContext::Map(capture staging)");
        std::vector<PixelBGRA> pixels(
            static_cast<std::size_t>(width_)
            * static_cast<std::size_t>(height_));
        for (UINT y = 0U; y < height_; ++y) {
            const auto* row = static_cast<const std::uint8_t*>(mapped.pData)
                + static_cast<std::size_t>(mapped.RowPitch) * y;
            for (UINT x = 0U; x < width_; ++x) {
                const auto offset = static_cast<std::size_t>(x) * 4U;
                pixels[static_cast<std::size_t>(y) * width_ + x] = {
                    row[offset + 0U],
                    row[offset + 1U],
                    row[offset + 2U],
                    row[offset + 3U],
                };
            }
        }
        d3dContext_->Unmap(staging.Get(), 0U);
        return pixels;
    }

private:
    void createDevice() {
        const UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        captureSurfaceRequireHr(
            D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_WARP,
                nullptr,
                flags,
                nullptr,
                0U,
                D3D11_SDK_VERSION,
                &d3dDevice_,
                nullptr,
                &d3dContext_),
            "D3D11CreateDevice(WARP capture)");

        ComPtr<IDXGIDevice> dxgiDevice;
        captureSurfaceRequireHr(d3dDevice_.As(&dxgiDevice),
                  "ID3D11Device::As(IDXGIDevice capture)");
        D2D1_FACTORY_OPTIONS factoryOptions{};
        captureSurfaceRequireHr(
            D2D1CreateFactory(
                D2D1_FACTORY_TYPE_SINGLE_THREADED,
                __uuidof(ID2D1Factory1),
                &factoryOptions,
                reinterpret_cast<void**>(d2dFactory_.GetAddressOf())),
            "D2D1CreateFactory(capture)");
        captureSurfaceRequireHr(
            d2dFactory_->CreateDevice(dxgiDevice.Get(), &d2dDevice_),
            "ID2D1Factory1::CreateDevice(capture)");
        captureSurfaceRequireHr(
            d2dDevice_->CreateDeviceContext(
                D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dContext_),
            "ID2D1Device::CreateDeviceContext(capture)");
    }

    UINT width_ = 0U;
    UINT height_ = 0U;
    float dpiX_ = 96.0F;
    float dpiY_ = 96.0F;
    ComPtr<ID3D11Device> d3dDevice_;
    ComPtr<ID3D11DeviceContext> d3dContext_;
    ComPtr<ID2D1Factory1> d2dFactory_;
    ComPtr<ID2D1Device> d2dDevice_;
    ComPtr<ID2D1DeviceContext> d2dContext_;
    ComPtr<ID3D11Texture2D> texture_;
    ComPtr<ID2D1Bitmap1> target_;
};

} // namespace avemotion::testsupport
