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
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

void Entity::ai_activate(Entity *player)
{
    switch (m_ai_type)
    {
        case WALKER:
            ai_walk();
            break;
            
        case GUARD:
            ai_guard(player);
            break;
            
        case FLYER:
            ai_flyer();
            break;
    }
}

void Entity::ai_walk()
{
    m_movement = glm::vec3(m_direction_x * 1.0f, 0.0f, 0.0f);
}

void Entity::ai_guard(Entity *player)
{
    switch (m_ai_state) {
        case IDLE:
            if (glm::distance(m_position, player->get_position()) < 3.0f) m_ai_state = WALKING;
            break;
            
        case WALKING:
            if (m_position.x > player->get_position().x) {
                m_movement = glm::vec3(-1.0f, 0.0f, 0.0f);
            } else {
                m_movement = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            break;
            
        case ATTACKING:
            break;
        default:
            break;
    }
}

void Entity::ai_flyer()
{
    float cycle = sin((float)SDL_GetTicks() * 0.002f);
    m_movement.x = m_direction_x * 1.0f;
    m_movement.y = cycle;
}

// Default constructor
Entity::Entity()
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
      m_speed(0.0f), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
      m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f),
      m_texture_id(0), m_velocity(0.0f), m_acceleration(0.0f), m_width(0.0f),
      m_height(0.0f), m_direction_x(1.0f)
{
    // Initialize m_walking with zeros or any default value
    for (int i = 0; i < SECONDS_PER_FRAME; ++i)
        for (int j = 0; j < SECONDS_PER_FRAME; ++j) m_walking[i][j] = 0;
}

// Parameterized constructor
Entity::Entity(GLuint texture_id, float speed, glm::vec3 acceleration, float jump_power,
               int walking[4][3], float animation_time, int animation_frames,
               int animation_index, int animation_cols, int animation_rows,
               float width, float height, EntityType entity_type)
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
      m_speed(speed), m_acceleration(acceleration), m_jumping_power(jump_power),
      m_animation_cols(animation_cols), m_animation_frames(animation_frames),
      m_animation_index(animation_index), m_animation_rows(animation_rows),
      m_animation_indices(nullptr), m_animation_time(animation_time),
      m_texture_id(texture_id), m_velocity(0.0f), m_width(width), m_height(height),
      m_entity_type(entity_type), m_direction_x(1.0f)
{
    face_right();
    set_walking(walking);
}

// Simpler constructor for partial initialization
Entity::Entity(GLuint texture_id, float speed, float width, float height, EntityType entity_type)
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
      m_speed(speed), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
      m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f),
      m_texture_id(texture_id), m_velocity(0.0f), m_acceleration(0.0f), m_width(width),
      m_height(height), m_entity_type(entity_type), m_direction_x(1.0f)
{
    // Initialize m_walking with zeros or any default value
    for (int i = 0; i < SECONDS_PER_FRAME; ++i)
        for (int j = 0; j < SECONDS_PER_FRAME; ++j) m_walking[i][j] = 0;
}

Entity::Entity(GLuint texture_id, float speed, float width, float height,
               EntityType entity_type, AIType ai_type, AIState ai_state, bool flip_by_dir = false)
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
      m_speed(speed), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
      m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f),
      m_texture_id(texture_id), m_velocity(0.0f), m_acceleration(0.0f), m_width(width),
      m_height(height), m_entity_type(entity_type), m_ai_type(ai_type), m_ai_state(ai_state),
      m_direction_x(1.0f), m_flip_by_dir(flip_by_dir)
{
    // Initialize m_walking with zeros or any default value
    for (int i = 0; i < SECONDS_PER_FRAME; ++i)
        for (int j = 0; j < SECONDS_PER_FRAME; ++j) m_walking[i][j] = 0;
}

Entity::~Entity() {}

void Entity::draw_sprite_from_texture_atlas(ShaderProgram *program, GLuint texture_id, int index)
{
    // Step 1: Calculate the UV location of the indexed frame
    float u_coord = (float)(index % m_animation_cols) / (float)m_animation_cols;
    float v_coord = (float)(index / m_animation_cols) / (float)m_animation_rows;

    // Step 2: Calculate its UV size
    float width = 1.0f / (float)m_animation_cols;
    float height = 1.0f / (float)m_animation_rows;

    // Step 3: Just as we have done before, match the texture coordinates to the vertices
    float tex_coords[] = {
        u_coord, v_coord + height, u_coord + width, v_coord + height, u_coord + width, v_coord,
        u_coord, v_coord + height, u_coord + width, v_coord, u_coord, v_coord
    };

    float vertices[] =
    {
        -0.5, -0.5, 0.5, -0.5, 0.5, 0.5,
        -0.5, -0.5, 0.5, 0.5, -0.5, 0.5
    };

    // Step 4: And render
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());

    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

bool const Entity::check_collision(Entity *other) const
{
    float x_distance = fabs(m_position.x - other->m_position.x) - ((m_width + other->m_width) / 2.0f);
    float y_distance = fabs(m_position.y - other->m_position.y) - ((m_height + other->m_height) / 2.0f);
    return x_distance < 0.0f && y_distance < 0.0f;
}

void const Entity::check_collision_y(Map *map)
{
    // Probes for tiles above
    glm::vec3 top = glm::vec3(m_position.x, m_position.y + (m_height / 2), m_position.z);
    glm::vec3 top_left = glm::vec3(m_position.x - (m_width / 2), m_position.y + (m_height / 2), m_position.z);
    glm::vec3 top_right = glm::vec3(m_position.x + (m_width / 2), m_position.y + (m_height / 2), m_position.z);
    
    // Probes for tiles below
    glm::vec3 bottom = glm::vec3(m_position.x, m_position.y - (m_height / 2), m_position.z);
    glm::vec3 bottom_left = glm::vec3(m_position.x - (m_width / 2), m_position.y - (m_height / 2), m_position.z);
    glm::vec3 bottom_right = glm::vec3(m_position.x + (m_width / 2), m_position.y - (m_height / 2), m_position.z);
    
    float penetration_x = 0;
    float penetration_y = 0;
    
    // If the map is solid, check the top three points
    if (map->is_solid(top, &penetration_x, &penetration_y) && m_velocity.y > 0) {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;
    }
    else if (map->is_solid(top_left, &penetration_x, &penetration_y) && m_velocity.y > 0) {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;
    }
    else if (map->is_solid(top_right, &penetration_x, &penetration_y) && m_velocity.y > 0) {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;
    }
    
    // And the bottom three points
    if (map->is_solid(bottom, &penetration_x, &penetration_y) && m_velocity.y < 0) {
        m_position.y += penetration_y;
        m_velocity.y = 0;
        m_collided_bottom = true;
    }
    else if (map->is_solid(bottom_left, &penetration_x, &penetration_y) && m_velocity.y < 0) {
        m_position.y += penetration_y;
        m_velocity.y = 0;
        m_collided_bottom = true;
    }
    else if (map->is_solid(bottom_right, &penetration_x, &penetration_y) && m_velocity.y < 0) {
        m_position.y += penetration_y;
        m_velocity.y = 0;
        m_collided_bottom = true;
    }
}

void const Entity::check_collision_x(Map *map)
{
    // Probes for tiles; the x-checking is much simpler
    glm::vec3 left = glm::vec3(m_position.x - (m_width / 2), m_position.y, m_position.z);
    glm::vec3 right = glm::vec3(m_position.x + (m_width / 2), m_position.y, m_position.z);
    
    float penetration_x = 0;
    float penetration_y = 0;
    
    if (map->is_solid(left, &penetration_x, &penetration_y) && m_velocity.x < 0) {
        m_position.x += penetration_x;
        m_velocity.x = 0;
        m_direction_x *= -1;
        m_collided_left = true;
    }
    if (map->is_solid(right, &penetration_x, &penetration_y) && m_velocity.x > 0) {
        m_position.x -= penetration_x;
        m_velocity.x = 0;
        m_direction_x *= -1;
        m_collided_right = true;
    }
    if (m_position.x < 0.0f || m_position.x > 24.0f) {
        m_direction_x *= -1;
    }
    if (m_position.x < 0.0f) {
        m_position.x = 0.0f;
    }
    if (m_position.x > 24.0f) {
        m_position.x = 24.0f;
    }
}

void Entity::update(float delta_time, Entity *player, Entity *collidable_entities, int collidable_entity_count, Map *map)
{
    if (!m_is_active) return;
    
    m_collided_top = false;
    m_collided_bottom = false;
    m_collided_left = false;
    m_collided_right = false;
    
    if (m_entity_type == ENEMY) ai_activate(player);
    
    if (m_animation_indices != nullptr) {
        if (glm::length(m_movement) != 0) {
            m_animation_time += delta_time;
            float frames_per_second = (float)1 / SECONDS_PER_FRAME;
            if (m_animation_time >= frames_per_second) {
                m_animation_time = 0.0f;
                m_animation_index++;
                if (m_animation_index >= m_animation_frames) {
                    m_animation_index = 0;
                }
            }
        }
    }
    
    m_velocity.x = m_movement.x * m_speed;
    if (m_entity_type == ENEMY && m_ai_type == FLYER) {
        m_velocity.y = m_movement.y * m_speed;
    }
    m_velocity += m_acceleration * delta_time;
    
    if (m_is_jumping) {
        m_is_jumping = false;
        m_velocity.y += m_jumping_power;
    }
    
    m_position.y += m_velocity.y * delta_time;
    check_collision_y(map);
    
    m_position.x += m_velocity.x * delta_time;
    check_collision_x(map);
    
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
}

void Entity::normalise_movement() { m_movement = glm::normalize(m_movement); }

void Entity::face_left() { m_animation_indices = m_walking[LEFT]; }
void Entity::face_right() { m_animation_indices = m_walking[RIGHT]; }
void Entity::face_up() { m_animation_indices = m_walking[UP]; }
void Entity::face_down() { m_animation_indices = m_walking[DOWN]; }

void Entity::move_left()
{
    m_movement.x = -1.0f;
    m_facing_dir = -1;
    face_left();
}
void Entity::move_right()
{
    m_movement.x = 1.0f;
    m_facing_dir = 1;
    face_right();
}

void Entity::move_up() { m_movement.y = 1.0f; face_up(); }
void Entity::move_down() { m_movement.y = -1.0f; face_down(); }

void const Entity::jump() { m_is_jumping = true; }

void Entity::set_walking(int walking[4][3])
{
    for (int i = 0; i < SECONDS_PER_FRAME; ++i)
        for (int j = 0; j < SECONDS_PER_FRAME; ++j) m_walking[i][j] = walking[i][j];
}

void Entity::render(ShaderProgram *program)
{
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
    if (m_flip_by_dir)
    {
        float sx = (m_direction_x > 0) ? -1.0f : 1.0f;   // flip when moving right
        m_model_matrix = glm::scale(m_model_matrix, glm::vec3(sx, 1.0f, 1.0f));
    }
    
    program->set_model_matrix(m_model_matrix);
    
    if (m_animation_indices != nullptr) {
        draw_sprite_from_texture_atlas(program, m_texture_id, m_animation_indices[m_animation_index]);
        return;
    }
    
    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float tex_coords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
    
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}
