// Testing purposes only

#include "Common.hlsli"


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

#define ElementsTypeStaticNormal                0x01
#define ElementsTypeStaticNormalTexture         0x03
#define ElementsTypeStaticColor                 0x04
#define ElementsTypeSkeletal                    0x08
#define ElementsTypeSkeletalColor               ElementsTypeSkeletal | ElementsTypeStaticColor
#define ElementsTypeSkeletalNormal              ElementsTypeSkeletal | ElementsTypeStaticNormal
#define ElementsTypeSkeletalNormalColor         ElementsTypeSkeletalNormal | ElementsTypeStaticColor
#define ElementsTypeSkeletalNormalTexture       ElementsTypeSkeletal | ElementsTypeStaticNormalTexture
#define ElementsTypeSkeletalNormalTextureColor  ElementsTypeSkeletalNormalTexture | ElementsTypeStaticColor

struct VertexElement
{
#if ELEMENTS_TYPE == ElementsTypeStaticNormal       
    uint ColorTSign;
    uint16_t2 Normal;
#elif ELEMENTS_TYPE == ElementsTypeStaticNormalTexture 
    uint ColorTSign;
    uint16_t2 Normal;
    uint16_t2 Tangent;
    float2 UV;
#elif ELEMENTS_TYPE == ElementsTypeStaticColor                 
#elif ELEMENTS_TYPE == ElementsTypeSkeletal                    
#elif ELEMENTS_TYPE == ElementsTypeSkeletalColor               
#elif ELEMENTS_TYPE == ElementsTypeSkeletalNormal              
#elif ELEMENTS_TYPE == ElementsTypeSkeletalNormalColor         
#elif ELEMENTS_TYPE == ElementsTypeSkeletalNormalTexture       
#elif ELEMENTS_TYPE == ElementsTypeSkeletalNormalTextureColor  
#endif
};

const static float InvIntervals = 2.0f / ((1 << 16) - 1);

ConstantBuffer<GlobalShaderData> GlobalData : register(b0, space0);
ConstantBuffer<PerObjectData> PerObjectBuffer : register(b1, space0);
StructuredBuffer<float3> VertexPositions : register(t0, space0);
StructuredBuffer<VertexElement> Elements : register(t1, space0);

VertexOut TestShaderVS(in uint VertexIdx : SV_VertexID)
{
    VertexOut vsOut;

    float4 position = float4(VertexPositions[VertexIdx], 1.0f);
    float4 worldPos = mul(PerObjectBuffer.World, position);

#if ELEMENT_TYPE == ElementsTypeStaticNormal
    VertexElement element = Elements[VertexIdx];
    float2 nXY = element.Normal * InvIntervals - 1.0f;
    float signs = (element.ColorTSign >> 24) & 0xff;

    float nsign = float(signs & 0x02) - 1;
    float3 normal = float3(nXY.x, nXY.y, sqrt(saturate(1.0f - dot(nXY, nXY))) * nsign);

    vsOut.HomogeneousPosition = mul(PerObjectBuffer.WorldViewProjection, position);
    vsOut.WorldPosition = worldPos.xyz;
    vsOut.WorldNormal = mul(float4(normal, 0.0f), PerObjectBuffer.InvWorld).xyz;
    vsOut.WorldTangent = 0.0f;
    vsOut.UV = 0.0f;
#elif ELEMENT_TYPE == ElementsTypeStaticNormalTexture
    VertexElement element = Elements[VertexIdx];
    float2 nXY = element.Normal * InvIntervals - 1.0f;
    float signs = (element.ColorTSign >> 24) & 0xff;

    float nsign = float(signs & 0x02) - 1;
    float3 normal = float3(nXY.x, nXY.y, sqrt(saturate(1.0f - dot(nXY, nXY))) * nsign);

    vsOut.HomogeneousPosition = mul(PerObjectBuffer.WorldViewProjection, position);
    vsOut.WorldPosition = worldPos.xyz;
    vsOut.WorldNormal = mul(float4(normal, 0.0f), PerObjectBuffer.InvWorld).xyz;
    vsOut.WorldTangent = 0.0f;
    vsOut.UV = 0.0f;
#else
    #undef ELEMENTS_TYPE
    vsOut.HomogeneousPosition = mul(PerObjectBuffer.WorldViewProjection, position);
    vsOut.WorldPosition = worldPos.xyz;
    vsOut.WorldNormal = 0.0f;
    vsOut.WorldTangent = 0.0f;
    vsOut.UV = 0.0f;
#endif

    return vsOut;
}

[earlydepthstencil]
PixelOut TestShaderPS(in VertexOut psIn)
{
    PixelOut psOut;
    psOut.Color = float4(psIn.WorldNormal, 1.f);

    return psOut;
}