#pragma once
#include <DxLib.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace App {

    // 見つけたホストの情報を保存する構造体
    struct HostInfo {
        IPDATA ip;
        std::string ipString;
        std::string playerName;
        int lastPingTime; // タイムアウト判定用
    };

    // ==========================================
    // バトル設定パケット（ホストからクライアントへ送るデータ）
    // ==========================================
    struct SetupPacket {
        int modeCursor;    // 0=クラシック, 1=ゼロワン
        int stocksCursor;  // 残機 (0, 1, 2)
        int scoreCursor;   // 目標スコア (0, 1, 2)
        int stageCursor;   // ステージ (0, 1, 2)
        int p1StartNum; int p1StartX; int p1StartY; // 1P初期設定
        int p2StartNum; int p2StartX; int p2StartY; // 2P初期設定
    };

    // ==========================================
    // バトル用パケット（ターンごとに送受信するデータ）
    // ==========================================
    enum class NetAction {
        MOVE,   // 移動フェーズでの操作
        ACTION  // 行動（攻撃・待機）フェーズでの操作
    };

    struct BattlePacket {
        NetAction actionType;
        int targetX;  // クリックしたX座標
        int targetY;  // クリックしたY座標
    };


    // ==========================================
    // NetworkManager: 通信マッチングと送受信を管理するシングルトン
    // ==========================================
    class NetworkManager {
    public:
        // 通信の現在の状態
        enum class State {
            OFFLINE,            // オフライン（初期状態）
            HOST_WAITING,       // ホストとしてTCP待機 ＆ UDPブロードキャスト中
            CLIENT_SEARCHING,   // クライアントとしてUDP検索中
            CONNECTED           // TCP接続完了（バトル中）
        };

        // シングルトン用関数
        static void CreateInstance();
        static NetworkManager* GetInstance();
        static void DeleteInstance();

        void Init();
        void Update();
        void Release();

        // ------------------------------------------
        // マッチング用関数
        // ------------------------------------------
        bool StartHost(const std::string& playerName);
        bool StartSearch();
        std::vector<HostInfo> GetHostList() const;
        bool ConnectToHost(IPDATA targetIP);
        void Disconnect();

        // 状態取得
        State GetState() const { return m_state; }
        std::string GetOpponentName() const { return m_oppName; }

        // 自分がホスト（部屋を立てた側）かどうか
        bool IsHost() const { return m_isHost; }

        // ------------------------------------------
        // パケットの送受信関数
        // ------------------------------------------
        void SendSetupPacket(const SetupPacket& packet);
        bool ReceiveSetupPacket(SetupPacket& outPacket);

        void SendBattlePacket(const BattlePacket& packet);
        bool ReceiveBattlePacket(BattlePacket& outPacket);

    private:
        NetworkManager();
        ~NetworkManager();

        static NetworkManager* s_instance;

        // 定数群
        static constexpr int TCP_PORT = 54321;
        static constexpr int UDP_PORT = 54322;
        static constexpr int BROADCAST_INTERVAL = 60;
        static constexpr int HOST_TIMEOUT_MS = 3000;

        State m_state;
        std::string m_myName;
        std::string m_oppName;

        int m_tcpHandle;
        int m_udpSocket;
        int m_broadcastTimer;

        bool m_isHost;

        std::unordered_map<std::string, HostInfo> m_hostList;

        std::string IpToString(IPDATA ip) const;
    };

} // namespace App