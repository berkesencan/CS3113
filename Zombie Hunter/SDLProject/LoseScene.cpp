/**
 * Author: Berk Esencan
 * Assignment: Zombie Hunter
 * Date due: 2025-04-25, 2:00pm
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 */
#include "LoseScene.h"
#include "Utility.h"
#include <SDL.h>

extern int g_player_lives;
extern int g_score; 

void LoseScene::initialise() {
    m_game_state.next_scene_id = -1;
    g_player_lives = 3;
}

void LoseScene::update(float delta_time) {
}

void LoseScene::render(ShaderProgram *program) {
    glm::mat4 identity = glm::mat4(1.0f);
    program->set_view_matrix(identity);

    Utility::draw_text(program, Utility::load_texture("assets/font1.png"), "YOU DIED", 0.8f, -0.3f, glm::vec3(-2.5f, 0.5f, 0.0f));
    Utility::draw_text(program, Utility::load_texture("assets/font1.png"),
                       "Score: " + std::to_string(g_score),        // SCOREBOARD
                       0.5f, -0.25f,  glm::vec3(-1.6f, -0.2f, 0.0f));

    Utility::draw_text(program, Utility::load_texture("assets/font1.png"), "Press ENTER to retry", 0.4f, -0.2f, glm::vec3(-2.5f, -1.0f, 0.0f));
}
