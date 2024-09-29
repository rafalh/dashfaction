struct VsInput
{
    float3 pos : POSITION;
    float3 norm : NORMAL;
    float4 color : COLOR;
    float4 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
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

cbuffer PerFrameBuffer : register(b2)
{
    float time;
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
    float3 world_pos = mul(float4(input.pos.xyz, 1), world_mat);
    float3 view_pos = mul(float4(world_pos, 1), view_mat);
    output.pos = mul(float4(view_pos, 1), proj_mat);
    output.norm = input.norm;
    output.uv0 = input.uv0.xy + input.uv0.zw * time;
    output.uv1 = input.uv1;
    output.color = input.color;
    output.world_pos_and_depth = float4(world_pos, view_pos.z);
    return output;
}
