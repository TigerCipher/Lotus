// Testing purposes only

struct GlobalShaderData
{
    float4x4 View;
    float4x4 Projection;
    float4x4 InvProjection;
    float4x4 ViewProjection;
    float4x4 InvViewProjection;


    float3 CameraPosition;
    float ViewWidth;

    float3 CameraDirection;
    float viewHeight;

    float DeltaTime;
};

struct PerObjectData
{
    float4x4 World;
    float4x4 InvWorld;
    float4x4 WorldViewProjection;
};

struct VertexOut
{
    float4 HomogeneousPosition : SV_POSITION;
    float3 WorldPosition : POSITION;
    float3 WorldNormal : NORMAL;
    float3 WorldTangent : TANGENT;
    float2 UV : TEXTURE;
};

struct PixelOut
{
    float4 Color : SV_TARGET0;
};

ConstantBuffer<GlobalShaderData> PerFrameBuffer : register(b0, space0);
ConstantBuffer<PerObjectData> PerObjectBuffer : register(b1, space0);
StructuredBuffer<float3> VertexPositions : register(t0, space0);

VertexOut TestShaderVS(in uint VertexIdx: SV_VertexID)
{
    VertexOut vsOut;

    vsOut.HomogeneousPosition = 0.0f;
    vsOut.WorldPosition = 0.0f;
    vsOut.WorldNormal = 0.0f;
    vsOut.WorldTangent = 0.0f;
    vsOut.UV = 0.0f;

    return vsOut;
}

[earlydepthstencil]
PixelOut TestShaderPS(in VertexOut psIn)
{
    PixelOut psOut;
    psOut.Color = 0.0f;

    return psOut;
}