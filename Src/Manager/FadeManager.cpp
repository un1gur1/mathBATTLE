#define NOMINMAX
#include "FadeManager.h"
#include <cmath>
#include <algorithm>

FadeManager::FadeManager()
    : m_state(State::NONE)
    , m_progress(0.0f)
    , m_fadeSpeed(0.02f)
    , m_splitCount(18) // 画面を18枚の短冊に分割
{
    // 色の設定（世界観に合わせたカラー）
    m_colBg = GetColor(10, 12, 18);      // ダークな背景
    m_colNeon = GetColor(0, 200, 255);   // ネオンブルーの発光
}

void FadeManager::StartFadeOut(float speed) {
    m_state = State::FADING_OUT;
    m_progress = 0.0f;
    m_fadeSpeed = speed;
}

void FadeManager::StartFadeIn(float speed) {
    m_state = State::FADING_IN;
    m_progress = 1.0f;
    m_fadeSpeed = speed;
}

void FadeManager::Update() {
    if (m_state == State::FADING_OUT) {
        m_progress += m_fadeSpeed;
        if (m_progress >= 1.0f) {
            m_progress = 1.0f;
            // フェードアウト完了は外部（SceneManager）で検知して次のシーンへ行く
        }
    }
    else if (m_state == State::FADING_IN) {
        m_progress -= m_fadeSpeed;
        if (m_progress <= 0.0f) {
            m_progress = 0.0f;
            m_state = State::NONE; // フェードイン完了で待機状態へ
        }
    }
}

void FadeManager::Draw() const {
    // フェードしていないときは何も描画しない
    if (m_state == State::NONE && m_progress <= 0.0f) return;

    int sw, sh;
    GetDrawScreenSize(&sw, &sh);

    float stripW = (float)sw / m_splitCount; // 1枚の短冊の最大の太さ

    for (int i = 0; i < m_splitCount; ++i) {
        // ★ 左から右へ時間差（ウェーブ）を作るためのディレイ計算
        // 全体の進捗のうち、0.4(40%)分を時間差として使う
        float delay = ((float)i / m_splitCount) * 0.4f;

        // この短冊のローカルの進捗 (0.0 ～ 1.0)
        float localP = (m_progress - delay) * (1.0f / 0.6f);
        localP = std::max(0.0f, std::min(localP, 1.0f));

        // ★ イージング（最初は速く、最後はフワッと止まる曲線）
        float easeP = 1.0f - std::pow(1.0f - localP, 3.0f); // EaseOutCubic

        // 現在の太さを計算
        float currentW = stripW * easeP;
        float startX = i * stripW;

        if (currentW > 0.0f) {
            // ① 背景となる真っ暗な短冊を描画
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
            DrawBox((int)startX, 0, (int)(startX + currentW), sh, m_colBg, TRUE);

            // ② 先端に「ネオンの発光」を重ねて描画（閉まりきっていない時だけ光る）
            if (currentW < stripW - 1.0f) {
                SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
                DrawBox((int)(startX + currentW - 6), 0, (int)(startX + currentW), sh, m_colNeon, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
        }
    }
}