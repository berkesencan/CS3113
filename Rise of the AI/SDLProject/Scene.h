#pragma once
#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Util.h"
#include "Entity.h"
#include "Map.h"


struct GameState {
    // ————— GAME OBJECTS ————— //
    Map *map          = nullptr;
    Entity *player    = nullptr;
    Entity *enemies   = nullptr;

    
    // ————— AUDIO ————— //
    Mix_Music *bgm    = nullptr;
    Mix_Chunk *jump_sfx = nullptr;
    Mix_Chunk *hit_sfx  = nullptr;
    Mix_Chunk *lose_sfx = nullptr;
    
    // ————— POINTERS TO OTHER SCENES ————— //
    int next_scene_id   = -1;
};

class Scene {
protected:
    GameState m_game_state;

public:
    virtual ~Scene() {}

    // ————— METHODS ————— //
    virtual void initialise() = 0;
    virtual void update(float delta_time) = 0;
    virtual void render(ShaderProgram *program) = 0;

    // ————— GETTERS ————— //
    GameState& get_state() { return m_game_state; }
    const GameState& get_state() const { return m_game_state; }
};
