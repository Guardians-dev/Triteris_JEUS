#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct PlayerInfo {
    int socket;
    std::string nickname;
    int score;
    std::vector<std::vector<int>> board;
    bool isReady;
    json currentPiece;
    std::vector<int> currentPos;
    int currentBlockType;

    PlayerInfo();
}; 