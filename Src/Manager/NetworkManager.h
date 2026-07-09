#pragma once
#include <DxLib.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue> // ← 追加：届いたデータを一時保存するトレイ用

namespace App {

    // 見つけたホストの情報を保存する構造体
    struct HostInfo {
        IPDATA ip;
        std::string ipString;
        std::string playerName;
        int lastPingTime; // タイムアウト判定用
    };

    // ==========================================
    // パケットの種類（ID）を定義
    // ==========================================
    enum class PacketID : int {
        SETUP = 1,
        BATTLE,
        CHAT,
        STAMP
    };

    // ==========================================
    // 各種パケット構造体（先頭に必ずIDを持たせる）
    // ==========================================

    // バトル設定パケット
    struct SetupPacket {
        PacketID id = PacketID::SETUP; // ← 追加
        int modeCursor;    // 0=クラシック, 1=ゼロワン
        int stocksCursor;  // 残機 (0, 1, 2)
        int scoreCursor;   // 目標スコア (0, 1, 2)
        int stageCursor;   // ステージ (0, 1, 2)
        int p1StartNum; int p1StartX; int p1StartY;
        int p2StartNum; int p2StartX; int p2StartY;
    };

    enum class NetAction {
        MOVE,   // 移動フェーズでの操作
        ACTION  // 行動（攻撃・待機）フェーズでの操作
    };

    // バトル用パケット
    struct BattlePacket {
        PacketID id = PacketID::BATTLE; // ← 追加
        NetAction actionType;
        int targetX;
        int targetY;
    };

    // チャット用パケット（追加）
    struct ChatPacket {
        PacketID id = PacketID::CHAT;
        char message[256]; // DXLibで扱いやすい固定長配列
    };

    // 手書きスタンプ（軌跡）用パケット（追加）
    struct StampPacket {
        PacketID id = PacketID::STAMP;
        int pointCount;            // 記録した座標の数
        int pointsX[500];          // X座標の配列（最大500点）
        int pointsY[500];          // Y座標の配列
    };


    // ==========================================
    // NetworkManager: 通信マッチングと送受信を管理するシングルトン
    // ==========================================
    class NetworkManager {
    public:
        enum class State {
            OFFLINE,
            HOST_WAITING,
            CLIENT_SEARCHING,
            CONNECTED
        };

        static void CreateInstance();
        static NetworkManager* GetInstance();
        static void DeleteInstance();

        void Init();
        void Update();
        void Release();

        bool StartHost(const std::string& playerName);
        bool StartSearch();
        std::vector<HostInfo> GetHostList() const;
        bool ConnectToHost(IPDATA targetIP);
        void Disconnect();

        State GetState() const { return m_state; }
        std::string GetOpponentName() const { return m_oppName; }
        bool IsHost() const { return m_isHost; }

        // ------------------------------------------
        // パケットの送信関数
        // ------------------------------------------
        void SendSetupPacket(const SetupPacket& packet);
        void SendBattlePacket(const BattlePacket& packet);
        void SendChatPacket(const ChatPacket& packet);
        void SendStampPacket(const StampPacket& packet);

        // ------------------------------------------
        // パケットの受信関数（キューから取り出す）
        // ------------------------------------------
        bool ReceiveSetupPacket(SetupPacket& outPacket);
        bool ReceiveBattlePacket(BattlePacket& outPacket);
        bool ReceiveChatPacket(ChatPacket& outPacket);
        bool ReceiveStampPacket(StampPacket& outPacket);

    private:
        NetworkManager();
        ~NetworkManager();

        static NetworkManager* s_instance;

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

        // 受信したデータを種類ごとに貯めておくキュー（トレイ）
        std::queue<SetupPacket> m_setupQueue;
        std::queue<BattlePacket> m_battleQueue;
        std::queue<ChatPacket> m_chatQueue;
        std::queue<StampPacket> m_stampQueue;

        std::string IpToString(IPDATA ip) const;
        void ClearQueues(); // キューの中身を空にする処理
    };

} // namespace App