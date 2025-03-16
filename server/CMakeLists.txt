cmake_minimum_required(VERSION 3.10)
project(TetrisServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 필요한 패키지 찾기
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

# 헤더 파일 경로 추가
include_directories(${PROJECT_SOURCE_DIR}/include)

# 소스 파일 목록
set(SOURCES
    src/GameManager.cpp
    src/NetworkManager.cpp
    src/PlayerInfo.cpp
    src/TetrisServer.cpp
    src/main.cpp
)

# 실행 파일 생성
add_executable(${PROJECT_NAME} ${SOURCES})

# 라이브러리 링크
target_link_libraries(${PROJECT_NAME} PRIVATE 
    nlohmann_json::nlohmann_json
    Threads::Threads
) 