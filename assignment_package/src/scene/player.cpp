#include "player.h"
#include <QString>
#include <iostream>
#include <string.h>
#include <vector>

bool gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit);

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      m_flyMode(false), m_isGrounded(true), mcr_camera(m_camera)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

void Player::processInputs(InputBundle &inputs) {
    // TODO: shubh: Simulate gravity with the grounded_flag
    float speedMod = 100.0;

    if(m_flyMode == false && m_isGrounded == false){
//        m_acceleration = glm::vec3(0 ,-speedMod, 0);
        m_acceleration = glm::vec3(0, 0, 0);
    }
    else{
        m_acceleration = glm::vec3(0);
    }


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

    if(m_isGrounded){
        m_acceleration.y = 0;
        if(inputs.spacePressed){ // can only jump on the ground
            m_acceleration.y += 15  * speedMod;
        }
    }

    float dpi = 0.035;

    float angle = fmod(dpi * abs(inputs.prevMouseY - inputs.mouseY), 360.0f);
    if(inputs.prevMouseY > inputs.mouseY){
        if(m_forward.y < 0.95){ // simulate neck movement. Cannot look straight up
            rotateOnRightLocal(angle);
        }
    }
    else{
        if(m_forward.y > -0.95){ // simulate neck movement. Cannot look straight down
            rotateOnRightLocal(-angle);
        }
    }

    angle = fmod(dpi * abs(inputs.prevMouseX - inputs.mouseX), 360.0f);
    if(inputs.prevMouseX > inputs.mouseX){
        rotateOnUpGlobal(angle);
    }
    else{
        rotateOnUpGlobal(-angle);
    }


}

void getBaseNeighbors(glm::vec3 pos, std::vector<glm::vec3> &out_neigh){
}

void Player::computePhysics(float dT, const Terrain &terrain) {
    /*
     * TODO: shubh
     * Change flyMode back to true and grounded to true
     * Complete the flight mode
     * Do Raymarch for obstacle collision check
     * Implement area casting
     * Click to add and delete a block
     */

    std::vector<glm::vec3> baseCoords;
    baseCoords.push_back(m_position);
    baseCoords.push_back(glm::vec3(m_position.x+1, m_position.y, m_position.z));
    baseCoords.push_back(glm::vec3(m_position.x, m_position.y+1, m_position.z));
    baseCoords.push_back(glm::vec3(m_position.x+1, m_position.y+1, m_position.z));

    std::vector<glm::vec3> vertexPos;
    vertexPos.push_back(glm::vec3(m_position.x, m_position.y, m_position.z));
    vertexPos.push_back(glm::vec3(m_position.x+1, m_position.y, m_position.z));
    vertexPos.push_back(glm::vec3(m_position.x, m_position.y+1, m_position.z));
    vertexPos.push_back(glm::vec3(m_position.x+1, m_position.y+1, m_position.z));
    vertexPos.push_back(glm::vec3(m_position.x, m_position.y, m_position.z+1));
    vertexPos.push_back(glm::vec3(m_position.x+1, m_position.y, m_position.z+1));
    vertexPos.push_back(glm::vec3(m_position.x, m_position.y+1, m_position.z+1));
    vertexPos.push_back(glm::vec3(m_position.x+1, m_position.y+1, m_position.z+1));
    vertexPos.push_back(glm::vec3(m_position.x, m_position.y, m_position.z+2));
    vertexPos.push_back(glm::vec3(m_position.x+1, m_position.y, m_position.z+2));
    vertexPos.push_back(glm::vec3(m_position.x, m_position.y+1, m_position.z+2));
    vertexPos.push_back(glm::vec3(m_position.x+1, m_position.y+1, m_position.z+2));

//    float out_dist;
//    glm::ivec3 out_blockHit;
//    m_isGrounded = true;
//    for(auto coord : baseCoords){ // checking if all the 4 base coords are in air
//        bool groundBelow = gridMarch(coord, glm::vec3(0,-200,0), terrain, &out_dist, &out_blockHit);
//        if(groundBelow){
//            if(out_dist > 0.1){
//                m_isGrounded = false;
//                break;
//            }
//        }
//    }

//    std::cout << "on ground : " << m_isGrounded << std::endl;

    float mu = 0.2; // friction coeff
    glm::vec3 fricForce = mu * m_velocity * 100.0f;
    m_acceleration -= fricForce;
    m_velocity[0] = abs(m_velocity[0]) < 0.1 ? 0 : m_velocity[0];
    m_velocity[1] = abs(m_velocity[1]) < 0.1 ? 0 : m_velocity[1];
    m_velocity[2] = abs(m_velocity[2]) < 0.1 ? 0 : m_velocity[2];

    m_velocity += m_acceleration * dT;
    glm::vec3 new_pos = m_position + m_velocity * dT;

    float minOutX = new_pos.x-m_position.x, minOutY = new_pos.y-m_position.y, minOutZ = new_pos.z-m_position.z;
//    for(const auto &pos : vertexPos){
//        float out_dist;
//        glm::ivec3 out_blockHit;
//        bool xHit = gridMarch(pos, glm::vec3(m_position.x-pos_old.x,0,0), terrain, &out_dist, &out_blockHit);
//        if(xHit){
//            minOutX = std::min(minOutX, out_dist);
//        }
//    }

//    for(const auto &pos : vertexPos){
//        float out_dist;
//        glm::ivec3 out_blockHit;
//        bool yHit = gridMarch(pos, glm::vec3(0,m_position.y-pos_old.y,0), terrain, &out_dist, &out_blockHit);
//        if(yHit){
//            minOutY = std::min(minOutY, out_dist);
//        }
//    }

//    for(const auto &pos : vertexPos){
//        float out_dist;
//        glm::ivec3 out_blockHit;
//        bool zHit = gridMarch(pos, glm::vec3(0,0, new_pos.z - m_position.z), terrain, &out_dist, &out_blockHit);
////        std::cout << "direction : " << new_pos.z - m_position.z << std::endl;
//        if(zHit){
//            minOutZ = std::min(minOutZ, out_dist);
////            std::cout << "hit z : " << glm::to_string(out_dist) << std::endl;
//        }
//    }

    float out_dist;
    glm::ivec3 out_blockHit;
    bool zHit = gridMarch(glm::vec3(32, 129, 34), glm::vec3(0,0, -0.125), terrain, &out_dist, &out_blockHit);
    std::cout << "hit z : " <<  zHit << "  " << glm::to_string(out_dist) << std::endl;

    moveRightGlobal(minOutX);
    moveUpLocal(minOutY);
    moveForwardGlobal(minOutZ);
//    std::cout << "acc : " << glm::to_string(m_acceleration) << std::endl;
//    std::cout << "vel : " << glm::to_string(m_velocity) << std::endl;
//    std::cout << "dT : " << dT << std::endl;
//    std::cout << "pos new : " << glm::to_string(m_position) << std::endl;

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

std::string getName(BlockType t) {
    std::string name = "unknown";
    if (t == BlockType::EMPTY) {
        name = "empty";
    } else if (t == BlockType::GRASS) {
        name = "grass";
    } else if (t == BlockType::DIRT) {
        name = "dirt";
    } else if (t == BlockType::STONE) {
        name = "stone";
    } else if (t == BlockType::WATER) {
        name = "water";
    }
    return name;
}

bool gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    float maxLen = glm::length(rayDirection); // Farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    int i;
    if(rayDirection.x != 0 &&  rayDirection.y == 0 && rayDirection.z == 0){
        i = 0;
    } else if(rayDirection.y != 0 && rayDirection.x == 0 && rayDirection.z == 0){
        i = 1;
    }
    else if(rayDirection.z != 0 && rayDirection.x == 0 && rayDirection.y == 0){
        i = 2;
    }
    else{
//        std::cout << "no coll" << std::endl;
        return false;
    }

    float curr_t = 0.f;
    while(curr_t < maxLen) {

        float interfaceAxis = i; // Track axis for which t is smallest
        float local_offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
        int nextIntercept = currCell[i] + local_offset;
        float min_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];

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


