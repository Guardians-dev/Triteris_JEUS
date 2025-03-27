#include "NetworkManager.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <string.h> // strerror 사용을 위해 추가
#include "SimpleMessagePack.hpp"

NetworkManager::NetworkManager(int port, EventBus& bus) : eventBus(bus) {
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        throw std::runtime_error("소켓 생성 실패");
    }

    int opt = 1;
    if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("소켓 옵션 설정 실패");
    }

    sockaddr_in serverAddr{};  // zero initialization 추가
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(listenSocket);  // 실패 시 소켓 정리
        throw std::runtime_error("바인드 실패: " + std::string(strerror(errno)));  // 구체적인 오류 메시지
    }

    if (listen(listenSocket, SOMAXCONN) < 0) {
        close(listenSocket);
        throw std::runtime_error("리슨 실패");
    }
    
    setupEventHandlers();
}

// MessagePack 관련 메서드 구현
std::string NetworkManager::packMessage(const MessageData& message) {
    return SimpleMessagePack::pack(message);
}

MessageData NetworkManager::unpackMessage(const std::string& data) {
    try {
        // MessagePack 데이터 역직렬화
        msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
        msgpack::object obj = oh.get();
        
        // 디버깅을 위한 출력
        std::cout << "MessagePack 객체: " << obj << std::endl;
        
        // MessageData로 변환
        MessageData result;
        if (obj.type == msgpack::type::MAP) {
            msgpack::object_map map = obj.via.map;
            for (unsigned int i = 0; i < map.size; ++i) {
                std::string key = map.ptr[i].key.as<std::string>();
                if (map.ptr[i].val.type == msgpack::type::STR) {
                    result[key] = map.ptr[i].val.as<std::string>();
                }
                // 다른 타입들도 필요한 경우 여기에 추가
            }
        }
        
        std::cout << "변환된 MessageData: " << result.dump() << std::endl;
        return result;
    }
    catch (const std::exception& e) {
        std::cerr << "메시지 언패킹 오류: " << e.what() << std::endl;
        throw;
    }
}

void NetworkManager::setupEventHandlers() {
    // 플레이어 추가 완료 이벤트 구독
    eventBus.subscribe("player_added", [this](const Event& event) {
        int playerId = event.data["player_id"].intValue;
        
        // 먼저 연결 응답 전송
        MessageData response;
        response["type"] = "connect_response";
        response["player_id"] = playerId;
        response["status"] = "success";
        
        std::cout << "플레이어 " << playerId << "에게 연결 응답 전송" << std::endl;
        sendToPlayer(playerId, packMessage(response));
        
        // 게임 상태 요청 이벤트 발행 - 새 플레이어에게 현재 게임 상태 전송
        MessageData requestData;
        requestData["player_id"] = playerId;
        eventBus.publish("request_game_state", requestData);
    });
    
    // 게임 상태 업데이트 이벤트 구독
    eventBus.subscribe("game_state_updated", [this](const Event& event) {
        std::cout << "게임 상태 업데이트 이벤트 수신, 브로드캐스트 시작" << std::endl;
        broadcastGameState(event.data);
    });
    
    // 플레이어 정보 응답 이벤트 구독 (소켓 정보)
    eventBus.subscribe("player_info_response", [this](const Event& event) {
        std::string action = event.data["action"].stringValue;
        
        if (action == "player_socket_info") {
            int playerId = event.data["player_id"].intValue;
            int socket = event.data["socket"].intValue;
            
            if (event.data.contains("message")) {
                std::string message = event.data["message"];
                std::cout << "플레이어 " << playerId << "에게 메시지 전송" << std::endl;
                
                int bytesSent = send(socket, message.c_str(), message.length(), 0);
                if (bytesSent < 0) {
                    std::cerr << "메시지 전송 실패: " << strerror(errno) << std::endl;
                } else {
                    std::cout << "전송된 바이트: " << bytesSent << std::endl;
                }
            }
        }
        else if (action == "all_players_info") {
            if (event.data.contains("game_state") && event.data.contains("players")) {
                MessageData gameState = event.data["game_state"];
                std::string packedMsg = packMessage(gameState);
                
                std::cout << "모든 플레이어에게 게임 상태 전송" << std::endl;
                
                MessageData players = event.data["players"];
                for (auto& item : players.objectValue) {
                    const std::string& id = item.first;
                    MessageData& player = item.second;
                    int socket = player["socket"].intValue;
                    int bytesSent = send(socket, packedMsg.c_str(), packedMsg.length(), 0);
                    if (bytesSent < 0) {
                        std::cerr << "플레이어 " << id << "에게 게임 상태 전송 실패: " << strerror(errno) << std::endl;
                    } else {
                        std::cout << "플레이어 " << id << "에게 " << bytesSent << " 바이트 전송" << std::endl;
                    }
                }
            }
        }
    });
    
    // 디버깅을 위한 이벤트 구독 추가
    eventBus.subscribe("client_connected", [](const Event& event) {
        std::cout << "클라이언트 연결 이벤트 수신: 플레이어 ID " << event.data["player_id"].intValue << std::endl;
    });
    
    eventBus.subscribe("client_message_received", [](const Event& event) {
        std::cout << "클라이언트 메시지 수신: " << event.data.dump() << std::endl;
    });

    // 게임 상태 변경 이벤트 구독
    eventBus.subscribe("game_state_changed", [this](const Event& event) {
        int playerId = event.data["player_id"].intValue;
        sendToPlayer(playerId, packMessage(event.data));
    });
}

void NetworkManager::handleClientMessages(int playerId, int clientSocket) {
    char headerBuffer[4];
    
    while (true) {
        // 메시지 길이 정보(4바이트) 읽기
        int bytesRead = recv(clientSocket, headerBuffer, 4, 0);
        if (bytesRead <= 0) {
            std::cout << "플레이어 " << playerId << " 연결 종료" << std::endl;
            eventBus.publish("client_disconnected", {{"player_id", playerId}});
            close(clientSocket);
            break;
        }
        
        if (bytesRead != 4) {
            std::cerr << "헤더 읽기 실패: " << bytesRead << " 바이트만 읽음" << std::endl;
            continue;
        }
        
        // 메시지 길이 계산
        uint32_t messageSize = 
            ((uint32_t)headerBuffer[0] << 24) |
            ((uint32_t)headerBuffer[1] << 16) |
            ((uint32_t)headerBuffer[2] << 8) |
            (uint32_t)headerBuffer[3];
            
        std::cout << "수신할 메시지 크기: " << messageSize << " 바이트" << std::endl;
            
        // 메시지 본문 읽기
        std::vector<char> messageBuffer(messageSize);
        bytesRead = recv(clientSocket, messageBuffer.data(), messageSize, 0);
        
        if (bytesRead <= 0) {
            std::cout << "플레이어 " << playerId << " 연결 종료" << std::endl;
            eventBus.publish("client_disconnected", {{"player_id", playerId}});
            close(clientSocket);
            break;
        }
        
        if (bytesRead != messageSize) {
            std::cerr << "메시지 읽기 실패: " << bytesRead << "/" << messageSize << " 바이트만 읽음" << std::endl;
            continue;
        }
        
        try {
            std::string messageData(messageBuffer.data(), messageSize);
            std::cout << "수신된 원시 데이터 크기: " << messageData.size() << " 바이트" << std::endl;
            std::cout << "원시 데이터 헥사값: ";
            for (size_t i = 0; i < std::min(messageSize, (uint32_t)32); ++i) {
                printf("%02x ", (unsigned char)messageBuffer[i]);
            }
            std::cout << std::endl;
            
            MessageData msg = unpackMessage(messageData);
            std::cout << "언패킹된 데이터: " << msg.dump() << std::endl;
            
            // 메시지 타입 확인 전에 키 존재 여부 검사
            if (!msg.contains("type")) {
                std::cerr << "잘못된 메시지 형식: 'type' 필드가 없습니다." << std::endl;
                std::cerr << "전체 메시지 내용: " << msg.dump() << std::endl;
                continue;
            }

            std::string messageType = msg["type"].stringValue;
            
            // connect 타입 메시지 처리
            if (messageType == "connect") {
                std::cout << "새로운 클라이언트 연결 요청" << std::endl;
                
                // 플레이어 ID와 소켓 정보를 포함하여 이벤트 발행
                MessageData connectData;
                connectData["type"] = "player_connect";
                connectData["player_id"] = playerId;
                connectData["socket"] = clientSocket;
                if (msg.contains("nickname")) {
                    connectData["nickname"] = msg["nickname"];
                }
                
                eventBus.publish("player_connect", connectData);
                continue;
            }
            
            // 플레이어 ID 추가
            msg["player_id"] = playerId;
            
            // 클라이언트 메시지 수신 이벤트 발행
            eventBus.publish("client_message_received", msg);
        }
        catch (const std::exception& e) {
            std::cerr << "메시지 파싱 오류: " << e.what() << std::endl;
            std::cerr << "메시지 크기: " << messageSize << " 바이트" << std::endl;
            // 디버깅을 위해 원시 데이터의 첫 몇 바이트를 16진수로 출력
            std::cerr << "원시 데이터 미리보기: ";
            for (size_t i = 0; i < std::min(messageSize, (uint32_t)16); ++i) {
                std::cerr << std::hex << (int)(unsigned char)messageBuffer[i] << " ";
            }
            std::cerr << std::dec << std::endl;
        }
    }
}

void NetworkManager::sendToPlayer(int playerId, const std::string& message) {
    // 플레이어 소켓 요청 이벤트 발행
    eventBus.publish("request_player_info", {
        {"action", "get_player_socket"},
        {"player_id", playerId},
        {"message", message}
    });
}

void NetworkManager::broadcastGameState(const MessageData& gameState) {
    MessageData enhancedState = gameState;
    enhancedState["type"] = "game_state_update";
    
    std::string packedMsg = packMessage(enhancedState);
    
    std::cout << "게임 상태 브로드캐스트" << std::endl;
    
    // 플레이어 정보 요청 이벤트 발행
    eventBus.publish("request_player_info", {
        {"action", "get_all_players"},
        {"game_state", enhancedState}
    });
}

NetworkManager::~NetworkManager() {
    close(listenSocket);
}

void NetworkManager::acceptClient() {
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSocket < 0) {
        std::cout << "클라이언트 연결 수락 실패" << std::endl;
        return;
    }

    static int nextPlayerId = 1;
    int playerId = nextPlayerId++;

    // 클라이언트 연결 이벤트 발행
    eventBus.publish("client_connected", {
        {"player_id", playerId},
        {"socket", clientSocket}
    });
    printf("클라이언트 연결 이벤트 발행\n");
    // 클라이언트 메시지 처리 스레드 시작
    std::thread([this, playerId, clientSocket]() {
        this->handleClientMessages(playerId, clientSocket);
    }).detach();
} 