#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include <cstring>
#include "Utils.h"

using Microsoft::WRL::ComPtr;

class ShaderPipeline
{
public:
    void Create(ID3D12Device* device,
        const D3D12_INPUT_ELEMENT_DESC* inputLayout, UINT inputCount,
        const char* vsSource, const char* psSource,
        DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat,
        bool enableBlend = false,
        bool depthWrite = true,
        D3D12_CULL_MODE cull = D3D12_CULL_MODE_BACK)
    {

        D3D12_DESCRIPTOR_RANGE ranges[4]{};

        ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[0].NumDescriptors = 1;
        ranges[0].BaseShaderRegister = 0; // t0
        ranges[0].RegisterSpace = 0;
        ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[1].NumDescriptors = 1;
        ranges[1].BaseShaderRegister = 1; // t1
        ranges[1].RegisterSpace = 0;
        ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[2].NumDescriptors = 1;
        ranges[2].BaseShaderRegister = 2; // t2
        ranges[2].RegisterSpace = 0;
        ranges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[3].NumDescriptors = 1;
        ranges[3].BaseShaderRegister = 3; // t3
        ranges[3].BaseShaderRegister = 3;
        ranges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER params[5]{};

        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0; // b0
        params[0].Descriptor.RegisterSpace = 0;
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[1].DescriptorTable.NumDescriptorRanges = 1;
        params[1].DescriptorTable.pDescriptorRanges = &ranges[0];
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[2].DescriptorTable.NumDescriptorRanges = 1;
        params[2].DescriptorTable.pDescriptorRanges = &ranges[1];
        params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[3].DescriptorTable.NumDescriptorRanges = 1;
        params[3].DescriptorTable.pDescriptorRanges = &ranges[2];
        params[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[4].DescriptorTable.NumDescriptorRanges = 1;
        params[4].DescriptorTable.pDescriptorRanges = &ranges[3];
        params[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC samplers[2]{};

        samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[0].MipLODBias = 0.0f;
        samplers[0].MaxAnisotropy = 1;
        samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplers[0].MinLOD = 0.0f;
        samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        samplers[0].RegisterSpace = 0;
        samplers[0].ShaderRegister = 0; // s0

        samplers[1] = samplers[0];
		samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplers[1].ShaderRegister = 1; // s1

        D3D12_ROOT_SIGNATURE_DESC rsDesc{};
        rsDesc.NumParameters = _countof(params);
        rsDesc.pParameters = params;
        rsDesc.NumStaticSamplers = _countof(samplers);
        rsDesc.pStaticSamplers = samplers;
        rsDesc.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        ComPtr<ID3DBlob> sig, err;
        HRESULT hr = D3D12SerializeRootSignature(
            &rsDesc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &sig,
            &err
        );
        if (FAILED(hr))
        {
            if (err) {
                OutputDebugStringA((char*)err->GetBufferPointer());
            }
            DXThrow(hr);
        }

        DXThrow(device->CreateRootSignature(
            0,
            sig->GetBufferPointer(),
            sig->GetBufferSize(),
            IID_PPV_ARGS(&m_root)));

        ComPtr<ID3DBlob> compileErrs;
        UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined(_DEBUG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        D3DCompile(
            vsSource, std::strlen(vsSource),
            nullptr, nullptr, nullptr,
            "main", "vs_5_1",
            compileFlags, 0,
            m_vsBlob.GetAddressOf(), &compileErrs);
        if (compileErrs) {
            OutputDebugStringA((char*)compileErrs->GetBufferPointer());
        }
        compileErrs.Reset();

        auto errshader = D3DCompile(
            psSource, std::strlen(psSource),
            nullptr, nullptr, nullptr,
            "main", "ps_5_1",
            compileFlags, 0,
            m_psBlob.GetAddressOf(), &compileErrs);
        if (compileErrs) {
            OutputDebugStringA((char*)compileErrs->GetBufferPointer());
			DXThrow(errshader);
        }

        m_cachedInputElements.assign(inputLayout, inputLayout + inputCount);
        m_cachedInputLayout = { m_cachedInputElements.data(), inputCount };
        m_cachedRTV = rtvFormat;
        m_cachedDSV = dsvFormat;

        auto blend = CreateBlend(enableBlend);
        auto depth = CreateDepth(depthWrite);

        {
            auto rastSolid = CreateRast(false, cull);
            auto psoDesc = CreatePso(rastSolid, blend, depth);
            DXThrow(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_psoSolid)));
        }
        {
            auto rastWire = CreateRast(true, cull);
            auto psoDesc = CreatePso(rastWire, blend, depth);
            DXThrow(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_psoWire)));
        }

        m_psoCurrent = m_psoSolid;
        this->device = device;

    }

    void setWireframe(bool enable)
    {
        m_psoCurrent = enable ? m_psoWire : m_psoSolid;
    }

    void Destroy()
    {
        m_psoSolid.Reset();
        m_psoWire.Reset();
        m_psoCurrent.Reset();
        m_root.Reset();
        m_vsBlob.Reset();
        m_psBlob.Reset();
        m_cachedInputElements.clear();
        m_cachedInputLayout = {};
        m_cachedRTV = DXGI_FORMAT_UNKNOWN;
        m_cachedDSV = DXGI_FORMAT_UNKNOWN;
        device = nullptr;
	}

    ID3D12PipelineState* PSO()  const { return m_psoCurrent.Get(); }
    ID3D12RootSignature* Root() const { return m_root.Get(); }

private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC CreatePso(
        const D3D12_RASTERIZER_DESC& rast,
        const D3D12_BLEND_DESC& blend,
        const D3D12_DEPTH_STENCIL_DESC& depth)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        pso.pRootSignature = m_root.Get();
        pso.VS = { m_vsBlob->GetBufferPointer(), m_vsBlob->GetBufferSize() };
        pso.PS = { m_psBlob->GetBufferPointer(), m_psBlob->GetBufferSize() };
        pso.InputLayout = m_cachedInputLayout;
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.RasterizerState = rast;
        pso.BlendState = blend;
        pso.DepthStencilState = depth;
        pso.DSVFormat = m_cachedDSV;
        pso.SampleDesc = { 1, 0 };
        pso.SampleMask = UINT_MAX;

        if (m_cachedRTV != DXGI_FORMAT_UNKNOWN)
        {
            pso.NumRenderTargets = 1;
            pso.RTVFormats[0] = m_cachedRTV;
            for (UINT i = 1; i < _countof(pso.RTVFormats); ++i)
                pso.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
        }
        else
        {
            pso.NumRenderTargets = 0;
            for (UINT i = 0; i < _countof(pso.RTVFormats); ++i)
                pso.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
        }

        return pso;
    }


    D3D12_BLEND_DESC CreateBlend(bool enableBlend);
    D3D12_DEPTH_STENCIL_DESC CreateDepth(bool depthWrite);
    D3D12_RASTERIZER_DESC CreateRast(bool wireframed, D3D12_CULL_MODE cull);


private:
    ComPtr<ID3D12RootSignature>    m_root;

    ComPtr<ID3D12PipelineState>    m_psoSolid;
    ComPtr<ID3D12PipelineState>    m_psoWire;
    ComPtr<ID3D12PipelineState>    m_psoCurrent;

    ComPtr<ID3DBlob>               m_vsBlob;
    ComPtr<ID3DBlob>               m_psBlob;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_cachedInputElements;
    D3D12_INPUT_LAYOUT_DESC        m_cachedInputLayout{};
    DXGI_FORMAT                    m_cachedRTV = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT                    m_cachedDSV = DXGI_FORMAT_UNKNOWN;

    ID3D12Device* device = nullptr;
};
