/**
* be752
* Author: Berk Esencan
* Assignment: Simple 2D Scene
* Date due: 2025-02-15, 11:59pm
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

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640 * 2,
              WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

// source: Tom and Jerry
constexpr char TOM_SPRITE_FILEPATH[]    = "tom.png",
               JERRY_SPRITE_FILEPATH[] = "jerry.png";

constexpr glm::vec3 INIT_SCALE       = glm::vec3(1.0f, 1.0f, 1.0f),
                    INIT_POS_TOM    = glm::vec3(-3.0f, -3.0f, 0.0f),
                    INIT_POS_JERRY = glm::vec3(2.0f, 0.0f, 0.0f);

constexpr float ROT_INCREMENT = 1.0f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
          g_tom_matrix,
          g_jerry_matrix,
          g_projection_matrix;

float g_previous_ticks = 0.0f;

glm::vec3 g_rotation_tom    = glm::vec3(0.0f, 0.0f, 0.0f),
          g_rotation_jerry = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_tom_texture_id,
       g_jerry_texture_id;


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


void initialise()
{
    // Initialise video
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Hello, Textures!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_tom_matrix       = glm::mat4(1.0f);
    g_jerry_matrix     = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    g_tom_texture_id    = load_texture(TOM_SPRITE_FILEPATH);
    g_jerry_texture_id = load_texture(JERRY_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


void update()
{
    //Initializing the scales and translation values for delta time
    static float tom_x = INIT_POS_TOM.x;
    static float tom_y = INIT_POS_TOM.y;
    static float jerry_scale = 1.0f;
    
    
    /* Delta time calculations */
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    /* Game logic */
    g_rotation_tom.y += ROT_INCREMENT * delta_time;
    g_rotation_jerry.y += -1 * ROT_INCREMENT * delta_time;

    /* Model matrix reset */
    g_tom_matrix    = glm::mat4(1.0f);
    g_jerry_matrix = glm::mat4(1.0f);

    /* Transformations */
    tom_x += delta_time * 0.3f;
    tom_y += delta_time * 0.3f;
    //Tom moving diagonally
    g_tom_matrix = glm::translate(g_tom_matrix, glm::vec3(tom_x, tom_y, 0.0f));
    //Tom rotating
    g_tom_matrix = glm::rotate(g_tom_matrix,                 // rotating in respect to tom
                                 g_rotation_tom.y,            // by the accumulated amt in the y-axis
                                 glm::vec3(0.0f, 0.0f, 1.0f)); // just in the y-axis
    g_tom_matrix = glm::scale(g_tom_matrix, INIT_SCALE);
    
    
    //Jerry orbitting around tom
    float orbit_radius = 2.0f;
    float jerry_x = tom_x + cos(g_rotation_jerry.y) * orbit_radius;
    float jerry_y = tom_y + sin(g_rotation_jerry.y) * orbit_radius;
    
    g_jerry_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(jerry_x, jerry_y, 0.0f));
    //g_jerry_matrix = glm::rotate(g_jerry_matrix,
                                  //g_rotation_jerry.y,
                                  //glm::vec3(0.0f, 0.0f, 1.0f));
    //Jerry getting bigger
    jerry_scale += delta_time * 0.1f;
    g_jerry_matrix = glm::scale(g_jerry_matrix, glm::vec3(jerry_scale, jerry_scale, 1.0f));
}


void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f  // Triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
                          0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_tom_matrix, g_tom_texture_id);
    draw_object(g_jerry_matrix, g_jerry_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


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

