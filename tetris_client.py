import pygame
import random
import socket
import json
import threading

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
        self.SCREEN_WIDTH = self.BLOCK_SIZE * (self.GRID_WIDTH + 15)  # 추가 공간 늘림
        self.SCREEN_HEIGHT = self.BLOCK_SIZE * self.GRID_HEIGHT
        
        # 폰트 설정
        self.font = pygame.font.Font(None, 36)
        
        # 화면 설정
        self.screen = pygame.display.set_mode((self.SCREEN_WIDTH, self.SCREEN_HEIGHT))
        pygame.display.set_caption("테트리스")
        
        # 게임 상태
        self.board = [[0 for _ in range(self.GRID_WIDTH)] for _ in range(self.GRID_HEIGHT)]
        self.current_piece = None
        self.next_piece = None  # 다음 블록 저장
        self.current_pos = [0, 0]
        self.game_over = False
        self.score = 0
        
        # 게임 속도
        self.clock = pygame.time.Clock()
        self.fall_time = 0
        self.fall_speed = 0.5

    def new_piece(self):
        # 다음 블록이 없으면 생성
        if self.next_piece is None:
            self.next_piece = random.choice(SHAPES)
        
        # 현재 블록을 다음 블록으로 설정
        self.current_piece = self.next_piece
        self.current_pos = [0, self.GRID_WIDTH//2 - len(self.current_piece[0])//2]
        
        # 새로운 다음 블록 생성
        self.next_piece = random.choice(SHAPES)

    def draw_board(self):
        # 게임 보드 그리기
        for y in range(self.GRID_HEIGHT):
            for x in range(self.GRID_WIDTH):
                if self.board[y][x]:
                    pygame.draw.rect(self.screen, COLORS["WHITE"],
                                  (x * self.BLOCK_SIZE, 
                                   y * self.BLOCK_SIZE,
                                   self.BLOCK_SIZE - 1, 
                                   self.BLOCK_SIZE - 1))
                    
    def draw_current_piece(self):
        # 현재 떨어지는 블록 그리기
        if self.current_piece:
            for y, row in enumerate(self.current_piece):
                for x, cell in enumerate(row):
                    if cell:
                        pygame.draw.rect(self.screen, COLORS["CYAN"],
                                      ((self.current_pos[1] + x) * self.BLOCK_SIZE,
                                       (self.current_pos[0] + y) * self.BLOCK_SIZE,
                                       self.BLOCK_SIZE - 1,
                                       self.BLOCK_SIZE - 1))
                                       
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
        # 점수 표시
        score_x = (self.GRID_WIDTH + 2) * self.BLOCK_SIZE
        score_y = 8 * self.BLOCK_SIZE
        
        score_text = self.font.render(f"SCORE", True, COLORS["WHITE"])
        score_value = self.font.render(f"{self.score}", True, COLORS["WHITE"])
        
        self.screen.blit(score_text, (score_x, score_y))
        self.screen.blit(score_value, (score_x, score_y + 40))

    def draw_other_player(self):
        # 다른 플레이어 화면 (예시)
        other_x = (self.GRID_WIDTH + 2) * self.BLOCK_SIZE
        other_y = 12 * self.BLOCK_SIZE
        
        # 다른 플레이어 영역 테두리
        pygame.draw.rect(self.screen, COLORS["WHITE"],
                      (other_x, other_y,
                       6 * self.BLOCK_SIZE,
                       8 * self.BLOCK_SIZE), 1)
        
        player_text = self.font.render("P2", True, COLORS["WHITE"])
        self.screen.blit(player_text, (other_x, other_y - 30))

    def run(self):
        self.new_piece()
        last_fall_time = pygame.time.get_ticks()
        
        while not self.game_over:
            current_time = pygame.time.get_ticks()
            delta_time = (current_time - last_fall_time) / 1000.0  # 초 단위로 변환

            # 이벤트 처리
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.game_over = True
                    
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_LEFT:
                        self.move(-1)
                    elif event.key == pygame.K_RIGHT:
                        self.move(1)
                    elif event.key == pygame.K_DOWN:
                        self.move_down()
                    elif event.key == pygame.K_UP:
                        self.rotate()
                    elif event.key == pygame.K_SPACE:  # 스페이스바로 즉시 떨어뜨리기
                        self.hard_drop()
            
            # 일정 시간마다 블록 떨어뜨리기
            if delta_time > self.fall_speed:
                self.move_down()
                last_fall_time = current_time
            
            # 화면 업데이트
            self.screen.fill(COLORS["BLACK"])
            self.draw_board()
            self.draw_current_piece()
            self.draw_next_piece()
            self.draw_score()
            self.draw_other_player()
            pygame.display.update()
            
            # 게임 속도 조절
            self.clock.tick(60)

    def move(self, dx):
        # 좌우 이동
        new_pos = self.current_pos[1] + dx
        if self.valid_move(self.current_piece, [self.current_pos[0], new_pos]):
            self.current_pos[1] = new_pos
            
    def move_down(self):
        # 아래로 이동
        new_pos = self.current_pos[0] + 1
        if self.valid_move(self.current_piece, [new_pos, self.current_pos[1]]):
            self.current_pos[0] = new_pos
        else:
            self.freeze_piece()
            self.new_piece()
            
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

    def rotate(self):
        # 블록 회전
        rotated_piece = list(zip(*self.current_piece[::-1]))  # 90도 회전
        if self.valid_move(rotated_piece, self.current_pos):
            self.current_piece = rotated_piece

    def freeze_piece(self):
        # 현재 조각을 보드에 고정
        for y, row in enumerate(self.current_piece):
            for x, cell in enumerate(row):
                if cell:
                    self.board[self.current_pos[0] + y][self.current_pos[1] + x] = 1
        
        # 완성된 줄 확인 및 제거
        self.clear_lines()

    def clear_lines(self):
        # 완성된 줄 제거
        lines_to_clear = []
        for y in range(self.GRID_HEIGHT):
            if all(self.board[y]):  # 한 줄이 모두 채워졌는지 확인
                lines_to_clear.append(y)
        
        if lines_to_clear:  # 지울 줄이 있다면
            # 서버에 알림
            self.send_to_server({
                "type": "line_clear",
                "lines": len(lines_to_clear)
            })
            
            # 로컬에서 줄 제거 및 점수 추가
            for line in lines_to_clear:
                del self.board[line]
                self.board.insert(0, [0 for _ in range(self.GRID_WIDTH)])
                self.score += 100

    def send_to_server(self, data):
        try:
            self.socket.send(json.dumps(data).encode())
        except:
            print("서버 통신 오류")

    def process_server_message(self, data):
        if data["type"] == "attack":
            # 다른 플레이어가 줄을 지워서 공격이 왔을 때
            self.handle_attack(data["lines"])
        elif data["type"] == "game_state":
            # 다른 플레이어들의 게임 상태 업데이트
            self.update_other_players(data["players"])

    def handle_attack(self, lines):
        # 공격으로 인한 방해 라인 추가
        for _ in range(lines):
            # 맨 위의 줄을 제거하고
            del self.board[0]
            # 맨 아래에 방해 줄 추가
            new_line = [1 for _ in range(self.GRID_WIDTH)]
            # 한 칸은 비워둠 (랜덤 위치)
            new_line[random.randint(0, self.GRID_WIDTH-1)] = 0
            self.board.append(new_line)

    def update_other_players(self, players_data):
        # 다른 플레이어들의 상태 업데이트
        for player_id, data in players_data.items():
            if str(player_id) != self.player_id:  # 자신이 아닌 경우만
                # 다른 플레이어 화면 업데이트
                self.other_players[player_id] = {
                    "board": data["board"],
                    "score": data["score"]
                }

    def hard_drop(self):
        # 블록을 즉시 바닥으로 떨어뜨리기
        while self.valid_move(self.current_piece, [self.current_pos[0] + 1, self.current_pos[1]]):
            self.current_pos[0] += 1
        self.freeze_piece()
        self.new_piece()

    def connect_to_server(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect(('localhost', 8080))
        # 서버 통신용 스레드 시작
        threading.Thread(target=self.handle_server_messages).start()

    def handle_server_messages(self):
        while True:
            try:
                data = self.socket.recv(1024)
                if not data:
                    break
                self.process_server_message(json.loads(data))
            except:
                break

if __name__ == "__main__":
    game = TetrisGame()
    game.run() 