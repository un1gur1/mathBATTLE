Texture2D g_Tex : register(t0);
SamplerState g_Sam : register(s0);

cbuffer cb0 : register(b0)
{
    float4 g_Params;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
    float2 UV : TEXCOORD0;
};

float rand(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

// ★追加：CRTの色味と暗さを完全にコピーする関数
float4 GetBaseColor(float2 uv, float enableCrt)
{
    if (enableCrt > 0.5f)
    {
        float offset = 0.002f;
        float r = g_Tex.Sample(g_Sam, uv + float2(offset, 0)).r;
        float g = g_Tex.Sample(g_Sam, uv).g;
        float b = g_Tex.Sample(g_Sam, uv - float2(offset, 0)).b;
        float4 col = float4(r, g, b, 1.0f);

        // 走査線で暗くする
        float scanline = sin(uv.y * 1080.0f * 3.14159f) * 0.05f;
        col.rgb -= scanline;

        // 四隅（ビネット）を暗くする
        float vignette = uv.x * uv.y * (1.0f - uv.x) * (1.0f - uv.y);
        vignette = clamp(pow(vignette * 15.0f, 0.2f), 0.0f, 1.0f);
        col.rgb *= vignette;

        return col;
    }
    return g_Tex.Sample(g_Sam, uv);
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.UV;
    float progress = g_Params.y;
    float time = g_Params.x;
    float enableCrt = g_Params.z;

    // 形状をブラウン管に歪ませる
    if (enableCrt > 0.5f)
    {
        float2 dc = abs(0.5f - uv);
        dc *= dc;
        uv.x -= 0.5f;
        uv.x *= 1.0f + (dc.y * 0.2f);
        uv.x += 0.5f;
        uv.y -= 0.5f;
        uv.y *= 1.0f + (dc.x * 0.2f);
        uv.y += 0.5f;
        if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
        {
            return float4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    // 元の色を取得するときは必ず GetBaseColor を使う
    if (progress <= 0.0f)
        return GetBaseColor(uv, enableCrt);
    if (progress >= 1.0f)
        return float4(0.02f, 0.04f, 0.08f, 1.0f);

    float4 cyberBlue = float4(0.02f, 0.38f, 0.90f, 1.0f);
    float4 darkBg = float4(0.02f, 0.04f, 0.08f, 1.0f);

    float scanPos = (uv.x + uv.y * 0.4f) / 1.4f;
    float currentScan = progress * 1.5f - 0.25f;
    float dist = currentScan - scanPos;

    float sparkNoise = (rand(float2(floor(uv.y * 180.0f), floor(time * 25.0f))) - 0.5f) * 0.06f;
    sparkNoise += sin(uv.y * 350.0f - time * 150.0f) * 0.015f;

    if (abs(dist + sparkNoise) < 0.005f)
        return float4(0.8f, 0.95f, 1.0f, 1.0f) * 1.5f;
    if (abs(dist - sparkNoise * 0.6f) < 0.003f)
        return cyberBlue * 2.5f;

    if (dist < 0.0f)
    {
        float4 baseColor = GetBaseColor(uv, enableCrt); // ★変更
        float preGlow = smoothstep(-0.1f, 0.0f, dist);
        return baseColor + (cyberBlue * preGlow * 0.8f);
    }

    float afterGlow = 1.0f - (dist / 0.4f);

    if (afterGlow > 0.0f)
    {
        float2 gridCount = float2(64.0f * (16.0f / 9.0f), 64.0f);
        float2 gridUV = frac(uv * gridCount);
        float2 blockID = floor(uv * gridCount);

        float blockRand = rand(blockID);
        float lineThick = 0.08f;
        bool isGridLine = (gridUV.x < lineThick || gridUV.y < lineThick);

        float pattern = step(0.4f, rand(blockID + floor(time * 6.0f)));
        bool isDataDot = (gridUV.x > 0.3f && gridUV.x < 0.7f && gridUV.y > 0.3f && gridUV.y < 0.7f) && pattern > 0.5f;

        if (isGridLine || isDataDot)
        {
            float flicker = sin(time * 40.0f + blockRand * 15.0f) * 0.25f + 0.75f;
            return cyberBlue * afterGlow * flicker * 1.6f;
        }

        float4 baseColor = GetBaseColor(uv, enableCrt); // ★変更
        float4 hologramColor = baseColor * cyberBlue * 1.3f;
        return lerp(darkBg, hologramColor, afterGlow * 0.5f);
    }

    return darkBg;
}