struct VsOutput
{
    float4 pos : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 color : COLOR;
    float depth : TEXCOORD2;
};

cbuffer PsConstantBuffer : register(b0)
{
    float4 vcolor_mul;
    float4 vcolor_mul_inv;
    float2 tex0_mul;
    float2 tex0_mul_inv;
    float alpha_test;
    float tex0_add_rgb;
    float tex1_mul_rgb;
    float tex1_mul_rgb_inv;
    float tex1_add_rgb;
    float output_add_rgb;
    float fog_far;
    float3 fog_color;
};

Texture2D tex0;
Texture2D tex1;
SamplerState samp0;
SamplerState samp1;

float4 main(VsOutput input) : SV_TARGET
{
    // return tex0.Sample(samp0, input.uv0);
    float4 tex0_color = tex0.Sample(samp0, input.uv0);
    float4 target = input.color * vcolor_mul.rgba + vcolor_mul_inv.rgba;
    target *= tex0_color * tex0_mul.xxxy + tex0_mul_inv.xxxy;

    clip(target.a - alpha_test);

    target.rgb += tex0_color.rgb * tex0_add_rgb;

    float4 tex1_color = tex1.Sample(samp1, input.uv1);
    target.rgb *= tex1_color.rgb * tex1_mul_rgb + tex1_mul_rgb_inv;
    target.rgb += tex1_color.rgb * tex1_add_rgb;
    target.rgb += output_add_rgb;

    float fog = clamp(input.depth / fog_far, 0, 1);
    target.rgb = fog * fog_color + (1 - fog) * target.rgb;

    return target;
}
