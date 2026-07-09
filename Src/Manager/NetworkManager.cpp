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
        , m_isHost(false)
    {
    }

    NetworkManager::~NetworkManager() {
        Release();
    }

    void NetworkManager::Init() {
        Disconnect();
    }

    void NetworkManager::ClearQueues() {
        std::queue<SetupPacket>().swap(m_setupQueue);
        std::queue<BattlePacket>().swap(m_battleQueue);
        std::queue<ChatPacket>().swap(m_chatQueue);
        std::queue<StampPacket>().swap(m_stampQueue);
    }

    void NetworkManager::Update() {
        // ==========================================
        // ホスト待機中の処理（UDPブロードキャスト ＆ TCP接続待ち）
        // ==========================================
        if (m_state == State::HOST_WAITING) {
            int newHandle = GetNewAcceptNetWork();
            if (newHandle != -1) {
                m_tcpHandle = newHandle;
                StopListenNetWork();
                if (m_udpSocket != -1) {
                    DeleteUDPSocket(m_udpSocket);
                    m_udpSocket = -1;
                }
                m_state = State::CONNECTED;
                return;
            }

            m_broadcastTimer++;
            if (m_broadcastTimer >= BROADCAST_INTERVAL) {
                m_broadcastTimer = 0;
                std::string sendStr = "MATHBATTLE_HOST:" + m_myName;
                IPDATA broadcastIP = { 255, 255, 255, 255 };
                NetWorkSendUDP(m_udpSocket, broadcastIP, UDP_PORT, sendStr.c_str(), sendStr.length() + 1);
            }
        }
        // ==========================================
        // クライアント検索中の処理
        // ==========================================
        else if (m_state == State::CLIENT_SEARCHING) {
            IPDATA senderIP;
            char recvBuf[256];

            while (NetWorkRecvUDP(m_udpSocket, &senderIP, nullptr, recvBuf, sizeof(recvBuf), 0) > 0) {
                std::string msg(recvBuf);
                if (msg.find("MATHBATTLE_HOST:") == 0) {
                    std::string hostName = msg.substr(16);
                    std::string ipStr = IpToString(senderIP);

                    HostInfo info;
                    info.ip = senderIP;
                    info.ipString = ipStr;
                    info.playerName = hostName;
                    info.lastPingTime = GetNowCount();
                    m_hostList[ipStr] = info;
                }
            }

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
        // ==========================================
        // TCP接続中（バトル中）の受信・仕分け処理
        // ==========================================
        else if (m_state == State::CONNECTED && m_tcpHandle != -1) {
            while (true) {
                int dataSize = GetNetWorkDataLength(m_tcpHandle);

                // パケットID（4バイト）すら届いていない場合は抜ける
                if (dataSize < sizeof(PacketID)) break;

                PacketID peekId;
                // 先頭のパケットIDだけを「覗き見（Peek）」する
                NetWorkRecvToPeek(m_tcpHandle, &peekId, sizeof(PacketID));

                bool packetReceived = false;

                // IDに応じて、必要なデータ量が届いていたら受信してキューに入れる
                switch (peekId) {
                case PacketID::SETUP:
                    if (dataSize >= sizeof(SetupPacket)) {
                        SetupPacket packet;
                        NetWorkRecv(m_tcpHandle, &packet, sizeof(SetupPacket));
                        m_setupQueue.push(packet);
                        packetReceived = true;
                    }
                    break;

                case PacketID::BATTLE:
                    if (dataSize >= sizeof(BattlePacket)) {
                        BattlePacket packet;
                        NetWorkRecv(m_tcpHandle, &packet, sizeof(BattlePacket));
                        m_battleQueue.push(packet);
                        packetReceived = true;
                    }
                    break;

                case PacketID::CHAT:
                    if (dataSize >= sizeof(ChatPacket)) {
                        ChatPacket packet;
                        NetWorkRecv(m_tcpHandle, &packet, sizeof(ChatPacket));
                        m_chatQueue.push(packet);
                        packetReceived = true;
                    }
                    break;

                case PacketID::STAMP:
                    if (dataSize >= sizeof(StampPacket)) {
                        StampPacket packet;
                        NetWorkRecv(m_tcpHandle, &packet, sizeof(StampPacket));
                        m_stampQueue.push(packet);
                        packetReceived = true;
                    }
                    break;

                default:
                    // 未知のパケットIDが来た場合のフェイルセーフ（エラー回避）
                    // 異常なデータを1バイトずつ捨てて復帰を試みる
                    char dump;
                    NetWorkRecv(m_tcpHandle, &dump, 1);
                    packetReceived = true;
                    break;
                }

                // データが途中までしか届いていない場合は、次のフレームで続きを待つ
                if (!packetReceived) break;
            }
        }
    }

    // ------------------------------------------
    // ホスト・クライアントの開始・切断
    // ------------------------------------------
    bool NetworkManager::StartHost(const std::string& playerName) {
        Disconnect();
        m_myName = playerName;
        m_isHost = true;
        if (PreparationListenNetWork(TCP_PORT) == -1) return false;

        m_udpSocket = MakeUDPSocket(-1);
        if (m_udpSocket == -1) {
            StopListenNetWork();
            return false;
        }

        m_state = State::HOST_WAITING;
        m_broadcastTimer = 0;
        return true;
    }

    bool NetworkManager::StartSearch() {
        Disconnect();
        m_isHost = false;
        m_udpSocket = MakeUDPSocket(UDP_PORT);
        if (m_udpSocket == -1) return false;

        m_hostList.clear();
        m_state = State::CLIENT_SEARCHING;
        return true;
    }

    std::vector<HostInfo> NetworkManager::GetHostList() const {
        std::vector<HostInfo> list;
        for (const auto& pair : m_hostList) list.push_back(pair.second);
        return list;
    }

    bool NetworkManager::ConnectToHost(IPDATA targetIP) {
        if (m_state != State::CLIENT_SEARCHING) return false;

        if (m_udpSocket != -1) {
            DeleteUDPSocket(m_udpSocket);
            m_udpSocket = -1;
        }

        m_tcpHandle = ConnectNetWork(targetIP, TCP_PORT);
        if (m_tcpHandle == -1) {
            m_state = State::OFFLINE;
            return false;
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
        StopListenNetWork();

        m_state = State::OFFLINE;
        m_hostList.clear();
        ClearQueues(); // 切断時にキューも綺麗にする
    }

    void NetworkManager::Release() {
        Disconnect();
    }

    std::string NetworkManager::IpToString(IPDATA ip) const {
        return std::to_string(ip.d1) + "." + std::to_string(ip.d2) + "." +
            std::to_string(ip.d3) + "." + std::to_string(ip.d4);
    }

    // ------------------------------------------
    // パケットの送信
    // ------------------------------------------
    void NetworkManager::SendSetupPacket(const SetupPacket& packet) {
        if (m_state != State::CONNECTED || m_tcpHandle == -1) return;
        NetWorkSend(m_tcpHandle, &packet, sizeof(SetupPacket));
    }

    void NetworkManager::SendBattlePacket(const BattlePacket& packet) {
        if (m_state != State::CONNECTED || m_tcpHandle == -1) return;
        NetWorkSend(m_tcpHandle, &packet, sizeof(BattlePacket));
    }

    void NetworkManager::SendChatPacket(const ChatPacket& packet) {
        if (m_state != State::CONNECTED || m_tcpHandle == -1) return;
        NetWorkSend(m_tcpHandle, &packet, sizeof(ChatPacket));
    }

    void NetworkManager::SendStampPacket(const StampPacket& packet) {
        if (m_state != State::CONNECTED || m_tcpHandle == -1) return;
        NetWorkSend(m_tcpHandle, &packet, sizeof(StampPacket));
    }

    // ------------------------------------------
    // パケットの受信（キューから取り出す）
    // ------------------------------------------
    bool NetworkManager::ReceiveSetupPacket(SetupPacket& outPacket) {
        if (m_setupQueue.empty()) return false;
        outPacket = m_setupQueue.front();
        m_setupQueue.pop();
        return true;
    }

    bool NetworkManager::ReceiveBattlePacket(BattlePacket& outPacket) {
        if (m_battleQueue.empty()) return false;
        outPacket = m_battleQueue.front();
        m_battleQueue.pop();
        return true;
    }

    bool NetworkManager::ReceiveChatPacket(ChatPacket& outPacket) {
        if (m_chatQueue.empty()) return false;
        outPacket = m_chatQueue.front();
        m_chatQueue.pop();
        return true;
    }

    bool NetworkManager::ReceiveStampPacket(StampPacket& outPacket) {
        if (m_stampQueue.empty()) return false;
        outPacket = m_stampQueue.front();
        m_stampQueue.pop();
        return true;
    }

} // namespace App