#pragma once

#include <string>
#include <vector>

namespace App {

    // ==========================================
    // BattleLogUI
    // 戦況ログの保持・スクロール・描画だけを担当する。
    // ==========================================
    class BattleLogUI {
    public:
        void Init();
        void AddLog(const std::string& message);
        void Scroll(int wheelDelta, float mouseX, float mouseY);
        void Draw() const;

    private:
        std::vector<std::string> m_actionLog;
        int m_logScrollOffset = 0;
    };

} // namespace App
