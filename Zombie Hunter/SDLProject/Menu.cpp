/**
 * Author: Berk Esencan
 * Assignment: Zombie Hunter
 * Date due: 2025-04-25, 2:00pm
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 */

#include "Menu.h"
#include "Utility.h"
#include <string>

extern int g_player_lives;
extern int g_score;

void Menu::initialise() {
    m_game_state.next_scene_id = -1;
    g_player_lives = 3;
    g_score = 0;
}

void Menu::update(float delta_time) {
}

void Menu::render(ShaderProgram *program) {
    glm::mat4 identity = glm::mat4(1.0f);
    program->set_view_matrix(identity);

    Utility::draw_text(program, Utility::load_texture("assets/font1.png"), "ZOMBIE HUNTER", 0.7f, -0.3f, glm::vec3(-3.2f, 1.0f, 0.0f));

    Utility::draw_text(program, Utility::load_texture("assets/font1.png"), "Press Enter to SAVE THE WORLD", 0.4f, -0.25f, glm::vec3(-3.0f, -0.5f, 0.0f));
}
