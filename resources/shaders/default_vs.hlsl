struct VsInput
{
    float3 pos : POSITION;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 color : COLOR;
};

cbuffer VsConstantBuffer : register(b0)
{
    float4x4 model_mat;
    float4x4 view_mat;
    float4x4 proj_mat;
};

cbuffer VsConstantBuffer2 : register(b1)
{
    float3x4 texture_mat;
};

struct VsOutput
{
    float4 pos : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 color : COLOR;
};

VsOutput main(VsInput input)
{
    VsOutput output;
    output.pos = mul(proj_mat, mul(view_mat, mul(model_mat, float4(input.pos.xyz, 1.0f))));
    output.uv0 = mul(texture_mat, float4(input.uv0, 1.0f, 0.0f)).xy;
    output.uv1 = input.uv1;
    output.color = input.color;
    return output;
}
