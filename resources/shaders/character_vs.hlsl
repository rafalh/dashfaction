struct VsInput
{
    float3 pos : POSITION;
    float2 uv0 : TEXCOORD0;
    float4 color : COLOR0;
    float4 weights : COLOR1;
    float4 indices : COLOR2;
};

cbuffer VsConstantBuffer : register(b0)
{
    float4x4 model_mat;
    float4x4 view_mat;
    float4x4 proj_mat;
};

cbuffer BoneMatricesConstantBuffer : register(b2)
{
    float4x4 matrices[50];
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
    float4 pos_h = float4(input.pos.xyz, 1.0f);
    float4 transformed =
        mul(matrices[input.indices[0] * 255.0f], pos_h) * input.weights[0] +
        mul(matrices[input.indices[1] * 255.0f], pos_h) * input.weights[1] +
        mul(matrices[input.indices[2] * 255.0f], pos_h) * input.weights[2] +
        mul(matrices[input.indices[3] * 255.0f], pos_h) * input.weights[3];
    output.pos = mul(proj_mat, mul(view_mat, mul(model_mat, transformed)));
    output.uv0 = input.uv0;
    output.uv1 = float2(0, 0);
    output.color = input.color;
    return output;
}
