struct VsInput
{
    float3 pos : POSITION;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 color : COLOR;
};

cbuffer ModelTransformBuffer : register(b0)
{
    float4x3 world_mat;
};

cbuffer ViewProjTransformBuffer : register(b1)
{
    float4x3 view_mat;
    float4x4 proj_mat;
};

cbuffer UvOffsetBuffer : register(b2)
{
    float2 uv_offset;
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
    float3 world_pos = mul(float4(input.pos.xyz, 1), world_mat);
    float3 view_pos = mul(float4(world_pos, 1), view_mat);
    output.pos = mul(float4(view_pos, 1), proj_mat);
    output.uv0 = input.uv0 + uv_offset;
    output.uv1 = input.uv1;
    output.color = input.color;
    output.depth = view_pos.z;
    return output;
}
