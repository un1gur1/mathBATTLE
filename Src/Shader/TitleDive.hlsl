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

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.UV;
    float progress = g_Params.y;
    float time = g_Params.x;

    // ねじれやズームのベースとなる進行度（後半に向けて加速する曲線）
    float easeProgress = pow(progress, 3.0f);
    
    // ==========================================
    // ★修正：ねじれ（easeProgress）と完全に同期して、徐々に白さを増していく
    // ==========================================
    float whiteout = easeProgress;

    float crtCurve = 0.2f * (1.0f - progress);
    float2 dc = abs(0.5f - uv);
    dc *= dc;
    uv.x -= 0.5f;
    uv.x *= 1.0f + (dc.y * crtCurve);
    uv.x += 0.5f;
    uv.y -= 0.5f;
    uv.y *= 1.0f + (dc.x * crtCurve);
    uv.y += 0.5f;

    // 画面外も、ねじれと同じペースで黒から白へフェードさせる
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
    {
        return lerp(float4(0.0f, 0.0f, 0.0f, 1.0f), float4(1.0f, 1.0f, 1.0f, 1.0f), whiteout);
    }

    float2 center = float2(0.5f, 0.5f);
    float2 toCenter = uv - center;
    float dist = length(toCenter);
    float angle = atan2(toCenter.y, toCenter.x);

    // 空間のねじれ（whiteoutと全く同じ easeProgress で加速！）
    float twist = (1.0f - dist) * easeProgress * 1.5f;
    angle += twist;

    float2 twistedUV = center + float2(cos(angle), sin(angle)) * dist;
    float zoom = 1.0f - (easeProgress * 0.85f);
    float2 warpedUV = (twistedUV - center) * zoom + center;

    float2 sliceID = floor(warpedUV * float2(300.0f, 300.0f));
    float sliceRand = rand(sliceID);
    
    if (progress > 0.1f)
    {
        float particleFlow = sliceRand * progress;
        warpedUV.x += sin(warpedUV.y * 150.0f + time * 10.0f) * 0.015f * particleFlow;
        warpedUV.y += (sliceRand - 0.5f) * 0.03f * particleFlow;
    }

    float colorShift = 0.002f * (1.0f - progress) + (easeProgress * 0.012f);
    float r = g_Tex.Sample(g_Sam, warpedUV + float2(colorShift, 0)).r;
    float g = g_Tex.Sample(g_Sam, warpedUV).g;
    float b = g_Tex.Sample(g_Sam, warpedUV - float2(colorShift, 0)).b;
    float4 baseColor = float4(r, g, b, 1.0f);

    float scanline = sin(uv.y * 1080.0f * 3.14159f) * 0.05f * (1.0f - progress);
    baseColor.rgb -= scanline;

    float vignette = uv.x * uv.y * (1.0f - uv.x) * (1.0f - uv.y);
    vignette = clamp(pow(vignette * 15.0f, 0.2f), 0.0f, 1.0f);
    baseColor.rgb *= lerp(1.0f, vignette, 1.0f - progress);

    float effectAlpha = sin(progress * 3.14159f);
    float threadID = floor(angle * 120.0f);
    float threadRand = rand(float2(threadID, 0.0f));
    float threadPulse = frac(dist * 3.0f - time * 12.0f - threadRand);
    float threadLight = step(0.92f, threadPulse) * 0.35f * effectAlpha;

    float4 cyberBlue = float4(0.01f, 0.45f, 0.95f, 1.0f);
    float4 cyanCore = float4(0.80f, 0.95f, 1.0f, 1.0f);
    float4 speedlineColor = lerp(cyberBlue, cyanCore, threadPulse * 0.5f);

    float4 outColor = baseColor + (speedlineColor * threadLight);
    
    // ねじれ・ズームと完全に同期して、徐々に光（白）に溶けていく
    return lerp(outColor, float4(1.0f, 1.0f, 1.0f, 1.0f), whiteout);
}