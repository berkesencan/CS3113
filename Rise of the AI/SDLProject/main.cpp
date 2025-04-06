/**
 * Author: Berk Esencan
 * Assignment: Rise of the AI
 * Date due: 2025-04-05, 11:59pm
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 */

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define LEVEL1_WIDTH 14
#define LEVEL1_HEIGHT 8
#define LEVEL1_LEFT_EDGE 5.0f

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include "Map.h"
#include "Utility.h"
#include "Scene.h"
#include "Menu.h"
#include "LevelA.h"
#include "LevelB.h"
#include "LevelC.h"
#include "WinScene.h"
#include "LoseScene.h"

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH   = 640,
              WINDOW_HEIGHT  = 480;

constexpr float BG_RED       = 0.1922f,
               BG_BLUE       = 0.549f,
               BG_GREEN      = 0.9059f,
               BG_OPACITY    = 1.0f;

constexpr int VIEWPORT_X     = 0,
              VIEWPORT_Y     = 0,
              VIEWPORT_WIDTH = WINDOW_WIDTH,
              VIEWPORT_HEIGHT= WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
              F_SHADER_PATH[]  = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

// ––––– ENUMS ––––– //
enum AppStatus { RUNNING, TERMINATED };

// ––––– GLOBAL VARIABLES ––––– //
Scene*      g_current_scene  = nullptr;
Menu*       g_menu           = nullptr;
LevelA*     g_levelA         = nullptr;
LevelB*     g_levelB         = nullptr;
LevelC*     g_levelC         = nullptr;
WinScene*   g_win_scene      = nullptr;
LoseScene*  g_lose_scene     = nullptr;

ShaderProgram g_shader_program;
glm::mat4     g_view_matrix       = glm::mat4(1.0f);
glm::mat4     g_projection_matrix = glm::mat4(1.0f);

SDL_Window*    g_display_window   = nullptr;
AppStatus      g_app_status       = RUNNING;

float g_previous_ticks = 0.0f;
float g_accumulator    = 0.0f;
bool  g_is_colliding_bottom = false;

int g_player_lives = 3;

// ––––– GENERAL FUNCTIONS ––––– //
void switch_to_scene(Scene* scene)
{
    g_current_scene = scene;
    g_current_scene->initialise(); // DON'T FORGET THIS STEP!
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Project 4: Rise of the AI",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_menu       = new Menu();
    g_levelA     = new LevelA();
    g_levelB     = new LevelB();
    g_levelC     = new LevelC();
    g_win_scene  = new WinScene();
    g_lose_scene = new LoseScene();

    switch_to_scene(g_menu);
}

void process_input()
{
    if (g_current_scene->get_state().player) {
        g_current_scene->get_state().player->set_movement(glm::vec3(0.0f));
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        g_app_status = TERMINATED;
                        break;

                    case SDLK_RETURN:
                        if (g_current_scene == g_menu ||
                            g_current_scene == g_lose_scene ||
                            g_current_scene == g_win_scene)
                        {
                            g_current_scene->get_state().next_scene_id = 1; // SCENE_LEVEL_A
                        }
                        break;

                    case SDLK_SPACE:
                        if (g_current_scene->get_state().player &&
                            g_current_scene->get_state().player->get_collided_bottom())
                        {
                            g_current_scene->get_state().player->jump();
                            Mix_PlayChannel(-1, g_current_scene->get_state().jump_sfx, 0);
                        }
                        break;

                    default:
                        break;
                }
                break;
        }
    }

    if (g_current_scene->get_state().player) {
        const Uint8* key_state = SDL_GetKeyboardState(NULL);

        if (key_state[SDL_SCANCODE_LEFT]) {
            g_current_scene->get_state().player->move_left();
        }
        else if (key_state[SDL_SCANCODE_RIGHT]) {
            g_current_scene->get_state().player->move_right();
        }
    }
}

void update()
{
    float ticks      = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP)
    {
        g_current_scene->update(FIXED_TIMESTEP);
        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;

    if (g_current_scene->get_state().player) {
        float px = g_current_scene->get_state().player->get_position().x;
        float py = g_current_scene->get_state().player->get_position().y;

        g_view_matrix = glm::mat4(1.0f);

        float leftLimit  = 5.0f;
        float rightLimit = 9.0f;

        if (px < leftLimit)  px = leftLimit;
        if (px > rightLimit) px = rightLimit;

        g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-px, -py, 0.0f));
    }

    int next_id = g_current_scene->get_state().next_scene_id;
    if (next_id != -1) {
        switch (next_id) {
            case 0:
                switch_to_scene(g_menu);
                break;
            case 1:
                switch_to_scene(g_levelA);
                break;
            case 2:
                switch_to_scene(g_levelB);
                break;
            case 3:
                switch_to_scene(g_levelC);
                break;
            case 4:
                switch_to_scene(g_win_scene);
                break;
            case 5:
                switch_to_scene(g_lose_scene);
                break;
        }
    }
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    g_shader_program.set_view_matrix(g_view_matrix);
    g_current_scene->render(&g_shader_program);

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

    delete g_menu;
    delete g_levelA;
    delete g_levelB;
    delete g_levelC;
    delete g_win_scene;
    delete g_lose_scene;
}

// ––––– DRIVER GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
