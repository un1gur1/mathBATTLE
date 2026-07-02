#pragma once
#include <memory>
#include <string>
#include <vector>
#include "../Object/Unit/Player/Player.h"
#include "../Object/Unit/Enemy/Enemy.h"
#include "../Object/Map/MapGrid.h"
#include "../Battle/BattleUI.h"

namespace App {

    // ==========================================
    // TutorialMaster: チュートリアルの進行を管理するクラス
    // ==========================================
    class TutorialMaster {
    public:
        // チュートリアルの進行ステップ（詳細化）
        enum class Step {
            Msg_Welcome,    // メッセージ: ようこそ
            Msg_MoveRule,   // メッセージ: 移動量ルールの説明
            Wait_Move1,     // プレイヤー入力待ち: 普通の移動
            Msg_OpRule,     // メッセージ: 演算子の能力とペナルティの説明
            Wait_Move2,     // プレイヤー入力待ち: 移動して演算子を取る
            Msg_BattleRule, // メッセージ: バトルの発生条件と反映先の説明
            Wait_Move3,     // プレイヤー入力待ち: 敵に隣接して攻撃
            Msg_DivideRule, // メッセージ: 割り算とワープ設置の説明
            Msg_Finish      // メッセージ: 完了・タイトルへ戻る
        };

        TutorialMaster();
        ~TutorialMaster();

        void Init();
        void Update();
        void Draw() const;
        void Release();

        // BattleMasterと同じコンポーネントを独立して持つ
        MapGrid m_mapGrid;
        std::unique_ptr<Player> m_player;
        std::unique_ptr<Enemy> m_enemy;
        std::unique_ptr<BattleUI> m_ui;

    private:
        // 内部で使用するヘルパー関数
        void NextStep();
        void DrawMessageWindow(const std::string& text) const;
        void DrawHighlightGrid(const IntVector2& gridPos) const;

        Step m_currentStep;
        int m_stepTimer;     // 現在のステップが始まってからの時間（文字送りなどに使用）
        IntVector2 m_hoverGrid;

        // チュートリアル用のターゲット座標（3ステップ分）
        IntVector2 m_targetMove1;
        IntVector2 m_targetMove2;
        IntVector2 m_targetMove3;

        int m_fontMsg;
    };

} // namespace App