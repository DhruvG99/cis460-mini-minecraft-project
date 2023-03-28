#include "player.h"
#include <QString>
#include <iostream>

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      m_flyMode(true), mcr_camera(m_camera)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

void Player::processInputs(InputBundle &inputs) {
    // TODO: shubh: Simulate gravity with the grounded_flag

    m_acceleration = glm::vec3(0);
    float speedMod = 10.0;

    if(inputs.wPressed){
        m_acceleration += m_forward * speedMod;
    }

    if(inputs.aPressed){
        m_acceleration += -m_right * speedMod;
    }

    if(inputs.sPressed){
        m_acceleration += -m_forward * speedMod;
    }

    if(inputs.dPressed){
        m_acceleration += m_right * speedMod;
    }

    if(inputs.spacePressed){ // TODO: shubh: add flag to check grounded while jumping
        m_acceleration.y += speedMod;
    }

    float dpi = 0.035;

    if(inputs.prevMouseY != inputs.mouseY){
        if(inputs.prevMouseY > inputs.mouseY){
            rotateOnRightLocal(dpi * abs(inputs.prevMouseY - inputs.mouseY));
        }
        else{
            rotateOnRightLocal(-dpi * abs(inputs.prevMouseY - inputs.mouseY));
        }
    }

    if(inputs.prevMouseX != inputs.mouseX){
        if(inputs.prevMouseX > inputs.mouseX){
            rotateOnUpGlobal(dpi * abs(inputs.prevMouseX - inputs.mouseX));
        }
        else{
            rotateOnUpGlobal(-dpi * abs(inputs.prevMouseX - inputs.mouseX));
        }
    }

}

void Player::computePhysics(float dT, const Terrain &terrain) {
    /*
     * TODO: shubh
     * Add the grounded flag first
     * Simulate gravity
     * Complete the flight mode
     * Do Raymarch for obstacle collision check
     * Implement area casting
     */
    glm::vec3 pos_old = m_position;

    float decayFactor = 0.9;
    m_velocity = decayFactor * m_velocity; // simulating friction
    m_velocity[0] = abs(m_velocity[0]) < 0.1 ? 0 : m_velocity[0];
    m_velocity[1] = abs(m_velocity[1]) < 0.1 ? 0 : m_velocity[1];
    m_velocity[2] = abs(m_velocity[2]) < 0.1 ? 0 : m_velocity[2];

    m_velocity += m_acceleration * dT;
    m_position += m_velocity * dT;
    moveRightGlobal(m_position.x-pos_old.x);
    moveUpLocal(m_position.y-pos_old.y);
    moveForwardGlobal(m_position.z-pos_old.z);
}

void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

void Player::toggleFlyMode(){
    m_flyMode = !m_flyMode;
}


QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}



bool gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    float maxLen = glm::length(rayDirection); // Farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

    int i;
    if(rayDirection == glm::vec3(1,0,0)){
        i = 0;
    } else if(rayDirection == glm::vec3(0,1,0)){
        i = 1;
    }
    else if(rayDirection == glm::vec3(0,0,1)){
        i = 2;
    }
    else{
        throw std::invalid_argument("Incorrect axis for gridMarch.");
    }

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        if(rayDirection[i] != 0) { // Is ray parallel to axis i?
            float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
            // If the player is *exactly* on an interface then
            // they'll never move if they're looking in a negative direction
            if(currCell[i] == rayOrigin[i] && offset == 0.f) {
                offset = -1.f;
            }
            int nextIntercept = currCell[i] + offset;
            float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
            axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
            if(axis_t < min_t) {
                min_t = axis_t;
                interfaceAxis = i;
            }
        }
        if(interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        curr_t += min_t; // min_t is declared in slide 7 algorithm
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0,0,0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
        // If currCell contains something other than EMPTY, return
        // curr_t
        BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);
        if(cellType != EMPTY) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }
    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}


