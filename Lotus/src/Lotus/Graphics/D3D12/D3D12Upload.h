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
//  File Name: D3D12Upload.h
//  Date File Created: 02/19/2023
//  Author: Matt
//
//  ------------------------------------------------------------------------------

#pragma once

#include "D3D12Common.h"

namespace lotus::graphics::d3d12::upload
{
class d3d12_upload_context
{
public:
    d3d12_upload_context(u32 aligned_size);

    DISABLE_COPY_AND_MOVE(d3d12_upload_context);

    ~d3d12_upload_context() = default;

    void end_upload();

    [[nodiscard]] constexpr id3d12_graphics_command_list* const command_list() const { return m_cmd_list; }
    [[nodiscard]] constexpr ID3D12Resource* const               upload_buffer() const { return m_upload_buffer; }
    [[nodiscard]] constexpr void* const                         cpu_address() const { return m_cpu_address; }

private:
    L_DBG(d3d12_upload_context() = default);

    id3d12_graphics_command_list* m_cmd_list{ nullptr };
    ID3D12Resource*               m_upload_buffer{ nullptr };
    void*                         m_cpu_address{ nullptr };
    u32                           m_frame_index{ invalid_id_u32 };
};

bool initialize();
void shutdown();

} // namespace lotus::graphics::d3d12::upload