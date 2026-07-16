Texture2D g_Tex : register(t0);
SamplerState g_Sam : register(s0);

cbuffer cb0 : register(b0)
{
    float4 g_Params1; // x: Progress(0.0~1.0), y: R, z: G, w: B
    float4 g_Params2; // x: EffectType(0,1,2), y: RandomSeed, z: 0, w: 0
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
    float2 UV : TEXCOORD0;
};

// 疑似乱数ジェネレーター
float rand(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.UV;
    float2 center = float2(0.5f, 0.5f);
    float dist = length(uv - center) * 2.0f;

    if (dist > 1.0f)
        return float4(0, 0, 0, 0);

    float progress = g_Params1.x;
    float3 baseColor = g_Params1.yzw;
    int type = (int) (g_Params2.x + 0.1f);
    float seed = g_Params2.y;

    float3 finalColor = float3(0, 0, 0);
    float alpha = 0.0f;

    // ==========================================
    // Type 0: ソニックブーム（色収差リング）
    // ==========================================
    if (type == 0)
    {
        float ringR = smoothstep(0.15f, 0.0f, abs(dist - progress));
        float ringG = smoothstep(0.15f, 0.0f, abs(dist - (progress * 0.9f)));
        float ringB = smoothstep(0.15f, 0.0f, abs(dist - (progress * 0.8f)));
        
        finalColor = float3(ringR, ringG, ringB) * baseColor * 2.5f;
        finalColor += baseColor * smoothstep(0.05f, 0.0f, abs(dist - progress)) * 3.0f;
        alpha = max(ringR, max(ringG, ringB)) * (1.0f - progress);
    }
    // ==========================================
    // Type 1: デジタルクラック（電子ひび割れ）
    // ==========================================
    else if (type == 1)
    {
        float angle = atan2(uv.y - center.y, uv.x - center.x);
        float n = sin(angle * 8.0f + seed) * 0.5f
                + sin(angle * 19.0f - seed) * 0.25f
                + sin(angle * 37.0f + seed * 1.5f) * 0.125f;
        n = n * 0.5f + 0.5f;
        
        float p = progress * 1.2f;
        float crack = smoothstep(0.05f, 0.0f, abs(dist - n * p));
        float core = smoothstep(0.3f, 0.0f, dist) * (1.0f - progress);
        
        finalColor = baseColor * (crack * 3.0f + core * 1.5f);
        finalColor += float3(1, 1, 1) * smoothstep(0.01f, 0.0f, abs(dist - n * p));
        alpha = (crack + core) * (1.0f - progress);
    }
    // ==========================================
    // Type 2: マイクロ・ブルグリッチ（★サイズ動的変化版！）
    // ==========================================
    else if (type == 2)
    {
        // ★追加：進行度(progress)に合わせて、ブロックの分割数を 15 から 100 へ激しく増やす！
        // （＝ 最初は大きな四角いバグ、最後は細かい塵のように消えていく）
        float blocks = lerp(15.0f, 100.0f, progress);
        float2 blockUV = floor(uv * blocks) / blocks;
        
        float r = rand(blockUV + seed);
        
        // 衝撃波の広がるリング（少し鋭く調整）
        float ring = smoothstep(0.3f, 0.0f, abs(length(blockUV - center) * 4.0f - progress));
        
        // ノイズの密度
        float flash = step(0.6f, r) * ring;
        
        // サイバーなシアン＆ディープブルー
        finalColor = float3(0.05f, 0.4f, 1.0f) * flash * 3.0f;
        if (r > 0.95f)
            finalColor += float3(0.5f, 1.0f, 1.0f);
        if (r > 0.85f && r <= 0.95f)
            finalColor += float3(0.1f, 0.1f, 0.8f);
        
        // ★透過度の落ち方も、単調なフェードアウトから「後半一気に消える」キレのある動きに修正
        alpha = flash * (1.0f - smoothstep(0.2f, 1.0f, progress));
    }

    return float4(finalColor, alpha);
}