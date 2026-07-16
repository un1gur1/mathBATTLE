#pragma once
#include <DxLib.h>

class FadeManager {
public:
    enum class State {
        NONE,
        FADING_OUT, // 画面が暗くなっていく（閉まる）
        FADING_IN   // 画面が明るくなっていく（開く）
    };

private:
    State m_state;
    float m_progress;  // 0.0f(透明) ～ 1.0f(真っ暗)
    float m_fadeSpeed; // 遷移スピード

    // サイバーブラインド用の設定
    int m_splitCount;  // 画面を何分割するか
    unsigned int m_colBg;   // ブラインドの色（暗い背景）
    unsigned int m_colNeon; // 先端の発光色（サイバーブルー）

public:
    FadeManager();

    // フェードアウト開始（スピードは 0.01 ～ 0.05 くらいがおすすめ）
    void StartFadeOut(float speed = 0.02f);

    // フェードイン開始
    void StartFadeIn(float speed = 0.02f);

    void Update();
    void Draw() const;

    float GetProgress() const { return m_progress; }

    // 現在フェード中かどうか（シーンのUpdateを止める判定などに使う）
    bool IsFading() const { return m_state != State::NONE; }

    // 画面が完全に覆われたか（この瞬間にSceneManagerで次のシーンへ切り替える）
    bool IsFadeOutDone() const { return m_state == State::FADING_OUT && m_progress >= 1.0f; }
};