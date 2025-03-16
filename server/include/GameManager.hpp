#pragma once
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include "PlayerInfo.hpp"

using json = nlohmann::json;
using namespace std;

class GameManager {
private:
    static const int GRID_WIDTH = 10;
    static const int GRID_HEIGHT = 20;
    map<int, PlayerInfo> players;
    bool gameStarted;

public:
    GameManager();
    void addPlayer(int playerId, int socket);
    void removePlayer(int playerId);
    void handleNewPiece(int playerId);
    void handleMove(int playerId, int direction);
    void handleRotate(int playerId);
    void handleHardDrop(int playerId);
    void handleMoveDown(int playerId);
    bool isValidMove(const vector<vector<int>>& board, const json& piece, const vector<int>& pos);
    void freezePiece(int playerId);
    void checkLines(int playerId);
    json getGameState() const;
    PlayerInfo& getPlayer(int playerId);
    const map<int, PlayerInfo>& getPlayers() const { return players; }

private:
    pair<json, int> generateNewPiece();
    json rotatePiece(const json& piece);
}; 