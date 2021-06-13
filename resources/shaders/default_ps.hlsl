struct VsOutput
{
    float4 pos : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 color : COLOR;
};

cbuffer PsConstantBuffer : register(b0)
{
    float2 vcolor_mul;
    float2 vcolor_mul_inv;
    float2 tex0_mul;
    float2 tex0_mul_inv;
    float tex0_add_rgb;
    float output_mul_rgb;
    float alpha_test;
};

Texture2D tex0;
Texture2D tex1;
SamplerState samp0;
SamplerState samp1;

float4 main(VsOutput input) : SV_TARGET
{
    //return tex0.Sample(samp0, input.uv0);

    float4 tex0_color = tex0.Sample(samp0, input.uv0);
    float4 target = input.color * vcolor_mul.xxxy + vcolor_mul_inv.xxxy;
    target *= (tex0_color * tex0_mul.xxxy) + tex0_mul_inv.xxxy;
    target.rgb += tex0_color.rgb * tex0_add_rgb;

    // TODO: make separate shader with alpha-test
    if (alpha_test - target.a > 0.9)
        discard;

    float4 tex1_color = tex1.Sample(samp1, input.uv1);
    target.rgb *= tex1_color.rgb;
    target.rgb *= output_mul_rgb;
    return target;

    // float4 tex0_color;
    // if (ts == TEXTURE_SOURCE_NONE) {
    //     tex0_color = float4(1, 1, 1, 1);
    // } else {
    //     tex0_color = tex0.Sample(samp0, input.uv0);
    // }

    // float4 target;
    // if (cs == COLOR_SOURCE_VERTEX && ts != TEXTURE_SOURCE_CLAMP) { // hack
    //     target.rgb = input.color.rgb;
    // }
    // else if (cs == COLOR_SOURCE_TEXTURE && ts == TEXTURE_SOURCE_CLAMP) {
    //     target.rgb = tex0_color.rgb;
    // }
    // else if (cs == COLOR_SOURCE_VERTEX_TIMES_TEXTURE) {
    //     target.rgb = input.color.rgb * tex0_color.rgb;
    // }
    // else if (cs == COLOR_SOURCE_VERTEX_PLUS_TEXTURE) {
    //     target.rgb = input.color.rgb + tex0_color.rgb;
    // }
    // else if (cs == COLOR_SOURCE_VERTEX_TIMES_TEXTURE_2X) {
    //     target.rgb = input.color.rgb * tex0_color.rgb;
    // }
    // if (as == ALPHA_SOURCE_VERTEX) {
    //     target.a = input.color.a;
    // }
    // else if (as == ALPHA_SOURCE_VERTEX_NONDARKENING) {
    //     target.a = input.color.a;
    // }
    // else if (as == ALPHA_SOURCE_TEXTURE) {
    //     target.a = tex0_color.a;
    // }
    // else if (as == ALPHA_SOURCE_VERTEX_TIMES_TEXTURE) {
    //     target.a = input.color.a * tex0_color.a;
    // }
    // if (ts >= 4) {
    //     target.rgb *= tex1.Sample(samp1, input.uv1).rgb;
    // }
    // if (ts == TEXTURE_SOURCE_CLAMP_1_WRAP_0_MOD2X) {
    //     target.rgb *= 2;
    // }
    // if (alpha_test == 1 && target.a < 0.1)
    //     discard;
    // return target;
}
