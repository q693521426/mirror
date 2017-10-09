#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"

#define WIN32_LEAN_AND_MEAN
#include "RenderStates.h"
#include "d3dx11effect.h"
#include "ModelConstants.h"

#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <complex>


CFirstPersonCamera			mCamera;

ID3D11Buffer*				mRoomVB = nullptr;
ID3D11Buffer*				mSkullVB = nullptr;
ID3D11Buffer*				mSkullIB = nullptr;
ID3DX11Effect*				g_pEffect = nullptr;
ID3D11ShaderResourceView*	mFloorDiffuseMapSRV = nullptr;
ID3D11ShaderResourceView*	mWallDiffuseMapSRV = nullptr;
ID3D11ShaderResourceView*	mMirrorDiffuseMapSRV = nullptr;
ID3D11InputLayout*			m_pVertexLayout = nullptr;
D3DXVECTOR3					m_Eye( -5.f, 3.f, -15.f );
D3DXVECTOR3					m_At( 0.f, 0.f, 0.f );

DirectionalLight mDirLights[3];
Material mRoomMat;
Material mSkullMat;
Material mMirrorMat;
Material mShadowMat;

D3DXMATRIX mView;
D3DXMATRIX mProj;
D3DXMATRIX mRoomWorld;
D3DXMATRIX mSkullWorld;

UINT mSkullIndexCount=0;
D3DXVECTOR3 mSkullTranslation;

RenderOptions mRenderOptions = Textures;

std::map<std::string,ID3DX11EffectTechnique*> mTech;
ID3DX11EffectMatrixVariable* WorldViewProj;
ID3DX11EffectMatrixVariable* World;
ID3DX11EffectMatrixVariable* WorldInvTranspose;
ID3DX11EffectMatrixVariable* TexTransform;
ID3DX11EffectVectorVariable* EyePosW;
ID3DX11EffectVectorVariable* FogColor;
ID3DX11EffectScalarVariable* FogStart;
ID3DX11EffectScalarVariable* FogRange;
ID3DX11EffectVariable* DirLights;
ID3DX11EffectVariable* Mat;
ID3DX11EffectShaderResourceVariable* DiffuseMap;

UINT screen_width = 1024,screen_height = 768;


void Initialize();
HRESULT BuildRoomGeometryBuffers(ID3D11Device* pd3dDevice);
HRESULT BuildSkullGeometryBuffers(ID3D11Device* pd3dDevice);
//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
	HRESULT hr = S_OK;
	    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    dwShaderFlags |= D3DCOMPILE_DEBUG;
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	V_RETURN(RenderStates::Initialize(pd3dDevice));
	V_RETURN(BuildRoomGeometryBuffers(pd3dDevice));
	V_RETURN(BuildSkullGeometryBuffers(pd3dDevice));
	
    ID3DBlob* pEffectBuffer = nullptr;

#if D3D_COMPILER_VERSION >= 46

    WCHAR szShaderPath[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( szShaderPath, MAX_PATH, L"Model.fx" ) );

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DX11CompileEffectFromFile( szShaderPath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, dwShaderFlags, 0, pd3dDevice, &g_pEffect, &pErrorBlob );

    if ( pErrorBlob )
    {
        OutputDebugStringA( reinterpret_cast<const char*>( pErrorBlob->GetBufferPointer() ) );
        pErrorBlob->Release();
    }

    if ( FAILED(hr) )
        return hr;

#else

    ID3DBlob* pEffectBuffer = nullptr;
    V_RETURN( DXUTCompileFromFile( L"BasicHLSLFX11.fx", nullptr, "none", "fx_5_0", dwShaderFlags, 0, &pEffectBuffer ) );
    hr = D3DX11CreateEffectFromMemory( pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, pd3dDevice, &g_pEffect );
    SAFE_RELEASE( pEffectBuffer );
    if ( FAILED(hr) )
        return hr;

#endif
   // V_RETURN(D3DX11CreateEffectFromMemory( pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, pd3dDevice, &g_pEffect ));
    SAFE_RELEASE( pEffectBuffer );

	V_RETURN(D3DX11CreateShaderResourceViewFromFile(pd3dDevice, 
		L"Textures/checkboard.dds", nullptr, nullptr, &mFloorDiffuseMapSRV, nullptr ));

	V_RETURN(D3DX11CreateShaderResourceViewFromFile(pd3dDevice, 
		L"Textures/brick01.dds", nullptr, nullptr, &mWallDiffuseMapSRV, nullptr ));

	V_RETURN(D3DX11CreateShaderResourceViewFromFile(pd3dDevice, 
		L"Textures/ice.dds", nullptr, nullptr, &mMirrorDiffuseMapSRV, nullptr ));
	{
		char* tech_str[]=
		{
			"Light1",
			"Light2",
			"Light3",

			"Light0Tex",
			"Light1Tex",
			"Light2Tex",
			"Light3Tex",

			"Light0TexAlphaClip",
			"Light1TexAlphaClip",
			"Light2TexAlphaClip",
			"Light3TexAlphaClip",

			"Light1Fog",
			"Light2Fog",
			"Light3Fog",

			"Light0TexFog",
			"Light1TexFog",
			"Light2TexFog",
			"Light3TexFog",

			"Light0TexAlphaClipFog",
			"Light1TexAlphaClipFog",
			"Light2TexAlphaClipFog",
			"Light3TexAlphaClipFog"
		};
		for(int i=0;i<ARRAYSIZE(tech_str);i++)
		{
			auto pTech = g_pEffect->GetTechniqueByName(tech_str[i]);
			mTech.emplace(tech_str[i],pTech);
		}
	}
	{
		WorldViewProj     = g_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
		World             = g_pEffect->GetVariableByName("gWorld")->AsMatrix();
		WorldInvTranspose = g_pEffect->GetVariableByName("gWorldInvTranspose")->AsMatrix();
		TexTransform      = g_pEffect->GetVariableByName("gTexTransform")->AsMatrix();
		EyePosW           = g_pEffect->GetVariableByName("gEyePosW")->AsVector();
		FogColor          = g_pEffect->GetVariableByName("gFogColor")->AsVector();
		FogStart          = g_pEffect->GetVariableByName("gFogStart")->AsScalar();
		FogRange          = g_pEffect->GetVariableByName("gFogRange")->AsScalar();
		DirLights         = g_pEffect->GetVariableByName("gDirLights");
		Mat               = g_pEffect->GetVariableByName("gMaterial");
		DiffuseMap        = g_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
	}

	D3D11_INPUT_ELEMENT_DESC LayoutBasic32[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(LayoutBasic32);

	D3DX11_PASS_DESC passDesc;
	mTech["Light1"]->GetPassByIndex(0)->GetDesc(&passDesc);
	V_RETURN(pd3dDevice->CreateInputLayout(LayoutBasic32, numElements, passDesc.pIAInputSignature, 
		passDesc.IAInputSignatureSize, &m_pVertexLayout));

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	HRESULT hr = S_OK;
	screen_width = pBackBufferSurfaceDesc->Width;
	screen_height = pBackBufferSurfaceDesc->Height;
	const float fAspect = static_cast<float>(screen_width) / static_cast<float>(screen_height);
	mCamera.SetProjParams(D3DX_PI * 0.25f, fAspect, 0.1f, 100.f);

    return hr;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{	
	mCamera.FrameMove(fElapsedTime);
	D3DXMATRIX skullScale,skullRotate,skullTranslation;
	D3DXMatrixScaling(&skullScale,0.45f,0.45f,0.45f);
	D3DXMatrixRotationY(&skullRotate,0.5f*D3DX_PI);
	D3DXMatrixTranslation(&skullTranslation,mSkullTranslation.x, mSkullTranslation.y, mSkullTranslation.z);
	mSkullWorld = skullScale*skullRotate*skullTranslation;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
                                  double fTime, float fElapsedTime, void* pUserContext )
{
    // Clear render target and the depth stencil 
    float ClearColor[4] = { 0.176f, 0.196f, 0.667f, 0.0f };

    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0, 0 );

	pd3dImmediateContext->IASetInputLayout(m_pVertexLayout);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	mView = *(mCamera.GetViewMatrix());
	mProj = *(mCamera.GetProjMatrix());
	D3DXMATRIX viewProj = mView * mProj;

	DirLights->SetRawValue(mDirLights, 0, 3*sizeof(DirectionalLight)); 
	EyePosW->SetRawValue((*mCamera.GetEyePt()),0,sizeof(D3DXVECTOR3));
	float fogColor[]={0.f,0.f,0.f,1.f};
	FogColor->SetFloatVector(fogColor);
	FogStart->SetFloat(2.f);
	FogRange->SetFloat(40.f);

	ID3DX11EffectTechnique* activeTech;
	ID3DX11EffectTechnique* activeSkullTech;

	switch(mRenderOptions)
	{
	case RenderOptions::Lighting:
		activeTech = mTech["Light3"];
		activeSkullTech = mTech["Light3"];
		break;
	case RenderOptions::Textures:
		activeTech = mTech["Light3Tex"];
		activeSkullTech = mTech["Light3"];
		break;
	case RenderOptions::TexturesAndFog:
		activeTech = mTech["Light3TexFog"];
		activeSkullTech = mTech["Light3Fog"];
		break;
	}

	D3DXMATRIX I;
	D3DXMatrixIdentity(&I);

	mRoomWorld = I;

	D3DX11_TECHNIQUE_DESC techDesc;

	activeTech->GetDesc(&techDesc);
	for(UINT p = 0;p<techDesc.Passes;++p)
	{
		ID3DX11EffectPass* pass = activeTech->GetPassByIndex( p );
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &mRoomVB, &stride, &offset);
		
		D3DXMATRIX worldInvTranspose = InverseTranspose(&mRoomWorld);
		D3DXMATRIX worldViewProj = mRoomWorld*mView*mProj;

		World->SetMatrix(mRoomWorld);
		WorldInvTranspose->SetMatrix(worldInvTranspose);
		WorldViewProj->SetMatrix(worldViewProj);
		TexTransform->SetMatrix(I);
		Mat->SetRawValue(&mRoomMat,0,sizeof(Material));

		DiffuseMap->SetResource(mFloorDiffuseMapSRV);
		pass->Apply(0,pd3dImmediateContext);
		pd3dImmediateContext->Draw(6,0);

		DiffuseMap->SetResource(mWallDiffuseMapSRV);
		pass->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->Draw(18, 6);
	}

	activeSkullTech->GetDesc(&techDesc);
	for(UINT p = 0;p<techDesc.Passes;++p)
	{
		ID3DX11EffectPass* pass = activeSkullTech->GetPassByIndex(p);

		pd3dImmediateContext->IASetVertexBuffers(0,1,&mSkullVB,&stride,&offset);
		pd3dImmediateContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

		D3DXMATRIX worldInvTranspose = InverseTranspose(&mSkullWorld);
		D3DXMATRIX worldViewProj = mSkullWorld * mView * mProj;
		
		World->SetMatrix(mSkullWorld);
		WorldInvTranspose->SetMatrix(worldInvTranspose);
		WorldViewProj->SetMatrix(worldViewProj);
		Mat->SetRawValue(&mSkullMat,0,sizeof(Material));

		pass->Apply(0,pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(mSkullIndexCount,0,0);
	}

	float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};

	// mark mirror
	activeTech->GetDesc(&techDesc);
	for(UINT p = 0;p<techDesc.Passes;p++)
	{
		ID3DX11EffectPass* pass = activeTech->GetPassByIndex(p);
		pd3dImmediateContext->IASetVertexBuffers(0,1,&mRoomVB,&stride,&offset);

		D3DXMATRIX worldInvTranspose = InverseTranspose(&mRoomWorld);
		D3DXMATRIX worldViewProj = mRoomWorld*mView*mProj;

		World->SetMatrix(mRoomWorld);
		WorldInvTranspose->SetMatrix(worldInvTranspose);
		WorldViewProj->SetMatrix(worldViewProj);
		TexTransform->SetMatrix(I);
		
		pd3dImmediateContext->OMSetBlendState(RenderStates::NoRenderTargetWritesBS, blendFactor, 0xffffffff);
		pd3dImmediateContext->OMSetDepthStencilState(RenderStates::MarkMirrorDSS, 1);
		
		pass->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->Draw(6, 24);

		pd3dImmediateContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
		pd3dImmediateContext->OMSetDepthStencilState(nullptr, 0);
	}

	// draw reflect
	activeSkullTech->GetDesc(&techDesc);
	for(UINT p=0;p<techDesc.Passes;++p)
	{
		ID3DX11EffectPass* pass = activeSkullTech->GetPassByIndex(p);
		
		pd3dImmediateContext->IASetVertexBuffers(0,1,&mSkullVB,&stride,&offset);
		pd3dImmediateContext->IASetIndexBuffer(mSkullIB,DXGI_FORMAT_R32_UINT,0);
		
		D3DXPLANE  mirrorPlane(0.0f,0.0f,1.0f,0.0f);
		D3DXMATRIX Reflect;
		D3DXMatrixReflect(&Reflect,&mirrorPlane);
		D3DXMATRIX world = mSkullWorld * Reflect;
		D3DXMATRIX worldInvTranspose = InverseTranspose(&world);
		D3DXMATRIX worldViewProj = world * mView * mProj;

		World->SetMatrix(world);
		WorldInvTranspose->SetMatrix(worldInvTranspose);
		WorldViewProj->SetMatrix(worldViewProj);
		Mat->SetRawValue(&mSkullMat,0,sizeof(Material));

		D3DXVECTOR3 oldLightDirections[3];
		for(int i=0;i<3;++i)
		{
			oldLightDirections[i] = mDirLights[i].Direction;
			D3DXVec3TransformNormal(&mDirLights[i].Direction,&mDirLights[i].Direction,&Reflect);
		}
		DirLights->SetRawValue(&mDirLights,0,sizeof(DirectionalLight));

		pd3dImmediateContext->RSSetState(RenderStates::CullClockwiseRS);
		pd3dImmediateContext->OMSetDepthStencilState(RenderStates::DrawReflectionDSS, 1);
		
		pass->Apply(0,pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(mSkullIndexCount,0,0);

		for(int i=0;i<3;++i)
		{
			mDirLights[i].Direction = oldLightDirections[i];
		}
		DirLights->SetRawValue(&mDirLights,0,sizeof(DirectionalLight));

		pd3dImmediateContext->RSSetState(nullptr);
		pd3dImmediateContext->OMSetDepthStencilState(nullptr, 0);
	}

	//draw mirror
	activeTech->GetDesc(&techDesc);
	for(UINT p=0;p<techDesc.Passes;++p)
	{
		ID3DX11EffectPass* pass = activeTech->GetPassByIndex(p);
		pd3dImmediateContext->IASetVertexBuffers(0,1,&mRoomVB,&stride,&offset);

		D3DXMATRIX worldInvTranspose = InverseTranspose(&mRoomWorld);
		D3DXMATRIX worldViewProj = mRoomWorld*mView*mProj;

		World->SetMatrix(mRoomWorld);
		WorldInvTranspose->SetMatrix(worldInvTranspose);
		WorldViewProj->SetMatrix(worldViewProj);
		TexTransform->SetMatrix(I);
		Mat->SetRawValue(&mMirrorMat,0,sizeof(Material));
		DiffuseMap->SetResource(mMirrorDiffuseMapSRV);

		pd3dImmediateContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);

		pass->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->Draw(6, 24);

	}
	//shadow
	activeSkullTech->GetDesc( &techDesc );
	for(UINT p = 0; p < techDesc.Passes; ++p)
    {
		ID3DX11EffectPass* pass = activeSkullTech->GetPassByIndex( p );

		pd3dImmediateContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
		pd3dImmediateContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

		D3DXPLANE  shadowPlane(0.0f,1.0f,0.0f,0.0f); // xz plane
		D3DXMATRIX shadowTransform,shadowOffsetY;
		D3DXVECTOR4 dirLight(-mDirLights[0].Direction,0.f);
		D3DXMatrixShadow(&shadowTransform,&dirLight,&shadowPlane);
		D3DXMatrixTranslation(&shadowOffsetY,0.0f,0.001f,0.0f);
	
		D3DXMATRIX world = mSkullWorld * shadowTransform * shadowOffsetY ;
		D3DXMATRIX worldInvTranspose = InverseTranspose(&world);
		D3DXMATRIX worldViewProj = world * mView * mProj;

		World->SetMatrix(world);
		WorldInvTranspose->SetMatrix(worldInvTranspose);
		WorldViewProj->SetMatrix(worldViewProj);
		Mat->SetRawValue(&mShadowMat,0,sizeof(Material));

		pd3dImmediateContext->OMSetDepthStencilState(RenderStates::NoDoubleBlendDSS, 0);
		pass->Apply(0, pd3dImmediateContext);
		pd3dImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);

		// Restore default states.
		pd3dImmediateContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
		pd3dImmediateContext->OMSetDepthStencilState(nullptr, 0);
	}
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	RenderStates::Release();
	SAFE_RELEASE(mRoomVB);
	SAFE_RELEASE(mSkullVB);
	SAFE_RELEASE(mSkullIB);
	SAFE_RELEASE(g_pEffect); 
	SAFE_RELEASE(mFloorDiffuseMapSRV); 
	SAFE_RELEASE(mWallDiffuseMapSRV);
	SAFE_RELEASE(mMirrorDiffuseMapSRV);
	SAFE_RELEASE(m_pVertexLayout);
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext )
{
	mCamera.HandleMessages(hWnd, uMsg, wParam, lParam);
    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
                       bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
                       int xPos, int yPos, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved( void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	Initialize();
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D11) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set general DXUT callbacks
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackMouse( OnMouse );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackDeviceRemoved( OnDeviceRemoved );

    // Set the D3D11 DXUT callbacks. Remove these sets if the app doesn't need to support D3D11
    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    // Perform any application-level initialization here

    DXUTInit( true, true, nullptr ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"MirrorDemo" );

    // Only require 10-level hardware
    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, screen_width, screen_height );
    DXUTMainLoop(); // Enter into the DXUT ren  der loop

    // Perform any application-level cleanup here

    return DXUTGetExitCode();
}

void Initialize()
{
	mRoomMat.Ambient  = D3DXVECTOR4(0.5f, 0.5f, 0.5f, 1.0f);
	mRoomMat.Diffuse  = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	mRoomMat.Specular = D3DXVECTOR4(0.4f, 0.4f, 0.4f, 16.0f);

	mSkullMat.Ambient  = D3DXVECTOR4(0.5f, 0.5f, 0.5f, 1.0f);
	mSkullMat.Diffuse  = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	mSkullMat.Specular = D3DXVECTOR4(0.4f, 0.4f, 0.4f, 16.0f);

	// Reflected material is transparent so it blends into mirror.
	mMirrorMat.Ambient  = D3DXVECTOR4(0.5f, 0.5f, 0.5f, 1.0f);
	mMirrorMat.Diffuse  = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 0.5f);
	mMirrorMat.Specular = D3DXVECTOR4(0.4f, 0.4f, 0.4f, 16.0f);

	mShadowMat.Ambient  = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 1.0f);
	mShadowMat.Diffuse  = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.5f);
	mShadowMat.Specular = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 16.0f);

	mDirLights[0].Ambient  = D3DXVECTOR4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[0].Diffuse  = D3DXVECTOR4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Specular = D3DXVECTOR4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Direction = D3DXVECTOR3(0.57735f, -0.57735f, 0.57735f);

	mDirLights[1].Ambient  = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[1].Diffuse  = D3DXVECTOR4(0.20f, 0.20f, 0.20f, 1.0f);
	mDirLights[1].Specular = D3DXVECTOR4(0.25f, 0.25f, 0.25f, 1.0f);
	mDirLights[1].Direction = D3DXVECTOR3(-0.57735f, -0.57735f, 0.57735f);

	mDirLights[2].Ambient  = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Diffuse  = D3DXVECTOR4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[2].Specular = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Direction = D3DXVECTOR3(0.0f, -0.707f, -0.707f);

	D3DXMatrixIdentity(&mSkullWorld);
	mSkullTranslation = D3DXVECTOR3(0.0f, 1.0f, -5.0f);

	mCamera.SetViewParams(&m_Eye, &m_At);
}

HRESULT BuildRoomGeometryBuffers(ID3D11Device* pd3dDevice)
{
	// Create and specify geometry.  For this sample we draw a floor
	// and a wall with a mirror on it.  We put the floor, wall, and
	// mirror geometry in one vertex buffer.
	//
	//   |--------------|
	//   |              |
    //   |----|----|----|
    //   |Wall|Mirr|Wall|
	//   |    | or |    |
    //   /--------------/
    //  /   Floor      /
	// /--------------/

	HRESULT hr = S_OK;
	Vertex::Basic32 v[30];

	// Floor: Observe we tile texture coordinates.
	v[0] = Vertex::Basic32(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
	v[1] = Vertex::Basic32(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex::Basic32( 7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);
	
	v[3] = Vertex::Basic32(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
	v[4] = Vertex::Basic32( 7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);
	v[5] = Vertex::Basic32( 7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f);

	// Wall: Observe we tile texture coordinates, and that we
	// leave a gap in the middle for the mirror.
	v[6]  = Vertex::Basic32(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[7]  = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[8]  = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f);
	
	v[9]  = Vertex::Basic32(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[10] = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f);
	v[11] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f);

	v[12] = Vertex::Basic32(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[13] = Vertex::Basic32(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[14] = Vertex::Basic32(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
	
	v[15] = Vertex::Basic32(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[16] = Vertex::Basic32(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
	v[17] = Vertex::Basic32(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f);

	v[18] = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[19] = Vertex::Basic32(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[20] = Vertex::Basic32( 7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f);
	
	v[21] = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[22] = Vertex::Basic32( 7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f);
	v[23] = Vertex::Basic32( 7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f);

	// Mirror
	v[24] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[25] = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[26] = Vertex::Basic32( 2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	
	v[27] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[28] = Vertex::Basic32( 2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[29] = Vertex::Basic32( 2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex::Basic32) * ARRAYSIZE(v);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = v;
    V_RETURN(pd3dDevice->CreateBuffer(&vbd, &vinitData, &mRoomVB));
	return S_OK;
}

HRESULT BuildSkullGeometryBuffers(ID3D11Device* pd3dDevice)
{
	HRESULT hr = S_OK;
	std::ifstream fin("skull.txt");
	
	if(!fin)
	{
		MessageBox(nullptr, L"skull.txt not found.", nullptr, 0);
		return S_FALSE;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;
	
	std::vector<Vertex::Basic32> vertices(vcount);
	for(UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	mSkullIndexCount = 3*tcount;
	std::vector<UINT> indices(mSkullIndexCount);
	for(UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i*3+0] >> indices[i*3+1] >> indices[i*3+2];
	}

	fin.close();

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;//A resource that can only be read by the GPU. It cannot be written by the GPU, and cannot be accessed at all by the CPU. This type of resource must be initialized when it is created, since it cannot be changed after creation.

	vbd.ByteWidth = sizeof(Vertex::Basic32) * vcount;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    V_RETURN(pd3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mSkullIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
    V_RETURN(pd3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullIB));
	return S_OK;
}