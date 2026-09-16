#include "SceneManager.h"
#include "GameScene.h"
#include "TitleScene/TitleScene.h"
#include "ResultScene.h"
#include "TutorialScene.h" 
#include "PauseMenu.h"
#include "../Input/InputManager.h"
#include "../Manager/ProceduralAudio.h"
#include"../Manager/PostProcessManager.h"
#include"../Manager/FadeManager.h"
#include <utility>

namespace App {
    // ==========================================
    // シングルトンインスタンス
    // ==========================================
    SceneManager* SceneManager::instance_ = nullptr;

    void SceneManager::SetGameEnd(bool isEnd) {
        isGameEnd_ = isEnd;
    }

    bool SceneManager::IsGameEnd() const {
        return isGameEnd_;
    }

    // ==========================================
    // コンストラクタ: 各種変数の初期化
    // ==========================================
    SceneManager::SceneManager()
        : scene_(nullptr)
        , load_(nullptr)
        , sceneId_(SCENE_ID::NONE)
        , nextSceneId_(SCENE_ID::NONE)
        , isChanging_(false)
        , isGameEnd_(false)
        , playerCount_(1)
        , gameMode_(0)
        , zeroOneScore_(501)
        , is1P_NPC_(false)
        , is2P_NPC_(true)
        , p1StartNum_(5)
        , p2StartNum_(7)
        , p1StartX_(1), p1StartY_(1)
        , p2StartX_(7), p2StartY_(7)
        , m_winnerPlayer(1)
        , m_p1Stats({ 0, 0, 0, 0, 0 })
        , m_p2Stats({ 0, 0, 0, 0, 0 })
        , isPaused_(false)
        , pauseMenu_(new PauseMenu())
    {
    }
    // ==========================================
    // デストラクタ: リソース解放
    // ==========================================
    SceneManager::~SceneManager() {
        if (scene_) {
            scene_->Release();
            delete scene_;
            scene_ = nullptr;
        }
        if (load_) {
            load_ = nullptr;
        }
        if (pauseMenu_) {
            delete pauseMenu_;
            pauseMenu_ = nullptr;
        }

        if (PostProcessManager::GetInstance()) {
            PostProcessManager::GetInstance()->Release();
            PostProcessManager::DeleteInstance();
        }

    }

    // ==========================================
    // 初期化: ゲーム開始時の設定
    // ==========================================
    void SceneManager::Init() {

        PostProcessManager::CreateInstance();
        PostProcessManager::GetInstance()->Init();

        isGameEnd_ = false;
        isChanging_ = false;
        isPaused_ = false;
        sceneId_ = SCENE_ID::NONE;
        nextSceneId_ = SCENE_ID::NONE;

        playerCount_ = 1;
        gameMode_ = 0;
        zeroOneScore_ = 501;

        is1P_NPC_ = false;
        is2P_NPC_ = true;
        p1StartNum_ = 5;
        p2StartNum_ = 7;
        p1StartX_ = 1;
        p1StartY_ = 1;
        p2StartX_ = 7;
        p2StartY_ = 7;

        // ★修正：起動時は一瞬でタイトル画面を作り、そこから「フェードイン（幕開け）」する！
        nextSceneId_ = SCENE_ID::TITLE;
        PerformSceneChange();
        m_fade.StartFadeIn(0.015f); // 少しゆっくり開く
    }

    void SceneManager::Init3D() {}

    void SceneManager::TogglePause() {
        isPaused_ = !isPaused_;
        ProceduralAudio::GetInstance().PlayPowerSE(9); // ポーズ効果音
        if (isPaused_ && pauseMenu_) pauseMenu_->Init();
    }

    // ==========================================
    // 更新処理: シーン切り替えとポーズ管理
    // ==========================================
    void SceneManager::Update() {
        // ★追加：フェード演出の更新
        m_fade.Update();

        // ★追加：フェードアウト（画面が閉まりきった）が完了したら、裏でシーンを切り替える！
        if (isChanging_ && m_fade.IsFadeOutDone()) {
            PerformSceneChange();
            m_fade.StartFadeIn(0.02f); // 切り替わったら幕を開ける
        }

        static bool prevEsc = false;
        bool escHit = (CheckHitKey(KEY_INPUT_ESCAPE) == 1);

        // ★修正：フェード中（シーン切り替え中）はポーズできないようにロックをかける
        if (escHit && !prevEsc && !isChanging_ && !m_fade.IsFading()) {
            TogglePause();
        }
        prevEsc = escHit;

        if (isPaused_) {
            if (pauseMenu_) {
                auto result = pauseMenu_->Update();

                if (result == PauseMenu::Result::RESUME) {
                    TogglePause();
                }
                else if (result == PauseMenu::Result::TITLE) {
                    isPaused_ = false;
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    ChangeScene(SCENE_ID::TITLE); // タイトルへ（フェードアウトが走る）
                }
                else if (result == PauseMenu::Result::EXIT) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    isGameEnd_ = true;
                }
            }
        }
        else {
            // 通常時: 現在のシーンを更新
            if (scene_) scene_->Update();
        }
    }

    // ==========================================
    // 描画処理: シーンとポーズメニューの表示
    // ==========================================
    void SceneManager::Draw() {
        auto* ppm = PostProcessManager::GetInstance();
        if (ppm) ppm->BeginDraw();

        if (scene_) scene_->Draw();

        if (isPaused_ && pauseMenu_) {
            pauseMenu_->Draw();
        }

        m_fade.Draw(); // ※フェードマネージャーの内部は空っぽでOK

        if (ppm) {
            EffectType currentEffect = EffectType::NONE;
            float intensity = 1.0f;
            bool enableCrt = false; // ★CRTを重ねがけするかのフラグ

            if (sceneId_ == SCENE_ID::TITLE) {
                TitleScene* pTitle = dynamic_cast<TitleScene*>(scene_);
                if (pTitle != nullptr) {
                    if (pTitle->IsWarping()) {
                        currentEffect = EffectType::TITLE_DIVE;
                        intensity = pTitle->GetWarpProgress();
                    }
                    else if (pTitle->IsInStandby()) {
                        currentEffect = EffectType::CRT;
                    }
                }
            }

            // ★ 起動時などのフェード中処理
            if (m_fade.IsFading()) {
                currentEffect = EffectType::FADE;
                intensity = m_fade.GetProgress();

                if (sceneId_ == SCENE_ID::TITLE) {
                    enableCrt = true;
                }
            }

            // 最後に第3引数として enableCrt を渡す
            ppm->EndDraw(currentEffect, intensity, enableCrt);
        }
    }

    void SceneManager::Delete() {
        if (scene_) {
            scene_->Release();
            delete scene_;
            scene_ = nullptr;
        }
        sceneId_ = SCENE_ID::NONE;
        nextSceneId_ = SCENE_ID::NONE;
        isChanging_ = false;
    }

    // ==========================================
    // シーン切り替え要求
    // ==========================================
    void SceneManager::ChangeScene(SCENE_ID nextId) {
        // ★修正：すでにフェード中なら無視する（連打バグ防止）
        if (isChanging_ || m_fade.IsFading()) return;

        nextSceneId_ = nextId;
        isChanging_ = true;
        m_fade.StartFadeOut(0.02f); // ★追加：いきなり切り替えず、まずは画面を閉じる！
    }

    // ==========================================
    // シーン切り替え実行: 実際のシーンオブジェクト生成
    // ==========================================
    void SceneManager::PerformSceneChange() {
        if (scene_) {
            scene_->Release();
            delete scene_;
            scene_ = nullptr;
        }

        switch (nextSceneId_) {
        case SCENE_ID::NONE:     scene_ = nullptr; break;
        case SCENE_ID::TITLE:    scene_ = new TitleScene(); break;
        case SCENE_ID::TUTORIAL: scene_ = new TutorialScene(); break;
        case SCENE_ID::GAME:     scene_ = new GameScene(); break;
        case SCENE_ID::RESULT:   scene_ = new ResultScene(); break;
        default:                 scene_ = nullptr; break;
        }

        sceneId_ = nextSceneId_;
        nextSceneId_ = SCENE_ID::NONE;
        isChanging_ = false;

        isPaused_ = false;

        if (scene_) {
            scene_->Init();
            scene_->Load();
            scene_->LoadEnd();
        }
    }

} // namespace App