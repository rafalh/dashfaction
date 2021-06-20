struct VsInput
{
    float3 pos : POSITION;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 color : COLOR;
};

cbuffer ModelTransformBuffer : register(b0)
{
    float4x4 model_mat;
};

cbuffer ViewProjTransformBuffer : register(b1)
{
    float4x4 view_mat;
    float4x4 proj_mat;
};

cbuffer TextureTransformBuffer : register(b2)
{
    float3x4 texture_mat;
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
    float4 view_pos = mul(view_mat, mul(model_mat, float4(input.pos.xyz, 1.0f)));
    output.pos = mul(proj_mat, view_pos);
    output.uv0 = mul(texture_mat, float4(input.uv0, 1.0f, 0.0f)).xy;
    output.uv1 = input.uv1;
    output.color = input.color;
    output.depth = view_pos.z;
    return output;
}
