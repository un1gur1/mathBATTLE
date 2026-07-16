Texture2D g_Tex : register(t0);
SamplerState g_Sam : register(s0);

// C++の CreateShaderConstantBuffer で送られてくるデータ
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

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.UV;
    
    // ブラウン管モニターのような画面端の丸み（歪み）
    float2 dc = abs(0.5f - uv);
    dc *= dc;
    uv.x -= 0.5f;
    uv.x *= 1.0f + (dc.y * (0.2f * g_Params.y));
    uv.x += 0.5f;
    uv.y -= 0.5f;
    uv.y *= 1.0f + (dc.x * (0.2f * g_Params.y));
    uv.y += 0.5f;

    // 歪ませて画面外になった部分は黒で塗りつぶす
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
    {
        return float4(0, 0, 0, 1);
    }

    // RGBを少しズラして抽出（色収差・クロマティックアベレーション）
    float offset = 0.002f * g_Params.y;
    float r = g_Tex.Sample(g_Sam, uv + float2(offset, 0)).r;
    float g = g_Tex.Sample(g_Sam, uv).g;
    float b = g_Tex.Sample(g_Sam, uv - float2(offset, 0)).b;
    float4 color = float4(r, g, b, 1.0f);

    // スキャンライン（ブラウン管特有の細かい横縞）
    float scanline = sin(uv.y * 1080.0f * 3.14159f) * 0.05f * g_Params.y;
    color.rgb -= scanline;

    // ビネット（四隅を暗くして画面の中央を際立たせる）
    float vignette = uv.x * uv.y * (1.0f - uv.x) * (1.0f - uv.y);
    vignette = clamp(pow(vignette * 15.0f, 0.2f), 0.0f, 1.0f);
    color.rgb *= vignette;

    return color;
}