#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>

#include <cassert>
#include <cstdint>
// #include <cstdio>
// TODO:Rotation can cause push the piece instead of disallow the action
// TODO:CHANGE TETRINO AS TETROMINO

//---2:08:16
#include "colors.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <cstdio>

#include <cstdlib>
#include <cstring>
// #include <wayland-client.h>

#define WIDTH 10
#define HEIGHT 22
#define VISIBLE_HEIGHT 20
#define GRID_SIZE 30

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

const float FRAMES_PER_DROP[] = {48, 43, 38, 33, 28, 23, 18, 13, 8, 6,
                                 5,  5,  5,  4,  4,  4,  3,  3,  3, 2,
                                 2,  2,  2,  2,  2,  2,  2,  2,  1};

const float TARGET_SECONDS_PER_FRAME = 1.f / 60.f;

struct Tetrino {
        const uint8_t *data;
        const int32_t side;
};

inline Tetrino tetrino(const uint8_t *data, int32_t side) {
    return {data, side};
}

const uint8_t TETRINO_1[] = {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t TETRINO_2[] = {2, 2, 2, 2};
const uint8_t TETRINO_3[] = {0, 0, 0, 3, 3, 3, 0, 3, 0};
const uint8_t TETRINO_4[] = {
    0, 0, 0, 4, 0, 0, 4, 4, 4,
};
const uint8_t TETRINO_5[] = {
    0, 0, 0, 0, 0, 5, 5, 5, 5,
};
const uint8_t TETRINO_6[] = {
    0, 0, 0, 0, 6, 6, 6, 6, 0,
};
const uint8_t TETRINO_7[] = {
    0, 0, 0, 7, 7, 0, 0, 7, 7,
};
const Tetrino TETRINOS[] = {
    tetrino(TETRINO_1, 4), tetrino(TETRINO_2, 2), tetrino(TETRINO_3, 3),
    tetrino(TETRINO_4, 3), tetrino(TETRINO_5, 3), tetrino(TETRINO_6, 3),
    tetrino(TETRINO_7, 3),
};
enum Game_Phase {
    GAME_PHASE_START,
    GAME_PHASE_PLAY,
    GAME_PHASE_LINE,
    GAME_PHASE_GAMEOVER,
};
enum Text_Align {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_MID,
    TEXT_ALIGN_RIGHT,
};

struct Piece_State {
        uint8_t tetrino_index;
        int32_t offset_row;
        int32_t offset_col;
        int32_t rotation;
};

struct Game_State {
        uint8_t board[WIDTH * HEIGHT];
        uint8_t lines[HEIGHT];
        Piece_State piece;
        Game_Phase phase;
        int32_t pending_line_count;

        int32_t points;
        int32_t start_level;
        int32_t level;
        int32_t line_count;

        float next_drop_time;
        float highlight_end_time;
        float time;
};

struct Input_State {

        int8_t left;
        int8_t right;
        int8_t up;
        int8_t down;
        int8_t a;

        int8_t dleft;
        int8_t dright;
        int8_t dup;
        int8_t ddown;
        int8_t da;
};

inline uint8_t matrix_get(const uint8_t *values, int32_t width, int32_t row,
                          int32_t col) {
    int32_t index = row * width + col;
    return values[index];
}
inline void matrix_set(uint8_t *values, int32_t width, int32_t row, int32_t col,
                       uint32_t value) {
    int32_t index = row * width + col;
    values[index] = value;
}

inline uint8_t tetrino_get(const Tetrino *tetrino, int32_t row, int32_t col,
                           int32_t rotation) {
    int32_t side = tetrino->side;
    switch (rotation) {
    case 0:
        return tetrino->data[row * side + col];
    case 1:
        return tetrino->data[(side - col - 1) * side + row];
    case 2:
        return tetrino->data[(side - row - 1) * side + (side - col - 1)];
    case 3:
        return tetrino->data[col * side + (side - row - 1)];
    }
    return 0;
}
inline uint8_t check_row_filled(const uint8_t *values, int32_t width,
                                int32_t row) {
    for (int32_t col = 0; col < width; ++col) {
        if (!matrix_get(values, width, row, col)) {
            return 0;
        }
    }
    return 1;
}
inline uint8_t check_row_empty(const uint8_t *values, int32_t width,
                               int32_t row) {
    for (int32_t col = 0; col < width; ++col) {
        if (matrix_get(values, width, row, col)) {
            return 0;
        }
    }
    return 1;
}

int32_t find_lines(const uint8_t *values, int32_t width, int32_t height,
                   uint8_t *lines_out) {
    int32_t count = 0;
    for (int32_t row = 0; row < height; ++row) {
        for (int32_t col = 0; col < width; ++col) {
            uint8_t filled = check_row_filled(values, width, row);
            lines_out[row] = filled;
            count += filled;
        }
    }
    return count;
}

void clear_lines(uint8_t *values, int32_t width, int32_t height,
                 const uint8_t *lines) {
    int32_t src_row = height - 1;
    for (int32_t dst_row = height - 1; dst_row >= 0; --dst_row) {
        while (src_row >= 0 && lines[src_row]) {
            --src_row;
        }

        if (src_row < 0) {
            memset(values + dst_row * width, 0, width);
        } else {
            memcpy(values + dst_row * width, values + src_row * width, width);
            --src_row;
        }
    }
}
bool check_piece_valid(const Piece_State *piece, const uint8_t *board,
                       int32_t width, int32_t height) {
    const Tetrino *tetrino = TETRINOS + piece->tetrino_index;
    assert(tetrino);

    for (int32_t row = 0; row < tetrino->side; ++row) {
        for (int32_t col = 0; col < tetrino->side; ++col) {

            uint8_t value = tetrino_get(tetrino, row, col, piece->rotation);

            if (value > 0) {
                int32_t board_row = piece->offset_row + row;
                int32_t board_col = piece->offset_col + col;

                if (board_row < 0) {
                    return false;
                }
                if (board_row >= height) {
                    return false;
                }
                if (board_col < 0) {
                    return false;
                }
                if (board_col >= width) {
                    return false;
                }
                if (matrix_get(board, width, board_row, board_col)) {
                    return false;
                }
            }
        }
    }
    return true;
}

void merge_piece(Game_State *game) {
    const Tetrino *tetrino = TETRINOS + game->piece.tetrino_index;
    for (int32_t row = 0; row < tetrino->side; ++row) {
        for (int32_t col = 0; col < tetrino->side; ++col) {
            uint8_t value =
                tetrino_get(tetrino, row, col, game->piece.rotation);
            if (value) {
                int32_t board_row = game->piece.offset_row + row;
                int32_t board_col = game->piece.offset_col + col;

                matrix_set(game->board, WIDTH, board_row, board_col, value);
            }
        }
    }
}
inline int32_t random_int(int32_t min, int32_t max) {
    int32_t range = max - min;
    return min + rand() % range;
}

inline float get_time_to_next_drop(int32_t level){
    if (level > 29) {
        level = 29;
    }
    return FRAMES_PER_DROP[level] * TARGET_SECONDS_PER_FRAME;
}

void spawn_piece(Game_State *game) {
    game->piece = {};
    game->piece.tetrino_index = random_int(0, ARRAY_COUNT(TETRINOS));
    game->piece.offset_col = WIDTH / 2;
    game->next_drop_time = game->time + get_time_to_next_drop(game->level);
}

inline bool soft_drop(Game_State *game) {
    ++game->piece.offset_row;
    if (!check_piece_valid(&game->piece, game->board, WIDTH, HEIGHT)) {
        --game->piece.offset_row;
        merge_piece(game);
        spawn_piece(game);
        return false;
    }

    game->next_drop_time = game->time + get_time_to_next_drop(game->level);
    return true;
}
inline int32_t compute_point(int32_t level, int32_t line_count) {
    switch (line_count) {
    case 1:
        return 40 * (level + 1);
    case 2:
        return 100 * (level + 1);
    case 3:
        return 300 * (level + 1);
    case 4:
        return 1200 * (level + 1);
    }
    return 0;
}
inline int32_t min(int x, int y) { return x < y ? x : y; }
inline int32_t max(int x, int y) { return x > y ? x : y; }
inline int32_t get_lines_for_next_level(int32_t start_level, int32_t level) {
    int32_t first_level_up_limit =
        min((start_level * 10 + 10), max(100, (start_level * 10 - 50)));
    if (level == start_level) {
        return first_level_up_limit;
    }
    int diff = level - start_level;
    return first_level_up_limit + diff * 10;
}

void update_game_start(Game_State *game, const Input_State *input){
    if (input->dup > 0){

        ++game->start_level;
    }
    else if (input->ddown > 0 && game->start_level > 0){

        --game->start_level;
    }

    else if (input->da > 0){

        memset(game->board, 0, WIDTH * HEIGHT);
        game->level = game->start_level;
        game->line_count = 0;
        game->points = 0;
        spawn_piece(game);
        game->phase = GAME_PHASE_PLAY;
    }
}
void update_game_gameover(Game_State *game, const Input_State *input){
    
    if (input->da > 0){
        
        game->phase = GAME_PHASE_START;

    }
}
void update_game_line(Game_State *game) {
    if (game->time >= game->highlight_end_time) {
        clear_lines(game->board, WIDTH, HEIGHT, game->lines);
        game->line_count += game->pending_line_count;
        game->points += compute_point(game->level, game->pending_line_count);

        int lines_for_next_level =
            get_lines_for_next_level(game->start_level, game->level);

        if (game->line_count >= lines_for_next_level) {
            ++game->level;
        }

        game->phase = GAME_PHASE_PLAY;
    }
}
void update_game_play(Game_State *game, const Input_State *input) {

    Piece_State piece = game->piece;
    if (input->dleft > 0) {
        --piece.offset_col;
    }
    if (input->dright > 0) {
        ++piece.offset_col;
    }
    if (input->dup > 0) {
        piece.rotation = (piece.rotation + 1) % 4;
    }
    if (check_piece_valid(&piece, game->board, WIDTH, HEIGHT)) {
        game->piece = piece;
    }
    if (input->ddown > 0) {
        soft_drop(game);
    }
    if (input->da > 0) {
        while (soft_drop(game));
    }
    while (game->time >= game->next_drop_time) {
        soft_drop(game);
    }

    game->pending_line_count =
        find_lines(game->board, WIDTH, HEIGHT, game->lines);
    if (game->pending_line_count > 0) {
        game->phase = GAME_PHASE_LINE;
        game->highlight_end_time = game->time + 0.5f;
    }

    int32_t game_over_row = max(0, HEIGHT - VISIBLE_HEIGHT - 1);
    if (!check_row_empty(game->board, WIDTH, game_over_row)) {
        game->phase = GAME_PHASE_GAMEOVER;
    }
}
void update_game(Game_State *game, const Input_State *input) {

    switch (game->phase) {
    case GAME_PHASE_START:
        update_game_start(game, input);
        break;
    case GAME_PHASE_PLAY:
        update_game_play(game, input);
        break;
    case GAME_PHASE_LINE:
        update_game_line(game);
        break;
    case GAME_PHASE_GAMEOVER:
        update_game_gameover(game, input);
        printf("Game Over");
        break;
    }
}
void fill_rect(SDL_Renderer *renderer, int32_t x, int32_t y, int32_t width,
               int32_t height, Color color) {
    SDL_Rect rect = {};
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}
void draw_rect(SDL_Renderer *renderer,
               int32_t x, int32_t y, int32_t width, int32_t height, Color color){
    SDL_Rect rect = {};
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, &rect);
}
void draw_string(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                 int x, int y, Text_Align alignment, Color color) {
    SDL_Color sdl_color = SDL_Color{color.r, color.g, color.b, color.a};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, sdl_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect rect;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;

    switch (alignment) {

    case TEXT_ALIGN_LEFT:
        rect.x = x;
        break;
    case TEXT_ALIGN_MID:
        rect.x = x - surface->w / 2;
        break;
    case TEXT_ALIGN_RIGHT:
        rect.x = x - surface->w;
        break;
    }

    SDL_RenderCopy(renderer, texture, 0, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}
void draw_cell(SDL_Renderer *renderer, int32_t row, int32_t col, uint8_t value,
               int32_t offset_x, int32_t offset_y, bool outline = false) {

    int32_t edge = GRID_SIZE / 8;

    Color base_color = BASE_COLORS[value];
    Color dark_color = LIGHT_COLORS[value];
    Color light_color = DARK_COLORS[value];

    int32_t x = col * GRID_SIZE + offset_x;
    int32_t y = row * GRID_SIZE + offset_y;

    if (outline){
        draw_rect(renderer, x, y, GRID_SIZE, GRID_SIZE, base_color);
        return;
    }


    fill_rect(renderer, x, y, GRID_SIZE, GRID_SIZE, dark_color);
    fill_rect(renderer, x + edge, y, GRID_SIZE - edge, GRID_SIZE - edge,
              light_color);
    fill_rect(renderer, x + edge, y + edge, GRID_SIZE - edge * 2,
              GRID_SIZE - edge * 2, base_color);
}

void draw_piece(SDL_Renderer *renderer, const Piece_State *piece,
                int32_t offset_x, int32_t offset_y, bool outline = false) {

    const Tetrino *tetrino = TETRINOS + piece->tetrino_index;
    
    for (int32_t row = 0; row < tetrino->side; ++row) {
        for (int32_t col = 0; col < tetrino->side; ++col) {
            
            uint8_t value = tetrino_get(tetrino, row, col, piece->rotation);
            
            if (value) {
                draw_cell(renderer, 
                          row + piece->offset_row,
                          col + piece->offset_col, 
                          value, 
                          offset_x, offset_y,
                          outline);
            }
        }
    }
}

void draw_board(SDL_Renderer *renderer, const uint8_t *board, uint32_t width,
                uint32_t height, int32_t offset_x, int32_t offset_y) {

    fill_rect(renderer, offset_x, offset_y, width * GRID_SIZE, height * GRID_SIZE, BASE_COLORS[0]);

    for (uint32_t row = 0; row < height; ++row) {
        for (uint32_t col = 0; col < width; ++col) {
           
            uint8_t value = matrix_get(board, width, row, col);
            //draw_cell(renderer, row, col, value, offset_x, offset_y);
            if (value)
            {
                draw_cell(renderer, row, col, value, offset_x, offset_y);
            }
        }
    }
}

void render_game(const Game_State *game, SDL_Renderer *renderer,
                 TTF_Font *font) {
    Color highlight_color = color(0xFF, 0xFF, 0xFF, 0xFF);

    int32_t margin_y = 60;

    draw_board(renderer, game->board, WIDTH, HEIGHT, 0, margin_y);
    //draw_piece(renderer, &game->piece, 0, 0);

    char buffer[4096];
    
    if (game->phase == GAME_PHASE_PLAY)
    {
        draw_piece(renderer, &game->piece, 0, margin_y);

        Piece_State piece = game->piece;
        while (check_piece_valid(&piece, game->board, WIDTH, HEIGHT))
        {
            piece.offset_row++;
        }
        --piece.offset_row;
        
        draw_piece(renderer, &piece, 0, margin_y, true);
        
    }

    if (game->phase == GAME_PHASE_LINE) {
        for (int row = 0; row < HEIGHT; ++row) {
            if (game->lines[row]) {
                
                int x = 0;
                int y = row * GRID_SIZE + margin_y;
                fill_rect(renderer, x, y, WIDTH * GRID_SIZE, GRID_SIZE,
                          highlight_color);
            }
        }
    } 
    else if (game->phase == GAME_PHASE_GAMEOVER) {
        printf("gameOver");
        int32_t x = WIDTH * GRID_SIZE / 2;
        int32_t y = (HEIGHT * GRID_SIZE + margin_y) / 2;//+ margin_y) / 2;
        draw_string(renderer, font, "GAME OVER",
                     x, y, TEXT_ALIGN_MID, highlight_color);
    }
    else if (game->phase == GAME_PHASE_START) {
        int32_t x = WIDTH * GRID_SIZE / 2;
        int32_t y = (HEIGHT * GRID_SIZE + margin_y) / 2;
        draw_string(renderer, font, "PRESS START",
                    x, y, TEXT_ALIGN_MID, highlight_color);
        snprintf(buffer, sizeof(buffer), "STARTING LEVEL: %d", game->start_level);
        draw_string(renderer, font, buffer,
                    x, y + 30, TEXT_ALIGN_MID, highlight_color);
    }
    
    fill_rect(renderer,
              0, margin_y,
              WIDTH * GRID_SIZE, (HEIGHT - VISIBLE_HEIGHT) * GRID_SIZE,
              color(0x00, 0x00, 0x00, 0x00));
    

    snprintf(buffer, sizeof(buffer), "LEVEL: %d", game->level);
    draw_string(renderer, font, buffer, 5, 5, TEXT_ALIGN_LEFT, highlight_color);

    snprintf(buffer, sizeof(buffer), "LINES: %d", game->line_count);
    draw_string(renderer, font, buffer, 5, 35, TEXT_ALIGN_LEFT, highlight_color);
    
    snprintf(buffer, sizeof(buffer), "POINTS: %d", game->points);
    draw_string(renderer, font, buffer, 5, 65, TEXT_ALIGN_LEFT, highlight_color);
    //draw_string(renderer, font, "TETRIS", WIDTH * GRID_SIZE, 0,
    //        TEXT_ALIGN_RIGHT, highlight_color);
}
int main() {
    printf("TETRIS");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return 1;
    }
    if (TTF_Init() < 0) {
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow(
        "Tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 300, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1,
        0); // SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // const char *font_name = NULL;
    // const char *font_name =
    // "/home/neos/.local/share/fonts/fonts/ttf/JetBrainsMono-Regular.ttf";
    const char *font_name = "novem___.ttf";

    TTF_Font *font = TTF_OpenFont(font_name, 24);

    if (!font) {
        printf("TTF_OPENFONT: %s \n", TTF_GetError());
    }

    bool quit = false;
    Game_State game = {};
    Input_State input = {};

    spawn_piece(&game);

    game.piece.tetrino_index = 2;

    while (!quit) {
        game.time = SDL_GetTicks() / 1000.0f;

        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                quit = true;
            }
        }
        int32_t key_count;
        const uint8_t *key_state = SDL_GetKeyboardState(&key_count);

        Input_State prev_input = input;

        input.left = key_state[SDL_SCANCODE_LEFT];
        input.right = key_state[SDL_SCANCODE_RIGHT];
        input.up = key_state[SDL_SCANCODE_UP];
        input.down = key_state[SDL_SCANCODE_DOWN];
        input.a = key_state[SDL_SCANCODE_SPACE];

        input.dleft = input.left - prev_input.left;
        input.dright = input.right - prev_input.right;
        input.dup = input.up - prev_input.up;
        input.ddown = input.down - prev_input.down;
        input.da = input.a - prev_input.a;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        update_game(&game, &input);
        render_game(&game, renderer, font);

        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();

    return 0;
}
