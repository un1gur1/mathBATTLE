#pragma once

namespace App {

    enum class TitleState {
        PRESS_START,
        WARP_DIVE,
        MAIN_MENU,
        BATTLE_SETUP,
        NETWORK_SETUP,
        OPTION_MENU,
        EXIT_CONFIRM
    };

    enum class NetSetupStep {
        SELECT_ROLE,
        HOST_WAITING,
        CLIENT_SEARCHING,
        CLIENT_WAIT_SETUP
    };

} // namespace App
