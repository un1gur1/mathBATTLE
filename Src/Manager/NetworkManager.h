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
        int lastPingTime; // タイムアウト（リストから消す）判定用
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

        static void CreateInstance();
        static NetworkManager* GetInstance();
        static void DeleteInstance();

        void Init();
        void Update();
        void Release();

        // ------------------------------------------
        // マッチング用関数
        // ------------------------------------------
        // [ホスト側] 名前をつけて部屋を立てる
        bool StartHost(const std::string& playerName);

        // [クライアント側] 部屋の検索を開始する
        bool StartSearch();

        // [クライアント側] 見つけたホストの一覧を取得する
        std::vector<HostInfo> GetHostList() const;

        // [クライアント側] 選択したホストにTCP接続する
        bool ConnectToHost(IPDATA targetIP);

        // 通信を切断し、オフラインに戻る
        void Disconnect();

        // 現在の状態を取得
        State GetState() const { return m_state; }

        // 相手の名前を取得
        std::string GetOpponentName() const { return m_oppName; }

    private:
        NetworkManager();
        ~NetworkManager();

        static NetworkManager* s_instance;

        // 定数群
        static constexpr int TCP_PORT = 54321; // バトル用TCPポート
        static constexpr int UDP_PORT = 54322; // マッチング用UDPポート
        static constexpr int BROADCAST_INTERVAL = 60; // UDP送信間隔（フレーム）
        static constexpr int HOST_TIMEOUT_MS = 3000;  // 3秒UDPが来なければリストから消す

        State m_state;
        std::string m_myName;
        std::string m_oppName;

        int m_tcpHandle; // TCP通信のハンドル
        int m_udpSocket; // UDP検索/送信のソケット

        int m_broadcastTimer; // UDP送信のタイマー

        // 見つけたホストのリスト (IPアドレス文字列をキーにする)
        std::unordered_map<std::string, HostInfo> m_hostList;

        // ヘルパー関数
        std::string IpToString(IPDATA ip) const;
    };

} // namespace App