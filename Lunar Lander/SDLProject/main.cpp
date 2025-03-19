/**
* Author: Berk Esencan
* Assignment: Lunar Lander
* Date due: 2025-3-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 5

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <cstdlib>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Entity* platforms;
};

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

constexpr float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr char SPRITESHEET_FILEPATH[] = "assets/george_0.png";
constexpr char PLATFORM_FILEPATH[]    = "assets/platformPack_tile027.png";

//Used the font inside the assets
constexpr char FONT_FILEPATH[] = "assets/font1.png";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;

//DELETED THE MUSIC!

// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

//The main lunar ladder variables
float g_fuel = 10.0f;
float g_rotation_speed = 2.0f;
float g_thrust = 4.0f;
float g_fuel_use_rate = 2.0f;
int g_landing_platform = PLATFORM_COUNT - 1;
bool g_game_over = false;
bool g_mission_accomplished = false;

// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Hello, Physics (again)!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    // ––––– VIDEO ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    //DELETED THE MUSIC!!
    
    
    // ––––– PLATFORMS ––––– //
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);

    g_state.platforms = new Entity[PLATFORM_COUNT];

    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        g_state.platforms[i].set_texture_id(platform_texture_id);
        g_state.platforms[i].set_position(glm::vec3(-3.5f + 1.5f * i, -3.2f, 0.0f));
        g_state.platforms[i].set_width(1.0f);
        g_state.platforms[i].set_height(0.4f);
        g_state.platforms[i].update(0.0f, NULL, 0);
    }
    
    
    //Added the winning/landing platform position and speed
    g_state.platforms[g_landing_platform].set_position(glm::vec3(3.0f, -3.0f, 0.0f));
    g_state.platforms[g_landing_platform].set_speed(0.8f);
    
    
    // ––––– PLAYER (GEORGE) ––––– //
    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH);

    int player_walking_animation[4][4] =
    {
        { 1, 5, 9, 13 },  // for George to move to the left,
        { 3, 7, 11, 15 }, // for George to move to the right,
        { 2, 6, 10, 14 }, // for George to move upwards,
        { 0, 4, 8, 12 }   // for George to move downwards
    };

    glm::vec3 acceleration = glm::vec3(0.0f,-4.905f, 0.0f);

    g_state.player = new Entity(
        player_texture_id,         // texture id
        1.0f,                      // speed
        acceleration,              // acceleration
        0.0f,                      // jumping power
        player_walking_animation,  // animation index sets
        0.0f,                      // animation time
        4,                         // animation frame amount
        0,                         // current animation index
        4,                         // animation column amount
        4,                         // animation row amount
        0.8f,                      // width
        0.8f                       // height
    );
    

    // Jumping
    g_state.player->set_jumping_power(3.0f);

    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;

                    default:
                        break;
                }

            default:
                break;
        }
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    
    //If not game over we can rotate the player
    //The arrow keys are to rotate player and with space bar
    //with using fuel you can move the player
    if (!g_game_over)
    {
        float rotation_delta = 0.0f;
        if (key_state[SDL_SCANCODE_LEFT])
        {
            rotation_delta += (g_rotation_speed * FIXED_TIMESTEP);
        }
        else if (key_state[SDL_SCANCODE_RIGHT])
        {
            rotation_delta -= (g_rotation_speed * FIXED_TIMESTEP);
        }
        float current_rot = g_state.player->get_rotation();
        g_state.player->set_rotation(current_rot + rotation_delta);
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }
    
    //I've did most of the mechanisms here
    while (delta_time >= FIXED_TIMESTEP)
    {
        if (!g_game_over)
        {
            
            Entity& moving_platform = g_state.platforms[g_landing_platform];
            float base_x = 3.0f;
            float amplitude = moving_platform.get_speed();
            float frequency = 1.0f;
            float new_x = base_x + amplitude * sinf(frequency * ticks);
            moving_platform.set_position(glm::vec3(new_x, -3.0f, 0.0f));
            moving_platform.update(FIXED_TIMESTEP, NULL, 0);

            float rot = g_state.player->get_rotation();
            glm::vec3 accel = g_state.player->get_acceleration();
            const Uint8* key_state = SDL_GetKeyboardState(NULL);
            //I've added the fuel key here which is the space bar
            bool is_thrusting = (key_state[SDL_SCANCODE_SPACE] && g_fuel > 0.0f);
            
            //Handling if player is thrusting(lower the fuel and move the player)
            if (is_thrusting)
            {
                float thrust_x = cosf(rot) * g_thrust;
                float thrust_y = sinf(rot) * g_thrust;
                accel.x = thrust_x;
                accel.y = -1.5f + thrust_y;
                g_fuel -= (g_fuel_use_rate * FIXED_TIMESTEP);
                if (g_fuel < 0.0f) g_fuel = 0.0f;
            }
            else
            {
                accel.x = 0.0f;
                accel.y = -1.5f;
            }
            g_state.player->set_acceleration(accel);

            g_state.player->update(FIXED_TIMESTEP, g_state.platforms, PLATFORM_COUNT);
            
            //Checking collision for platforms
            for (int i = 0; i < PLATFORM_COUNT; i++)
            {
                if (g_state.player->check_collision(&g_state.platforms[i]))
                {
                    if (i == g_landing_platform) g_mission_accomplished = true;
                    g_game_over = true;
                    break;
                }
            }
            if (g_state.player->get_position().y < -4.0f) g_game_over = true;
        }
        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;
}

void shutdown()
{
    SDL_Quit();

    delete [] g_state.platforms;
    delete g_state.player;
}

void draw_text(ShaderProgram* program, GLuint font_texture_id, const std::string& text, float size, float spacing, glm::vec3 position)
{
    float width = 16.0f;
    float height = 16.0f;

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    program->set_model_matrix(model_matrix);

    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    for (int i = 0; i < (int) text.size(); i++)
    {
        int spriteIndex = (unsigned int)text[i];
        float u = (float)(spriteIndex % 16) / 16.0f;
        float v = (float)(spriteIndex / 16) / 16.0f;

        float charWidth = 1.0f / 16.0f;
        float charHeight = 1.0f / 16.0f;

        float x = (size + spacing) * i;
        float y = 0.0f;

        vertices.insert(vertices.end(),
        {
            x + (-0.5f * size), y + (0.5f * size),
            x + (-0.5f * size), y + (-0.5f * size),
            x + (0.5f * size),  y + (0.5f * size),
            x + (0.5f * size),  y + (-0.5f * size),
            x + (0.5f * size),  y + (0.5f * size),
            x + (-0.5f * size), y + (-0.5f * size)
        });

        texture_coordinates.insert(texture_coordinates.end(),
        {
            u,               v,
            u,               v + charHeight,
            u + charWidth,   v,
            u + charWidth,   v + charHeight,
            u + charWidth,   v,
            u,               v + charHeight
        });
    }

    glBindTexture(GL_TEXTURE_2D, font_texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());

    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    g_state.player->render(&g_program);

    for (int i = 0; i < PLATFORM_COUNT; i++) g_state.platforms[i].render(&g_program);
    
    
    //I've added the mission accomplished and failed texts to the render!
    if (g_game_over)
    {
        if (g_mission_accomplished)
        {
            draw_text(&g_program, load_texture(FONT_FILEPATH), "MISSION ACCOMPLISHED!",
                      0.5f, -0.25f, glm::vec3(-2.2f, 0.0f, 0.0f));
        }
        else
        {
            draw_text(&g_program, load_texture(FONT_FILEPATH), "MISSION FAILED!",
                      0.5f, -0.25f, glm::vec3(-1.8f, 0.0f, 0.0f));
        }
    }
    
    //I've added Fuel UI
    std::string fuel_text = "Fuel: " + std::to_string((int)std::ceil(g_fuel));
    draw_text(&g_program, load_texture(FONT_FILEPATH), fuel_text,
              0.4f, -0.2f, glm::vec3(-4.8f, 3.2f, 0.0f));

    SDL_GL_SwapWindow(g_display_window);
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
