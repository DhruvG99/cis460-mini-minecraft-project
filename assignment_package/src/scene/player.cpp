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
    m_acceleration = glm::vec3(0.0f);
    float speedMod = 10.0;
    float accThresh = 0.1;


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
    // TODO: shubh. Add condition for space key too
    // TODO: shubh. Not sure if the rotation should be on global or local axis
    float rotDeg = 0.5;

    if(inputs.prevMouseX != inputs.mouseX){
        if(inputs.prevMouseX > inputs.mouseX){
            rotateOnUpLocal(rotDeg);
        }
        else{
            rotateOnUpLocal(-rotDeg);
        }
    }

    if(inputs.prevMouseY != inputs.mouseY){
        if(inputs.prevMouseY > inputs.mouseY){
            rotateOnRightLocal(rotDeg);
        }
        else{
            rotateOnRightLocal(-rotDeg);
        }
    }
}

void Player::computePhysics(float dT, const Terrain &terrain) {
    glm::vec3 pos_old = m_position;

    float decayFactor = 0.9;
    m_velocity = decayFactor * m_velocity; // simulating friction
    m_velocity[0] = abs(m_velocity[0]) < 0.1 ? 0 : m_velocity[0];
    m_velocity[1] = abs(m_velocity[1]) < 0.1 ? 0 : m_velocity[1];
    m_velocity[2] = abs(m_velocity[2]) < 0.1 ? 0 : m_velocity[2];

    m_velocity += m_acceleration * dT;
    m_position += m_velocity * dT;
    moveForwardGlobal(m_position[2]-pos_old[2]);
    moveRightGlobal(m_position[0]-pos_old[0]);
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
