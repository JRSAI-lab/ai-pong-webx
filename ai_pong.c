#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PADDLE_WIDTH 15
#define PADDLE_HEIGHT 80
#define BALL_SIZE 12
#define PADDLE_SPEED 8
#define BALL_SPEED_X 7
#define BALL_SPEED_Y 7
#define WINNING_SCORE 11
#define FONT_SCALE 8  // Big digits for scores

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
bool running = true;
int score_left = 0, score_right = 0;

typedef struct {
    int x, y, w, h;
    int vx, vy;  // Only for ball
} Entity;

Entity left_paddle = {20, SCREEN_HEIGHT/2 - PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT, 0, 0};
Entity right_paddle = {SCREEN_WIDTH - 20 - PADDLE_WIDTH, SCREEN_HEIGHT/2 - PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT, 0, 0};
Entity ball = {SCREEN_WIDTH/2 - BALL_SIZE/2, SCREEN_HEIGHT/2 - BALL_SIZE/2, BALL_SIZE, BALL_SIZE, BALL_SPEED_X, BALL_SPEED_Y};

// Simple 7-segment style digit patterns (8x8 bitmap, MSB top-left)
const unsigned char digits[10][8] = {
    {0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C},  // 0
    {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E},  // 1
    {0x7E, 0x02, 0x02, 0x7C, 0x40, 0x40, 0x40, 0x7E},  // 2
    {0x7E, 0x02, 0x02, 0x3C, 0x02, 0x02, 0x02, 0x7E},  // 3
    {0x18, 0x28, 0x48, 0x7E, 0x08, 0x08, 0x08, 0x08},  // 4
    {0x7E, 0x40, 0x40, 0x7C, 0x02, 0x02, 0x02, 0x7E},  // 5
    {0x3C, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x3C},  // 6
    {0x7E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x40},  // 7
    {0x3C, 0x42, 0x42, 0x3C, 0x42, 0x42, 0x42, 0x3C},  // 8
    {0x3C, 0x42, 0x42, 0x3E, 0x02, 0x02, 0x02, 0x7E}   // 9
};

void draw_digit(int x, int y, int digit, SDL_Color color) {
    for (int row = 0; row < 8; row++) {
        unsigned char row_data = digits[digit][row];
        for (int col = 0; col < 8; col++) {
            if (row_data & (1 << (7 - col))) {
                SDL_Rect pixel = {x + col * FONT_SCALE, y + row * FONT_SCALE, FONT_SCALE, FONT_SCALE};
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
}

void draw_score(int score, int x_start) {
    SDL_Color white = {255, 255, 255, 255};
    char buf[16];
    sprintf(buf, "%d", score);
    int x = x_start;
    for (int i = 0; buf[i]; i++) {
        int d = buf[i] - '0';
        draw_digit(x, 50, d, white);
        x += 8 * FONT_SCALE + 10;
    }
}

bool collides(Entity* a, Entity* b) {
    return a->x < b->x + b->w &&
           a->x + a->w > b->x &&
           a->y < b->y + b->h &&
           a->y + a->h > b->y;
}

void reset_ball() {
    ball.x = SCREEN_WIDTH / 2 - BALL_SIZE / 2;
    ball.y = SCREEN_HEIGHT / 2 - BALL_SIZE / 2;
    ball.vx = (ball.vx > 0 ? BALL_SPEED_X : -BALL_SPEED_X);
    ball.vy = (rand() % (BALL_SPEED_Y * 2 + 1)) - BALL_SPEED_Y;
}

void update() {
    // Input
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_W] && left_paddle.y > 0) left_paddle.y -= PADDLE_SPEED;
    if (keys[SDL_SCANCODE_S] && left_paddle.y < SCREEN_HEIGHT - PADDLE_HEIGHT) left_paddle.y += PADDLE_SPEED;
    if (keys[SDL_SCANCODE_UP] && right_paddle.y > 0) right_paddle.y -= PADDLE_SPEED;
    if (keys[SDL_SCANCODE_DOWN] && right_paddle.y < SCREEN_HEIGHT - PADDLE_HEIGHT) right_paddle.y += PADDLE_SPEED;

    // Ball movement
    ball.x += ball.vx;
    ball.y += ball.vy;

    // Wall bounce
    if (ball.y <= 0 || ball.y + BALL_SIZE >= SCREEN_HEIGHT) {
        ball.vy = -ball.vy;
        ball.x += ball.vx;  // Prevent sticking
    }

    // Paddle collision & bounce (angle based on hit position)
    if (collides(&ball, &left_paddle)) {
        ball.vx = BALL_SPEED_X;
        ball.vy = ((ball.y + BALL_SIZE / 2 - (left_paddle.y + PADDLE_HEIGHT / 2)) / (PADDLE_HEIGHT / 2.0f)) * BALL_SPEED_Y;
    }
    if (collides(&ball, &right_paddle)) {
        ball.vx = -BALL_SPEED_X;
        ball.vy = ((ball.y + BALL_SIZE / 2 - (right_paddle.y + PADDLE_HEIGHT / 2)) / (PADDLE_HEIGHT / 2.0f)) * BALL_SPEED_Y;
    }

    // Scoring
    if (ball.x < 0) {
        score_right++;
        reset_ball();
    } else if (ball.x > SCREEN_WIDTH) {
        score_left++;
        reset_ball();
    }

    // Win check
    if (score_left >= WINNING_SCORE || score_right >= WINNING_SCORE) {
        running = false;
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Dotted center line
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int y = 0; y < SCREEN_HEIGHT; y += 20) {
        SDL_RenderDrawRect(renderer, &(SDL_Rect){SCREEN_WIDTH/2 - 1, y, 2, 10});
    }

    // Paddles & ball (white)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &(SDL_Rect){left_paddle.x, left_paddle.y, left_paddle.w, left_paddle.h});
    SDL_RenderFillRect(renderer, &(SDL_Rect){right_paddle.x, right_paddle.y, right_paddle.w, right_paddle.h});
    SDL_RenderFillRect(renderer, &(SDL_Rect){ball.x, ball.y, ball.w, ball.h});

    // Scores
    draw_score(score_left, 100);
    draw_score(score_right, SCREEN_WIDTH - 200);

    SDL_RenderPresent(renderer);
}

void handle_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT ||
            (e.type == SDL_KEYDOWN && (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_q))) {
            running = false;
        }
    }
}

#ifdef __EMSCRIPTEN__
void loop() {
    if (!running) {
        emscripten_cancel_main_loop();
        return;
    }
    handle_events();
    update();
    render();
}
#endif

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        printf("SDL Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Ai-Pong WebX - Game of the Year!",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!window || !renderer) {
        printf("Window/Renderer failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    srand(SDL_GetTicks());

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop, 0, 1);
#else
    while (running) {
        handle_events();
        update();
        render();
        SDL_Delay(16);
    }
#endif

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
