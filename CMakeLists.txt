cmake_minimum_required(VERSION 3.10)
project(Tetris)

# MessagePack 헤더 파일 경로 제거
# include_directories(${PROJECT_SOURCE_DIR}/third_party/msgpack-c/include)

# 서버 빌드
add_subdirectory(server) 