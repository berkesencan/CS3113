/**
 * be752
 * Author: Berk Esencan
 * Assignment: Pong
 * Date due: 2025-03-15, 11:59pm
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 **/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <cmath>
#include <cassert>
#include <SDL.h>
#include <SDL_opengl.h>
#include <iostream>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640 * 2;
constexpr int WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 0.9765625f;
constexpr float BG_GREEN   = 0.97265625f;
constexpr float BG_BLUE    = 0.9609375f;
constexpr float BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0;
constexpr int VIEWPORT_Y      = 0;
constexpr int VIEWPORT_WIDTH  = WINDOW_WIDTH;
constexpr int VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl";
constexpr char F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0f;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

//The textures
constexpr char TOM_SPRITE_FILEPATH[]    = "tom.png";     // Tom's paddle
constexpr char JERRY_SPRITE_FILEPATH[]  = "jerry.png";   // Jerry's paddle
constexpr char BALL_SPRITE_FILEPATH[]   = "cheesejerry.png"; // Ball
constexpr char TOM_WIN_FILEPATH[]       = "tom.png";     // Tom wins
constexpr char JERRY_WIN_FILEPATH[]     = "jerry.png";   // Jerry wins
constexpr char BACKGROUND_FILEPATH[]    = "TomandJerryBackground.png"; // Background

//The bounds
constexpr float LEFT_BOUND   = -5.0f;
constexpr float RIGHT_BOUND  =  5.0f;
constexpr float TOP_BOUND    =  3.75f;
constexpr float BOTTOM_BOUND = -3.75f;

// Paddle information
constexpr float PADDLE_WIDTH       = 1.0f;
constexpr float PADDLE_HEIGHT      = 2.0f;
constexpr float PADDLE_HALF_WIDTH  = PADDLE_WIDTH  * 0.5f;
constexpr float PADDLE_HALF_HEIGHT = PADDLE_HEIGHT * 0.5f;
constexpr float PADDLE_SPEED       = 5.0f;

// Paddle initial positions
constexpr float TOM_X      = -4.5f;  // far left
constexpr float JERRY_X    =  4.5f;  // far right
constexpr float INIT_PADDLE_Y = 0.0f;

// Ball size and initial movement
constexpr float BALL_SIZE       = 0.5f;
constexpr float BALL_HALF_SIZE  = BALL_SIZE * 0.5f;
constexpr float BALL_SPEED      = 2.0f;

// The number of points needed to declare a winner
constexpr int POINTS_TO_WIN = 5;

// Global variables
SDL_Window*   g_display_window   = nullptr;
AppStatus     g_app_status       = RUNNING;
ShaderProgram g_shader_program;

glm::mat4 g_view_matrix;
glm::mat4 g_projection_matrix;

glm::mat4 g_tom_matrix;
glm::mat4 g_jerry_matrix;
glm::mat4 g_ball_matrices[3];  // Up to 3 balls
glm::mat4 g_background_matrix; // Background matrix

//Paddle position and velocity
float g_tom_y          = INIT_PADDLE_Y;
float g_tom_velocity   = 0.0f;
float g_jerry_y        = INIT_PADDLE_Y;
float g_jerry_velocity = 0.0f;

//Ball information
struct Ball {
    float x = 0.0f;
    float y = 0.0f;
    float vel_x = BALL_SPEED;
    float vel_y = BALL_SPEED;
    bool  active = false;
};
Ball g_balls[3];

// Game states
bool  g_game_over          = false;
bool  g_single_player_mode = false;
float g_jerry_ai_dir       = 1.0f;
int   g_active_balls       = 1;
int   g_winner             = 0;

// Scoring
int   g_tom_score          = 0;
int   g_jerry_score        = 0;

// Timing
float g_previous_ticks = 0.0f;

// Texture IDs
GLuint g_tom_texture_id   = 0;
GLuint g_jerry_texture_id = 0;
GLuint g_ball_texture_id  = 0;
GLuint g_tom_win_texture_id  = 0;
GLuint g_jerry_win_texture_id = 0;
GLuint g_background_texture_id = 0;


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void reinit_ball(int i) {
    g_balls[i].active = (i < g_active_balls);
    //Each ball in the center
    g_balls[i].x = 0.0f;
    g_balls[i].y = 0.0f;

    //Velocuty of balls
    if (i % 2 == 0) {
        g_balls[i].vel_x = BALL_SPEED;
    } else {
        g_balls[i].vel_x = -BALL_SPEED;
    }
    g_balls[i].vel_y = (i == 2) ? -BALL_SPEED : BALL_SPEED;
}

void initialise() {
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Tom & Jerry Pong!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);

    if (!g_display_window) {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(
        LEFT_BOUND, RIGHT_BOUND,
        BOTTOM_BOUND, TOP_BOUND,
        -1.0f, 1.0f);

    g_shader_program.set_view_matrix(g_view_matrix);
    g_shader_program.set_projection_matrix(g_projection_matrix);
    
    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    // Load textures
    g_tom_texture_id       = load_texture(TOM_SPRITE_FILEPATH);
    g_jerry_texture_id     = load_texture(JERRY_SPRITE_FILEPATH);
    g_ball_texture_id      = load_texture(BALL_SPRITE_FILEPATH);
    g_tom_win_texture_id   = load_texture(TOM_WIN_FILEPATH);
    g_jerry_win_texture_id = load_texture(JERRY_WIN_FILEPATH);
    g_background_texture_id= load_texture(BACKGROUND_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Init transforms
    g_tom_matrix      = glm::mat4(1.0f);
    g_jerry_matrix    = glm::mat4(1.0f);
    g_background_matrix = glm::mat4(1.0f);

    //Default 1 ball
    g_active_balls = 1;
    for (int i = 0; i < 3; i++) {
        reinit_ball(i);
    }

    // Starting paddles
    g_tom_y    = INIT_PADDLE_Y;
    g_jerry_y  = INIT_PADDLE_Y;

    // Reset scores
    g_tom_score    = 0;
    g_jerry_score  = 0;

    g_previous_ticks = static_cast<float>(SDL_GetTicks()) / MILLISECONDS_IN_SECOND;
}

//Collision check
bool check_collision(float x1, float y1, float halfW1, float halfH1,
                     float x2, float y2, float halfW2, float halfH2) {
    return (std::fabs(x1 - x2) <= (halfW1 + halfW2)) &&
           (std::fabs(y1 - y2) <= (halfH1 + halfH2));
}


//For keyboard inputs
void process_input() {
    if (g_app_status == TERMINATED) return;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            g_app_status = TERMINATED;
            return;
        }
        else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_t:
                    g_single_player_mode = !g_single_player_mode;
                    if (g_single_player_mode) {
                        g_jerry_ai_dir = 1.0f;  //If pressed T jerry becomes AI
                    }
                    break;

                //Case for 1,2,3 balls
                case SDLK_1:
                    g_active_balls = 1;
                    for (int i = 0; i < 3; i++) {
                        reinit_ball(i);
                    }
                    break;
                case SDLK_2:
                    g_active_balls = 2;
                    for (int i = 0; i < 3; i++) {
                        reinit_ball(i);
                    }
                    break;
                case SDLK_3:
                    g_active_balls = 3;
                    for (int i = 0; i < 3; i++) {
                        reinit_ball(i);
                    }
                    break;
            }
        }
    }

    if (g_game_over) return;

    // Paddle controls
    const Uint8* keys = SDL_GetKeyboardState(NULL);

    float tom_input = 0.0f;
    if (keys[SDL_SCANCODE_W]) tom_input += 1.0f;
    if (keys[SDL_SCANCODE_S]) tom_input -= 1.0f;
    g_tom_velocity = tom_input * PADDLE_SPEED;

    // If multiplayer Jerry is controlled by arrows
    if (!g_single_player_mode) {
        float jerry_input = 0.0f;
        if (keys[SDL_SCANCODE_UP])   jerry_input += 1.0f;
        if (keys[SDL_SCANCODE_DOWN]) jerry_input -= 1.0f;
        g_jerry_velocity = jerry_input * PADDLE_SPEED;
    }
    else {
        // Simple up/down AI JERRY
        g_jerry_velocity = g_jerry_ai_dir * PADDLE_SPEED;
    }
}


void check_for_winner() {
    if (g_tom_score >= POINTS_TO_WIN) {
        g_winner = 1;
        g_game_over = true;
    }
    else if (g_jerry_score >= POINTS_TO_WIN) {
        g_winner = 2;
        g_game_over = true;
    }
}


void update() {
    float ticks = static_cast<float>(SDL_GetTicks()) / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    if (g_app_status == TERMINATED || g_game_over) return;

    // Updating Tom paddle
    g_tom_y += g_tom_velocity * delta_time;
    float tom_top    = g_tom_y + PADDLE_HALF_HEIGHT;
    float tom_bottom = g_tom_y - PADDLE_HALF_HEIGHT;
    if (tom_top > TOP_BOUND) {
        g_tom_y = TOP_BOUND - PADDLE_HALF_HEIGHT;
    }
    if (tom_bottom < BOTTOM_BOUND) {
        g_tom_y = BOTTOM_BOUND + PADDLE_HALF_HEIGHT;
    }

    // Updating Jerry paddle
    g_jerry_y += g_jerry_velocity * delta_time;
    float jerry_top    = g_jerry_y + PADDLE_HALF_HEIGHT;
    float jerry_bottom = g_jerry_y - PADDLE_HALF_HEIGHT;

    if (g_single_player_mode) {
        //Jerry's AI
        if (jerry_top > TOP_BOUND) {
            g_jerry_y = TOP_BOUND - PADDLE_HALF_HEIGHT;
            g_jerry_ai_dir = -1.0f;
        }
        else if (jerry_bottom < BOTTOM_BOUND) {
            g_jerry_y = BOTTOM_BOUND + PADDLE_HALF_HEIGHT;
            g_jerry_ai_dir = 1.0f;
        }
    }
    else {
        if (jerry_top > TOP_BOUND) {
            g_jerry_y = TOP_BOUND - PADDLE_HALF_HEIGHT;
        }
        if (jerry_bottom < BOTTOM_BOUND) {
            g_jerry_y = BOTTOM_BOUND + PADDLE_HALF_HEIGHT;
        }
    }

    //Ball updates
    for (int i = 0; i < 3; i++) {
        if (!g_balls[i].active) continue;

        // Moving the ball
        g_balls[i].x += g_balls[i].vel_x * delta_time;
        g_balls[i].y += g_balls[i].vel_y * delta_time;

        //Bouncing
        float ball_top    = g_balls[i].y + BALL_HALF_SIZE;
        float ball_bottom = g_balls[i].y - BALL_HALF_SIZE;
        if (ball_top > TOP_BOUND) {
            g_balls[i].y = TOP_BOUND - BALL_HALF_SIZE;
            g_balls[i].vel_y = -g_balls[i].vel_y;
        }
        else if (ball_bottom < BOTTOM_BOUND) {
            g_balls[i].y = BOTTOM_BOUND + BALL_HALF_SIZE;
            g_balls[i].vel_y = -g_balls[i].vel_y;
        }

        //To see who scores
        float ball_left  = g_balls[i].x - BALL_HALF_SIZE;
        float ball_right = g_balls[i].x + BALL_HALF_SIZE;
        
        // If ball goes out on left Jerry scores
        if (ball_left < LEFT_BOUND) {
            g_jerry_score++;
            // Reinitialize that ball in center
            reinit_ball(i);
            check_for_winner();
        }
        // If ball goes out on right Tom scores
        else if (ball_right > RIGHT_BOUND) {
            g_tom_score++;
            // Reinitialize that ball in center
            reinit_ball(i);
            check_for_winner();
        }
        else {
            // If still in play, check collisions with paddles
            bool collide_tom = check_collision(
                g_balls[i].x, g_balls[i].y,
                BALL_HALF_SIZE, BALL_HALF_SIZE,
                TOM_X, g_tom_y,
                PADDLE_HALF_WIDTH, PADDLE_HALF_HEIGHT
            );
            if (collide_tom && (g_balls[i].vel_x < 0.0f)) {
                g_balls[i].vel_x = -g_balls[i].vel_x;
                g_balls[i].x = TOM_X + (PADDLE_HALF_WIDTH + BALL_HALF_SIZE);
            }

            bool collide_jerry = check_collision(
                g_balls[i].x, g_balls[i].y,
                BALL_HALF_SIZE, BALL_HALF_SIZE,
                JERRY_X, g_jerry_y,
                PADDLE_HALF_WIDTH, PADDLE_HALF_HEIGHT
            );
            if (collide_jerry && (g_balls[i].vel_x > 0.0f)) {
                g_balls[i].vel_x = -g_balls[i].vel_x;
                g_balls[i].x = JERRY_X - (PADDLE_HALF_WIDTH + BALL_HALF_SIZE);
            }
        }

        //Matrix for the ball
        g_ball_matrices[i] = glm::mat4(1.0f);
        g_ball_matrices[i] = glm::translate(g_ball_matrices[i],
                                            glm::vec3(g_balls[i].x,
                                                      g_balls[i].y,
                                                      0.0f));
    }

    // Matrix for the paddle
    g_tom_matrix = glm::mat4(1.0f);
    g_tom_matrix = glm::translate(g_tom_matrix,
                                  glm::vec3(TOM_X, g_tom_y, 0.0f));

    g_jerry_matrix = glm::mat4(1.0f);
    g_jerry_matrix = glm::translate(g_jerry_matrix,
                                    glm::vec3(JERRY_X, g_jerry_y, 0.0f));
}


void draw_object(const glm::mat4 &model_matrix,
                 GLuint texture_id,
                 float width,
                 float height) {
    g_shader_program.set_model_matrix(model_matrix);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    float half_w = width * 0.5f;
    float half_h = height * 0.5f;

    float vertices[] = {
        // Triangle 1
        -half_w, -half_h, half_w, -half_h, half_w,  half_h,
        // Triangle 2
        -half_w, -half_h, half_w,  half_h, -half_w,  half_h
    };

    float tex_coords[] = {
        // match corners above
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,

        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(),
                          2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(),
                          2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
}


void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Drawing background
    draw_object(g_background_matrix,
                g_background_texture_id,
                RIGHT_BOUND - LEFT_BOUND,
                TOP_BOUND - BOTTOM_BOUND);

    // Drawing paddles
    draw_object(g_tom_matrix,   g_tom_texture_id,   PADDLE_WIDTH, PADDLE_HEIGHT);
    draw_object(g_jerry_matrix, g_jerry_texture_id, PADDLE_WIDTH, PADDLE_HEIGHT);

    // Drawing active balls
    for (int i = 0; i < 3; i++) {
        if (g_balls[i].active) {
            draw_object(g_ball_matrices[i],
                        g_ball_texture_id,
                        BALL_SIZE,
                        BALL_SIZE);
        }
    }

    // Showing the winner
    if (g_game_over && g_winner != 0) {
        glm::mat4 winner_matrix = glm::mat4(1.0f);
        // Center the winner texture
        winner_matrix = glm::translate(winner_matrix, glm::vec3(0.0f, 0.0f, 0.0f));

        if (g_winner == 1) {
            // Tom wins drawing tom
            draw_object(winner_matrix, g_tom_win_texture_id, 4.0f, 2.0f);
        } else {
            // Jerry wins drawing jerry
            draw_object(winner_matrix, g_jerry_win_texture_id, 4.0f, 2.0f);
        }
    }

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    initialise();

    while (g_app_status == RUNNING) {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
