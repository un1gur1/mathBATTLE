#include "TutorialScene.h"

namespace App {

    TutorialScene::TutorialScene()
        : m_frameCount(0)
    {
    }

    TutorialScene::~TutorialScene() {
    }

    void TutorialScene::Init() {
        m_frameCount = 0;
        m_tutorialMaster.Init(); // チュートリアルマスターの初期化
    }

    void TutorialScene::Load() {}
    void TutorialScene::LoadEnd() {}

    void TutorialScene::Update() {
        m_tutorialMaster.Update();
        ++m_frameCount;
    }

    void TutorialScene::Draw() {
        m_tutorialMaster.Draw();
    }

    void TutorialScene::Release() {
        m_tutorialMaster.Release();
    }

} // namespace App