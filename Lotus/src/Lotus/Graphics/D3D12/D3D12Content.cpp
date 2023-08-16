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

struct pso_id
{
    id::id_type gpass = id::invalid_id;
    id::id_type depth = id::invalid_id;
};

struct submesh_view
{
    D3D12_VERTEX_BUFFER_VIEW position_buffer_view{};
    D3D12_VERTEX_BUFFER_VIEW element_buffer_view{};
    D3D12_INDEX_BUFFER_VIEW  index_buffer_view{};
    D3D_PRIMITIVE_TOPOLOGY   primitive_topology{};
    u32                      element_type{};
};

struct d3d12_render_item
{
    id::id_type entity_id;
    id::id_type submesh_gpu_id;
    id::id_type material_id;
    id::id_type pso_id;
    id::id_type depth_pso_id;
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

utl::free_list<d3d12_render_item>    render_items{};
utl::free_list<scope<id::id_type[]>> render_item_ids{};
std::mutex                           render_item_mutex{};

utl::vector<ID3D12PipelineState*>    pipeline_states;
std::unordered_map<u64, id::id_type> pso_map;
std::mutex                           pso_mutex{};


struct
{
    utl::vector<lotus::content::lod_offset> lod_offsets{};
    utl::vector<id::id_type>                geometry_ids{};
} frame_cache;

id::id_type create_root_signature(material_type::type type, shader_flags::flags flags);

class d3d12_material_stream
{
public:
    explicit d3d12_material_stream(u8* const material_buffer) : m_buffer(material_buffer) { initialize(); }

    explicit d3d12_material_stream(scope<u8[]>& material_buffer, material_init_info info)
    {
        assert(!material_buffer);

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

        assert(shader_count && flags);

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

        assert(shader_idx == (u32) _mm_popcnt_u32(m_shader_flags));
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
        assert(m_buffer);
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


constexpr D3D_PRIMITIVE_TOPOLOGY get_d3d_primitive_topology(const primitive_topology::type type)
{
    using namespace lotus::content;
    assert(type < primitive_topology::count);
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

constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE get_d3d_primitive_topology_type(D3D_PRIMITIVE_TOPOLOGY topology)
{
    switch (topology)
    {
    case D3D_PRIMITIVE_TOPOLOGY_POINTLIST: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
    case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    default: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    }
}

constexpr D3D12_ROOT_SIGNATURE_FLAGS get_root_signature_flags(shader_flags::flags flags)
{
    D3D12_ROOT_SIGNATURE_FLAGS default_flags = d3dx::d3d12_root_signature_desc::default_flags;

    if (flags & shader_flags::vertex)
        default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
    if (flags & shader_flags::hull)
        default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
    if (flags & shader_flags::domain)
        default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
    if (flags & shader_flags::geometry)
        default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    if (flags & shader_flags::pixel)
        default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
    if (flags & shader_flags::amplification)
        default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
    if (flags & shader_flags::mesh)
        default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

    return default_flags;
}

id::id_type create_root_signature(material_type::type type, shader_flags::flags flags)
{
    assert(type < material_type::count);
    static_assert(sizeof(type) == sizeof(u32) && sizeof(flags) == sizeof(u32));

    const u64 key = ((u64) type << 32) | flags;
    if (const auto pair = mtl_rs_map.find(key); pair != mtl_rs_map.end())
    {
        assert(pair->first == key);
        return pair->second;
    }

    ID3D12RootSignature* root_sig = nullptr;

    switch (type)
    {
    case material_type::opaque:
    {
        using params = gpass::opaque_root_parameter;
        d3dx::d3d12_root_parameter parameters[params::count]{};
        parameters[params::global_shader_data].as_cbv(D3D12_SHADER_VISIBILITY_ALL, 0);

        D3D12_SHADER_VISIBILITY buffer_visibility{};
        D3D12_SHADER_VISIBILITY data_visibility{};

        if (flags & shader_flags::vertex)
        {
            buffer_visibility = D3D12_SHADER_VISIBILITY_VERTEX;
            data_visibility   = D3D12_SHADER_VISIBILITY_VERTEX;
        } else if (flags & shader_flags::mesh)
        {
            buffer_visibility = D3D12_SHADER_VISIBILITY_MESH;
            data_visibility   = D3D12_SHADER_VISIBILITY_MESH;
        }
        if ((flags & shader_flags::hull) || (flags & shader_flags::geometry) || (flags & shader_flags::amplification))
        {
            buffer_visibility = D3D12_SHADER_VISIBILITY_ALL;
            data_visibility   = D3D12_SHADER_VISIBILITY_ALL;
        }

        if ((flags & shader_flags::pixel) || (flags & shader_flags::compute))
        {
            data_visibility = D3D12_SHADER_VISIBILITY_ALL;
        }

        parameters[params::position_buffer].as_srv(buffer_visibility, 0);
        parameters[params::element_buffer].as_srv(buffer_visibility, 1);
        // TODO: Make visible to any stage that needs to sample textures
        parameters[params::srv_indices].as_srv(D3D12_SHADER_VISIBILITY_PIXEL, 2);
        parameters[params::per_object_data].as_cbv(data_visibility, 1);

        root_sig = d3dx::d3d12_root_signature_desc{ &parameters[0], params::count, get_root_signature_flags(flags) }.create();
    }
    break;
    }

    assert(root_sig);

    const auto id = (id::id_type) root_signatures.size();
    root_signatures.emplace_back(root_sig);
    mtl_rs_map[key] = id;
    NAME_D3D_OBJ_INDEXED(root_sig, key, L"GPass Root Signature - Key");

    return id;
}

id::id_type create_pso_if_needed(const u8* const stream_ptr, u64 aligned_stream_size, bool is_depth)
{
    const u64 key = math::calculate_crc32_u64(stream_ptr, aligned_stream_size);

    {
        std::lock_guard lock{ pso_mutex };
        if (const auto pair = pso_map.find(key); pair != pso_map.end())
        {
            assert(pair->first == key);
            return pair->second;
        }
    }

    auto const           stream = (d3dx::d3d12_pipeline_state_subobject_stream* const) stream_ptr;
    ID3D12PipelineState* pso{ d3dx::create_pipeline_state(stream, sizeof(d3dx::d3d12_pipeline_state_subobject_stream)) };

    {
        std::lock_guard lock{ pso_mutex };
        const auto      id = (u32) pipeline_states.size();
        pipeline_states.emplace_back(pso);
        NAME_D3D_OBJ_INDEXED(pipeline_states.back(), key,
                             is_depth ? L"Depth-Only Pipeline State Object - Key" : L"GPass Pipeline State Object - Key");

        assert(id::is_valid(id));
        pso_map[key] = id;
        return id;
    }
}

#pragma intrinsic(_BitScanForward)
shader_type::type get_shader_type(u32 flag)
{
    assert(flag);
    ulong index;
    _BitScanForward(&index, flag);
    return (shader_type::type) index;
}

pso_id create_pso(id::id_type material_id, D3D12_PRIMITIVE_TOPOLOGY primitive_topology, [[maybe_unused]] u32 elements_type)
{
    constexpr u64 aligned_stream_size = math::align_size_up<sizeof(u64)>(sizeof(d3dx::d3d12_pipeline_state_subobject_stream));
    auto const    stream_ptr          = (u8* const) alloca(aligned_stream_size);
    ZeroMemory(stream_ptr, aligned_stream_size);
    new (stream_ptr) d3dx::d3d12_pipeline_state_subobject_stream{};

    d3dx::d3d12_pipeline_state_subobject_stream& stream = *(d3dx::d3d12_pipeline_state_subobject_stream* const) stream_ptr;

    {
        std::lock_guard             lock(material_mutex);
        const d3d12_material_stream material(materials[material_id].get());

        D3D12_RT_FORMAT_ARRAY rt_array{};
        rt_array.NumRenderTargets = 1;
        rt_array.RTFormats[0]     = gpass::main_buffer_format;

        stream.render_target_formats = rt_array;
        stream.root_signature        = root_signatures[material.root_signature_id()];
        stream.primitive_topology    = get_d3d_primitive_topology_type(primitive_topology);
        stream.depth_stencil_format  = gpass::depth_buffer_format;
        stream.rasterizer            = d3dx::rasterizer_state.backface_cull;
        stream.depth_stencil1        = d3dx::depth_state.reversed_readonly;
        stream.blend                 = d3dx::blend_state.disabled;

        const shader_flags::flags flags = material.shader_flags();
        D3D12_SHADER_BYTECODE     shaders[shader_type::count]{};
        u32                       shader_idx = 0;
        for (u32 i = 0; i < shader_type::count; ++i)
        {
            if (flags & (1 << i))
            {
                const u32 key{ get_shader_type(flags & (1 << i)) == shader_type::vertex ? elements_type : invalid_id_u32 };
                const lotus::content::compiled_shader_ptr shader{ lotus::content::get_shader(material.shader_ids()[shader_idx], key) };
                assert(shader);
                shaders[i].pShaderBytecode = shader->byte_code();
                shaders[i].BytecodeLength  = shader->byte_code_size();
                ++shader_idx;
            }
        }

        stream.vs = shaders[shader_type::vertex];
        stream.ps = shaders[shader_type::pixel];
        stream.ds = shaders[shader_type::domain];
        stream.hs = shaders[shader_type::hull];
        stream.gs = shaders[shader_type::geometry];
        stream.cs = shaders[shader_type::compute];
        stream.as = shaders[shader_type::amplification];
        stream.ms = shaders[shader_type::mesh];
    }

    pso_id pair{};
    pair.gpass = create_pso_if_needed(stream_ptr, aligned_stream_size, false);

    stream.ps             = D3D12_SHADER_BYTECODE{};
    stream.depth_stencil1 = d3dx::depth_state.reversed;
    pair.depth            = create_pso_if_needed(stream_ptr, aligned_stream_size, true);


    return pair;
}

} // anonymous namespace

bool initialize()
{
    return true;
}

void shutdown()
{
    for (auto& item : root_signatures)
    {
        core::release(item);
    }
    mtl_rs_map.clear();
    root_signatures.clear();

    for (auto& item : pipeline_states)
    {
        core::release(item);
    }
    pso_map.clear();
    pipeline_states.clear();
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

void get_views(const id::id_type* const gpu_ids, u32 id_count, const views_cache& cache)
{
    assert(gpu_ids && id_count);
    assert(cache.position_buffers && cache.element_buffers && cache.index_buffer_views && cache.primitive_topologies &&
           cache.elements_types);

    std::lock_guard lock(submesh_mutex);

    for (u32 i = 0; i < id_count; ++i)
    {
        const submesh_view& view      = submesh_views[gpu_ids[i]];
        cache.position_buffers[i]     = view.position_buffer_view.BufferLocation;
        cache.element_buffers[i]      = view.element_buffer_view.BufferLocation;
        cache.index_buffer_views[i]   = view.index_buffer_view;
        cache.primitive_topologies[i] = view.primitive_topology;
        cache.elements_types[i]       = view.element_type;
    }
}

} // namespace submesh


namespace texture
{
void get_descriptor_indices(const id::id_type* const texture_ids, u32 id_count, u32* const indices)
{
    assert(texture_ids && id_count && indices);

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
    assert(buffer);
    return materials.add(std::move(buffer));
}

void remove(id::id_type id)
{
    std::lock_guard lock(material_mutex);
    materials.remove(id);
}

void get_materials(const id::id_type* const material_ids, u32 material_count, const materials_cache& cache)
{
    assert(material_ids && material_count);
    assert(cache.root_signatures && cache.material_types);
    std::lock_guard lock(material_mutex);

    for (u32 i = 0; i < material_count; ++i)
    {
        const d3d12_material_stream stream(materials[material_ids[i]].get());
        cache.root_signatures[i] = root_signatures[stream.root_signature_id()];
        cache.material_types[i]  = stream.material_type();
    }
}

} // namespace material


namespace render_item
{

// Format:
// buffer[0] = geometry_content_id
// buffer[1 .. n] = d3d12_render_item_ids -> n = number of low level render item ids which must == the number of submeshes/material ids
// buffer[n+1] = id::invalid_id
id::id_type add(id::id_type entity_id, id::id_type geometry_content_id, u32 material_count, const id::id_type* const material_ids)
{
    assert(id::is_valid(entity_id) && id::is_valid(geometry_content_id));
    assert(material_count && material_ids);
    const auto gpu_ids = (id::id_type* const) alloca(material_count * id::size);
    lotus::content::get_submesh_gpu_ids(geometry_content_id, material_count, gpu_ids);

    const submesh::views_cache views_cache{
        (D3D12_GPU_VIRTUAL_ADDRESS* const) alloca(material_count * sizeof(D3D12_GPU_VIRTUAL_ADDRESS)),
        (D3D12_GPU_VIRTUAL_ADDRESS* const) alloca(material_count * sizeof(D3D12_GPU_VIRTUAL_ADDRESS)),
        (D3D12_INDEX_BUFFER_VIEW* const) alloca(material_count * sizeof(D3D12_INDEX_BUFFER_VIEW)),
        (D3D_PRIMITIVE_TOPOLOGY* const) alloca(material_count * sizeof(D3D_PRIMITIVE_TOPOLOGY)),
        (u32* const) alloca(material_count * sizeof(u32)),
    };

    submesh::get_views(gpu_ids, material_count, views_cache);

    scope<id::id_type[]> items = create_scope<id::id_type[]>(id::size * (1 + (u64) material_count + 1));

    items[0]                    = geometry_content_id;
    id::id_type* const item_ids = &items[1];

    std::lock_guard lock(render_item_mutex);

    for (u32 i = 0; i < material_count; ++i)
    {
        d3d12_render_item item{};
        item.entity_id      = entity_id;
        item.submesh_gpu_id = gpu_ids[i];
        item.material_id    = material_ids[i];
        auto [gpass, depth] = create_pso(item.material_id, views_cache.primitive_topologies[i], views_cache.elements_types[i]);
        item.pso_id         = gpass;
        item.depth_pso_id   = depth;

        assert(id::is_valid(item.submesh_gpu_id) && id::is_valid(item.material_id));
        item_ids[i] = render_items.add(item);
    }

    // Mark end of ids list
    item_ids[material_count] = id::invalid_id;

    return render_item_ids.add(std::move(items));
}

void remove(id::id_type id)
{
    std::lock_guard          lock(render_item_mutex);
    const id::id_type* const item_ids = &render_item_ids[id][1];

    for (u32 i = 0; item_ids[i] != id::invalid_id; ++i)
    {
        render_items.remove(item_ids[i]);
    }

    render_item_ids.remove(id);
}

void get_d3d12_render_item_ids(const frame_info& info, utl::vector<id::id_type>& d3d12_render_item_ids)
{
    assert(info.render_item_ids && info.thresholds && info.render_item_count);
    assert(d3d12_render_item_ids.empty());

    frame_cache.lod_offsets.clear();
    frame_cache.geometry_ids.clear();
    const u32 count = info.render_item_count;

    std::lock_guard lock(render_item_mutex);

    for (u32 i = 0; i < count; ++i)
    {
        const id::id_type* const buffer = render_item_ids[info.render_item_ids[i]].get();
        frame_cache.geometry_ids.emplace_back(buffer[0]);
    }

    lotus::content::get_lod_offsets(frame_cache.geometry_ids.data(), info.thresholds, count, frame_cache.lod_offsets);
    assert(frame_cache.lod_offsets.size() == count);

    u32 d3d12_render_item_count = 0;
    for (u32 i = 0; i < count; ++i)
    {
        d3d12_render_item_count += frame_cache.lod_offsets[i].count;
    }

    assert(d3d12_render_item_count);
    d3d12_render_item_ids.resize(d3d12_render_item_count);

    u32 item_idx = 0;
    for (u32 i = 0; i < count; ++i)
    {
        const id::id_type* const          item_ids = &render_item_ids[info.render_item_ids[i]][1];
        const lotus::content::lod_offset& lod_offset{ frame_cache.lod_offsets[i] };
        memcpy(&d3d12_render_item_ids[item_idx], &item_ids[lod_offset.offset], id::size * lod_offset.count);
        item_idx += lod_offset.count;
        assert(item_idx <= d3d12_render_item_count);
    }

    assert(item_idx <= d3d12_render_item_count);
}

void get_items(const id::id_type* const d3d12_render_item_ids, u32 id_count, const items_cache& cache)
{
    assert(d3d12_render_item_ids && id_count);
    assert(cache.entity_ids && cache.submesh_gpu_ids && cache.material_ids && cache.psos && cache.depth_psos);


    std::lock_guard lock1(render_item_mutex);
    std::lock_guard lock2(pso_mutex);

    for (u32 i = 0; i < id_count; ++i)
    {
        const auto& [entity_id, submesh_gpu_id, material_id, pso_id, depth_pso_id] = render_items[d3d12_render_item_ids[i]];

        cache.entity_ids[i]      = entity_id;
        cache.submesh_gpu_ids[i] = submesh_gpu_id;
        cache.material_ids[i]    = material_id;
        cache.psos[i]            = pipeline_states[pso_id];
        cache.depth_psos[i]      = pipeline_states[depth_pso_id];
    }
}

} // namespace render_item


} // namespace lotus::graphics::d3d12::content
