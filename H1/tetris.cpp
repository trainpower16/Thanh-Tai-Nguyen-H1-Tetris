// TETRIS!!! //

#include "splashkit.h"
#include "vector"
#include "ctime"

using namespace std;

// Constants

const int CELL_SIZE = 30; // Size of each cell in the grid
const int GRID_WIDTH = 10; // Number of columns in the grid
const int GRID_HEIGHT = 20; // Number of rows in the grid
const int SCREEN_WIDTH = CELL_SIZE * GRID_WIDTH; // Width of the grid
const int SCREEN_HEIGHT = CELL_SIZE * GRID_HEIGHT; // Height of the grid
const int SIDEBAR_WIDTH = 200; // Width of the sidebar
const int WINDOW_WIDTH = SCREEN_WIDTH + SIDEBAR_WIDTH; // Total window width (grid + sidebar)
const int WINDOW_HEIGHT = SCREEN_HEIGHT; // Total window height
const int MAX_LEVEL = 5; // Maximum selectable starting level

// STRUCTS
// Struct for position on the grid
struct Position 
{
    int x, y;
    Position(int x = 0, int y = 0) : x(x), y(y) {}
};

// Struct for tetromino details
struct Tetromino 
{
    int shape;      // Shape index
    int rotation;   // Rotation index 
    Position pos;   // Top-left position of the 4x4 tetromino grid

    Tetromino(int shape = 0, int rotation = 0, int x = 3, int y = 0) : shape(shape), rotation(rotation), pos(x, y) {}
};

// Struct for game stats
struct GameStats 
{
    int score;
    int level;
    int linesCleared;
    int highScore;
    double gameTime;

    GameStats() : score(0), level(1), linesCleared(0), highScore(0), gameTime(0) {}

    // Reset stats for a new game, starting at a given level
    void reset(int startLevel) 
    {
        score = 0;
        level = startLevel;
        linesCleared = 0;
        gameTime = 0;
    }
};

// Holds the current game stat
struct GameState 
{
    bool gameStarted;
    bool gamePaused;
    bool gameOver;
    bool showLevelSelect;
    int selectedLevel;
    double startTime;

    GameState() : gameStarted(false), gamePaused(false), gameOver(false), showLevelSelect(true), selectedLevel(1), startTime(0) {}

    // Function to start a new game
    void startGame() 
    {
        gameStarted = true;
        showLevelSelect = false;
        gameOver = false;
        gamePaused = false;
        startTime = current_ticks();
    }

    // Function to end the current game
    void endGame() 
    {
        gameOver = true;
        gameStarted = false;
        showLevelSelect = true;
    }

    // Function to toggle pause state and update timer if unpausing
    void togglePause() 
    {
        gamePaused = !gamePaused;
        if (!gamePaused) 
        {
            startTime = current_ticks();
        }
    }
};

// Struct to hold all loaded audio and image assets, and music fade state
struct Assets 
{
    bitmap background;
    bitmap logo;
    sound_effect click_sfx;
    sound_effect clear_line_sfx;
    music bgm;
    bool fading_out;
    float fade_volume;

    Assets() : fading_out(false), fade_volume(0.0f) {}

    // Load all required assets from files
    void initialize() {
        background = load_bitmap("background", "background.jpg");
        logo = load_bitmap("logo", "logo.png");

        load_music("bgm", "music.mp3");
        load_sound_effect("click", "click.mp3");
        load_sound_effect("clear", "clear.mp3");

        bgm = music_named("bgm");
        click_sfx = sound_effect_named("click");
        clear_line_sfx = sound_effect_named("clear");
    }
};

// Struct for handle timing for piece drop and frame delta calculation
struct GameTimer 
{
    int dropTimer; // Counts frames for automatic drop
    double lastTime; // Last frame time (seconds)

    GameTimer() : dropTimer(0), lastTime(current_ticks() / 1000.0) {}

    // Function to reset timers for a new game
    void reset() {
        dropTimer = 0;
        lastTime = current_ticks() / 1000.0;
    }

    // Function to get time elapsed since last frame=
    double getDeltaTime() 
    {
        double now = current_ticks() / 1000.0;
        double dt = now - lastTime;
        lastTime = now;
        return dt;
    }
};

// GLOBAL GAME OBJECTS
GameStats stats; // Holds score, level, etc.
GameState state; // Holds flags and level selection
Assets assets; // Holds images and sounds
GameTimer gameTimer; // Handles drop and frame timing
Tetromino currentPiece; // The currently falling tetromino

vector<vector<int>> board(GRID_HEIGHT, vector<int>(GRID_WIDTH, 0)); // 2D grid representing the game board

// SHAPE DEFINITIONS 
// SHAPES[shape][rotation][y][x]: 7 shapes, 4 rotations, 4x4 grid for each
const int SHAPES[7][4][4][4] =
{
    // I shape 
    {
        { {0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0} }, 
        { {0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0} }, 
        { {0,0,0,0}, {0,0,0,0}, {1,1,1,1}, {0,0,0,0} }, 
        { {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} }  
    },
    // J shape 
    {
        { {1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,1,1,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0} },
        { {0,0,0,0}, {1,1,1,0}, {0,0,1,0}, {0,0,0,0} },
        { {0,1,0,0}, {0,1,0,0}, {1,1,0,0}, {0,0,0,0} }
    },
    // L shape 
    {
        { {0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,1,0,0}, {0,1,0,0}, {0,1,1,0}, {0,0,0,0} },
        { {0,0,0,0}, {1,1,1,0}, {1,0,0,0}, {0,0,0,0} },
        { {1,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0} }
    },
    // O shape (square)
    {
        { {1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0} },
        { {1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0} },
        { {1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0} },
        { {1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0} }
    },
    // S shape 
    {
        { {0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,1,0,0}, {0,1,1,0}, {0,0,1,0}, {0,0,0,0} },
        { {0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0} },
        { {1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0} }
    },
    // T shape 
    {
        { {0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,1,0,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0} },
        { {0,0,0,0}, {1,1,1,0}, {0,1,0,0}, {0,0,0,0} },
        { {0,1,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0} }
    },
    // Z shape 
    {
        { {1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,0,1,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0} },
        { {0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0} },
        { {0,1,0,0}, {1,1,0,0}, {1,0,0,0}, {0,0,0,0} }
    }
};

// Color for each shape
const color SHAPE_COLORS[7] = {COLOR_CYAN, COLOR_BLUE, COLOR_ORANGE, COLOR_YELLOW, COLOR_GREEN, COLOR_PURPLE, COLOR_RED};

// GAME FUNCTIONS 
// Checks if the given tetromino collides with the board or boundaries
bool check_collision(const Tetromino& piece) 
{
    for (int ri = 0; ri < 4; ri++) 
    {
        for (int ci = 0; ci < 4; ci++) 
        {
            if (SHAPES[piece.shape][piece.rotation][ri][ci]) 
            {
                int bci = piece.pos.x + ci; // Board column index
                int bri = piece.pos.y + ri; // Board row index

                // Check for out-of-bounds or overlap with filled cell
                if (bci < 0 || bci >= GRID_WIDTH || bri >= GRID_HEIGHT || (bri >= 0 && board[bri][bci])) 
                {
                    return true;
                }
            }
        }
    }
    return false;
}

// Function to load the highest score
void load_highest_score() 
{
    json data = json_from_file("highscore.json");    

    if (json_has_key(data, "highest_score")) 
    {
        stats.highScore = json_read_number_as_int(data, "highest_score");  
    }

    free_json(data);
}

// Function to save the current highest score
void save_highest_score() 
{
    json data = create_json();
    json_set_number(data, "highest_score", stats.highScore);
    json_to_file(data, "highscore.json");
}

// Function to spawn a new random tetromino at the top of the board
void spawn_new_tetromino() 
{
    currentPiece = Tetromino(rand() % 7, 0, 3, 0);

    // If the new piece collides immediately, the game is over
    if (check_collision(currentPiece)) 
    {
        state.endGame();

        // Update high score if needed
        if (stats.score > stats.highScore) 
        {
            stats.highScore = stats.score;
            save_highest_score();
        }
    }
}

// Function to lock the current tetromino into the board (makes its cells permanent)
void lock_tetromino() 
{
    for (int y = 0; y < 4; y++) 
    {
        for (int x = 0; x < 4; x++) 
        {
            if (SHAPES[currentPiece.shape][currentPiece.rotation][y][x]) 
            {
                int nx = currentPiece.pos.x + x;
                int ny = currentPiece.pos.y + y;
                // Only lock cells within the visible board
                if (ny >= 0) board[ny][nx] = currentPiece.shape + 1;
            }
        }
    }
}

// Function to check for and clears any full lines, updates score and level
void clear_lines() 
{
    int lines = 0; // Number of lines cleared this call
    
    // Loop from bottom row to top
    for (int y = GRID_HEIGHT - 1; y >= 0; y--) 
    {
        bool full = true;

        // Check if row is full
        for (int x = 0; x < GRID_WIDTH; x++) 
        {
            if (board[y][x] == 0) 
            {
                full = false;
                break;
            }
        }
        if (full) 
        {
            lines++;

            // Move all rows above down by one
            for (int ty = y; ty > 0; ty--) 
            {
                board[ty] = board[ty - 1];
            }
            // Clear the top row (now empty)
            board[0] = vector<int>(GRID_WIDTH, 0);

            y++; // Recheck this row (since it now has new content)
        }
    }
    if (lines > 0) 
    {
        play_sound_effect(assets.clear_line_sfx);
        stats.linesCleared += lines;
        stats.score += lines * 100 * stats.level;
        // Level up every 5 lines, up to MAX_LEVEL
        stats.level = min(MAX_LEVEL, state.selectedLevel + stats.linesCleared / 5);
    }
}

// Function to draw the current state of the board
void draw_grid() 
{
    for (int y = 0; y < GRID_HEIGHT; y++) 
    {
        for (int x = 0; x < GRID_WIDTH; x++) 
        {
            if (board[y][x]) 
            {
                // Filled cell: draw colored rectangle
                fill_rectangle(SHAPE_COLORS[board[y][x] - 1], x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1);
            } 
            else 
            {
                // Empty cell: draw gray outline
                draw_rectangle(COLOR_GRAY, x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            }
        }
    }
}

// Function to draw the current falling tetromino
void draw_tetromino() 
{
    // Only draw if game is running and not paused/over
    if (!state.gameStarted || state.gamePaused || state.gameOver) 
    {
        return;
    }
    for (int y = 0; y < 4; y++) 
    {
        for (int x = 0; x < 4; x++) 
        {
            if (SHAPES[currentPiece.shape][currentPiece.rotation][y][x]) 
            {
                fill_rectangle(SHAPE_COLORS[currentPiece.shape], (currentPiece.pos.x + x) * CELL_SIZE, (currentPiece.pos.y + y) * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1);
            }
        }
    }
}

// Function to draw the "ghost" tetromino (shows where the piece would land if dropped)
void draw_ghost() 
{
    if (!state.gameStarted || state.gamePaused || state.gameOver) 
    {
        return;
    }

    // Copy the current piece and move it down until it would collide
    Tetromino ghost = currentPiece;
    while (!check_collision(Tetromino(ghost.shape, ghost.rotation, ghost.pos.x, ghost.pos.y + 1))) 
    {
        ghost.pos.y++;
    }

    // Draw the ghost as an outline at its landing position
    for (int y = 0; y < 4; y++) 
    {
        for (int x = 0; x < 4; x++) 
        {
            if (SHAPES[ghost.shape][ghost.rotation][y][x]) 
            {
                draw_rectangle(SHAPE_COLORS[ghost.shape], (ghost.pos.x + x) * CELL_SIZE, (ghost.pos.y + y) * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1);
            }
        }
    }
}

// Function to draw the entire game
void draw_game()
{
    // Draw background
    draw_bitmap(assets.background, 0, 0);
    // Draw a semi-transparent overlay for contrast
    fill_rectangle(rgba_color(0, 0, 0, 204), 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Draw the grid
    draw_grid();

    // Draw logo
    if (!state.gameStarted)
    {
        int orig_w = bitmap_width(assets.logo);
        int orig_h = bitmap_height(assets.logo);
        double scale = 0.40;
        int tgt_w = static_cast<int>(orig_w * scale);
        int tgt_h = static_cast<int>(orig_h * scale);
        int x = (SCREEN_WIDTH - tgt_w) / 2 - 228;
        int y = 20;
        drawing_options opts = option_scale_bmp(scale, scale);
        draw_bitmap(assets.logo, x, y, opts);
    }

    // Draw ghost piece and current falling piece
    draw_ghost();
    draw_tetromino();

    // Draw sidebar background
    fill_rectangle(rgba_color(64, 64, 64, 102), SCREEN_WIDTH, 0, SIDEBAR_WIDTH, SCREEN_HEIGHT);

    // Draw score, level, and time
    draw_text("SCORE", COLOR_WHITE, "04B_30__.TTF", 30, SCREEN_WIDTH + 10, 20);
    draw_text(to_string(stats.score), COLOR_YELLOW, "04B_30__.TTF", 20, SCREEN_WIDTH + 10, 50);
    draw_text("LEVEL", COLOR_WHITE, "04B_30__.TTF", 30, SCREEN_WIDTH + 10, 80);
    draw_text(to_string(stats.level), COLOR_YELLOW, "04B_30__.TTF", 20, SCREEN_WIDTH + 10, 110);

    int seconds = static_cast<int>(stats.gameTime / 1000.0);
    draw_text("TIME: " + to_string(seconds) + "s", COLOR_WHITE, "04B_30__.TTF", 20, SCREEN_WIDTH + 10, 140);

    // Draw level select menu
    if (state.showLevelSelect) 
    {
        draw_text("SELECT LEVEL", COLOR_WHITE, "04B_30__.TTF", 15, SCREEN_WIDTH + 10, 170);
        for (int i = 1; i <= MAX_LEVEL; i++) 
        {
            color c = (i == state.selectedLevel) ? COLOR_CYAN : COLOR_WHITE;
            draw_text("Level " + to_string(i), c, "04B_30__.TTF", 15, SCREEN_WIDTH + 30, 170 + i * 20);
        }
    }

    // Draw keybinds
    if (state.gameStarted && !state.showLevelSelect)
    {
        const int kb_x = SCREEN_WIDTH + 10;
        int kb_y = 200;               
        const int line_h = 20;
        const string font = "04B_30__.TTF";
        const int size = 10;

        draw_text("KEYBINDS:", COLOR_WHITE, font, 8, kb_x, kb_y);
        kb_y += line_h;
        draw_text("Left Arrow : Left", COLOR_YELLOW, font, size, kb_x, kb_y);
        kb_y += line_h;
        draw_text("Right Arrow : Right", COLOR_YELLOW, font, size, kb_x, kb_y);
        kb_y += line_h;
        draw_text("Up Arrow : Rotate", COLOR_YELLOW, font, size, kb_x, kb_y);
        kb_y += line_h;
        draw_text("Down Arrow : Soft Drop", COLOR_YELLOW, font, size, kb_x, kb_y);
        kb_y += line_h;
        draw_text("Space : Hard Drop", COLOR_YELLOW, font, size, kb_x, kb_y);
        kb_y += line_h;
        draw_text("Esc : Pause", COLOR_YELLOW, font, size, kb_x, kb_y);
    }

    // Draw highest score
    draw_text("HIGHEST", COLOR_WHITE, "04B_30__.TTF", 30, SCREEN_WIDTH + 10, 500);
    draw_text(to_string(stats.highScore), COLOR_YELLOW, "04B_30__.TTF", 20, SCREEN_WIDTH + 10, 530);
}

// Function to draw the PLAY and RESTART buttons
void draw_buttons() 
{
    if (!state.gameStarted) 
    {
        // PLAY button
        fill_rectangle(COLOR_GREEN, SCREEN_WIDTH + 20, 300, 120, 40);
        draw_text("PLAY", COLOR_WHITE, "Litebulb 8-bit.TTF", 50, SCREEN_WIDTH + 55, 300);
    } 
    else if (state.gamePaused || state.gameOver) 
    {
        // RESTART button
        fill_rectangle(COLOR_RED, SCREEN_WIDTH + 20, 360, 120, 40);
        draw_text("RESTART", COLOR_WHITE, "Litebulb 8-bit.TTF", 50, SCREEN_WIDTH + 30, 360);
    }
}

// Function to reset the game state for a new game
void reset_game() 
{
    // Clear the board
    board = vector<vector<int>>(GRID_HEIGHT, vector<int>(GRID_WIDTH, 0));
    stats.reset(state.selectedLevel);
    state.startTime = current_ticks();
    spawn_new_tetromino();
    gameTimer.reset();
}

// Calculates the drop delay (speed) based on level and score
int get_drop_delay()
{
    // Set a base delay: higher levels start faster
    int base_delay = 70 - (stats.level - 1) * 8;

    // Increase drop speed based on score milestones
    int milestones = min(stats.score / 1000, 4);

    // Each milestone reduces the delay by 4 ticks
    int bonus_speed = milestones * 4;

    // Pieces will never drop faster than every 20 ticks.
    const int MAX_DROP = 20;

    // Calculate the final delay, applying milestone bonuses and enforcing the minimum.
    int final_delay = base_delay - bonus_speed;
    return max(final_delay, MAX_DROP);
}

// Function to handle mouse clicks on the PLAY and RESTART buttons
void button_clicks(point_2d mouse) 
{
    // PLAY button
    if (!state.gameStarted && mouse.x > SCREEN_WIDTH + 20 && mouse.x < SCREEN_WIDTH + 140 && mouse.y > 300 && mouse.y < 340) 
    {
        play_sound_effect(assets.click_sfx);
        state.startGame();
        play_music(assets.bgm, -1);
        reset_game();
    }

    // RESTART button: Only show if the game is paused
    if ((state.gamePaused) && mouse.x > SCREEN_WIDTH + 20 && mouse.x < SCREEN_WIDTH + 140 && mouse.y > 360 && mouse.y < 400) 
    {
        play_sound_effect(assets.click_sfx);
        state.gameOver = false;
        state.gamePaused = false;
        state.gameStarted = false;
        state.showLevelSelect = true;
    }
}

// Function to handle mouse clicks for selecting the starting level
void level_selection(point_2d mouse) 
{
    if (!state.showLevelSelect)
    {
        return;
    }

    // Loop through all selectable levels
    for (int i = 1; i <= MAX_LEVEL; i++) 
    {
        // Check if mouse is within the bounds of a level option
        if (mouse.x > SCREEN_WIDTH + 30 && mouse.x < SCREEN_WIDTH + 130 && 
            mouse.y > 160 + i * 20 && mouse.y < 180 + i * 20) 
        {
            state.selectedLevel = i;
        }
    }
}

// Function to handle keyboard input for gameplay (movement, rotation, hard drop)
void gameplay_input() 
{
    // Move left if left arrow is pressed and no collision
    if (key_typed(LEFT_KEY) && !check_collision(Tetromino(currentPiece.shape, currentPiece.rotation, currentPiece.pos.x - 1, currentPiece.pos.y)))
    {
        currentPiece.pos.x--;
    }
    
    // Move right if right arrow is pressed and no collision
    if (key_typed(RIGHT_KEY) && !check_collision(Tetromino(currentPiece.shape, currentPiece.rotation, currentPiece.pos.x + 1, currentPiece.pos.y))) 
    {
        currentPiece.pos.x++;
    }
    
    // Rotate if up arrow is pressed and no collision after rotation
    if (key_typed(UP_KEY)) 
    {
        int newRot = (currentPiece.rotation + 1) % 4;
        if (!check_collision(Tetromino(currentPiece.shape, newRot, currentPiece.pos.x, currentPiece.pos.y))) 
        {
            currentPiece.rotation = newRot;
        }
    }
    
    // Hard drop (spacebar): move piece down until it collides, then lock it
    if (key_typed(SPACE_KEY)) 
    {
        while (!check_collision(Tetromino(currentPiece.shape, currentPiece.rotation, currentPiece.pos.x, currentPiece.pos.y + 1))) 
        {
            currentPiece.pos.y++;
        }
        lock_tetromino();
        clear_lines();
        spawn_new_tetromino();
    }
}

// Function to handle pause/unpause input (ESC key)
void pause_input() 
{
    if (key_typed(ESCAPE_KEY)) 
    {
        state.togglePause();
        if (state.gamePaused)
        {
            pause_music();
        }
        else
        {
            resume_music();
        }
    }
}

// Function to the game timer and state
void update_game_state() 
{
    if (!state.gameStarted || state.gamePaused || state.gameOver) 
    {
        return;
    }

    // Update game timer
    stats.gameTime += current_ticks() - state.startTime;
    state.startTime = current_ticks();
}

// Function to handle piece dropping (automatic and soft drop), and locks piece if needed
void update_timers() 
{
    // Only update timers if the game is running and not paused or over
    if (!state.gameStarted || state.gamePaused || state.gameOver) 
    {
        return;
    }

    // If the down arrow key is held, soft drop the piece every 5 ticks
    if (key_down(DOWN_KEY) && gameTimer.dropTimer % 5 == 0) 
    {
        // Check if moving down by 1 would cause a collision
        if (!check_collision(Tetromino(currentPiece.shape, currentPiece.rotation, currentPiece.pos.x, currentPiece.pos.y + 1))) 
        {
            currentPiece.pos.y++;
        }
    }
    
    // Increment the drop timer every frame
    gameTimer.dropTimer++;

    // If the drop timer reaches the current drop delay threshold (based on level/score)
    if (gameTimer.dropTimer >= get_drop_delay()) 
    {
        gameTimer.dropTimer = 0;

        // Try to move the piece down by one row
        if (!check_collision(Tetromino(currentPiece.shape, currentPiece.rotation, currentPiece.pos.x, currentPiece.pos.y + 1))) 
        {
            currentPiece.pos.y++;
        } 
        else 
        {
            lock_tetromino();
            clear_lines();
            spawn_new_tetromino();
        }
    }
}

// Function to handle music fade-in and fade-out effects
void music_fade(double dt) 
{
    // Fade-in
    if (state.gameStarted && !state.gameOver && !assets.fading_out && assets.fade_volume < 1.0f) 
    {
        assets.fade_volume += static_cast<float>(dt / 2.0);
        assets.fade_volume = min(assets.fade_volume, 1.0f);
        set_music_volume(assets.fade_volume);
    }

    // Start fade-out when game is over and music is playing
    if (!assets.fading_out && state.gameOver && music_playing()) 
    {
        assets.fading_out = true;
    }

    // Fade-out
    if (assets.fading_out) 
    {
        assets.fade_volume -= static_cast<float>(dt / 2.0);
        if (assets.fade_volume <= 0.0f) 
        {
            assets.fade_volume = 0.0f;
            stop_music();
            assets.fading_out = false;
        }
        set_music_volume(assets.fade_volume);
    }
}

// Function to initialize the game: window, images, audio, and highest score
void initialize_game() 
{
    srand(time(NULL)); // Seed random number generator
    open_window("Tetris", WINDOW_WIDTH, WINDOW_HEIGHT); // Open game window
    
    assets.initialize(); // Load images and audio
    load_highest_score(); // Load highest score from file
}

// MAIN FUNCTION 
int main() 
{
    initialize_game(); // Initialize all game resources and state
    
    while (!window_close_requested("Tetris")) 
    {
        process_events(); // Handle window and input events
        point_2d mouse = mouse_position(); // Get current mouse position
        
        double dt = gameTimer.getDeltaTime(); // Calculate time since last frame

        // Handle mouse input for buttons and level selection
        if (mouse_clicked(LEFT_BUTTON)) 
        {
            button_clicks(mouse); // Handle PLAY/RESTART button clicks
            level_selection(mouse); // Handle level selection clicks
        }
        
        // Handle keyboard/game state input
        if (state.gameStarted && !state.gamePaused && !state.gameOver) 
        {
            update_game_state(); // Update timer and state
            pause_input(); // Handle pause input
            gameplay_input(); // Handle movement/rotation/drop
            update_timers(); // Handle piece dropping
        }
        else if (state.gameStarted) 
        {
            pause_input(); // Allow pause/unpause even if paused/over
        }

        // Handle music fade-in/fade-out
        music_fade(dt);

        draw_game();      // Draw the game scene
        draw_buttons();   // Draw UI buttons
        refresh_screen(60); // Refresh at 60 FPS
    }

    return 0;
}