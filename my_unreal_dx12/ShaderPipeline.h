#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <string>
#include "Utils.h" 
#include <vector>
using Microsoft::WRL::ComPtr;

class ShaderPipeline
{
public:
    void Create(ID3D12Device* device,
        const D3D12_INPUT_ELEMENT_DESC* inputLayout, UINT inputCount,
        const char* vsSource, const char* psSource,
        DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat)
    {
        D3D12_DESCRIPTOR_RANGE range{};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 1;
        range.BaseShaderRegister = 0; // t0
        range.RegisterSpace = 0;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER params[2]{};
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0;
        params[0].Descriptor.RegisterSpace = 0;
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[1].DescriptorTable.NumDescriptorRanges = 1;
        params[1].DescriptorTable.pDescriptorRanges = &range;
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC samp{};
        samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samp.ShaderRegister = 0; // s0
        samp.RegisterSpace = 0;
        samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rsDesc{};
        rsDesc.NumParameters = 2;
        rsDesc.pParameters = params;
		rsDesc.NumStaticSamplers = 1;
		rsDesc.pStaticSamplers = &samp;
        rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> sig, err;
        DXThrow(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err));
        DXThrow(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&m_root)));

        ComPtr<ID3DBlob> compileErrs;
        DXThrow(D3DCompile(vsSource, ::strlen(vsSource), nullptr, nullptr, nullptr, "main", "vs_5_1", 0, 0, m_vsBlob.GetAddressOf(), &compileErrs));
        DXThrow(D3DCompile(psSource, ::strlen(psSource), nullptr, nullptr, nullptr, "main", "ps_5_1", 0, 0, m_psBlob.GetAddressOf(), &compileErrs));

        m_cachedInputElements.assign(inputLayout, inputLayout + inputCount);
        m_cachedInputLayout = { m_cachedInputElements.data(), inputCount };
        m_cachedRTV = rtvFormat;
        m_cachedDSV = dsvFormat;

        auto rast = CreateRast(false);

        auto blend = CreateBlend();

        auto ds = CreateDepth();

        auto pso = CreatePso(rast, blend, ds);

        DXThrow(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&m_pso)));
        this->device = device;
    }

    void setWireframe(bool enable)
    {
        auto rast = CreateRast(enable);

		auto blend = CreateBlend();

        auto ds = CreateDepth();

        auto pso = CreatePso(rast, blend, ds);

        ComPtr<ID3D12PipelineState> newPSO;
        DXThrow(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&newPSO)));
        m_pso = newPSO;
    }

    

    ID3D12PipelineState* PSO() const { return m_pso.Get(); }
    ID3D12RootSignature* Root() const { return m_root.Get(); }

private:

    D3D12_GRAPHICS_PIPELINE_STATE_DESC CreatePso(const D3D12_RASTERIZER_DESC& rast, const D3D12_BLEND_DESC& blend, const D3D12_DEPTH_STENCIL_DESC& ds)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        pso.pRootSignature = m_root.Get();
        pso.VS = { m_vsBlob->GetBufferPointer(), m_vsBlob->GetBufferSize() };
        pso.PS = { m_psBlob->GetBufferPointer(), m_psBlob->GetBufferSize() };
        pso.InputLayout = m_cachedInputLayout;
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.RasterizerState = rast;
        pso.BlendState = blend;
        pso.DepthStencilState = ds;
        pso.NumRenderTargets = 1;
        pso.RTVFormats[0] = m_cachedRTV;
        pso.DSVFormat = m_cachedDSV;
        pso.SampleDesc = { 1, 0 };
        pso.SampleMask = UINT_MAX;

        return pso;
    }

    D3D12_DEPTH_STENCIL_DESC CreateDepth()
    {
        D3D12_DEPTH_STENCIL_DESC ds{};
        ds.DepthEnable = TRUE;
        ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        return ds;
    }

    D3D12_RASTERIZER_DESC CreateRast(bool wireframed)
    {
        D3D12_RASTERIZER_DESC rast{};
        rast.FillMode = wireframed ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
        rast.CullMode = D3D12_CULL_MODE_BACK;
        rast.FrontCounterClockwise = FALSE;
        rast.DepthClipEnable = TRUE;

        return rast;
    }

    D3D12_BLEND_DESC CreateBlend()
	{
        D3D12_BLEND_DESC blend{};
        blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	    return blend;
	}

private:

    ComPtr<ID3D12RootSignature> m_root;
    ComPtr<ID3D12PipelineState> m_pso;

    ComPtr<ID3DBlob> m_vsBlob;
    ComPtr<ID3DBlob> m_psBlob;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_cachedInputElements;
    D3D12_INPUT_LAYOUT_DESC m_cachedInputLayout{};
    DXGI_FORMAT m_cachedRTV = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT m_cachedDSV = DXGI_FORMAT_UNKNOWN;

    ID3D12Device* device = nullptr;
};
