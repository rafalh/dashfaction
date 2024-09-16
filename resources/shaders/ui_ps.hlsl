struct VsOutput
{
    float4 pos : SV_POSITION;
    float3 norm : NORMAL;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 world_pos_and_depth : TEXCOORD2;
};

Texture2D tex0;
SamplerState samp0;

float4 main(VsOutput input) : SV_TARGET
{
    return tex0.Sample(samp0, input.uv0) * input.color;
}
