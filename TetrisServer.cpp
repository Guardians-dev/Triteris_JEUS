class TetrisServer {
private:
    static const int MAX_PLAYERS = 3;
    struct PlayerInfo {
        SOCKET socket;
        std::string nickname;
        int score;
        std::vector<std::vector<int>> board;
        bool isReady;
    };

    std::map<int, PlayerInfo> players;
    bool gameStarted;
    
public:
    TetrisServer(int port) {
        // 서버 초기화 코드
    }

    void handleClientMessage(int playerId, const std::string& message) {
        // JSON 형식으로 클라이언트와 통신
        json msg = json::parse(message);
        
        switch(msg["type"].get<std::string>()) {
            case "move":
                broadcastMove(playerId, msg);
                break;
            case "score_update":
                updateScore(playerId, msg["score"]);
                break;
            case "line_clear":
                handleLineClear(playerId, msg);
                break;
            case "game_over":
                handleGameOver(playerId);
                break;
        }
    }

private:
    void broadcastGameState() {
        // 모든 플레이어에게 게임 상태 전송
        json gameState;
        for (const auto& [id, player] : players) {
            gameState["players"][id] = {
                {"score", player.score},
                {"board", player.board},
                {"nickname", player.nickname}
            };
        }
        broadcastToAll(gameState.dump());
    }

    void handleLineClear(int playerId, const json& msg) {
        int linesCleared = msg["lines"];
        
        // 플레이어 점수 업데이트
        players[playerId].score += linesCleared * 100;
        
        // 다른 플레이어들에게 공격 전송
        for (auto& [id, player] : players) {
            if (id != playerId) {  // 자신 제외
                json attack;
                attack["type"] = "attack";
                attack["lines"] = linesCleared - 1;  // 1줄은 공격에서 제외
                sendToPlayer(id, attack.dump());
            }
        }
        
        // 전체 게임 상태 업데이트
        broadcastGameState();
    }
}; 