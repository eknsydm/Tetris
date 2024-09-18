#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_video.h>

#include <cassert>
#include <cstdint>
//#include <cstdio>
//TODO:Rotation can cause push the piece instead of disallow the action
//TODO:CHANGE TETRINO AS TETROMINO 
//---1:02:12
#include <SDL2/SDL.h>
#include <cstdio>

#include <wayland-client.h>

#define WIDTH 10
#define HEIGHT 22
#define VISIBLE_HEIGHT 20
#define GRID_SIZE 30

struct Color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};
inline Color
color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return {r,g,b,a};
}

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
    GAME_PHASE_PLAY
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
    Piece_State piece;
    Game_Phase phase;
};

struct Input_State{

    int8_t left;
    int8_t right;
    int8_t up;

    int8_t dleft;
    int8_t dright;
    int8_t dup;
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
void soft_drop(Game_State *game)
{
    ++game->piece.offset_row;
    if (!check_piece_valid(&game->piece, game->board, WIDTH, HEIGHT))
    {
        --game->piece.offset_row;
        merge_piece(game);
        spawn_piece(game);
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


}
void update_game(Game_State *game,
        const Input_State *input){

    switch (game->phase) {
        case GAME_PHASE_PLAY:
            return update_game_play(game,input);
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
        
    Color base_color = color(175,0,0,255);
    Color dark_color = color(40,0,0,255);
    Color light_color = color(255,190,190,255);

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
            if (value)
            {
                draw_cell(renderer, row, col, value, offset_x, offset_y);
            }
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
        
        input.dleft = input.left - prev_input.left;
        input.dright = input.right - prev_input.right;
        input.dup = input.up - prev_input.up;

        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderClear(renderer);

        update_game(&game, &input);
        render_game(&game, renderer);

        SDL_RenderPresent(renderer);
    }
    return 0;
}
