/**
 * Author: Berk Esencan
 * Assignment: Rise of the AI
 * Date due: 2025-04-05, 11:59pm
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 */

#include "LevelA.h"
#include "Utility.h"
#include "Entity.h"
#include <SDL_mixer.h>

#define LEVELA_WIDTH 14
#define LEVELA_HEIGHT 8

extern int g_player_lives;

constexpr int SCENE_LEVEL_B = 2;
constexpr int SCENE_LOSE    = 5;


unsigned int LEVELA_DATA[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4,
    3, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0,
    3, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0,
    3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0
    
};

LevelA::~LevelA()
{
    if(m_game_state.enemies) delete[] m_game_state.enemies;
    if(m_game_state.player)  delete   m_game_state.player;
    if(m_game_state.map)     delete   m_game_state.map;

    if(m_game_state.jump_sfx)  Mix_FreeChunk(m_game_state.jump_sfx);
    if(m_game_state.hit_sfx)   Mix_FreeChunk(m_game_state.hit_sfx);
    if(m_game_state.lose_sfx)  Mix_FreeChunk(m_game_state.lose_sfx);
    if(m_game_state.bgm)       Mix_FreeMusic(m_game_state.bgm);
}

void LevelA::initialise()
{
    m_game_state.next_scene_id = -1;

    GLuint map_texture_id = Utility::load_texture("assets/B_tileset.png");
    m_game_state.map = new Map(LEVELA_WIDTH, LEVELA_HEIGHT,
                               LEVELA_DATA,
                               map_texture_id,
                               1.0f,    // tile size
                               5,       // tile_count_x
                               10);      // tile_count_y

    // Player
    GLuint player_texture_id = Utility::load_texture("assets/GreenPlayer.png");
    int player_walking[4][4] = {
        {4,5,6,7},  // left
        {8,9,10,11}, // right
        {12,13,14,15}, // up even tho we dont use it
        {0,1,2,3}   // down even tho we dont use it
    };

    glm::vec3 accel(0.0f, -9.81f, 0.0f);
    m_game_state.player = new Entity(player_texture_id, 3.0f, accel, 7.0f,
                                     player_walking, 0.0f, 4, 0, 4, 4,
                                     1.0f, 1.0f, PLAYER);
    m_game_state.player->set_position(glm::vec3(2.0f, 0.0f, 0.0f));


    // Enemy
    GLuint enemy_texture_id = Utility::load_texture("assets/Ghost.png");
    m_game_state.enemies = new Entity[ENEMY_COUNT];

    // Walker
    m_game_state.enemies[0] = Entity(enemy_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, WALKER, IDLE);
    m_game_state.enemies[0].set_position(glm::vec3(3.0f, 0.0f, 0.0f));
    m_game_state.enemies[0].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));

    //Audio
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    m_game_state.bgm      = Mix_LoadMUS("assets/Background.mp3");
    m_game_state.jump_sfx = Mix_LoadWAV("assets/bouncy.wav");
    m_game_state.hit_sfx  = Mix_LoadWAV("assets/hit.wav");
    m_game_state.lose_sfx = Mix_LoadWAV("assets/lose.wav");

    if(m_game_state.bgm){
        Mix_PlayMusic(m_game_state.bgm, -1);
        Mix_VolumeMusic(MIX_MAX_VOLUME / 4);
    }
}


void LevelA::update(float delta_time)
{
    m_game_state.player->update(delta_time, m_game_state.player, m_game_state.enemies,
                                ENEMY_COUNT, m_game_state.map);

    for(int i = 0; i < ENEMY_COUNT; i++){
        m_game_state.enemies[i].update(delta_time, m_game_state.player, nullptr, 0,
                                       m_game_state.map);
    }
    
    for(int i = 0; i < ENEMY_COUNT; i++){
        if(m_game_state.player->check_collision(&m_game_state.enemies[i])){
            if(m_game_state.hit_sfx) Mix_PlayChannel(-1, m_game_state.hit_sfx, 0);
            g_player_lives--;
            if(g_player_lives <= 0){
                if(m_game_state.lose_sfx) Mix_PlayChannel(-1, m_game_state.lose_sfx, 0);
                m_game_state.next_scene_id = SCENE_LOSE;
            } else {
                m_game_state.player->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
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
            m_game_state.player->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
            m_game_state.player->set_velocity(glm::vec3(0.0f));
        }
    }
    // If player goes to the right edge of the screen it goes to next level
    if(m_game_state.player->get_position().x > 12.0f){
        m_game_state.next_scene_id = SCENE_LEVEL_B;
    }
}

void LevelA::render(ShaderProgram *program)
{
    m_game_state.map->render(program);
    m_game_state.player->render(program);
    for(int i = 0; i < ENEMY_COUNT; i++){
        m_game_state.enemies[i].render(program);
    }
    Utility::draw_text(program, Utility::load_texture("assets/font1.png"),
                       "Lives: " + std::to_string(g_player_lives), 0.5f, -0.25f,
                       glm::vec3(5.0f, -2.0f, 0.0f));
}

