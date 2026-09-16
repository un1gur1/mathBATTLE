#define NOMINMAX
#include <DxLib.h>
#include "BattleLogUI.h"
#include "BattleUICommon.h"
#include "../Utility/AppConfig.h"

#include <algorithm>

namespace App {
    using namespace Config;
    using namespace BattleUIDraw;

    void BattleLogUI::Init() {
        m_actionLog.clear();
        m_logScrollOffset = 0;
    }

    void BattleLogUI::AddLog(const std::string& message) {
        m_actionLog.push_back(message);
        if (m_actionLog.size() > 100) {
            m_actionLog.erase(m_actionLog.begin());
        }
        m_logScrollOffset = std::max(0, static_cast<int>(m_actionLog.size()) - 6);
    }

    void BattleLogUI::Scroll(int wheelDelta, float mouseX, float mouseY) {
        if (mouseX < 40 || mouseX > 540 || mouseY < LOG_PANEL_Y || mouseY > LOG_PANEL_Y + 200) {
            return;
        }

        m_logScrollOffset -= wheelDelta;
        const int maxOffset = std::max(0, static_cast<int>(m_actionLog.size()) - 6);
        m_logScrollOffset = std::clamp(m_logScrollOffset, 0, maxOffset);
    }

    void BattleLogUI::Draw() const {
        const int f22 = GetCachedFont(22);
        const int f20 = GetCachedFont(20);

        DrawCyberPanel(40, LOG_PANEL_Y, 500, 200, COL_PANEL_BG(), GetColor(100, 100, 120), 150);
        DrawStringToHandle(55, LOG_PANEL_Y + 10, "Å° êÌãµÉçÉO", COL_DISABLE(), f22);
        DrawLine(50, LOG_PANEL_Y + 35, 530, LOG_PANEL_Y + 35, GetColor(60, 60, 70), 1);

        const int maxLogOffset = std::max(0, static_cast<int>(m_actionLog.size()) - 6);
        if (maxLogOffset > 0) {
            DrawBox(520, LOG_PANEL_Y + 45, 525, LOG_PANEL_Y + 185, GetColor(30, 30, 40), TRUE);
            const float scrollRatio = static_cast<float>(m_logScrollOffset) / maxLogOffset;
            const int thumbY = LOG_PANEL_Y + 45 + static_cast<int>(scrollRatio * (140 - 30));
            DrawBox(520, thumbY, 525, thumbY + 30, GetColor(100, 150, 200), TRUE);
        }

        const int startIdx = m_logScrollOffset;
        const int endIdx = std::min(static_cast<int>(m_actionLog.size()), startIdx + 6);
        for (int i = startIdx; i < endIdx; ++i) {
            const int drawY = LOG_PANEL_Y + 45 + (i - startIdx) * 24;
            DrawFormatStringToHandle(55, drawY, GetColor(180, 220, 160), f20, "%s", m_actionLog[i].c_str());
        }
    }

} // namespace App
