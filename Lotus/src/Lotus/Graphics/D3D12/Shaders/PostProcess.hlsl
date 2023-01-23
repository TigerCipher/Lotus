
struct PostProcessConstants
{
    uint GPassMainBufferIndex;
};

ConstantBuffer<PostProcessConstants> ShaderParams : register(b1);
Texture2D Textures[] : register(t0, space0);

float4 PostProcessPS(in noperspective float4 Position : SV_Position, in noperspective float2 UV : TEXCOORD) : SV_Target0
{
    Texture2D gpassMain = Textures[ShaderParams.GPassMainBufferIndex];
    float4 color = float4(gpassMain[Position.xy].xyz, 1.0f);

    return color;
}