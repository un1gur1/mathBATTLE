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
    float dist = length(uv - center) * 2.0f; // 中心0.0、端1.0

    if (dist > 1.0f)
        return float4(0, 0, 0, 0);

    float time = g_Params.x;
    float3 baseColor = g_Params.yzw;
    float rollAngle = g_Params2.x;

    // UVから角度を計算（HUDリングの分割に使う）
    float angle = atan2(uv.y - center.y, uv.x - center.x);
    float rotAngle = angle - rollAngle;

    float3 finalColor = float3(0, 0, 0);

    // ==========================================
    // 1. アウターリング（デジタルHUD風のネオン枠）
    // ==========================================
    // 太い破線リング（タイトル画面では自転する）
    float dash = sin(rotAngle * 6.0f); // 6分割のロックオンサイト風
    float dashMask = step(0.0f, dash); // くっきり分断
    float thickRing = (smoothstep(0.75f, 0.8f, dist) - smoothstep(0.9f, 0.95f, dist)) * dashMask;

    // 一番外側の細い実線リング
    float thinRing = smoothstep(0.92f, 0.95f, dist) - smoothstep(0.98f, 1.0f, dist);

    // リングの発光を合成
    float ringMask = max(thickRing, thinRing);
    finalColor += baseColor * ringMask * 2.5f; // 強めにネオン発光

    // ==========================================
    // 2. ホログラム・インナーコア（透けるガラス＋データ線）
    // ==========================================
    float innerMask = smoothstep(0.85f, 0.8f, dist);
    
    // 背景のサイバーグリッドが透ける、暗いベースカラー
    float3 coreColor = baseColor * 0.15f;

    // 常に上から下へ流れるデジタルの走査線（スキャンライン）
    float scanline = sin((uv.y + time * 0.8f) * 50.0f);
    scanline = smoothstep(0.85f, 1.0f, scanline);
    coreColor += baseColor * scanline * 0.6f;

    // センター文字を浮かび上がらせるための明滅（パルス）バックライト
    float centerPulse = (sin(time * 4.0f) * 0.5f + 0.5f) * 0.3f + 0.2f;
    float centerGlow = smoothstep(0.6f, 0.0f, dist) * centerPulse;
    coreColor += baseColor * centerGlow * 1.5f;

    // 内側の色を合成
    finalColor += coreColor * innerMask;

    // ==========================================
    // 3. 透明度のコントロール（透過ホログラム表現）
    // ==========================================
    // 外周のHUDリングは不透明、内側のデータコアは「半透明 (0.85f)」にしてサイバー感を出す
    float alpha = max(ringMask, innerMask * 0.85f);
    alpha *= smoothstep(1.0f, 0.96f, dist); // フチのアンチエイリアス

    return float4(finalColor, alpha);
}