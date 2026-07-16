Texture2D g_Tex : register(t0);
SamplerState g_Sam : register(s0);

cbuffer cb0 : register(b0)
{
    float4 g_Params; // x: Time, y: R, z: G, w: B
    float4 g_Params2; // x: Angle
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
    float2 center = float2(0.5f, 0.5f);
    
    // 中心からの距離 (0.0 ～ 1.0)
    float dist = length(uv - center) * 2.0f;
    if (dist > 1.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    float3 baseColor = g_Params.yzw;

    // ==========================================
    // マット（つや消し）＆超明瞭フラットデザイン
    // ==========================================
    
    // 1. テカテカを一切排除した、落ち着いた発色のマットカラー
    float3 surfaceColor = baseColor * 0.85f;

    // 2. 視認性を極限まで高める「シャープなアウトライン」
    // 外側のフチを急激に暗く落とすことで、背景からパキッと切り離す
    float border = smoothstep(0.85f, 0.95f, dist);
    surfaceColor = lerp(surfaceColor, baseColor * 0.15f, border);

    // 3. フラットになりすぎて背景に溶けるのを防ぐための「微細な陰影」
    // ハイライト（白飛び）は入れず、上下にほんの少しだけ暗さを足して質量を持たせる
    float gradient = (uv.y - 0.5f) * 2.0f; // -1.0(上) ～ 1.0(下)
    surfaceColor -= gradient * 0.1f;

    // 4. 内側のわずかなインナーシャドウ（文字をより浮き上がらせる）
    float innerShadow = smoothstep(0.6f, 0.85f, dist);
    surfaceColor -= innerShadow * 0.2f;

    // アンチエイリアス（フチのギザギザ防止）
    float alpha = smoothstep(1.0f, 0.96f, dist);

    return float4(surfaceColor, alpha);
}