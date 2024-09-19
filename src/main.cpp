#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>

#include <cassert>
#include <cstdint>
//#include <cstdio>
//TODO:Rotation can cause push the piece instead of disallow the action
//TODO:CHANGE TETRINO AS TETROMINO 
//---1:29:02
#include <SDL2/SDL.h>
#include <cstdio>
#include "colors.h"

#include <cstring>
#include <wayland-client.h>

#define WIDTH 10
#define HEIGHT 22
#define VISIBLE_HEIGHT 20
#define GRID_SIZE 30

const float FRAMES_PER_DROP[] = {
    48,
    43,
    38,
    33,
    28,
    23,
    18,
    13,
    8,
    6,
    5,
    5,
    5,
    4,
    4,
    4,
    3,
    3,
    3,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    1
};

const float TARGET_SECONDS_PER_FRAME = 1.f / 60.f;

struct Tetrino
{
    const uint8_t *data;
    const int32_t side;
};

inline Tetrino
tetrino(const uint8_t *data, int32_t side)
{
    return { data, side };
}

const uint8_t TETRINO_1[] = {
    0, 0, 0, 0,
    1, 1, 1, 1,
    0, 0, 0, 0,
    0, 0, 0, 0
};
const uint8_t TETRINO_2[] = {
    2, 2,
    2, 2
};
const uint8_t TETRINO_3[] = {
    0,0,0,
    3,3,3,
    0,3,0
};
const Tetrino TETRINOS[] = {
    tetrino(TETRINO_1, 4),
    tetrino(TETRINO_2, 2),
    tetrino(TETRINO_3, 3),
};
enum Game_Phase
{
    GAME_PHASE_PLAY,
    GAME_PHASE_LINE,
};

struct Piece_State
{
    uint8_t tetrino_index;
    int32_t offset_row;
    int32_t offset_col;
    int32_t rotation;
};

struct Game_State{

    uint8_t board[WIDTH * HEIGHT];
    uint8_t lines[HEIGHT];
    Piece_State piece;
    Game_Phase phase;

    int32_t level;

    float next_drop_time;
    float highlight_end_time;
    float time;
};

struct Input_State{

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

inline uint8_t
matrix_get(const uint8_t *values, int32_t width, int32_t row, int32_t col)
{
    int32_t index = row * width + col;
    return values[index];
}
inline void
matrix_set(uint8_t *values, int32_t width, int32_t row, int32_t col, uint32_t value)
{
    int32_t index =row * width + col;
    values[index] = value;
}

inline uint8_t
tetrino_get(const Tetrino *tetrino, int32_t row, int32_t col, int32_t rotation)
{
    int32_t side = tetrino->side;
    switch (rotation)
    {
    case 0:
        return tetrino->data[row * side + col];
    case 1:
        return tetrino->data[(side - col -1) * side + row];
    case 2:
        return tetrino->data[(side - row -1) * side + (side - col - 1)];
    case 3:
        return tetrino->data[col * side + (side - row -1)];
    }
    return 0;
}
inline uint8_t
check_row_filled(const uint8_t *values, int32_t width, int32_t row)
{
    for (int32_t col = 0; col < width; ++col)
    {
        if (!matrix_get(values, width, row, col))
        {
            return 0;
        }
    }
    return 1;
}

int32_t find_lines(const uint8_t *values, int32_t width, int32_t height, uint8_t *lines_out)
{
    int32_t count = 0;
    for (int32_t row = 0; row < height; ++row)
    {
        for (int32_t col = 0; col < width; ++col)
        {
            uint8_t filled = check_row_filled(values, width, row);
            lines_out[row] = filled;
            count += filled;
        }
    }
    return count;
}

void clear_lines(uint8_t *values, int32_t width, int32_t height, const uint8_t *lines)
{
    int32_t src_row = height - 1;
    for (int32_t dst_row = height - 1; dst_row >= 0; --dst_row)
    {
        while(src_row >= 0 && lines[src_row])
        {
            --src_row;
        }
        
        if (src_row < 0)
        {
            memset(values + dst_row * width, 0, width);
        }
        else
        {
            memcpy(values + dst_row * width,
                   values + src_row * width,
                   width);
            --src_row;
        }
    }
}
bool check_piece_valid(const Piece_State *piece,
                       const uint8_t *board, 
                       int32_t width, 
                       int32_t height)
{
    const Tetrino *tetrino = TETRINOS + piece->tetrino_index;
    assert(tetrino);

    for (int32_t row = 0; row < tetrino->side; ++row)
    {
        for (int32_t col = 0; col < tetrino->side; ++col){
            uint8_t value = tetrino_get(tetrino, row, col, piece->rotation);
            if (value > 0)
            {
                int32_t board_row = piece->offset_row + row;
                int32_t board_col = piece->offset_col + col;
                
                if (board_row < 0)
                {
                    return false;
                }
                if (board_row >= height)
                {
                    return false;  
                }
                if (board_col < 0)
                {
                    return false;
                }
                if (board_col >= width)
                {
                    return false;
                }
                if (matrix_get(board, width, board_row, board_col))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

void merge_piece(Game_State *game)
{
    const Tetrino *tetrino = TETRINOS + game->piece.tetrino_index;
    for (int32_t row = 0; row < tetrino->side; ++row)
    {
        for (int32_t col = 0; col < tetrino->side; ++col)
        {
            uint8_t value = tetrino_get(tetrino, row, col, game->piece.rotation);
            if (value)
            {
                int32_t board_row = game->piece.offset_row + row;
                int32_t board_col = game->piece.offset_col + col;

                matrix_set(game->board, WIDTH, board_row, board_col, value);
            }
        }
    }
}
void spawn_piece(Game_State *game)
{
    game->piece = {};
    game->piece.offset_col = WIDTH / 2;
}

inline float
get_time_to_next_drop(int32_t level)
{
    if (level > 29)
    {
        level = 29;
    }
    return FRAMES_PER_DROP[level] * TARGET_SECONDS_PER_FRAME;
}
bool soft_drop(Game_State *game)
{
    ++game->piece.offset_row;
    if (!check_piece_valid(&game->piece, game->board, WIDTH, HEIGHT))
    {
        --game->piece.offset_row;
        merge_piece(game);
        spawn_piece(game);
        return false;
    }

    game->next_drop_time = game->time + get_time_to_next_drop(game->level);
    return true;
}
void update_game_line(Game_State *game)
{
    if (game->time >= game->highlight_end_time)
    {
        clear_lines(game->board, WIDTH, HEIGHT, game->lines);
        game->phase = GAME_PHASE_PLAY;
    }
}
void update_game_play(Game_State *game, 
        const Input_State *input)
{
     Piece_State piece = game->piece;
     if (input->dleft > 0)
     {
         --piece.offset_col;
     }
     if (input->dright > 0)
     {
        ++piece.offset_col;
     }
     if (input->dup > 0)
     {
        piece.rotation = (piece.rotation + 1) % 4; 
     }
     if (check_piece_valid(&piece, game->board, WIDTH, HEIGHT))
     {
         game->piece = piece;
     }
     if (input->ddown > 0)
     {
         soft_drop(game);
     }
     if (input->da > 0)
     {
         while(soft_drop(game));
     }
     while (game->time >= game->next_drop_time)
     {
         soft_drop(game);
     }

     uint32_t line_count = find_lines(game->board,
             WIDTH,
             HEIGHT,
             game->lines);
     if (line_count > 0)
     {
         game->phase = GAME_PHASE_LINE;
         game->highlight_end_time = game->time + 0.5f;
     }



}
void update_game(Game_State *game,
        const Input_State *input){

    switch (game->phase) {
        case GAME_PHASE_PLAY:
            return update_game_play(game,input);
            break;
        case GAME_PHASE_LINE:
            update_game_line(game);
            break;
    }
}
void fill_rect(SDL_Renderer *renderer,
               int32_t x, int32_t y, int32_t width, int32_t height,
               Color color)
{
    SDL_Rect rect = {};
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}
void draw_cell(SDL_Renderer *renderer,
               int32_t row, int32_t col, uint8_t value,
               int32_t offset_x, int32_t offset_y)
{

    int32_t edge = GRID_SIZE / 8;
        
    Color base_color = BASE_COLORS[value];
    Color dark_color = LIGHT_COLORS[value];
    Color light_color = DARK_COLORS[value];

    int32_t x = col * GRID_SIZE + offset_x;
    int32_t y = row * GRID_SIZE + offset_y;
    fill_rect(renderer, x, y, GRID_SIZE, GRID_SIZE, dark_color);
    fill_rect(renderer, x + edge, y, GRID_SIZE - edge, GRID_SIZE - edge, light_color);
    fill_rect(renderer, x + edge, y + edge, 
            GRID_SIZE - edge * 2, 
            GRID_SIZE - edge * 2, base_color);
}

void draw_piece (SDL_Renderer *renderer,
                const Piece_State *piece,
                int32_t offset_x, int32_t offset_y)
{
    const Tetrino *tetrino = TETRINOS + piece->tetrino_index;
    for (int32_t row = 0; row < tetrino->side; ++row)
    {
        for (int32_t col = 0; col < tetrino->side; ++col)
        {
            uint8_t value = tetrino_get(tetrino, row, col, piece->rotation);
            if (value)
            {
                draw_cell(renderer , 
                        row + piece->offset_row, 
                        col + piece->offset_col, 
                        value,
                        offset_x, 
                        offset_y);
            }
        }
    }
}

void draw_board(SDL_Renderer *renderer,
                const uint8_t *board,
                uint32_t width, uint32_t height,
                int32_t offset_x, int32_t offset_y)
{
    for (uint32_t row = 0; row < height; ++row)
    {
        for (uint32_t col = 0; col < width; ++col)
        {
            uint8_t value = matrix_get(board, width, row, col);
            draw_cell(renderer, row, col, value, offset_x, offset_y);
            //if (value)
            //{
            //    draw_cell(renderer, row, col, value, offset_x, offset_y);
            //}
        }
    }
}

void render_game(const Game_State *game,
                 SDL_Renderer *renderer)
{   
    draw_board(renderer, game->board, WIDTH, HEIGHT, 0, 0);
    draw_piece(renderer, &game->piece, 0, 0);
}
int main()
{
    printf("TETRIS");
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow(
            "Tetris",
            SDL_WINDOWPOS_UNDEFINED, 
            SDL_WINDOWPOS_UNDEFINED, 
            400, 
            720, 
            0);//SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(
            window, 
            -1,
            0);//SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    bool quit = false;
    Game_State game = {};
    Input_State input = {};

    spawn_piece(&game);

    game.piece.tetrino_index = 2;
    
    while(!quit)
    {
        game.time = SDL_GetTicks() / 1000.0f;

        SDL_Event e;
        while(SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
            if (e.key.keysym.sym == SDLK_ESCAPE)
            {
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
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderClear(renderer);

        update_game(&game, &input);
        render_game(&game, renderer);

        SDL_RenderPresent(renderer);
    }
    return 0;
}
