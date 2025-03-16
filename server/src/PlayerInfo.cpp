#include "PlayerInfo.hpp"

PlayerInfo::PlayerInfo() : 
    socket(-1),
    score(0),
    isReady(false),
    board(20, std::vector<int>(10, 0)),
    currentPos({0, 5}),
    currentBlockType(0) {
} 