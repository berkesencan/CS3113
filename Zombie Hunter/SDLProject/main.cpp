/**
 * Author: Berk Esencan
 * Assignment: Zombie Hunter
 * Date due: 2025-04-25, 2:00pm
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
//VIEWPORT SCREEN SIZES
constexpr int WINDOW_WIDTH   = 1280,
              WINDOW_HEIGHT  = 920;

//PURPLE BACKGROUND BGM COLOR
constexpr float BG_RED       = 0.4922f,
               BG_BLUE       = 0.549f,
               BG_GREEN      = 0.9059f,
               BG_OPACITY    = 1.0f;

constexpr int VIEWPORT_X     = 0,
              VIEWPORT_Y     = 0,
              VIEWPORT_WIDTH = WINDOW_WIDTH,
              VIEWPORT_HEIGHT= WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
              F_SHADER_PATH[]  = "shaders/fragment_textured.glsl",
                F_EFFECT_PATH[]  = "shaders/effects_textured.glsl";  //TO CHANGE THE SHADER DAMAGE EFFECT



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
ShaderProgram g_effect_program;       // FOR SHADER EFFECT
glm::mat4     g_view_matrix       = glm::mat4(1.0f);
glm::mat4     g_projection_matrix = glm::mat4(1.0f);

SDL_Window*    g_display_window   = nullptr;
AppStatus      g_app_status       = RUNNING;

constexpr float SHOOT_COOLDOWN = 1.0f;   // seconds between shots
float g_shoot_timer = 0.0f;


float g_previous_ticks = 0.0f;
float g_accumulator    = 0.0f;
bool  g_is_colliding_bottom = false;

int g_player_lives = 3;

int g_score = 0;

// bullet globals
std::vector<Entity*> g_bullets;     // active bullets
GLuint               g_bullet_tex;  // loaded once

// ––––– GENERAL FUNCTIONS ––––– //
void switch_to_scene(Scene* scene)
{
    g_current_scene = scene;
    g_current_scene->initialise(); // DON'T FORGET THIS STEP!
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Project 5: Zombie Hunter",
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
    
    //SHADER EFFECT FOR THE CHARACTER
    g_effect_program.load(V_SHADER_PATH, F_EFFECT_PATH);


    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.00f, 3.75f, -1.0f, 1.0f); //TO CHANGE THE CAMERA

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    //SHADER EFFECT
    g_effect_program.set_projection_matrix(g_projection_matrix);
    g_effect_program.set_view_matrix(g_view_matrix);

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

    //loading bullet texture once
    g_bullet_tex = Utility::load_texture("assets/bullet3.png");

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

        // PRESS SHIFT TO SHOOT
        static bool shift_prev = false;
        bool shift_now = key_state[SDL_SCANCODE_LSHIFT] || key_state[SDL_SCANCODE_RSHIFT];

        if (shift_now && !shift_prev && g_shoot_timer <= 0.0f)      //COOLDOWN
        {
            Entity* player = g_current_scene->get_state().player;
            if (player)
            {
                Entity* bullet = new Entity(g_bullet_tex, 6.0f, 0.2f, 0.2f, BULLET);
                bullet->set_position(player->get_position());
                bullet->set_movement(glm::vec3((float)player->get_facing_dir(), 0.0f, 0.0f));
                g_bullets.push_back(bullet);
                g_shoot_timer = SHOOT_COOLDOWN;                      //RESET
            }
        }
        shift_prev = shift_now;
    }
}

void update()
{
    float ticks      = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    g_shoot_timer -= FIXED_TIMESTEP;
    if (g_shoot_timer < 0.0f) g_shoot_timer = 0.0f;

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

    
//SHOOTING + ENEMY CODE
    
    // Updating bullets + hit‑test + enemies + platform
    for (std::vector<Entity*>::iterator it = g_bullets.begin(); it != g_bullets.end(); )
    {
        Entity* b = *it;
        b->update(FIXED_TIMESTEP, nullptr, nullptr, 0, g_current_scene->get_state().map);

        //Checking if bullet off screen
        bool off =  b->get_position().x < 0.5f  || b->get_position().x > 23.5f ||
                    b->get_position().y < -10.0f|| b->get_position().y > 10.0f;

        //Checking if bullet hits the map
        bool map_hit =  b->get_collided_left()   || b->get_collided_right() ||
                        b->get_collided_top()    || b->get_collided_bottom();

        bool enemy_hit = false;
        if (!off && !map_hit)
        {
            // testing one Entity pointer is active)
            auto test_enemy = [&](Entity* e)->bool{
                return e && e->get_is_active() && e->check_collision(b);
            };

            //If it hit enemy
            Entity* arr   = g_current_scene->get_state().enemies;
            int     count = 0;
            if (arr == g_levelA->get_state().enemies) count = g_levelA->ENEMY_COUNT;
            else if (arr == g_levelB->get_state().enemies) count = g_levelB->ENEMY_COUNT;
            else if (arr == g_levelC->get_state().enemies) count = g_levelC->ENEMY_COUNT;

            for (int i = 0; i < count && !enemy_hit; ++i)
                if (test_enemy(&arr[i])) {
                    if (arr[i].take_hit())      // returns true when enemy just died
                           g_score++;
                    Mix_Chunk* s = g_current_scene->get_state().enemy_hit_sfx;
                    if (s) Mix_PlayChannel(-1, s, 0);

                       enemy_hit = true;
                       break;
                }

            //If it hit enemy for the spawned
            if (!enemy_hit)
            {
                const std::vector<Entity*>* dyn = nullptr;
                if (g_current_scene == g_levelA) dyn = &g_levelA->get_spawned_enemies();
                else if (g_current_scene == g_levelB) dyn = &g_levelB->get_spawned_enemies();
                else if (g_current_scene == g_levelC) dyn = &g_levelC->get_spawned_enemies();

                if (dyn)
                    for (Entity* e : *dyn)
                        if (test_enemy(e)) {
                            if (e->take_hit()){
                                g_score++;
                            }
                            Mix_Chunk* s = g_current_scene->get_state().enemy_hit_sfx;
                            if (s) Mix_PlayChannel(-1, s, 0);
                            enemy_hit = true;
                            break;
                        }
            }
        }

        // Removing bullet
        if (off || map_hit || enemy_hit)
        {
            delete b;
            it = g_bullets.erase(it);
        }
        else
        {
            ++it;
        }
    }


    if (g_current_scene->get_state().player) {
        float px = g_current_scene->get_state().player->get_position().x;
        float py = g_current_scene->get_state().player->get_position().y;

        g_view_matrix = glm::mat4(1.0f);

        float leftLimit  = 5.0f;
        float rightLimit = 19.0f;

        if (px < leftLimit)  px = leftLimit;
        if (px > rightLimit) px = rightLimit;
        
        //Clamping the bottom
        if (g_current_scene->get_state().map)
            {
                float mapBottom     = g_current_scene->get_state().map->get_bottom_bound();
                
                float halfHeight    = 3.00f;
                float lowestCameraY = mapBottom + halfHeight;

                if (py < lowestCameraY) py = lowestCameraY;
            }

        g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-px, -py, 0.0f));
    }

    int next_id = g_current_scene->get_state().next_scene_id;
    if (next_id != -1) {
        for (Entity* b : g_bullets) delete b;
        g_bullets.clear();
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
    g_effect_program.set_view_matrix(g_view_matrix); 
    g_current_scene->render(&g_shader_program);

    // Drawing bullets last
    for (Entity* b : g_bullets) {
        b->render(&g_shader_program);
    }

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    // free bullets so game doesn't crash
    for (Entity* b : g_bullets) delete b;
    g_bullets.clear();

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
