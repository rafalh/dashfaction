struct VsOutput
{
    float4 pos : SV_POSITION;
    float3 norm : NORMAL;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 world_pos_and_depth : TEXCOORD2;
};

cbuffer RenderModeBuffer : register(b0)
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

struct PointLight {
    float3 pos;
    float radius;
    float3 color;
};
cbuffer LightsBuffer : register(b1)
{
    float4 ambient_light;
    PointLight point_lights[4];
};

Texture2D tex0;
Texture2D tex1;
SamplerState samp0;
SamplerState samp1;

float4 main(VsOutput input) : SV_TARGET
{
    float4 tex0_color = tex0.Sample(samp0, input.uv0);
    float4 target = input.color * vcolor_mul.rgba + vcolor_mul_inv.rgba;
    target *= tex0_color * tex0_mul.xxxy + tex0_mul_inv.xxxy;

    clip(target.a - alpha_test);

    target.rgb += tex0_color.rgb * tex0_add_rgb;

    float3 light_color = tex1.Sample(samp1, input.uv1).rgb;
    for (int i = 0; i < 4; ++i) {
        float3 light_vec = point_lights[i].pos - input.world_pos_and_depth.xyz;
        float3 light_dir = normalize(light_vec);
        float dist = length(light_vec);
        float atten = saturate(dist / point_lights[i].radius);
        float intensity = (1 - atten) * saturate(dot(input.norm, light_dir));
        light_color += point_lights[i].color * intensity;
    }
    light_color = saturate(light_color);

    target.rgb *= light_color * tex1_mul_rgb + tex1_mul_rgb_inv;
    target.rgb += light_color * tex1_add_rgb;
    target.rgb += output_add_rgb;

    float fog = saturate(input.world_pos_and_depth.w / fog_far);
    target.rgb = fog * fog_color + (1 - fog) * target.rgb;

    return target;
}
