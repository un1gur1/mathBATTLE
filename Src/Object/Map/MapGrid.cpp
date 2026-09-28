#define NOMINMAX
#include "MapGrid.h"
#include <DxLib.h>
#include <cmath>
#include <string>
#include <algorithm>

#include "../../Shader/CrystalOrbShader.h" 

namespace App {

    // ヘッダーを変更せずにシェーダーを管理するための静的変数
    static int g_psGridCrystalHandle = -1;
    static int g_cbGridCrystalHandle = -1;
    static int g_gridFontHandle = -1;

    // ==========================================
    // コンストラクタ: マップグリッドの基本設定
    // ==========================================
    MapGrid::MapGrid(int tileSize, Vector2 offset)
        : m_tileSize(tileSize)           // マス目1個のサイズ（ピクセル）
        , m_offset(offset)               // マップ全体の左上座標
        , m_ruleMode(BattleRuleMode::CLASSIC)
        , m_totalTurns(0)                // 経過ターン数
        , m_currentCycleTick(0)          // アイテム再出現カウンター
        , m_spawnInterval(3)             // アイテム再出現間隔（ターン数）
    {
        // ★初回生成時のみシェーダーとフォントを読み込む
        if (g_psGridCrystalHandle == -1) {
            g_psGridCrystalHandle = LoadPixelShaderFromMem(g_ps_CrystalOrb, sizeof(g_ps_CrystalOrb));
            g_cbGridCrystalHandle = CreateShaderConstantBuffer(sizeof(float) * 8);
            g_gridFontHandle = CreateFontToHandle("HGP創英角ﾎﾟｯﾌﾟ体", 42, 2, DX_FONTTYPE_ANTIALIASING);
        }
    }

    // ==========================================
    // 出現地点の初期化: ルールモードに応じた配置
    // ==========================================
    void MapGrid::InitializeSpawnPoints() {
        m_spawnPoints.clear();

        if (m_ruleMode == BattleRuleMode::CLASSIC) {
            InitializeClassicSpawns();   // ノーマルモード用配置
        }
        else {
            InitializeZeroOneSpawns();   // カウントモード用配置
        }
    }

    // ==========================================
    // ルールモード＆ステージ設定
    // ==========================================
    void MapGrid::SetRuleModeAndStage(BattleRuleMode mode, int stageIndex) {
        m_ruleMode = mode;
        m_stageIndex = stageIndex;

        // アイテム再出現間隔の設定（モード別）
        if (m_ruleMode == BattleRuleMode::CLASSIC) {
            m_spawnInterval = 3;
        }
        else {
            m_spawnInterval = 2;
        }
        InitializeSpawnPoints();
    }

    // ==========================================
    // ノーマルモードのアイテム配置
    // ==========================================
    void MapGrid::InitializeClassicSpawns() {
        m_spawnPoints.clear();
        struct PointDef { int x, y; char op; };
        std::vector<PointDef> defs;

        if (m_stageIndex == 0) {
            defs = {
                { 4, 1, '*' }, { 4, 7, '+' }, { 1, 4, '-' }, { 7, 4, '-' },
                { 2, 2, '-' }, { 6, 2, '/' }, { 2, 6, '/' }, { 6, 6, '-' },
                { 4, 4, '+' }, { 3, 3, '-' }, { 5, 5, '-' }
            };
        }
        else if (m_stageIndex == 1) {
            defs = {
                { 4, 0, '-' }, { 7, 1, '-' }, { 8, 4, '-' }, { 7, 7, '-' },
                { 4, 8, '-' }, { 1, 7, '-' }, { 0, 4, '-' }, { 1, 1, '-' },
                { 4, 2, '-' }, { 6, 2, '-' }, { 6, 4, '-' }, { 6, 6, '-' },
                { 4, 6, '-' }, { 2, 6, '-' }, { 2, 4, '-' }, { 2, 2, '-' },
                { 4, 4, '-' }
            };
        }
        else {
            defs = {
                { 4, 0, '/' }, { 7, 1, '*' }, { 8, 4, '/' }, { 7, 7, '+' },
                { 4, 8, '/' }, { 1, 7, '-' }, { 0, 4, '/' }, { 1, 1, '*' },
                { 4, 1, '-' }, { 6, 1, '*' }, { 7, 4, '-' }, { 6, 7, '*' },
                { 4, 7, '-' }, { 2, 7, '*' }, { 1, 4, '-' }, { 2, 1, '*' },
                { 4, 2, '+' }, { 6, 2, '/' }, { 6, 4, '*' }, { 6, 6, '-' },
                { 4, 6, '+' }, { 2, 6, '/' }, { 2, 4, '*' }, { 2, 2, '-' },
                { 4, 4, '*' }
            };
        }

        std::vector<char> cycleSeq = { '+', '-', '*', '/' };

        for (const auto& d : defs) {
            SpawnPoint sp;
            sp.pos = { d.x, d.y };

            if (m_stageIndex == 2) {
                sp.sequence = cycleSeq;
                auto it = std::find(cycleSeq.begin(), cycleSeq.end(), d.op);
                if (it != cycleSeq.end()) {
                    sp.currentIndex = (int)std::distance(cycleSeq.begin(), it);
                }
                else {
                    sp.currentIndex = 0;
                }
            }
            else {
                sp.sequence = { d.op };
                sp.currentIndex = 0;
            }

            sp.currentSymbol = sp.sequence[sp.currentIndex];
            sp.isAvailable = true;
            m_spawnPoints.push_back(sp);
        }
    }

    // ==========================================
    // カウントモードのアイテム配置
    // ==========================================
    void MapGrid::InitializeZeroOneSpawns() {
        m_spawnPoints.clear();
        std::vector<char> zeroOneSeq = { '+', '-', '*', '/', '+', '-' };

        struct PointDef { int x, y, startIdx; };
        std::vector<PointDef> defs;

        if (m_stageIndex == 0) {
            defs = {
                { 4, 2, 0 }, { 4, 6, 1 }, { 2, 4, 1 }, { 6, 4, 0 },
                { 3, 3, 0 }, { 5, 3, 1 }, { 3, 5, 2 }, { 5, 5, 3 },
                { 2, 2, 4 }, { 6, 2, 5 }, { 2, 6, 3 }, { 6, 6, 2 },
                { 4, 4, 2 }, { 4, 1, 0 }, { 4, 7, 4 }, { 1, 4, 1 }, { 7, 4, 5 }
            };
        }
        else if (m_stageIndex == 1) {
            defs = {
                { 4, 0, 1 }, { 7, 1, 1 }, { 8, 4, 1 }, { 7, 7, 1 },
                { 4, 8, 1 }, { 1, 7, 1 }, { 0, 4, 1 }, { 1, 1, 1 },
                { 4, 2, 1 }, { 6, 2, 1 }, { 6, 4, 1 }, { 6, 6, 1 },
                { 4, 6, 1 }, { 2, 6, 1 }, { 2, 4, 1 }, { 2, 2, 1 },
                { 4, 4, 1 }
            };
        }
        else {
            defs = {
                { 4, 0, 0 }, { 7, 1, 2 }, { 8, 4, 4 }, { 7, 7, 1 },
                { 4, 8, 3 }, { 1, 7, 5 }, { 0, 4, 0 }, { 1, 1, 2 },
                { 4, 1, 1 }, { 6, 1, 3 }, { 7, 4, 5 }, { 6, 7, 0 },
                { 4, 7, 2 }, { 2, 7, 4 }, { 1, 4, 1 }, { 2, 1, 3 },
                { 4, 2, 2 }, { 6, 2, 4 }, { 6, 4, 0 }, { 6, 6, 3 },
                { 4, 6, 5 }, { 2, 6, 1 }, { 2, 4, 4 }, { 2, 2, 2 },
                { 3, 3, 0 }, { 5, 3, 3 }, { 5, 5, 1 }, { 3, 5, 4 },
                { 4, 4, 2 }
            };
        }

        for (const auto& d : defs) {
            SpawnPoint sp;
            sp.pos = { d.x, d.y };
            sp.sequence = zeroOneSeq;
            sp.currentIndex = d.startIdx % zeroOneSeq.size();
            sp.currentSymbol = sp.sequence[sp.currentIndex];
            sp.isAvailable = true;
            m_spawnPoints.push_back(sp);
        }
    }

    // ==========================================
    // アイテム取得処理
    // ==========================================
    char MapGrid::PickUpItem(int x, int y) {
        for (auto& sp : m_spawnPoints) {
            if (sp.pos.x == x && sp.pos.y == y && sp.isAvailable) {
                char picked = sp.currentSymbol;
                sp.isAvailable = false;

                if (!(m_ruleMode == BattleRuleMode::CLASSIC && m_stageIndex == 2)) {
                    sp.currentIndex = (sp.currentIndex + 1) % sp.sequence.size();
                    sp.currentSymbol = sp.sequence[sp.currentIndex];
                }
                return picked;
            }
        }
        return '\0';
    }

    // ==========================================
    // ターン更新: アイテムの再出現と循環
    // ==========================================
    void MapGrid::UpdateTurn() {
        m_totalTurns++;
        m_currentCycleTick++;

        int currentInterval = (m_stageIndex == 2) ? 2 : m_spawnInterval;

        if (m_currentCycleTick >= currentInterval) {
            m_currentCycleTick = 0;

            for (auto& sp : m_spawnPoints) {
                if (!sp.isAvailable) {
                    sp.isAvailable = true;
                }

                if (m_stageIndex == 2) {
                    sp.currentIndex = (sp.currentIndex + 1) % sp.sequence.size();
                    sp.currentSymbol = sp.sequence[sp.currentIndex];
                }
            }
        }
    }

    // ==========================================
    // 座標変換系
    // ==========================================
    IntVector2 MapGrid::ScreenToGrid(const Vector2& pos) const {
        return {
            static_cast<int>((pos.x - m_offset.x) / m_tileSize),
            static_cast<int>((pos.y - m_offset.y) / m_tileSize)
        };
    }

    Vector2 MapGrid::GetCellCenter(int x, int y) const {
        return {
            m_offset.x + x * m_tileSize + m_tileSize / 2.0f,
            m_offset.y + y * m_tileSize + m_tileSize / 2.0f
        };
    }

    bool MapGrid::IsWithinBounds(int x, int y) const {
        return x >= 0 && x < 9 && y >= 0 && y < 9;
    }

    char MapGrid::GetItemAt(int x, int y) const {
        for (const auto& sp : m_spawnPoints) {
            if (sp.pos.x == x && sp.pos.y == y && sp.isAvailable) {
                return sp.currentSymbol;
            }
        }
        return '\0';
    }

    int MapGrid::GetNumberChipAt(int x, int y) const {
        for (const auto& chip : m_numberChips) {
            if (chip.pos.x == x && chip.pos.y == y) return chip.value;
        }
        return 0;
    }

    int MapGrid::PickUpNumberChip(int x, int y) {
        for (auto it = m_numberChips.begin(); it != m_numberChips.end(); ++it) {
            if (it->pos.x == x && it->pos.y == y) {
                const int value = it->value;
                m_numberChips.erase(it);
                return value;
            }
        }
        return 0;
    }

    void MapGrid::SetNumberChipAt(int x, int y, int value) {
        if (!IsWithinBounds(x, y)) return;
        value = std::clamp(value, 1, 9);

        for (auto& chip : m_numberChips) {
            if (chip.pos.x == x && chip.pos.y == y) {
                chip.value = value;
                return;
            }
        }
        m_numberChips.push_back(NumberChip{ IntVector2{ x, y }, value });
    }

    void MapGrid::ClearNumberChips() {
        m_numberChips.clear();
    }

    bool MapGrid::HasAnyItemAt(int x, int y) const {
        return GetItemAt(x, y) != '\0' || GetNumberChipAt(x, y) != 0;
    }


    // ==========================================
    // マップ描画: グリッド＆最高級シェーダーアイテムの表示
    // ==========================================
    void MapGrid::Draw() const {
        // 1. グリッド描画
        unsigned int lineCol = GetColor(40, 45, 60);
        unsigned int fillCol = GetColor(15, 18, 25);

        for (int y = 0; y < 9; y++) {
            for (int x = 0; x < 9; x++) {
                Vector2 pos = GetCellCenter(x, y);
                DrawBox((int)pos.x - 39, (int)pos.y - 39, (int)pos.x + 39, (int)pos.y + 39, lineCol, FALSE);
                DrawBox((int)pos.x - 38, (int)pos.y - 38, (int)pos.x + 38, (int)pos.y + 38, fillCol, TRUE);
            }
        }

        // 2. アイテム描画（シェーダー版・直立固定）
        if (g_psGridCrystalHandle != -1 && g_cbGridCrystalHandle != -1) {
            double time = GetNowCount() / 1000.0;
            float pulse = (float)(sin(time * 5.0) * 4.0);

            for (size_t i = 0; i < m_spawnPoints.size(); ++i) {
                const auto& sp = m_spawnPoints[i];
                Vector2 basePos = GetCellCenter(sp.pos.x, sp.pos.y);
                float drawX = basePos.x;
                float drawY = basePos.y;

                // --- 取得済みアイテムの表示（ホログラム） ---
                if (!sp.isAvailable) {
                    SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
                    DrawCircleAA(drawX, drawY, 22.0f + pulse / 2.0f, 64, GetColor(80, 100, 120), FALSE, 2.0f);

                    char nextOp = sp.currentSymbol;
                    if (m_stageIndex == 2) {
                        nextOp = sp.sequence[(sp.currentIndex + 1) % sp.sequence.size()];
                    }

                    SetFontSize(24);
                    char waitStr[2] = { nextOp, '\0' };
                    int tw = GetDrawStringWidth(waitStr, 1);
                    DrawString((int)drawX - tw / 2, (int)drawY - 12, waitStr, GetColor(100, 140, 180));
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    continue;
                }

                // --- アイテムの色設定 ---
                unsigned int auraCol;
                float r = 0.0f, g = 0.0f, b = 0.0f;
                if (sp.currentSymbol == '+') { r = 0.85f; g = 0.10f; b = 0.20f; auraCol = GetColor(200, 30, 50); } // 赤
                else if (sp.currentSymbol == '-') { r = 0.10f; g = 0.45f; b = 0.95f; auraCol = GetColor(30, 120, 220); } // 青
                else if (sp.currentSymbol == '*') { r = 0.15f; g = 0.80f; b = 0.25f; auraCol = GetColor(40, 180, 60); }  // 緑
                else { r = 0.70f; g = 0.15f; b = 0.90f; auraCol = GetColor(180, 40, 200); } // 紫

                // オーラ（盤面に漏れるエネルギー）
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 45);
                DrawCircleAA(drawX, drawY, 34.0f + pulse, 64, auraCol, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                // --- ① シェーダーを【ON】にして「マットな樹脂トークン」を描く ---
                SetUsePixelShader(g_psGridCrystalHandle);
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);

                float* cb = (float*)GetBufferShaderConstantBuffer(g_cbGridCrystalHandle);
                cb[0] = (float)time;
                cb[1] = r; cb[2] = g; cb[3] = b;
                cb[4] = 0.0f; // ★回転させず常に0.0f（直立）
                cb[5] = 0.0f; cb[6] = 0.0f; cb[7] = 0.0f;
                UpdateShaderConstantBuffer(g_cbGridCrystalHandle);
                SetShaderConstantBuffer(g_cbGridCrystalHandle, DX_SHADERTYPE_PIXEL, 0);

                float size = 30.0f;
                VERTEX2DSHADER v[6];
                for (int idx = 0; idx < 6; ++idx) {
                    v[idx].pos = VGet(0, 0, 0); v[idx].rhw = 1.0f;
                    v[idx].dif = GetColorU8(255, 255, 255, 255);
                    v[idx].spc = GetColorU8(0, 0, 0, 0);
                }
                v[0].pos.x = drawX - size; v[0].pos.y = drawY - size; v[0].u = 0.0f; v[0].v = 0.0f;
                v[1].pos.x = drawX + size; v[1].pos.y = drawY - size; v[1].u = 1.0f; v[1].v = 0.0f;
                v[2].pos.x = drawX - size; v[2].pos.y = drawY + size; v[2].u = 0.0f; v[2].v = 1.0f;
                v[3].pos.x = drawX + size; v[3].pos.y = drawY - size; v[3].u = 1.0f; v[3].v = 0.0f;
                v[4].pos.x = drawX + size; v[4].pos.y = drawY + size; v[4].u = 1.0f; v[4].v = 1.0f;
                v[5].pos.x = drawX - size; v[5].pos.y = drawY + size; v[5].u = 0.0f; v[5].v = 1.0f;

                DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);

                // --- ② シェーダーを【OFF】にしてから「文字」を描画する ---
                SetUsePixelShader(-1);

                char symStr[2] = { sp.currentSymbol, '\0' };
                int tw = GetDrawStringWidthToHandle(symStr, 1, g_gridFontHandle);

                // シンプルで高速な通常描画（直立固定）
                // 影の描画
                DrawStringToHandle((int)drawX - tw / 2 + 2, (int)drawY - 21 + 2, symStr, GetColor(10, 15, 30), g_gridFontHandle);
                // 本体の描画（白）
                DrawStringToHandle((int)drawX - tw / 2, (int)drawY - 21, symStr, GetColor(255, 255, 255), g_gridFontHandle);
            }
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // ROUND_BATTLE: 数字チップ
        for (const auto& chip : m_numberChips) {
            if (chip.value < 1 || chip.value > 9) continue;

            const Vector2 center = GetCellCenter(chip.pos.x, chip.pos.y);
            const float pulse = static_cast<float>(std::sin(GetNowCount() / 180.0) * 3.0);
            const unsigned int edge = GetColor(100, 255, 210);
            const unsigned int fill = GetColor(15, 55, 55);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
            DrawCircleAA(center.x, center.y, 29.0f + pulse, 48, fill, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_ADD, 180);
            DrawCircleAA(center.x, center.y, 32.0f + pulse, 48, edge, FALSE, 3.0f);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            if (g_gridFontHandle != -1) {
                const std::string valueText = std::to_string(chip.value);
                const int tw = GetDrawStringWidthToHandle(
                    valueText.c_str(), static_cast<int>(valueText.size()), g_gridFontHandle);
                DrawStringToHandle(
                    static_cast<int>(center.x) - tw / 2,
                    static_cast<int>(center.y) - 21,
                    valueText.c_str(),
                    GetColor(235, 255, 250),
                    g_gridFontHandle);
            }
        }
    }


    // ==========================================
    // アイテム全消去（チュートリアル・カスタム用）
    // ==========================================
    void MapGrid::ClearItems() {
        m_spawnPoints.clear();
        m_numberChips.clear();
    }

    // ==========================================
    // アイテム強制配置 / 消去（チュートリアル・カスタム用）
    // ==========================================
    void MapGrid::SetItemAt(int x, int y, char symbol) {
        // 既に同じマスに出現ポイントがある場合は状態を上書き
        for (auto& sp : m_spawnPoints) {
            if (sp.pos.x == x && sp.pos.y == y) {
                if (symbol == '\0') {
                    sp.isAvailable = false; // \0 なら非表示（消去）にする
                }
                else {
                    sp.currentSymbol = symbol;
                    sp.isAvailable = true;
                }
                return;
            }
        }

        // 削除指定（\0）なのに元々存在しなかった場合は何もしない
        if (symbol == '\0') return;

        // 新しいマスなら出現ポイントを新規作成
        SpawnPoint sp;
        sp.pos = { x, y };
        sp.sequence = { symbol };
        sp.currentIndex = 0;
        sp.currentSymbol = symbol;
        sp.isAvailable = true;
        m_spawnPoints.push_back(sp);
    }

} // namespace App