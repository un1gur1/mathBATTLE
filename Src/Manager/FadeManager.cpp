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
    
}