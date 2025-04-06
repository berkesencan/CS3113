/**
 * Author: Berk Esencan
 * Assignment: Rise of the AI
 * Date due: 2025-04-05, 11:59pm
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 */
#include "LevelC.h"
#include "Utility.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <cmath>

extern int g_player_lives;

constexpr int SCENE_WIN  = 4;
constexpr int SCENE_LOSE = 5;

#define LEVELC_WIDTH 14
#define LEVELC_HEIGHT 8

unsigned int LEVELC_DATA[] = {
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0,
    3, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0,
    3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0
};



LevelC::~LevelC(){
    if(m_game_state.enemies) delete[] m_game_state.enemies;
    if(m_game_state.player)  delete   m_game_state.player;
    if(m_game_state.map)     delete   m_game_state.map;

    if(m_game_state.jump_sfx) Mix_FreeChunk(m_game_state.jump_sfx);
    if(m_game_state.hit_sfx)  Mix_FreeChunk(m_game_state.hit_sfx);
    if(m_game_state.lose_sfx) Mix_FreeChunk(m_game_state.lose_sfx);
    if(m_game_state.bgm)      Mix_FreeMusic(m_game_state.bgm);
}

void LevelC::initialise(){
    m_game_state.next_scene_id = -1;

    GLuint map_texture_id = Utility::load_texture("assets/B_tileset.png");
    m_game_state.map = new Map(LEVELC_WIDTH, LEVELC_HEIGHT, LEVELC_DATA, map_texture_id, 1.0f, 5, 10);

    // Player
    GLuint player_texture_id = Utility::load_texture("assets/GreenPlayer.png");
    int player_walking[4][4] = {
        {4,5,6,7},  // left
        {8,9,10,11}, // right
        {12,13,14,15}, // up even tho we dont use it
        {0,1,2,3}   // down even tho we dont use it
    };
    
    glm::vec3 accel(0.0f, -9.81f, 0.0f);
    m_game_state.player = new Entity(
        player_texture_id,  // texture id
        3.0f,               // speed
        accel,              // acceleration
        7.0f,               // jumping power
        player_walking,     // animation index sets
        0.0f,               // animation time
        4,                  // animation frame amount
        0,                  // current animation index
        4,                  // animation column amount
        4,                  // animation row amount
        1.0f,               // width
        1.0f,               // height
        PLAYER              // entity type
    );
    
    m_game_state.player->set_position(glm::vec3(2.0f, 2.0f, 0.0f));

    // Enemies
    GLuint enemy_texture_id = Utility::load_texture("assets/Ghost.png");
    m_game_state.enemies = new Entity[ENEMY_COUNT];

    // Guard
    m_game_state.enemies[0] = Entity(enemy_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, GUARD, IDLE);
    m_game_state.enemies[0].set_position(glm::vec3(7.0f, 0.0f, 0.0f));
    m_game_state.enemies[0].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));

    // Walker
    m_game_state.enemies[1] = Entity(enemy_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, WALKER, IDLE);
    m_game_state.enemies[1].set_position(glm::vec3(3.0f, -4.0f, 0.0f));
    m_game_state.enemies[0].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));

    // Flyer
    m_game_state.enemies[2] = Entity(enemy_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, FLYER, IDLE);
    m_game_state.enemies[2].set_position(glm::vec3(9.0f, 0.0f, 0.0f));

    // Audio
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    m_game_state.jump_sfx = Mix_LoadWAV("assets/bouncy.wav");
    m_game_state.hit_sfx  = Mix_LoadWAV("assets/hit.wav");
    m_game_state.lose_sfx = Mix_LoadWAV("assets/lose.wav");

    if(m_game_state.bgm){
        Mix_PlayMusic(m_game_state.bgm, -1);
        Mix_VolumeMusic(MIX_MAX_VOLUME / 4);
    }
}

void LevelC::update(float delta_time)
{
    m_game_state.player->update(delta_time, m_game_state.player, m_game_state.enemies, ENEMY_COUNT, m_game_state.map);

    for(int i = 0; i < ENEMY_COUNT; i++){
        m_game_state.enemies[i].update(delta_time, m_game_state.player, nullptr, 0, m_game_state.map);
    }

    for(int i = 0; i < ENEMY_COUNT; i++){
        if(m_game_state.player->check_collision(&m_game_state.enemies[i])){
            if(m_game_state.hit_sfx) Mix_PlayChannel(-1, m_game_state.hit_sfx, 0);
            g_player_lives--;
            if(g_player_lives <= 0){
                if(m_game_state.lose_sfx) Mix_PlayChannel(-1, m_game_state.lose_sfx, 0);
                m_game_state.next_scene_id = SCENE_LOSE;
            } else {
                m_game_state.player->set_position(glm::vec3(0.0f, 2.0f, 0.0f));
                m_game_state.player->set_velocity(glm::vec3(0.0f));
            }
        }
    }
    //The player dies when fall down
    float fall_threshold = -10.0f;
    if(m_game_state.player->get_position().y < fall_threshold) {
        g_player_lives--;
        if(g_player_lives <= 0){
            if(m_game_state.lose_sfx) Mix_PlayChannel(-1, m_game_state.lose_sfx, 0);
            m_game_state.next_scene_id = SCENE_LOSE;
        } else {
            // Reset
            m_game_state.player->set_position(glm::vec3(0.0f, 2.0f, 0.0f));
            m_game_state.player->set_velocity(glm::vec3(0.0f));
        }
    }
    // If player goes to the right edge of the screen it goes to next level
    if(m_game_state.player->get_position().x > 12.0f){
        m_game_state.next_scene_id = SCENE_WIN;
    }
}

void LevelC::render(ShaderProgram *program){
    m_game_state.map->render(program);
    m_game_state.player->render(program);

    for(int i=0; i<ENEMY_COUNT; i++){
        m_game_state.enemies[i].render(program);
    }

    Utility::draw_text(program, Utility::load_texture("assets/font1.png"), "Lives: " + std::to_string(g_player_lives), 0.5f, -0.25f, glm::vec3(5.0f, -2.0f, 0.0f));
}
