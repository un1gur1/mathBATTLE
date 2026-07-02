#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <DxLib.h>

#include "NetworkManager.h"

namespace App {

    NetworkManager* NetworkManager::s_instance = nullptr;

    void NetworkManager::CreateInstance() {
        if (s_instance == nullptr) {
            s_instance = new NetworkManager();
        }
    }

    NetworkManager* NetworkManager::GetInstance() {
        return s_instance;
    }

    void NetworkManager::DeleteInstance() {
        if (s_instance != nullptr) {
            delete s_instance;
            s_instance = nullptr;
        }
    }

    NetworkManager::NetworkManager()
        : m_state(State::OFFLINE)
        , m_tcpHandle(-1)
        , m_udpSocket(-1)
        , m_broadcastTimer(0)
    {
    }

    NetworkManager::~NetworkManager() {
        Release();
    }

    void NetworkManager::Init() {
        Disconnect(); // まずは全てリセット
    }

    void NetworkManager::Update() {
        // ==========================================
        // ホスト待機中の処理（UDPを叫びつつ、TCP接続を待つ）
        // ==========================================
        if (m_state == State::HOST_WAITING) {
            // 1. クライアントからのTCP接続要求が来たかチェック
            int newHandle = GetNewAcceptNetWork();
            if (newHandle != -1) {
                // 接続キター！
                m_tcpHandle = newHandle;

                // もう他の人は入れないので待機をやめる
                StopListenNetWork();
                if (m_udpSocket != -1) {
                    DeleteUDPSocket(m_udpSocket);
                    m_udpSocket = -1;
                }

                m_state = State::CONNECTED;
                // ※この後、TCPで「俺の名前」と「相手の名前」を交換する処理を入れますが、まずは接続まで
                return;
            }

            // 2. 定期的にUDPブロードキャストで「部屋があるぞ」と叫ぶ
            m_broadcastTimer++;
            if (m_broadcastTimer >= BROADCAST_INTERVAL) {
                m_broadcastTimer = 0;

                // 送信する文字列を作成 (例: "MATHBATTLE_HOST:Taro")
                std::string sendStr = "MATHBATTLE_HOST:" + m_myName;

                // ブロードキャストIP (255.255.255.255) に送信
                IPDATA broadcastIP = { 255, 255, 255, 255 };
                NetWorkSendUDP(m_udpSocket, broadcastIP, UDP_PORT, sendStr.c_str(), sendStr.length() + 1);
            }
        }
        // ==========================================
        // クライアント検索中の処理（UDPを受信してリストを作る）
        // ==========================================
        else if (m_state == State::CLIENT_SEARCHING) {
            IPDATA senderIP;
            char recvBuf[256];

            // UDPパケットが届いているかチェック（届いている分だけループで全部読む）

            while (NetWorkRecvUDP(m_udpSocket, &senderIP, nullptr, recvBuf, sizeof(recvBuf), 0) > 0) {
                std::string msg(recvBuf);

                // もし「MATHBATTLE_HOST:」から始まる魔法の言葉だったら
                if (msg.find("MATHBATTLE_HOST:") == 0) {
                    std::string hostName = msg.substr(16); // 名前部分を切り出す
                    std::string ipStr = IpToString(senderIP);

                    // リストに登録（または更新）
                    HostInfo info;
                    info.ip = senderIP;
                    info.ipString = ipStr;
                    info.playerName = hostName;
                    info.lastPingTime = GetNowCount();
                    m_hostList[ipStr] = info;
                }
            }

            // タイムアウト処理（3秒以上UDPが届かなかった部屋はリストから消す）
            int currentTime = GetNowCount();
            for (auto it = m_hostList.begin(); it != m_hostList.end(); ) {
                if (currentTime - it->second.lastPingTime > HOST_TIMEOUT_MS) {
                    it = m_hostList.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
    }

    // ホストとして部屋を立てる
    bool NetworkManager::StartHost(const std::string& playerName) {
        Disconnect(); // まずリセット

        m_myName = playerName;
        m_isHost = true;
        // 1. 本番のバトル用(TCP)の接続待ちを開始
        if (PreparationListenNetWork(TCP_PORT) == -1) {
            return false; // エラー
        }

        // 2. ブロードキャスト送信用のUDPソケットを作る（送信専用なのでポート-1で空きポートを使う）
        m_udpSocket = MakeUDPSocket(-1);
        if (m_udpSocket == -1) {
            StopListenNetWork();
            return false;
        }

        m_state = State::HOST_WAITING;
        m_broadcastTimer = 0;
        return true;
    }

    // クライアントとして部屋探しを開始する
    bool NetworkManager::StartSearch() {
        Disconnect();

        m_isHost = false;
        // 受信専用のUDPソケットを作る（ホストが送信してくるUDP_PORTを指定して待つ）
        m_udpSocket = MakeUDPSocket(UDP_PORT);
        if (m_udpSocket == -1) {
            return false;
        }

        m_hostList.clear();
        m_state = State::CLIENT_SEARCHING;
        return true;
    }

    // 見つけたホストの一覧を返す
    std::vector<HostInfo> NetworkManager::GetHostList() const {
        std::vector<HostInfo> list;
        for (const auto& pair : m_hostList) {
            list.push_back(pair.second);
        }
        return list;
    }

    // 選んだホストにTCP接続する
    bool NetworkManager::ConnectToHost(IPDATA targetIP) {
        if (m_state != State::CLIENT_SEARCHING) return false;

        // UDPの検索を止める
        if (m_udpSocket != -1) {
            DeleteUDPSocket(m_udpSocket);
            m_udpSocket = -1;
        }

        // TCPでホストに接続！
        m_tcpHandle = ConnectNetWork(targetIP, TCP_PORT);
        if (m_tcpHandle == -1) {
            m_state = State::OFFLINE;
            return false; // 接続失敗
        }

        m_state = State::CONNECTED;
        return true;
    }

    void NetworkManager::Disconnect() {
        if (m_tcpHandle != -1) {
            CloseNetWork(m_tcpHandle);
            m_tcpHandle = -1;
        }
        if (m_udpSocket != -1) {
            DeleteUDPSocket(m_udpSocket);
            m_udpSocket = -1;
        }
        StopListenNetWork(); // 念のためTCP待機も止める

        m_state = State::OFFLINE;
        m_hostList.clear();
    }

    void NetworkManager::Release() {
        Disconnect();
    }

    std::string NetworkManager::IpToString(IPDATA ip) const {
        return std::to_string(ip.d1) + "." + std::to_string(ip.d2) + "." +
            std::to_string(ip.d3) + "." + std::to_string(ip.d4);
    }

    void NetworkManager::SendSetupPacket(const SetupPacket& packet) {
        if (m_state != State::CONNECTED || m_tcpHandle == -1) return;
        // 構造体をそのままバイトデータとして送信
        NetWorkSend(m_tcpHandle, &packet, sizeof(SetupPacket));
    }

    // クライアントが設定データを受け取る
    bool NetworkManager::ReceiveSetupPacket(SetupPacket& outPacket) {
        if (m_state != State::CONNECTED || m_tcpHandle == -1) return false;

        // データが届いているか確認（届いていなければ 0 が返る）
        int dataSize = GetNetWorkDataLength(m_tcpHandle);
        if (dataSize >= sizeof(SetupPacket)) {
            // 届いていたら受け取る
            NetWorkRecv(m_tcpHandle, &outPacket, sizeof(SetupPacket));
            return true;
        }
        return false;

    }

    // バトル中の行動データを送信する
    void NetworkManager::SendBattlePacket(const BattlePacket& packet) {
        if (m_state != State::CONNECTED || m_tcpHandle == -1) return;
        NetWorkSend(m_tcpHandle, &packet, sizeof(BattlePacket));
    }

    // バトル中の行動データを受信する
    bool NetworkManager::ReceiveBattlePacket(BattlePacket& outPacket) {
        if (m_state != State::CONNECTED || m_tcpHandle == -1) return false;

        int dataSize = GetNetWorkDataLength(m_tcpHandle);
        if (dataSize >= sizeof(BattlePacket)) {
            NetWorkRecv(m_tcpHandle, &outPacket, sizeof(BattlePacket));
            return true;
        }
        return false;
    }

} // namespace App