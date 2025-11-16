#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdexcept>

void Texture::LoadFromFile(GraphicsDevice& gd,
    const char* path,
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpu,
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpu)
{
    int w = 0, h = 0, comp = 0;
    unsigned char* data = stbi_load(path, &w, &h, &comp, 4);
    if (!data) {
        throw std::runtime_error("Failed to load image");
    }

    auto device = gd.Device();

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = (UINT)w;
    desc.Height = (UINT)h;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapDef{};
    heapDef.Type = D3D12_HEAP_TYPE_DEFAULT;

    DXThrow(device->CreateCommittedResource(
        &heapDef,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_tex)));

    UINT64 uploadSize = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};
    UINT numRows = 0;
    UINT64 rowSize = 0, totalBytes = 0;
    device->GetCopyableFootprints(&desc, 0, 1, 0, &fp, &numRows, &rowSize, &totalBytes);
    uploadSize = totalBytes;

    D3D12_HEAP_PROPERTIES heapUp{};
    heapUp.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufDesc{};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = uploadSize;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.SampleDesc.Quality = 0;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    DXThrow(device->CreateCommittedResource(
        &heapUp,
        D3D12_HEAP_FLAG_NONE,
        &bufDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_upload)));

    uint8_t* mapped = nullptr;
    D3D12_RANGE r{ 0,0 };
    DXThrow(m_upload->Map(0, &r, reinterpret_cast<void**>(&mapped)));

    const size_t srcPitch = size_t(w) * 4;
    for (UINT row = 0; row < numRows; ++row) {
        memcpy(
            mapped + fp.Offset + row * fp.Footprint.RowPitch,
            data + row * srcPitch,
            srcPitch
        );
    }

    m_upload->Unmap(0, nullptr);
    stbi_image_free(data);

    ComPtr<ID3D12CommandAllocator> alloc;
    DXThrow(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc)));

    ComPtr<ID3D12GraphicsCommandList> list;
    DXThrow(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        alloc.Get(),
        nullptr,
        IID_PPV_ARGS(&list)));

    D3D12_TEXTURE_COPY_LOCATION dst{};
    dst.pResource = m_tex.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src{};
    src.pResource = m_upload.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = fp;

    list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = m_tex.Get();
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    b.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    list->ResourceBarrier(1, &b);

    DXThrow(list->Close());
    ID3D12CommandList* lists[]{ list.Get() };
    gd.Queue()->ExecuteCommandLists(1, lists);
    gd.WaitGPU();

    m_srvCPU = srvCpu;
    m_srvGPU = srvGpu;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MostDetailedMip = 0;
    srv.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(m_tex.Get(), &srv, m_srvCPU);
}

void Texture::InitWhite1x1(GraphicsDevice& gd,
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpu,
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpu)
{
    LoadFromFile(gd, "white.jpg", srvCpu, srvGpu);
}
