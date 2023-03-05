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
//  File Name: D3D12Content.cpp
//  Date File Created: 02/19/2023
//  Author: Matt
//
//  ------------------------------------------------------------------------------

#include "D3D12Content.h"

#include "D3D12Core.h"
#include "Util/IOStream.h"
#include "Content/ContentToEngine.h"
#include "Graphics/Renderer.h"
#include "D3D12GPass.h"


namespace lotus::graphics::d3d12::content
{

namespace
{

struct submesh_view
{
    D3D12_VERTEX_BUFFER_VIEW position_buffer_view{};
    D3D12_VERTEX_BUFFER_VIEW element_buffer_view{};
    D3D12_INDEX_BUFFER_VIEW  index_buffer_view{};
    D3D_PRIMITIVE_TOPOLOGY   primitive_topology{};
    u32                      element_type{};
};

utl::free_list<ID3D12Resource*> submesh_buffers{};
utl::free_list<submesh_view>    submesh_views{};
std::mutex                      submesh_mutex{};

utl::free_list<d3d12_texture> textures{};
std::mutex                    texture_mutex{};

utl::vector<ID3D12RootSignature*>    root_signatures{};
std::unordered_map<u64, id::id_type> mtl_rs_map{}; // maps material type and shader flags to index in the root_signatures array
utl::free_list<scope<u8[]>>          materials{};
std::mutex                           material_mutex{};

id::id_type create_root_signature(material_type::type type, shader_flags::flags flags);

class d3d12_material_stream
{
public:
    explicit d3d12_material_stream(u8* const material_buffer) : m_buffer(material_buffer) { initialize(); }

    explicit d3d12_material_stream(scope<u8[]>& material_buffer, material_init_info info)
    {
        LASSERT(!material_buffer);

        u32 shader_count = 0;
        u32 flags        = 0;

        for (u32 i = 0; i < shader_type::count; ++i)
        {
            if (id::is_valid(info.shader_ids[i]))
            {
                ++shader_count;
                flags |= (1 << i);
            }
        }

        LASSERT(shader_count && flags);

        const u32 buffer_size = sizeof(material_type::type) +                             // material type
                                sizeof(shader_flags::flags) +                             // shader flags
                                sizeof(id::id_type) +                                     // root signature id
                                sizeof(u32) +                                             // texture count
                                sizeof(id::id_type) * shader_count +                      // shader ids
                                (sizeof(id::id_type) + sizeof(u32)) * info.texture_count; // texture ids & descriptor indices

        material_buffer  = create_scope<u8[]>(buffer_size);
        m_buffer         = material_buffer.get();
        u8* const buffer = m_buffer;

        *(material_type::type*) buffer                      = info.type;
        *(shader_flags::flags*) &buffer[shader_flags_index] = (shader_flags::flags) flags;
        *(id::id_type*) &buffer[root_signature_index]       = create_root_signature(info.type, (shader_flags::flags) flags);
        *(u32*) &buffer[texture_count_index]                = info.texture_count;

        initialize();

        if (info.texture_count)
        {
            memcpy(m_texture_ids, info.texture_ids, info.texture_count * sizeof(id::id_type));
            texture::get_descriptor_indices(m_texture_ids, info.texture_count, m_descriptor_indices);
        }

        u32 shader_idx = 0;
        for (u32 i = 0; i < shader_type::count; ++i)
        {
            if (id::is_valid(info.shader_ids[i]))
            {
                m_shader_ids[shader_idx] = info.shader_ids[i];
                ++shader_idx;
            }
        }

        LASSERT(shader_idx == (u32) _mm_popcnt_u32(m_shader_flags));
    }

    DISABLE_COPY_AND_MOVE(d3d12_material_stream);

    [[nodiscard]] constexpr id::id_type*        texture_ids() const { return m_texture_ids; }
    [[nodiscard]] constexpr u32*                descriptor_indices() const { return m_descriptor_indices; }
    [[nodiscard]] constexpr id::id_type*        shader_ids() const { return m_shader_ids; }
    [[nodiscard]] constexpr id::id_type         root_signature_id() const { return m_root_signature_id; }
    [[nodiscard]] constexpr u32                 texture_count() const { return m_texture_count; }
    [[nodiscard]] constexpr material_type::type material_type() const { return m_type; }
    [[nodiscard]] constexpr shader_flags::flags shader_flags() const { return m_shader_flags; }

private:
    void initialize()
    {
        LASSERT(m_buffer);
        u8* const buffer = m_buffer;

        m_type              = *(material_type::type*) buffer;
        m_shader_flags      = *(shader_flags::flags*) &buffer[shader_flags_index];
        m_root_signature_id = *(id::id_type*) &buffer[root_signature_index];
        m_texture_count     = *(u32*) &buffer[texture_count_index];
        m_shader_ids        = (id::id_type*) &buffer[shader_ids_index];

        // _mm_popcnt_u32 is an intel intrinsic function
        // asm instruction: popcnt r32, r32
        // Desc: Count the number of bits set to 1 in unsigned 32-bit integer a, and return that count in dst.
        m_texture_ids        = m_texture_count ? &m_shader_ids[_mm_popcnt_u32(m_shader_flags)] : nullptr;
        m_descriptor_indices = m_texture_count ? &m_texture_ids[m_texture_count] : nullptr;
    }

    constexpr static u32 shader_flags_index   = sizeof(material_type::type);
    constexpr static u32 root_signature_index = shader_flags_index + sizeof(shader_flags::flags);
    constexpr static u32 texture_count_index  = root_signature_index + sizeof(id::id_type);
    constexpr static u32 shader_ids_index     = texture_count_index + sizeof(u32);

    u8*                 m_buffer{};
    id::id_type*        m_texture_ids{};
    u32*                m_descriptor_indices{};
    id::id_type*        m_shader_ids{};
    id::id_type         m_root_signature_id{};
    u32                 m_texture_count{};
    material_type::type m_type{};
    shader_flags::flags m_shader_flags{};
};


D3D_PRIMITIVE_TOPOLOGY get_d3d_primitive_topology(const primitive_topology::type type)
{
    using namespace lotus::content;
    LASSERT(type < primitive_topology::count);
    switch (type)
    {
    case primitive_topology::point_list: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case primitive_topology::line_list: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case primitive_topology::line_strip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case primitive_topology::triangle_list: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case primitive_topology::triangle_strip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

constexpr D3D12_ROOT_SIGNATURE_FLAGS get_root_signature_flags(shader_flags::flags flags)
{
    D3D12_ROOT_SIGNATURE_FLAGS default_flags = d3dx::d3d12_root_signature_desc::default_flags;

    if (flags & shader_flags::vertex)           default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
    if (flags & shader_flags::hull)             default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
    if (flags & shader_flags::domain)           default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
    if (flags & shader_flags::geometry)         default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    if (flags & shader_flags::pixel)            default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
    if (flags & shader_flags::amplification)    default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
    if (flags & shader_flags::mesh)             default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

    return default_flags;
}

id::id_type create_root_signature(material_type::type type, shader_flags::flags flags)
{
    LASSERT(type < material_type::count);
    static_assert(sizeof(type) == sizeof(u32) && sizeof(flags) == sizeof(u32));

    const u64 key = ((u64)type << 32) | flags;
    auto pair = mtl_rs_map.find(key);
    if(pair != mtl_rs_map.end())
    {
        LASSERT(pair->first == key);
        return pair->second;
    }

    ID3D12RootSignature* root_sig = nullptr;

    switch(type)
    {
    case material_type::opaque:
    {
        using params = gpass::opaque_root_parameter;
        d3dx::d3d12_root_parameter parameters[params::count]{};
        parameters[params::per_frame_data].as_cbv(D3D12_SHADER_VISIBILITY_ALL, 0);

        D3D12_SHADER_VISIBILITY buffer_visibility{};
        D3D12_SHADER_VISIBILITY data_visibility{};

        if(flags & shader_flags::vertex)
        {
            buffer_visibility = D3D12_SHADER_VISIBILITY_VERTEX;
            data_visibility   = D3D12_SHADER_VISIBILITY_VERTEX;
        } else if(flags & shader_flags::mesh)
        {
            buffer_visibility = D3D12_SHADER_VISIBILITY_MESH;
            data_visibility   = D3D12_SHADER_VISIBILITY_MESH;
        }
        if((flags & shader_flags::hull) || (flags & shader_flags::geometry) || (flags & shader_flags::amplification))
        {
            buffer_visibility = D3D12_SHADER_VISIBILITY_ALL;
            data_visibility   = D3D12_SHADER_VISIBILITY_ALL;
        }

        if((flags & shader_flags::pixel) || (flags & shader_flags::compute))
        {
            data_visibility = D3D12_SHADER_VISIBILITY_ALL;
        }

        parameters[params::position_buffer].as_srv(buffer_visibility, 0);
        parameters[params::element_buffer].as_srv(buffer_visibility, 1);
        parameters[params::srv_indices].as_srv(D3D12_SHADER_VISIBILITY_PIXEL, 2); // TODO: Make visible to any stage that needs to sample textures
        parameters[params::per_object_data].as_cbv(data_visibility, 1);

        root_sig = d3dx::d3d12_root_signature_desc{&parameters[0], params::count, get_root_signature_flags(flags)}.create();
    }break;
    }

    LASSERT(root_sig);

    const id::id_type id = (id::id_type)root_signatures.size();
    root_signatures.emplace_back(root_sig);
    mtl_rs_map[key] = id;
    NAME_D3D_OBJ_INDEXED(root_sig, key, L"GPass Root Signature - Key");

    return id;
}

} // anonymous namespace

bool initialize()
{
    return true;
}

void shutdown()
{
    for(auto& item : root_signatures)
    {
        core::release(item);
    }
    mtl_rs_map.clear();
    root_signatures.clear();
}

namespace submesh
{

/**
 * \brief
 * Data should contain the following format:
 *
 * u32 element_size, u32 vertex_count,
 *
 * u32 index_count, u32 elements_type, u32 primitive_topology,
 *
 * u8 positions[sizeof(f32) * 3 * vertex_count],
 *
 * u8 elements[sizeof(element_size) * vertex_count],
 *
 * u8 indices[index_size * index_count]
 *
 * NOTE:
 * - This will advance the data pointer
 *
 * - Position and element buffers must be padded as a multiple of 4 bytes
 *
 * - - Defined as D3D12_STANDARD_MAXIMUM_ELEMENT_ALIGNMENT_BYTE_MULTIPLE
 */
id::id_type add(const u8*& data)
{
    utl::blob_stream_reader blob{ data };

    const u32 element_size       = blob.read<u32>();
    const u32 vertex_count       = blob.read<u32>();
    const u32 index_count        = blob.read<u32>();
    const u32 elements_type      = blob.read<u32>();
    const u32 primitive_topology = blob.read<u32>();
    const u32 index_size         = vertex_count < (1 << 16) ? sizeof(u16) : sizeof(u32);

    const u32 pos_buffer_size   = sizeof(vec3) * vertex_count;
    const u32 elem_buffer_size  = element_size * vertex_count;
    const u32 index_buffer_size = index_size * index_count;

    constexpr u32 alignment                = D3D12_STANDARD_MAXIMUM_ELEMENT_ALIGNMENT_BYTE_MULTIPLE;
    const u32     aligned_pos_buffer_size  = (u32) math::align_size_up<alignment>(pos_buffer_size);
    const u32     aligned_elem_buffer_size = (u32) math::align_size_up<alignment>(elem_buffer_size);

    const u32 total_buffer_size = aligned_pos_buffer_size + aligned_elem_buffer_size + index_buffer_size;

    ID3D12Resource* res = d3dx::create_buffer(blob.position(), total_buffer_size);

    blob.skip(total_buffer_size);
    data = blob.position();

    submesh_view view{};
    view.position_buffer_view.BufferLocation = res->GetGPUVirtualAddress();
    view.position_buffer_view.SizeInBytes    = pos_buffer_size;
    view.position_buffer_view.StrideInBytes  = sizeof(vec3);

    if (element_size)
    {
        view.element_buffer_view.BufferLocation = res->GetGPUVirtualAddress() + aligned_pos_buffer_size;
        view.element_buffer_view.SizeInBytes    = elem_buffer_size;
        view.element_buffer_view.StrideInBytes  = element_size;
    }

    view.index_buffer_view.BufferLocation = res->GetGPUVirtualAddress() + aligned_pos_buffer_size + aligned_elem_buffer_size;
    view.index_buffer_view.Format         = index_size == sizeof(u16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    view.index_buffer_view.SizeInBytes    = index_buffer_size;


    view.primitive_topology = get_d3d_primitive_topology((primitive_topology::type) primitive_topology);
    view.element_type       = elements_type;


    std::lock_guard lock(submesh_mutex);

    submesh_buffers.add(res);
    return submesh_views.add(view);
}

void remove(id::id_type id)
{
    std::lock_guard lock(submesh_mutex);

    submesh_views.remove(id);

    core::deferred_release(submesh_buffers[id]);
    submesh_buffers.remove(id);
}

} // namespace submesh


namespace texture
{
void get_descriptor_indices(const id::id_type* const texture_ids, u32 id_count, u32* const indices)
{
    LASSERT(texture_ids && id_count && indices);

    std::lock_guard lock(texture_mutex);

    for (u32 i = 0; i < id_count; ++i)
    {
        indices[i] = textures[i].srv().index;
    }
}
} // namespace texture


namespace material
{

// Output:
// struct {
//      material_type::type type,
//      shader_flags::flags flags,
//      id::id_type root_signature_id,
//      u32 texture_count,
//      id::id_type shader_ids[shader_count],
//      id::id_type texture_ids[texture_count],
//      u32* descriptor_indices[texture_count]
// } d3d12_material
id::id_type add(material_init_info info)
{
    scope<u8[]>     buffer;
    std::lock_guard lock(material_mutex);

    d3d12_material_stream stream(buffer, info);
    LASSERT(buffer);
    return materials.add(std::move(buffer));
}

void remove(id::id_type id)
{
    std::lock_guard lock(material_mutex);
    materials.remove(id);
}

} // namespace material


} // namespace lotus::graphics::d3d12::content
