struct VsInput
{
    float4 pos : POSITIONT;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
};

struct VsOutput
{
    float4 pos : SV_POSITION;
    float3 norm : NORMAL;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 world_pos_and_depth : TEXCOORD2;
};

VsOutput main(VsInput input)
{
    VsOutput output;
    float w = input.pos.w;
    output.pos = input.pos;
    output.norm = float3(0, 0, 0); // dummy normal
    output.uv0 = input.uv0;
    output.uv1 = float2(0, 0);
    output.color = input.color;
    output.world_pos_and_depth = float4(0, 0, 0, w);
    return output;
}
