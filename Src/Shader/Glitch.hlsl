Texture2D g_Tex : register(t0);
SamplerState g_Sam : register(s0);

cbuffer cb0 : register(b0)
{
    float4 g_Params; // x: Time, y: Intensity, z: 0, w: 0
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
    float2 UV : TEXCOORD0;
};

// 座標と時間からランダムなノイズを作る関数
float rand(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float time = g_Params.x;
    float intensity = g_Params.y;
    float2 uv = input.UV;

    // 画面を横のブロック状に切り裂いてズラす
    float noiseLine = rand(float2(time, floor(uv.y * 20.0f)));
    if (noiseLine > 0.9f)
    {
        uv.x += (rand(float2(time, uv.y)) - 0.5f) * 0.1f * intensity;
    }

    // RGBを強烈に引き裂く
    float split = 0.03f * intensity * rand(float2(time, 0.0f));
    float r = g_Tex.Sample(g_Sam, uv + float2(split, 0)).r;
    float g = g_Tex.Sample(g_Sam, uv).g;
    float b = g_Tex.Sample(g_Sam, uv - float2(split, 0)).b;

    // 一瞬だけ画面が白く発光するフラッシュノイズ
    float whiteNoise = rand(uv + time);
    if (whiteNoise > 0.98f && intensity > 0.5f)
    {
        r += 0.5f;
        g += 0.5f;
        b += 0.5f;
    }

    return float4(r, g, b, 1.0f);
}