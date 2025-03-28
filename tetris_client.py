import pygame
import random
import socket
import json
import threading
import sys
import time
import msgpack  # MessagePack 라이브러리 추가
import struct   # 바이너리 데이터 처리를 위한 라이브러리

# 색상 정의
COLORS = {
    "BLACK": (0, 0, 0),
    "WHITE": (255, 255, 255),
    "CYAN": (0, 255, 255),
    "YELLOW": (255, 255, 0),
    "PURPLE": (255, 0, 255),
    "GREEN": (0, 255, 0),
    "RED": (255, 0, 0),
    "BLUE": (0, 0, 255),
    "ORANGE": (255, 165, 0)
}

# 테트리스 블록 모양 정의
SHAPES = [
    [[1, 1, 1, 1]],  # I
    [[1, 1], [1, 1]],  # O
    [[1, 1, 1], [0, 1, 0]],  # T
    [[1, 1, 1], [1, 0, 0]],  # L
    [[1, 1, 1], [0, 0, 1]],  # J
    [[1, 1, 0], [0, 1, 1]],  # S
    [[0, 1, 1], [1, 1, 0]]   # Z
]

class TetrisGame:
    def __init__(self):
        pygame.init()
        
        # 게임 설정
        self.BLOCK_SIZE = 30
        self.GRID_WIDTH = 10
        self.GRID_HEIGHT = 20
        
        # 화면 크기 수정 (메인 게임 + 사이드 정보)
        self.SCREEN_WIDTH = self.BLOCK_SIZE * (self.GRID_WIDTH + 15)
        self.SCREEN_HEIGHT = self.BLOCK_SIZE * self.GRID_HEIGHT
        
        # 폰트 설정
        self.font = pygame.font.Font(None, 36)
        
        # 화면 설정
        self.screen = pygame.display.set_mode((self.SCREEN_WIDTH, self.SCREEN_HEIGHT))
        pygame.display.set_caption("테트리스")
        
        # 게임 상태
        self.board = [[0 for _ in range(self.GRID_WIDTH)] for _ in range(self.GRID_HEIGHT)]
        self.current_piece = None
        self.next_piece = None
        self.current_pos = [0, 0]
        self.game_over = False
        self.score = 0
        
        # 게임 속도
        self.clock = pygame.time.Clock()
        self.fall_time = 0
        self.fall_speed = 0.5
        
        # 네트워크 관련 초기화
        self.socket = None
        self.player_id = None
        self.other_players = {}
        
        # 색상 추가
        self.GRID_COLOR = COLORS["WHITE"]
        self.BACKGROUND_COLOR = (40, 40, 40)
        
        # 블록 색상 정의
        self.BLOCK_COLORS = {
            0: self.BACKGROUND_COLOR,  # 빈 공간
            1: COLORS["CYAN"],     # I 블록
            2: COLORS["YELLOW"],   # O 블록
            3: COLORS["PURPLE"],   # T 블록
            4: COLORS["ORANGE"],   # L 블록
            5: COLORS["BLUE"],     # J 블록
            6: COLORS["GREEN"],    # S 블록
            7: COLORS["RED"]       # Z 블록
        }
        
        # 서버 연결 시도
        try:
            self.connect_to_server()
            if not self.socket or not self.player_id:  # 연결 실패 확인
                print("서버 연결 실패")
                self.quit_game()
                return
        except Exception as e:
            print(f"서버 연결 실패: {e}")
            self.quit_game()
            return

    def quit_game(self):
        pygame.quit()
        sys.exit()

    def new_piece(self):
        # 다음 블록이 없으면 생성
        if self.next_piece is None:
            self.next_piece = random.choice(SHAPES)
        
        # 현재 블록을 다음 블록으로 설정
        self.current_piece = self.next_piece
        self.current_pos = [0, self.GRID_WIDTH//2 - len(self.current_piece[0])//2]
        
        # 충돌 체크 후 서버에 상태 전송
        if not self.valid_move(self.current_piece, self.current_pos):
            self.send_to_server({
                "type": "check_game_over",
                "board": self.board,
                "current_piece": self.current_piece,
                "position": self.current_pos
            })
        
        # 새로운 다음 블록 생성
        self.next_piece = random.choice(SHAPES)

    def draw_board(self):
        # 배경 그리드 그리기
        for y in range(self.GRID_HEIGHT):
            for x in range(self.GRID_WIDTH):
                pygame.draw.rect(self.screen, self.GRID_COLOR,
                              (x * self.BLOCK_SIZE, 
                               y * self.BLOCK_SIZE,
                               self.BLOCK_SIZE, 
                               self.BLOCK_SIZE), 1)

        # 블록 그리기
        for y in range(self.GRID_HEIGHT):
            for x in range(self.GRID_WIDTH):
                if self.board[y][x] > 0:  # 0이 아닌 값은 블록
                    color = self.BLOCK_COLORS[self.board[y][x]]
                    pygame.draw.rect(self.screen, color,
                                  (x * self.BLOCK_SIZE + 1, 
                                   y * self.BLOCK_SIZE + 1,
                                   self.BLOCK_SIZE - 2, 
                                   self.BLOCK_SIZE - 2))

    def draw_current_piece(self):
        if self.current_piece and "block_type" in self.current_piece:
            color = self.BLOCK_COLORS[self.current_piece["block_type"]]
            for y, row in enumerate(self.current_piece["shape"]):
                for x, cell in enumerate(row):
                    if cell:
                        pygame.draw.rect(self.screen, color,
                                      ((self.current_pos[1] + x) * self.BLOCK_SIZE + 1,
                                       (self.current_pos[0] + y) * self.BLOCK_SIZE + 1,
                                       self.BLOCK_SIZE - 2,
                                       self.BLOCK_SIZE - 2))

    def draw_next_piece(self):
        # 다음 블록 미리보기 그리기
        next_piece_x = (self.GRID_WIDTH + 2) * self.BLOCK_SIZE
        next_piece_y = 2 * self.BLOCK_SIZE
        
        # "NEXT" 텍스트 표시
        next_text = self.font.render("NEXT", True, COLORS["WHITE"])
        self.screen.blit(next_text, (next_piece_x, self.BLOCK_SIZE))
        
        # 다음 블록 그리기
        if self.next_piece:
            for y, row in enumerate(self.next_piece):
                for x, cell in enumerate(row):
                    if cell:
                        pygame.draw.rect(self.screen, COLORS["YELLOW"],
                                      (next_piece_x + x * self.BLOCK_SIZE,
                                       next_piece_y + y * self.BLOCK_SIZE,
                                       self.BLOCK_SIZE - 1,
                                       self.BLOCK_SIZE - 1))

    def draw_score(self):
        # 점수 표시 위치와 크기 조정
        score_x = self.GRID_WIDTH * self.BLOCK_SIZE + 50
        score_y = self.SCREEN_HEIGHT // 2
        score_text = self.font.render(f"P{self.player_id}: {self.score}", True, COLORS["WHITE"])
        self.screen.blit(score_text, (score_x, score_y))

    def draw_other_players(self):
        offset_x = (self.GRID_WIDTH + 2) * self.BLOCK_SIZE
        offset_y = 8 * self.BLOCK_SIZE
        mini_block_size = self.BLOCK_SIZE // 2

        for idx, (player_id, state) in enumerate(self.other_players.items()):
            if str(player_id) != str(self.player_id):
                # 플레이어 라벨
                player_text = self.font.render(f"P{player_id}", True, COLORS["WHITE"])
                self.screen.blit(player_text, (offset_x + idx * 120, offset_y - 30))

                # 보드 배경과 테두리
                board_rect = pygame.Rect(
                    offset_x + idx * 120,
                    offset_y,
                    mini_block_size * self.GRID_WIDTH,
                    mini_block_size * self.GRID_HEIGHT
                )
                pygame.draw.rect(self.screen, self.BACKGROUND_COLOR, board_rect)
                pygame.draw.rect(self.screen, COLORS["WHITE"], board_rect, 2)

                # 보드 상태 표시 (고정된 블록들)
                if "board" in state:
                    for y, row in enumerate(state["board"]):
                        for x, cell in enumerate(row):
                            if cell:  # cell이 0이 아닌 경우에만 그리기
                                color = self.BLOCK_COLORS[cell]  # 블록 타입에 따른 색상 사용
                                pygame.draw.rect(self.screen, color,
                                    (offset_x + idx * 120 + x * mini_block_size + 1,
                                     offset_y + y * mini_block_size + 1,
                                     mini_block_size - 2,
                                     mini_block_size - 2))

                # 현재 움직이는 블록 표시
                if ("current_piece" in state and "position" in state and 
                    state["current_piece"] is not None and state["position"] is not None):
                    piece = state["current_piece"]
                    pos = state["position"]
                    try:
                        color = self.BLOCK_COLORS[piece["block_type"]]  # 블록 타입에 따른 색상
                        for y, row in enumerate(piece["shape"]):
                            for x, cell in enumerate(row):
                                if cell:
                                    pygame.draw.rect(self.screen, color,
                                        (offset_x + idx * 120 + (pos[1] + x) * mini_block_size + 1,
                                         offset_y + (pos[0] + y) * mini_block_size + 1,
                                         mini_block_size - 2,
                                         mini_block_size - 2))
                    except Exception as e:
                        print(f"블록 그리기 오류: {e}")

    def run(self):
        last_fall_time = pygame.time.get_ticks()
        
        # 서버에 새 블록 요청
        self.send_to_server({"type": "request_new_piece"})
        
        while not self.game_over:
            current_time = pygame.time.get_ticks()
            delta_time = (current_time - last_fall_time) / 1000.0
            
            # 이벤트 처리
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.game_over = True
                
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_LEFT:
                        self.send_to_server({
                            "type": "move_request",
                            "direction": -1
                        })
                    elif event.key == pygame.K_RIGHT:
                        self.send_to_server({
                            "type": "move_request",
                            "direction": 1
                        })
                    elif event.key == pygame.K_DOWN:
                        self.send_to_server({
                            "type": "move_down_request"
                        })
                    elif event.key == pygame.K_UP:
                        self.send_to_server({
                            "type": "rotate_request"
                        })
                    elif event.key == pygame.K_SPACE:
                        self.send_to_server({
                            "type": "hard_drop_request"
                        })
            
            # 일정 시간마다 블록 떨어뜨리기
            if delta_time > self.fall_speed:
                self.send_to_server({
                    "type": "move_down_request"
                })
                last_fall_time = current_time
            
            # 화면 업데이트
            self.screen.fill(self.BACKGROUND_COLOR)
            self.draw_board()
            self.draw_current_piece()
            self.draw_next_piece()
            self.draw_score()
            self.draw_other_players()
            
            pygame.display.update()
            self.clock.tick(60)

    def valid_move(self, piece, pos):
        # 이동 가능 여부 확인
        for y, row in enumerate(piece):
            for x, cell in enumerate(row):
                if cell:
                    if (pos[0] + y >= self.GRID_HEIGHT or
                        pos[1] + x < 0 or
                        pos[1] + x >= self.GRID_WIDTH or
                        self.board[pos[0] + y][pos[1] + x]):
                        return False
        return True

    def freeze_piece(self):
        # 현재 조각을 보드에 고정
        for y, row in enumerate(self.current_piece):
            for x, cell in enumerate(row):
                if cell:
                    self.board[self.current_pos[0] + y][self.current_pos[1] + x] = 1
        
        # 완성된 줄 확인 및 제거
        self.clear_lines()

    def clear_lines(self):
        # 완성된 줄 확인
        lines_to_clear = []
        for y in range(self.GRID_HEIGHT):
            if all(self.board[y]):  # 한 줄이 모두 채워졌는지 확인
                lines_to_clear.append(y)
        
        if lines_to_clear:  # 지울 줄이 있다면
            # 로컬에서 줄 제거 및 점수 추가
            for line in sorted(lines_to_clear, reverse=True):
                del self.board[line]
                self.board.insert(0, [0 for _ in range(self.GRID_WIDTH)])
                self.score += 100
            
            # 서버에 알림 (줄 위치 정보와 업데이트된 보드 상태 포함)
            self.send_to_server({
                "type": "line_clear",
                "lines": len(lines_to_clear),
                "positions": lines_to_clear,
                "board": self.board,  # 현재 보드 상태도 함께 전송
                "score": self.score
            })

    def pack_message(self, data):
        """데이터를 MessagePack 형식으로 변환"""
        packed_data = msgpack.packb(data)
        # 메시지 길이 정보 추가 (4바이트)
        header = struct.pack(">I", len(packed_data))
        return header + packed_data
    
    def unpack_message(self, data):
        """MessagePack 형식의 데이터를 파이썬 객체로 변환"""
        try:
            result = msgpack.unpackb(data, raw=False, use_list=False)
            print(f"언패킹된 메시지: {result}")  # 디버깅용
            return result
        except Exception as e:
            print(f"메시지 언패킹 오류: {e}")
            raise

    def connect_to_server(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            print(f"서버({('localhost', 12345)})에 연결 시도 중...")
            self.socket.connect(('localhost', 12345))
            print("서버에 연결됨, 초기 메시지 전송 중...")
            
            # 초기 연결 메시지 전송 - send_message 함수 사용
            self.send_message({
                "type": "connect",
                "nickname": f"Player{random.randint(1000, 9999)}"
            })
            
            # 타임아웃 설정 (10초)
            self.socket.settimeout(10)
            
            # 서버로부터 응답 직접 받기 (동기식)
            print("서버 응답 대기 중...")
            
            try:
                # 메시지 길이 정보(4바이트) 읽기
                header = self.socket.recv(4)
                if not header or len(header) != 4:
                    raise ConnectionError("서버로부터 응답 헤더를 받지 못했습니다")
                
                # 메시지 길이 계산
                message_size = struct.unpack(">I", header)[0]
                print(f"수신할 메시지 크기: {message_size} 바이트")
                
                # 메시지 본문 읽기
                message_data = b""
                remaining = message_size
                
                while remaining > 0:
                    chunk = self.socket.recv(min(4096, remaining))
                    if not chunk:
                        raise ConnectionError("서버와의 연결이 종료되었습니다")
                    message_data += chunk
                    remaining -= len(chunk)
                
                # MessagePack 데이터 파싱
                response = self.unpack_message(message_data)
                print(f"서버 응답: {response}")
                
                # 응답 처리
                if response.get("type") == "connect_response":
                    self.player_id = response.get("player_id")
                    print(f"플레이어 ID 설정됨: {self.player_id}")
                    
                    # 타임아웃 제거 및 메시지 수신 스레드 시작
                    self.socket.settimeout(None)
                    self.receiver_thread = threading.Thread(target=self.handle_server_messages, daemon=True)
                    self.receiver_thread.start()
                else:
                    raise ValueError(f"예상치 못한 응답 타입: {response.get('type')}")
                
            except socket.timeout:
                print("서버 응답 대기 중 타임아웃 발생")
                raise TimeoutError("서버 응답 시간 초과 (10초)")
                
        except Exception as e:
            print(f"서버 연결 중 오류 발생: {e}")
            if self.socket:
                self.socket.close()
                self.socket = None
            raise e

    def send_to_server(self, data):
        try:
            # send_message 함수 사용
            self.send_message(data)
        except Exception as e:
            print(f"서버 통신 오류: {e}")

    def handle_server_messages(self):
        while True:
            try:
                # 메시지 길이 정보(4바이트) 읽기
                header = self.socket.recv(4)
                if not header:
                    print("서버와의 연결이 종료되었습니다.")
                    self.game_over = True
                    break
                
                if len(header) != 4:
                    print(f"불완전한 헤더 수신: {len(header)} 바이트")
                    continue
                
                # 메시지 길이 계산
                message_size = struct.unpack(">I", header)[0]
                print(f"수신할 메시지 크기: {message_size} 바이트")
                
                # 메시지 본문 읽기
                message_data = b""
                remaining = message_size
                
                while remaining > 0:
                    chunk = self.socket.recv(min(4096, remaining))
                    if not chunk:
                        print("서버와의 연결이 종료되었습니다.")
                        self.game_over = True
                        break
                    message_data += chunk
                    remaining -= len(chunk)
                
                if remaining > 0:
                    # 메시지를 완전히 받지 못함
                    continue
                
                # MessagePack 데이터 파싱
                try:
                    parsed_message = self.unpack_message(message_data)
                    print(f"처리할 메시지: {parsed_message}")  # 디버깅용
                    self.process_server_message(parsed_message)
                except Exception as e:
                    print(f"메시지 파싱 오류: {e}, 데이터 길이: {len(message_data)}")
                    
            except socket.error as e:
                print(f"소켓 오류: {e}")
                self.game_over = True
                break
            except Exception as e:
                print(f"메시지 수신 중 오류: {e}")
                self.game_over = True
                break

    def process_server_message(self, data):
        try:
            message_type = data.get("type")
            print(f"메시지 타입: {message_type}")  # 디버깅용
            
            if message_type == "connect_response":
                self.player_id = data.get("player_id")
                print(f"플레이어 ID 설정됨: {self.player_id}")
                
            elif message_type == "game_state_update" or message_type == "game_state":
                if "players" in data:
                    player_data = data["players"].get(str(self.player_id))
                    if player_data:
                        # 게임 상태 업데이트
                        self.board = player_data.get("board", self.board)
                        self.current_piece = player_data.get("current_piece")
                        self.current_pos = player_data.get("position")
                        self.score = player_data.get("score", self.score)
                    
                    # 다른 플레이어 상태 업데이트
                    self.other_players = {
                        k: v for k, v in data["players"].items() 
                        if str(k) != str(self.player_id)
                    }
                    
            elif message_type == "game_over":
                if str(data.get("player_id")) == str(self.player_id):
                    self.game_over = True
                
            elif message_type == "error":
                print(f"서버 오류: {data.get('message', '알 수 없는 오류')}")
            
        except Exception as e:
            print(f"메시지 처리 중 오류: {e}, 데이터: {data}")

    def send_message(self, message):
        try:
            print(f"전송할 메시지: {message}")
            
            # MessagePack으로 직렬화
            packed_msg = msgpack.packb(message, use_bin_type=True)
            msg_len = len(packed_msg)
            
            # 직렬화된 데이터 확인
            print(f"직렬화된 메시지 크기: {msg_len} 바이트")
            print("직렬화된 데이터 헥사값:", ' '.join(f'{b:02x}' for b in packed_msg))
            
            # 헤더(4바이트) + 메시지 전송
            header = struct.pack(">I", msg_len)
            full_message = header + packed_msg
            
            print(f"전체 메시지 크기: {len(full_message)} 바이트")
            print("전송할 헤더 헥사값:", ' '.join(f'{b:02x}' for b in header))
            
            # 전송
            self.socket.sendall(full_message)
            print(f"메시지 전송 완료")
            
        except Exception as e:
            print(f"메시지 전송 중 오류 발생: {e}")
    
    def handle_key_press(self, key):
        if key == pygame.K_LEFT:
            self.send_message({"type": "move_left"})
        elif key == pygame.K_RIGHT:
            self.send_message({"type": "move_right"})
        elif key == pygame.K_UP:
            self.send_message({"type": "rotate"})
        elif key == pygame.K_DOWN:
            self.send_message({"type": "move_down"})
        elif key == pygame.K_SPACE:
            self.send_message({"type": "hard_drop"})

if __name__ == "__main__":
    game = TetrisGame()
    if game.socket and game.player_id:  # 서버 연결이 성공한 경우에만 게임 실행
        game.run() 