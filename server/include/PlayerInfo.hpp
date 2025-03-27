#pragma once

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include "Event.hpp"
#include "SimpleMessagePack.hpp"

class PlayerInfo {
private:
    EventBus& eventBus;
    std::map<int, MessageData> players;  // 플레이어 ID를 키로 하는 맵
    
    // 개별 플레이어 정보 (이전 멤버 변수들)
    int socket;
    int score;
    bool isReady;
    std::vector<std::vector<int>> board;
    std::vector<int> currentPos;
    int currentBlockType;
    MessageData currentPiece;  // 추가된 멤버 변수

public:
    PlayerInfo(EventBus& bus);
    
    // 플레이어 관리 메서드
    void addPlayer(int playerId, int socket);
    void removePlayer(int playerId);
    bool hasPlayer(int playerId) const;
    
    // 플레이어 정보 조회/설정 메서드
    int getPlayerSocket(int playerId) const;
    void setPlayerNickname(int playerId, const std::string& nickname);
    MessageData getAllPlayers() const;
    
    // 이벤트 핸들러 설정
    void setupEventHandlers();
    
    // 게터/세터 메서드 추가
    MessageData& getCurrentPiece() { return currentPiece; }
    void setCurrentPiece(const MessageData& piece) { currentPiece = piece; }
    std::vector<int>& getCurrentPos() { return currentPos; }
    void setCurrentPos(const std::vector<int>& pos) { currentPos = pos; }
    int getCurrentBlockType() const { return currentBlockType; }
    void setCurrentBlockType(int type) { currentBlockType = type; }
    std::vector<std::vector<int>>& getBoard() { return board; }
    int getScore() const { return score; }
    void setScore(int s) { score = s; }
    
    // friend 클래스 추가
    friend class GameManager;

    // 기본 생성자 추가
    PlayerInfo() : eventBus(GlobalEventBus::getInstance()), socket(-1), score(0), isReady(false) {
        board = std::vector<std::vector<int>>(20, std::vector<int>(10, 0));
        currentPos = {0, 5};
        currentBlockType = 0;
    }
}; 