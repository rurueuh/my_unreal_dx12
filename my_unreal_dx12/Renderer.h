#pragma once
#include <d3d12.h>
#include "GraphicsDevice.h"
#include "SwapChain.h"
#include "DepthBuffer.h"
#include "CommandContext.h"
#include "ShaderPipeline.h"
#include "ConstantBuffer.h"
#include "Mesh.h"
#include "ShadowMap.h"

class Renderer
{
public:
    void Initialize(GraphicsDevice& gd, SwapChain& sc, DepthBuffer& db)
    {
        m_gd = &gd; m_sc = &sc; m_db = &db;
        m_cmd.Initialize(gd.Device());
        m_viewport = { 0, 0, static_cast<float>(sc.Width()), static_cast<float>(sc.Height()), 0.0f, 1.0f };
        m_scissor = { 0, 0, static_cast<LONG>(sc.Width()), static_cast<LONG>(sc.Height()) };
    }

    void SetPipeline(const ShaderPipeline& pipe)
    {
        m_pipe = &pipe;
    }

    void BeginShadowPass(ShadowMap& sm, const ShaderPipeline& pipe)
    {
        ID3D12GraphicsCommandList* cmd = m_cmd.Get();

        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        b.Transition.pResource = sm.Resource();
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        cmd->ResourceBarrier(1, &b);

        auto dsv = sm.DSV();
        cmd->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
        cmd->RSSetViewports(1, &sm.Viewport());
        cmd->RSSetScissorRects(1, &sm.Scissor());

        cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        cmd->SetGraphicsRootSignature(pipe.Root());
        cmd->SetPipelineState(pipe.PSO());
    }

    void EndShadowPass(ShadowMap& sm)
    {
        ID3D12GraphicsCommandList* cmd = m_cmd.Get();

        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        b.Transition.pResource = sm.Resource();
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        cmd->ResourceBarrier(1, &b);
    }

    void BeginFrame(UINT frameIndex)
    {
        m_cmd.Begin(frameIndex);

        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = m_sc->BackBuffer(frameIndex);
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        m_cmd.Get()->ResourceBarrier(1, &b);

        auto rtv = m_sc->CurrentRTV();
        auto dsv = m_db->DSV();
        m_cmd.Get()->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
        m_cmd.Get()->RSSetViewports(1, &m_viewport);
        m_cmd.Get()->RSSetScissorRects(1, &m_scissor);

        const float clear[4]{ 0.02f, 0.1f, 0.2f, 1.0f };
        m_cmd.Get()->ClearRenderTargetView(rtv, clear, 0, nullptr);
        m_cmd.Get()->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        m_cmd.Get()->SetGraphicsRootSignature(m_pipe->Root());
        m_cmd.Get()->SetPipelineState(m_pipe->PSO());
    }

    void DrawMesh(const Mesh& mesh,
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr,
        D3D12_GPU_DESCRIPTOR_HANDLE texHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE shadowHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE normalHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE metalRoughHandle
    );

    void DrawMeshRange(const Mesh& mesh,
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr,
        D3D12_GPU_DESCRIPTOR_HANDLE texHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE shadowHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE normalHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE metalRoughHandle,
        UINT indexStart,
        UINT indexCount
    );

    void DrawMeshShadow(const Mesh& mesh, D3D12_GPU_VIRTUAL_ADDRESS cbAddr)
    {
        ID3D12GraphicsCommandList* cmd = m_cmd.Get();

        cmd->SetGraphicsRootConstantBufferView(0, cbAddr); // b0

        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->IASetVertexBuffers(0, 1, &mesh.VBV());
        cmd->IASetIndexBuffer(&mesh.IBV());
        cmd->DrawIndexedInstanced(mesh.IndexCount(), 1, 0, 0, 0);
    }

    void EndFrame(UINT frameIndex)
    {
        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = m_sc->BackBuffer(frameIndex);
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        m_cmd.Get()->ResourceBarrier(1, &b);

        m_cmd.End();
        ID3D12CommandList* lists[]{ m_cmd.Get() };
        m_gd->Queue()->ExecuteCommandLists(1, lists);
        DXThrow(m_sc->Swap()->Present(1, 0));
        m_gd->WaitGPU();
        m_sc->UpdateFrameIndex();
    }

    void BindMainRenderTargets()
    {
        ID3D12GraphicsCommandList* cmd = m_cmd.Get();

        auto rtv = m_sc->CurrentRTV();
        auto dsv = m_db->DSV();

        cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
        cmd->RSSetViewports(1, &m_viewport);
        cmd->RSSetScissorRects(1, &m_scissor);

        if (m_pipe)
        {
            cmd->SetGraphicsRootSignature(m_pipe->Root());
            cmd->SetPipelineState(m_pipe->PSO());
        }
    }

    void OnResize(UINT newW, UINT newH)
    {
        m_viewport = { 0, 0, static_cast<float>(newW), static_cast<float>(newH), 0.0f, 1.0f };
        m_scissor = { 0, 0, static_cast<LONG>(newW), static_cast<LONG>(newH) };
    }

    ID3D12GraphicsCommandList* GetCommandList() { return m_cmd.Get(); }

private:
    GraphicsDevice* m_gd = nullptr;
    SwapChain* m_sc = nullptr;
    DepthBuffer* m_db = nullptr;
    const ShaderPipeline* m_pipe = nullptr;

    CommandContext   m_cmd;
    D3D12_VIEWPORT   m_viewport{};
    D3D12_RECT       m_scissor{};

};
