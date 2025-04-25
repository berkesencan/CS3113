/**
 * Author: Berk Esencan
 * Assignment: Zombie Hunter
 * Date due: 2025-04-25, 2:00pm
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 */
#pragma once
#include "Scene.h"

class LevelA : public Scene {
public:
    int ENEMY_COUNT = 1;
    
    //enemy spawner
    static constexpr float SPAWN_INTERVAL = 4.0f;
    static constexpr int   MAX_SPAWNED    = 5;
    static constexpr float SAFE_RADIUS    = 2.0f;

    float m_spawn_timer = SPAWN_INTERVAL;
    std::vector<Entity*> m_spawned_enemies;
    std::vector<Entity*>& get_spawned_enemies() { return m_spawned_enemies; }
    
    //Level timer
    static constexpr float LEVEL_TIME = 45.0f;
    float m_level_timer = LEVEL_TIME;

    ~LevelA();
    void initialise() override;
    void update(float delta_time) override;
    void render(ShaderProgram *program) override;
    
    void spawn_enemy();   

};
