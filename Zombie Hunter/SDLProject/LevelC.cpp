/**
 * Author: Berk Esencan
 * Assignment: Zombie Hunter
 * Date due: 2025-04-25, 2:00pm
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 */

#include "LevelC.h"
#include "Utility.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <cstdlib>
#include <ctime>
#include <cmath>

#define LEVELC_WIDTH  25
#define LEVELC_HEIGHT 7

extern int g_player_lives;
extern int g_score;
extern glm::mat4 g_view_matrix;

extern ShaderProgram g_shader_program;
extern ShaderProgram g_effect_program;

constexpr int SCENE_WIN  = 4;
constexpr int SCENE_LOSE = 5;

unsigned int LEVELC_DATA[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,
    5,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,5,
    5,5,5,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,5,5,5,
    5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,
    5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    7,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};


LevelC::~LevelC()
{
    if (m_game_state.enemies) delete[] m_game_state.enemies;
    if (m_game_state.player)  delete   m_game_state.player;
    if (m_game_state.map)     delete   m_game_state.map;

    for (Entity* e : m_spawned_enemies) delete e;

    if (m_game_state.jump_sfx) Mix_FreeChunk(m_game_state.jump_sfx);
    if (m_game_state.hit_sfx)  Mix_FreeChunk(m_game_state.hit_sfx);
    if (m_game_state.enemy_hit_sfx) Mix_FreeChunk(m_game_state.enemy_hit_sfx);
    if (m_game_state.lose_sfx) Mix_FreeChunk(m_game_state.lose_sfx);
    if (m_game_state.bgm)      Mix_FreeMusic(m_game_state.bgm);
}

void LevelC::initialise()
{
    std::srand((unsigned)std::time(nullptr));

    m_game_state.next_scene_id = -1;
    m_level_timer  = LEVEL_TIME;
    m_spawn_timer  = SPAWN_INTERVAL;
    m_spawned_enemies.clear();

    GLuint map_texture_id = Utility::load_texture("assets/FireSet.png");
    m_game_state.map = new Map(LEVELC_WIDTH, LEVELC_HEIGHT, LEVELC_DATA, map_texture_id, 1.0f, 5, 5);

    // Player
    GLuint player_texture_id = Utility::load_texture("assets/Halo.png");
    int player_walking[4][3] = {
        {1, 5, 9},   // left
        {3, 7, 11},  // right
        {2, 6, 10},  // up (not used)
        {0, 4, 8}    // down (not used)
    };

    glm::vec3 accel(0.0f, -9.81f, 0.0f);
    m_game_state.player = new Entity(
        player_texture_id,  // texture id
        3.0f,               // speed
        accel,              // acceleration
        7.0f,               // jumping power
        player_walking,     // animation index sets
        0.0f,               // animation time
        3,                  // animation frame amount
        0,                  // current animation index
        4,                  // animation column amount
        3,                  // animation row amount
        1.0f,               // width
        1.0f,               // height
        PLAYER              // entity type
    );
    m_game_state.player->set_position(glm::vec3(2.0f, 2.0f, 0.0f));

    // Enemy
    GLuint enemy_texture_id = Utility::load_texture("assets/ZOMBIE_WALKER.png");
    GLuint flyer_texture_id = Utility::load_texture("assets/Flyer2.png");
    GLuint guard_texture_id = Utility::load_texture("assets/Guard.png");
    m_game_state.enemies = new Entity[ENEMY_COUNT];

    // Walker
    m_game_state.enemies[0] = Entity(guard_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, GUARD, IDLE, true);
    m_game_state.enemies[0].set_health(2);
    m_game_state.enemies[0].set_position(glm::vec3(7.0f, 0.0f, 0.0f));
    m_game_state.enemies[0].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));

    m_game_state.enemies[1] = Entity(enemy_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, WALKER, IDLE, true);
    m_game_state.enemies[1].set_position(glm::vec3(3.0f, -4.0f, 0.0f));
    m_game_state.enemies[1].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));

    m_game_state.enemies[2] = Entity(flyer_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, FLYER, IDLE, true);
    m_game_state.enemies[2].set_position(glm::vec3(9.0f, 0.0f, 0.0f));
    m_game_state.enemies[2].set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));

    // Audio
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    m_game_state.jump_sfx = Mix_LoadWAV("assets/jump.wav");
    m_game_state.hit_sfx  = Mix_LoadWAV("assets/hit.wav");
    m_game_state.enemy_hit_sfx = Mix_LoadWAV("assets/enemy_hit.wav");
    m_game_state.lose_sfx = Mix_LoadWAV("assets/lose.wav");
}

void LevelC::update(float delta_time)
{
    m_game_state.player->update(delta_time, m_game_state.player, m_game_state.enemies, ENEMY_COUNT, m_game_state.map);

    for (int i = 0; i < ENEMY_COUNT; ++i)
        m_game_state.enemies[i].update(delta_time, m_game_state.player, nullptr, 0, m_game_state.map);

    for (int i = 0; i < ENEMY_COUNT; ++i)
    {
        if (m_game_state.enemies[i].get_is_active() && m_game_state.player->check_collision(&m_game_state.enemies[i]))
        {
            if (m_game_state.hit_sfx) Mix_PlayChannel(-1, m_game_state.hit_sfx, 0);
            g_player_lives--;
            if (g_player_lives <= 0)
            {
                if (m_game_state.lose_sfx) Mix_PlayChannel(-1, m_game_state.lose_sfx, 0);
                m_game_state.next_scene_id = SCENE_LOSE;
            }
            else
            {
                // Reset
                m_game_state.player->set_position(glm::vec3(0.0f, 2.0f, 0.0f));
                m_game_state.player->set_velocity(glm::vec3(0.0f));
            }
        }
    }

    // The player dies when fall down
    float fall_threshold = -10.0f;
    if (m_game_state.player->get_position().y < fall_threshold)
    {
        g_player_lives--;
        if (g_player_lives <= 0)
        {
            if (m_game_state.lose_sfx) Mix_PlayChannel(-1, m_game_state.lose_sfx, 0);
            m_game_state.next_scene_id = SCENE_LOSE;
        }
        else
        {
            // Reset
            m_game_state.player->set_position(glm::vec3(0.0f, 2.0f, 0.0f));
            m_game_state.player->set_velocity(glm::vec3(0.0f));
        }
    }

    // If timer ends next level
    m_level_timer -= delta_time;
    if (m_level_timer <= 0.0f)
    {
        m_game_state.next_scene_id = SCENE_WIN;
        return;   // skip the rest
    }

    m_spawn_timer -= delta_time;
    if (m_spawn_timer <= 0.0f)
    {
        m_spawn_timer = SPAWN_INTERVAL;
        if ((int)m_spawned_enemies.size() < MAX_SPAWNED) spawn_enemy();
    }

    // spawned enemies
    for (size_t i = 0; i < m_spawned_enemies.size(); )
    {
        Entity* e = m_spawned_enemies[i];
        e->update(delta_time, m_game_state.player, nullptr, 0, m_game_state.map);

        // destroying if fallen off
        if (!e->get_is_active() || e->get_position().y < -20.0f)
        {
            delete e;
            m_spawned_enemies.erase(m_spawned_enemies.begin() + i);
        }
        else ++i;
    }

    for (Entity* e : m_spawned_enemies)
    {
        if (e->get_is_active() && m_game_state.player->check_collision(e))
        {
            if (m_game_state.hit_sfx) Mix_PlayChannel(-1, m_game_state.hit_sfx, 0);
            g_player_lives--;
            if (g_player_lives <= 0)
            {
                if (m_game_state.lose_sfx) Mix_PlayChannel(-1, m_game_state.lose_sfx, 0);
                m_game_state.next_scene_id = SCENE_LOSE;
            }
            else
            {
                // Reset
                m_game_state.player->set_position(glm::vec3(0.0f, 2.0f, 0.0f));
                m_game_state.player->set_velocity(glm::vec3(0.0f));
            }
        }
    }
}

void LevelC::render(ShaderProgram *program)
{
    m_game_state.map->render(program);

    ShaderProgram* p = (g_player_lives <= 1) ? &g_effect_program : &g_shader_program;
    m_game_state.player->render(p);

    for (int i = 0; i < ENEMY_COUNT; ++i)
        if (m_game_state.enemies[i].get_is_active())
            m_game_state.enemies[i].render(program);

    for (Entity* e : m_spawned_enemies)
        if (e->get_is_active()) e->render(program);

    glm::mat4 identity = glm::mat4(1.0f);
    program->set_view_matrix(identity);

    Utility::draw_text(program, Utility::load_texture("assets/font1.png"),
                       "Lives: " + std::to_string(g_player_lives),
                       0.5f, -0.25f, glm::vec3(-4.8f, 3.2f, 0.0f));
    Utility::draw_text(program, Utility::load_texture("assets/font1.png"),
                       "Score: " + std::to_string(g_score),   // SCOREBOARD
                       0.5f, -0.25f, glm::vec3(-0.6f, 3.2f, 0.0f));
    Utility::draw_text(program, Utility::load_texture("assets/font1.png"),
                       "Time: " + std::to_string((int)ceil(m_level_timer)),
                       0.5f, -0.25f, glm::vec3(3.0f, 3.2f, 0.0f));

    program->set_view_matrix(g_view_matrix);
}

void LevelC::spawn_enemy()
{
    if (!m_game_state.player) return;

    const glm::vec3 player_pos = m_game_state.player->get_position();

    // Safe circle around the player so the enemy doesn't spawn there
    const float TWO_PI = 6.2831853f;
    float angle  = ((float)rand() / RAND_MAX) * TWO_PI;
    float radius = SAFE_RADIUS + 1.5f;

    glm::vec3 candidate;
    bool found = false;

    for (int attempts = 0; attempts < 20 && !found; ++attempts)
    {
        candidate.x = player_pos.x + cosf(angle) * radius;
        candidate.y = player_pos.y + sinf(angle) * radius;
        candidate.z = 0.0f;

        // to stay inside the map
        float left   = m_game_state.map->get_left_bound();
        float right  = m_game_state.map->get_right_bound();
        float bottom = m_game_state.map->get_bottom_bound();

        candidate.x = glm::clamp(candidate.x, left + 0.5f, right - 0.5f);
        candidate.y = std::max(bottom + 0.5f, candidate.y);   // Never spawn below map

        float penx = 0.0f, peny = 0.0f;
        if (!m_game_state.map->is_solid(candidate, &penx, &peny))
            found = true;
        else
        {
            angle  += 0.5f;
            radius += 0.5f;
        }
    }

    if (!found) return;

    // SPAWN ENEMY
    GLuint enemy_texture_id = Utility::load_texture("assets/ZOMBIE_WALKER.png");
    GLuint flyer_texture_id = Utility::load_texture("assets/Flyer2.png");
    GLuint guard_texture_id = Utility::load_texture("assets/Guard.png");
    int r = std::rand() % 3;
    Entity* e;
    if (r == 0)
    {
        e = new Entity(guard_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, GUARD, IDLE, true);
        e->set_health(2);
        e->set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    }
    else if (r == 1)
    {
        e = new Entity(enemy_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, WALKER, IDLE, true);
        e->set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    }
    else
    {
        e = new Entity(flyer_texture_id, 1.0f, 1.0f, 1.0f, ENEMY, FLYER, IDLE, true);
        e->set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));
    }
    e->set_position(candidate);
    m_spawned_enemies.push_back(e);
}
