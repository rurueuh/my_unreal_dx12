#include "Renderer.h"
#include "WindowDX12.h"

void Renderer::DrawMesh(const Mesh& mesh,
                        D3D12_GPU_VIRTUAL_ADDRESS cbAddr,
                        D3D12_GPU_DESCRIPTOR_HANDLE texHandle,
                        D3D12_GPU_DESCRIPTOR_HANDLE shadowHandle)
{
    ID3D12GraphicsCommandList* cmd = m_cmd.Get();

    cmd->SetGraphicsRootConstantBufferView(0, cbAddr);
    cmd->SetGraphicsRootDescriptorTable(1, texHandle);
    cmd->SetGraphicsRootDescriptorTable(2, shadowHandle);

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->IASetVertexBuffers(0, 1, &mesh.VBV());
    cmd->IASetIndexBuffer(&mesh.IBV());
    cmd->DrawIndexedInstanced(mesh.IndexCount(), 1, 0, 0, 0);
}