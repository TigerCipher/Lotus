

#if !defined(LOTUS_COMMON_HLSLI) && !defined(__cplusplus)
#error Do not include this header in shader files. Only include it with Common.hlsli
#endif

struct GlobalShaderData
{
    float4x4 View;
    float4x4 Projection;
    float4x4 InvProjection;
    float4x4 ViewProjection;
    float4x4 InvViewProjection;


    float3 CameraPosition;
    float  ViewWidth;

    float3 CameraDirection;
    float  ViewHeight;

    uint NumDirectionalLights;
    
    float DeltaTime;
};

struct PerObjectData
{
    float4x4 World;
    float4x4 InvWorld;
    float4x4 WorldViewProjection;
};

struct DirectionalLightParameters
{
    float3 Direction;
    float Intensity;
    
    float3 Color;
    float _pad;
};

#ifdef __cplusplus
static_assert((sizeof(PerObjectData) % 16) == 0, "Make sure PerObjectData is formatted in 16-byte chunks with no implicit padding");
static_assert((sizeof(DirectionalLightParameters) % 16) == 0, "Make sure DirectionalLightParameters is formatted in 16-byte chunks with no implicit padding");
#endif