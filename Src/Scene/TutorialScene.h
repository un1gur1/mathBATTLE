#pragma once
#include "SceneBase.h"
#include "../Manager/TutorialMaster.h"

namespace App {

    // ==========================================
    // TutorialScene: チュートリアル画面のシーン
    // ==========================================
    class TutorialScene : public SceneBase {
    public:
        TutorialScene();
        ~TutorialScene() override;

        void Init() override;
        void Load() override;
        void LoadEnd() override;
        void Update() override;
        void Draw() override;
        void Release() override;

    private:
        TutorialMaster m_tutorialMaster; // チュートリアル専用の進行管理クラス
        int m_frameCount;
    };

} // namespace App