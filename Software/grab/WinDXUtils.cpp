#include "WinDXUtils.hpp"

#include <QtGlobal>
#include <QString>

#include <DXGI.h>
#include <D3D10_1.h>
#include <D3D10.h>
#include <d3d9.h>

#include "../src/debug.h"

#define DXGI_PRESENT_FUNC_ORD 8
#define D3D9_SCPRESENT_FUNC_ORD 3
#define D3D9_PRESENT_FUNC_ORD 17

namespace WinUtils
{

UINT GetDxgiPresentOffset(HWND hwnd) {
    Q_ASSERT(hwnd != NULL);
    IDXGIFactory1 * factory = NULL;
    IDXGIAdapter * adapter;
    HRESULT hresult = CreateDXGIFactory(IID_IDXGIFactory, reinterpret_cast<void **>(&factory));
    if (hresult != S_OK) {
        qCritical() << "Can't create DXGIFactory. " << hresult;
        return NULL;
    }
    hresult = factory->EnumAdapters(0, &adapter);
    if (hresult != S_OK) {
        qCritical() << "Can't enumerate adapters. " << hresult;
        return NULL;
    }

    DXGI_MODE_DESC dxgiModeDesc;
    dxgiModeDesc.Width                      = 1;
    dxgiModeDesc.Height                     = 1;
    dxgiModeDesc.RefreshRate.Numerator      = 1;
    dxgiModeDesc.RefreshRate.Denominator    = 1;
    dxgiModeDesc.Format                     = DXGI_FORMAT_R8G8B8A8_UNORM;
    dxgiModeDesc.ScanlineOrdering           = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    dxgiModeDesc.Scaling                    = DXGI_MODE_SCALING_UNSPECIFIED;

    DXGI_SAMPLE_DESC dxgiSampleDesc = {1, 0};

    DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
    memset(&dxgiSwapChainDesc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));
    dxgiSwapChainDesc.BufferDesc    = dxgiModeDesc;
    dxgiSwapChainDesc.SampleDesc    = dxgiSampleDesc;
    dxgiSwapChainDesc.BufferUsage   = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgiSwapChainDesc.BufferCount   = 2;
    dxgiSwapChainDesc.OutputWindow  = hwnd;
    dxgiSwapChainDesc.Windowed      = true;
    dxgiSwapChainDesc.SwapEffect    = DXGI_SWAP_EFFECT_DISCARD;
    dxgiSwapChainDesc.Flags         = 0;

    IDXGISwapChain * pSc;
    ID3D10Device  * pDev;
    hresult = D3D10CreateDeviceAndSwapChain(adapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &dxgiSwapChainDesc, &pSc, &pDev);
    if (S_OK != hresult) {
        qCritical() << Q_FUNC_INFO << QString("Can't create D3D10Device and SwapChain. hresult = 0x%1").arg(QString::number(hresult, 16));
        return NULL;
    }


    uintptr_t * pvtbl = reinterpret_cast<uintptr_t*>(pSc);
    uintptr_t presentFuncPtr = *pvtbl + sizeof(void *)*DXGI_PRESENT_FUNC_ORD;

    char buf[100];
    sprintf(buf, "presentFuncPtr=%x", presentFuncPtr);
    DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;

    intptr_t hDxgi = reinterpret_cast<intptr_t>(GetModuleHandle(L"dxgi.dll"));
    int presentFuncOffset = static_cast<UINT>(presentFuncPtr - hDxgi);

    sprintf(buf, "presentFuncOffset=%x", presentFuncOffset);
    DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;


    pSc->Release();
    pDev->Release();
    adapter->Release();
    factory->Release();

    return presentFuncOffset;
}

typedef IDirect3D9 * (WINAPI *Direct3DCreate9Func)(UINT SDKVersion);
UINT GetD3D9PresentOffset(HWND hWnd){
    IDirect3D9 *pD3D = NULL;
    HINSTANCE hD3d9 = LoadLibrary(L"d3d9.dll");
    Direct3DCreate9Func createDev = reinterpret_cast<Direct3DCreate9Func>(GetProcAddress(hD3d9, "Direct3DCreate9"));
    pD3D = createDev(D3D_SDK_VERSION);
    if ( !pD3D)
    {
        qCritical() << "Test_DirectX9 Direct3DCreate9(%d) call FAILED" << D3D_SDK_VERSION ;
        return NULL;
    }

    // step 3: Get IDirect3DDevice9
    D3DDISPLAYMODE d3ddm;
    HRESULT hRes = pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm );
    if (FAILED(hRes))
    {
        qCritical() << QString("Test_DirectX9 GetAdapterDisplayMode failed. 0x%x").arg( hRes);
        return NULL;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = true;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

    IDirect3DDevice9 *pD3DDevice;
    hRes = pD3D->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,	// the device we suppose any app would be using.
        hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
        &d3dpp, &pD3DDevice);
    if (FAILED(hRes))
    {
        qCritical() << QString("Test_DirectX9 CreateDevice failed. 0x%x").arg(hRes);
        return NULL;
    }

    IDirect3DSwapChain9 *pD3DSwapChain;
    hRes = pD3DDevice->GetSwapChain( 0, &pD3DSwapChain);
    if (FAILED(hRes))
    {
        qCritical() << QString("Test_DirectX9 GetSwapChain failed. 0x%x").arg(hRes);
        return NULL;
    }
    else
    {
        uintptr_t * pvtbl = *((uintptr_t**)(pD3DDevice));
        uintptr_t presentFuncPtr = pvtbl[D3D9_PRESENT_FUNC_ORD];
        char buf[100];
        sprintf(buf, "presentFuncPtr=%x", presentFuncPtr);
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;

        UINT presentFuncOffset = (presentFuncPtr - reinterpret_cast<UINT>(hD3d9));

        sprintf(buf, "presentFuncOffset=%x", presentFuncOffset);
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;


        pD3DSwapChain->Release();
        pD3DDevice->Release();
        pD3D->Release();

        return presentFuncOffset;
    }
}

UINT GetD3D9SCPresentOffset(HWND hWnd){
    IDirect3D9 *pD3D = NULL;
    HINSTANCE hD3d9 = LoadLibrary(L"d3d9.dll");
    Direct3DCreate9Func createDev = reinterpret_cast<Direct3DCreate9Func>(GetProcAddress(hD3d9, "Direct3DCreate9"));
    pD3D = createDev(D3D_SDK_VERSION);
    if ( !pD3D)
    {
        qCritical() << "Test_DirectX9 Direct3DCreate9(%d) call FAILED" << D3D_SDK_VERSION ;
        return NULL;
    }

    // step 3: Get IDirect3DDevice9
    D3DDISPLAYMODE d3ddm;
    HRESULT hRes = pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm );
    if (FAILED(hRes))
    {
        qCritical() << QString("Test_DirectX9 GetAdapterDisplayMode failed. 0x%x").arg( hRes);
        return NULL;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = true;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

    IDirect3DDevice9 *pD3DDevice;
    hRes = pD3D->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,	// the device we suppose any app would be using.
        hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
        &d3dpp, &pD3DDevice);
    if (FAILED(hRes))
    {
        qCritical() << QString("Test_DirectX9 CreateDevice failed. 0x%x").arg(hRes);
        return NULL;
    }

    IDirect3DSwapChain9 *pD3DSwapChain;
    hRes = pD3DDevice->GetSwapChain( 0, &pD3DSwapChain);
    if (FAILED(hRes))
    {
        qCritical() << QString("Test_DirectX9 GetSwapChain failed. 0x%x").arg(hRes);
        return NULL;
    }
    else
    {
        uintptr_t * pvtbl = *((uintptr_t**)(pD3DSwapChain));
        uintptr_t presentFuncPtr = pvtbl[D3D9_SCPRESENT_FUNC_ORD];
        char buf[100];
        sprintf(buf, "presentFuncPtr=%x", presentFuncPtr);
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;

        void *pD3d9 = reinterpret_cast<void *>(hD3d9);
        UINT presentFuncOffset = (presentFuncPtr - reinterpret_cast<UINT>(pD3d9));

        sprintf(buf, "presentFuncOffset=%x", presentFuncOffset);
        DEBUG_LOW_LEVEL << Q_FUNC_INFO << buf;


        pD3DSwapChain->Release();
        pD3DDevice->Release();
        pD3D->Release();

        return presentFuncOffset;
    }
}

} // namespace WinUtils
