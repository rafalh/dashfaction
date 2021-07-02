struct VsInput
{
    float4 pos : POSITIONT;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 color : COLOR;
};

struct VsOutput
{
    float4 pos : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 color : COLOR;
    float depth : TEXCOORD2;
};

VsOutput main(VsInput input)
{
    VsOutput output;
    float w = input.pos.w;
    output.pos.xyz = input.pos.xyz * w;
    output.pos.w = w;
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;
    output.color = input.color;
    output.depth = w;
    return output;
}
