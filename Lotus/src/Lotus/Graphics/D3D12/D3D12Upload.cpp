//  ------------------------------------------------------------------------------
//
//  Lotus
//     Copyright 2023 Matthew Rogers
//
//     Licensed under the Apache License, Version 2.0 (the "License");
//     you may not use this file except in compliance with the License.
//     You may obtain a copy of the License at
//
//         http://www.apache.org/licenses/LICENSE-2.0
//
//     Unless required by applicable law or agreed to in writing, software
//     distributed under the License is distributed on an "AS IS" BASIS,
//     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//     See the License for the specific language governing permissions and
//     limitations under the License.
//
//  File Name: D3D12Upload.cpp
//  Date File Created: 02/19/2023
//  Author: Matt
//
//  ------------------------------------------------------------------------------

#include "D3D12Upload.h"
#include "D3D12Core.h"


namespace lotus::graphics::d3d12::upload
{
namespace
{
struct upload_frame
{
    ID3D12CommandAllocator*       cmd_allocator = nullptr;
    id3d12_graphics_command_list* cmd_list      = nullptr;
    ID3D12Resource*               upload_buffer = nullptr;
    void*                         cpu_address   = nullptr;
    u64                           fence_value   = 0;

    void wait_and_release();

    void release()
    {
        wait_and_release();
        core::release(cmd_allocator);
        core::release(cmd_list);
    }

    [[nodiscard]] constexpr bool is_ready() const { return upload_buffer == nullptr; }
};

constexpr u32       upload_frame_count = 4;
upload_frame        upload_frames[upload_frame_count]{};
ID3D12CommandQueue* upload_cmd_queue   = nullptr;
ID3D12Fence1*       upload_fence       = nullptr;
u64                 upload_fence_value = 0;
HANDLE              fence_event{};
std::mutex          frame_mutex{};
std::mutex          queue_mutex{};


void upload_frame::wait_and_release()
{
    assert(upload_fence && fence_event);
    if (upload_fence->GetCompletedValue() < fence_value)
    {
        DX_CALL(upload_fence->SetEventOnCompletion(fence_value, fence_event));
        WaitForSingleObject(fence_event, INFINITE);
    }

    core::release(upload_buffer);
    cpu_address = nullptr;
}


bool init_failed()
{
    shutdown();
    return false;
}


u32 get_available_upload_frame()
{
    u32 index {invalid_id_u32};
    const u32 count = upload_frame_count;
    upload_frame* const frames = &upload_frames[0];

    for (u32 i = 0; i < count; ++i)
    {
        if(frames[i].is_ready())
        {
            index = i;
            break;
        }
    }

    // No frames were ready
    if(index == invalid_id_u32)
    {
        index = 0;
        while(!frames[index].is_ready())
        {
            index = (index + 1) % count;
            std::this_thread::yield();
        }
    }

    return index;
}

} // anonymous namespace



d3d12_upload_context::d3d12_upload_context(u32 aligned_size)
{
    assert(upload_cmd_queue);

    {
        std::lock_guard lock(frame_mutex);
        m_frame_index = get_available_upload_frame();
        assert(m_frame_index != invalid_id_u32);
        // Prevent other threads from picking this frame - make is_ready false
        upload_frames[m_frame_index].upload_buffer = (ID3D12Resource*)1;
    }

    upload_frame& frame = upload_frames[m_frame_index];
    frame.upload_buffer = d3dx::create_buffer(nullptr, aligned_size, true);
    NAME_D3D_OBJ_INDEXED(frame.upload_buffer, aligned_size, L"UploadBuffer - size");

    const D3D12_RANGE range{};
    DX_CALL(frame.upload_buffer->Map(0, &range, &frame.cpu_address));
    assert(frame.cpu_address);

    m_cmd_list = frame.cmd_list;
    m_upload_buffer = frame.upload_buffer;
    m_cpu_address = frame.cpu_address;
    assert(m_cmd_list && m_upload_buffer && m_cpu_address);

    DX_CALL(frame.cmd_allocator->Reset());
    DX_CALL(frame.cmd_list->Reset(frame.cmd_allocator, nullptr));

}

void d3d12_upload_context::end_upload()
{
    assert(m_frame_index != invalid_id_u32);
    upload_frame& frame = upload_frames[m_frame_index];
    id3d12_graphics_command_list* const cmd_list = frame.cmd_list;
    DX_CALL(cmd_list->Close());

    std::lock_guard lock(queue_mutex);
    ID3D12CommandList* const cmd_lists[]{cmd_list};
    ID3D12CommandQueue* const cmd_queue = upload_cmd_queue;
    cmd_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);

    ++upload_fence_value;
    frame.fence_value = upload_fence_value;
    DX_CALL(cmd_queue->Signal(upload_fence, frame.fence_value));

    // Wait for copy queue to finish
    frame.wait_and_release();
    L_DBG(new (this) d3d12_upload_context());
}

bool initialize()
{
    id3d12_device* const device = core::device();
    assert(device && !upload_cmd_queue);

    HRESULT hr{ S_OK };

    for (u32 i = 0; i < upload_frame_count; ++i)
    {
        upload_frame& frame = upload_frames[i];
        DX_CALL(hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, L_PTR(&frame.cmd_allocator)));
        if (FAILED(hr))
            return init_failed();

        DX_CALL(hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, frame.cmd_allocator, nullptr,
                                               L_PTR(&frame.cmd_list)));
        if (FAILED(hr))
            return init_failed();

        DX_CALL(frame.cmd_list->Close());

        NAME_D3D_OBJ_INDEXED(frame.cmd_allocator, i, L"Upload Command Allocator");
        NAME_D3D_OBJ_INDEXED(frame.cmd_list, i, L"Upload Command List");
    }

    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Type     = D3D12_COMMAND_LIST_TYPE_COPY;

    DX_CALL(hr = device->CreateCommandQueue(&desc, L_PTR(&upload_cmd_queue)));
    if (FAILED(hr))
        return init_failed();

    NAME_D3D_OBJ(upload_cmd_queue, L"Upload Copy Queue");

    DX_CALL(hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, L_PTR(&upload_fence)));
    if (FAILED(hr))
        return init_failed();
    NAME_D3D_OBJ(upload_fence, L"Upload Fence");

    fence_event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    assert(fence_event);
    if (!fence_event)
        return init_failed();

    return true;
}


void shutdown()
{
    for (auto& upload_frame : upload_frames)
    {
        upload_frame.release();
    }

    if (fence_event)
    {
        CloseHandle(fence_event);
        fence_event = nullptr;
    }

    core::release(upload_cmd_queue);
    core::release(upload_fence);
    upload_fence_value = 0;
}


} // namespace lotus::graphics::d3d12::upload