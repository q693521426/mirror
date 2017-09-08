#include "DXUT.h"
#include "RenderStates.h"

ID3D11RasterizerState* RenderStates::WireframeRS = nullptr;
ID3D11RasterizerState* RenderStates::NoCullRS = nullptr;
ID3D11RasterizerState* RenderStates::CullClockwiseRS = nullptr;

ID3D11BlendState* RenderStates::AlphaToCoverageBS = nullptr;
ID3D11BlendState* RenderStates::TransparentBS = nullptr;
ID3D11BlendState* RenderStates::NoRenderTargetWritesBS = nullptr;

ID3D11DepthStencilState* RenderStates::MarkMirrorDSS = nullptr;
ID3D11DepthStencilState* RenderStates::DrawReflectionDSS = nullptr;
ID3D11DepthStencilState* RenderStates::NoDoubleBlendDSS = nullptr;

RenderStates::RenderStates(void)
{
}


RenderStates::~RenderStates(void)
{
}

HRESULT RenderStates::Initialize(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	D3D11_RASTERIZER_DESC wireframeDesc;
	ZeroMemory(&wireframeDesc,sizeof(wireframeDesc));
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_BACK;
	wireframeDesc.FrontCounterClockwise = false;
	wireframeDesc.DepthClipEnable = true;
	V_RETURN(device->CreateRasterizerState(&wireframeDesc,&WireframeRS));

	D3D11_RASTERIZER_DESC noCullDesc;
	ZeroMemory(&noCullDesc,sizeof(noCullDesc));
	noCullDesc.FillMode = D3D11_FILL_SOLID;
	noCullDesc.CullMode = D3D11_CULL_NONE;
	noCullDesc.FrontCounterClockwise = false;
	noCullDesc.DepthClipEnable = true;
	V_RETURN(device->CreateRasterizerState(&noCullDesc,&NoCullRS));
	
	D3D11_RASTERIZER_DESC cullClockwiseDesc;
	ZeroMemory(&cullClockwiseDesc,sizeof(cullClockwiseDesc));
	cullClockwiseDesc.FillMode = D3D11_FILL_SOLID;
	cullClockwiseDesc.CullMode = D3D11_CULL_BACK;
	cullClockwiseDesc.FrontCounterClockwise = true;
	cullClockwiseDesc.DepthClipEnable = true;
	V_RETURN(device->CreateRasterizerState(&cullClockwiseDesc,&CullClockwiseRS));

	D3D11_BLEND_DESC alphaToCoverageDesc;
	ZeroMemory(&alphaToCoverageDesc,sizeof(alphaToCoverageDesc));
	alphaToCoverageDesc.AlphaToCoverageEnable = true;
	alphaToCoverageDesc.IndependentBlendEnable = false;
	alphaToCoverageDesc.RenderTarget[0].BlendEnable = false;
	alphaToCoverageDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	V_RETURN(device->CreateBlendState(&alphaToCoverageDesc,&AlphaToCoverageBS));

	D3D11_BLEND_DESC transparentDesc;
	ZeroMemory(&transparentDesc,sizeof(transparentDesc));
	transparentDesc.AlphaToCoverageEnable = false;
	transparentDesc.IndependentBlendEnable = false;

	transparentDesc.RenderTarget[0].BlendEnable = true;
	transparentDesc.RenderTarget[0].SrcBlend		= D3D11_BLEND_SRC_ALPHA;
	transparentDesc.RenderTarget[0].DestBlend		= D3D11_BLEND_INV_SRC_ALPHA; 
	transparentDesc.RenderTarget[0].BlendOp			= D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].SrcBlendAlpha	= D3D11_BLEND_ONE;
	transparentDesc.RenderTarget[0].DestBlendAlpha	= D3D11_BLEND_ZERO;
	transparentDesc.RenderTarget[0].BlendOpAlpha	= D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	
	V_RETURN(device->CreateBlendState(&transparentDesc,&TransparentBS));

	D3D11_BLEND_DESC noRenderTargetWritesDesc;
	ZeroMemory(&noRenderTargetWritesDesc,sizeof(noRenderTargetWritesDesc));
	noRenderTargetWritesDesc.AlphaToCoverageEnable = false;
	noRenderTargetWritesDesc.IndependentBlendEnable = false;

	noRenderTargetWritesDesc.RenderTarget[0].SrcBlend		= D3D11_BLEND_ONE;
	noRenderTargetWritesDesc.RenderTarget[0].DestBlend		= D3D11_BLEND_ZERO;
	noRenderTargetWritesDesc.RenderTarget[0].BlendOp		= D3D11_BLEND_OP_ADD;
	noRenderTargetWritesDesc.RenderTarget[0].SrcBlendAlpha	= D3D11_BLEND_ONE;
	noRenderTargetWritesDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	noRenderTargetWritesDesc.RenderTarget[0].BlendOpAlpha	= D3D11_BLEND_OP_ADD;
	noRenderTargetWritesDesc.RenderTarget[0].RenderTargetWriteMask = 0;
	V_RETURN(device->CreateBlendState(&noRenderTargetWritesDesc,&NoRenderTargetWritesBS));
	
	D3D11_DEPTH_STENCIL_DESC mirrorDesc;
	mirrorDesc.DepthEnable		= true;
	mirrorDesc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ZERO; //Turn off writes to the depth-stencil buffer
	mirrorDesc.DepthFunc		= D3D11_COMPARISON_LESS;
	mirrorDesc.StencilEnable	= true;
	mirrorDesc.StencilReadMask	= 0xff;
	mirrorDesc.StencilWriteMask	= 0xff;

	mirrorDesc.FrontFace.StencilFailOp		= D3D11_STENCIL_OP_KEEP;
	mirrorDesc.FrontFace.StencilDepthFailOp	= D3D11_STENCIL_OP_KEEP;
	mirrorDesc.FrontFace.StencilPassOp		= D3D11_STENCIL_OP_REPLACE;
	mirrorDesc.FrontFace.StencilFunc		= D3D11_COMPARISON_ALWAYS;	

	//mirrorDesc.BackFace.StencilFailOp		= D3D11_STENCIL_OP_KEEP;
	//mirrorDesc.BackFace.StencilDepthFailOp	= D3D11_STENCIL_OP_KEEP;
	//mirrorDesc.BackFace.StencilPassOp		= D3D11_STENCIL_OP_REPLACE;
	//mirrorDesc.BackFace.StencilFunc			= D3D11_COMPARISON_ALWAYS;

	V_RETURN(device->CreateDepthStencilState(&mirrorDesc,&MarkMirrorDSS));

	D3D11_DEPTH_STENCIL_DESC drawReflectionDesc;
	drawReflectionDesc.DepthEnable		= true;
	drawReflectionDesc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ALL;
	drawReflectionDesc.DepthFunc		= D3D11_COMPARISON_LESS;
	drawReflectionDesc.StencilEnable	= true;
	drawReflectionDesc.StencilReadMask	= 0xff;
	drawReflectionDesc.StencilWriteMask	= 0xff;

	drawReflectionDesc.FrontFace.StencilFailOp		= D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.FrontFace.StencilDepthFailOp	= D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.FrontFace.StencilPassOp		= D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.FrontFace.StencilFunc		= D3D11_COMPARISON_EQUAL;	

	//drawReflectionDesc.BackFace.StencilFailOp		= D3D11_STENCIL_OP_KEEP;
	//drawReflectionDesc.BackFace.StencilDepthFailOp	= D3D11_STENCIL_OP_KEEP;
	//drawReflectionDesc.BackFace.StencilPassOp		= D3D11_STENCIL_OP_KEEP;
	//drawReflectionDesc.BackFace.StencilFunc			= D3D11_COMPARISON_EQUAL;

	V_RETURN(device->CreateDepthStencilState(&mirrorDesc,&MarkMirrorDSS));

	return S_OK;
}

void RenderStates::Release()
{
	SAFE_RELEASE(WireframeRS);
	SAFE_RELEASE(NoCullRS);
	SAFE_RELEASE(CullClockwiseRS);

	SAFE_RELEASE(AlphaToCoverageBS);
	SAFE_RELEASE(TransparentBS);
	SAFE_RELEASE(NoRenderTargetWritesBS);

	SAFE_RELEASE(MarkMirrorDSS);
	SAFE_RELEASE(DrawReflectionDSS);
	SAFE_RELEASE(NoDoubleBlendDSS);
}