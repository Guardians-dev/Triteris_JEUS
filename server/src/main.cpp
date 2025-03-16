#include "TetrisServer.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        int port = 12345;  // 기본 포트
        if (argc > 1) {
            port = std::atoi(argv[1]);
        }
        
        TetrisServer server(port);
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "오류: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 