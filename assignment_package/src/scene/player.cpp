#include "player.h"
#include <QString>
#include <iostream>
#include <string.h>
#include <vector>

bool checkCollision(glm::vec3 origin, glm::vec3 rayDirection, int axis, const Terrain &terrain, float &out_dist, glm::ivec3 &out_blockHit);
bool gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit);
std::string getName(BlockType t);

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      m_flyMode(true), m_isGrounded(false), mcr_camera(m_camera)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

void Player::processInputs(InputBundle &inputs) {
    float speedMod = 100.0;
    if(m_flyMode == false && m_isGrounded == false){
        m_acceleration = glm::vec3(0, -3.0*speedMod, 0);
    } else{
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

    if(m_flyMode){
        if(inputs.qPressed){
            m_acceleration += -m_up * speedMod;
        }

        if(inputs.ePressed){
            m_acceleration += m_up * speedMod;
        }
    }

    if(m_isGrounded){
        m_acceleration.y = 0;
        if(inputs.spacePressed){ // can only jump on the ground
            m_acceleration.y += 50  * speedMod;
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


std::vector<glm::vec3> Player::getPoints(glm::vec3 origin, glm::vec3 subdirection){
    std::vector<glm::vec3> vertexPos;

    if(subdirection.y != 0){ // only check the bottom cube
        vertexPos.push_back(glm::vec3(origin.x, origin.y, origin.z));
    } else if(subdirection.z != 0){ // check nearby cubes too
        vertexPos.push_back(glm::vec3(origin.x, origin.y, origin.z));
        vertexPos.push_back(glm::vec3(origin.x+1, origin.y, origin.z));
        vertexPos.push_back(glm::vec3(origin.x, origin.y+1, origin.z));
        vertexPos.push_back(glm::vec3(origin.x+1, origin.y+1, origin.z));
    } else{
        vertexPos.push_back(glm::vec3(origin.x, origin.y, origin.z));
        vertexPos.push_back(glm::vec3(origin.x, origin.y, origin.z+1));
        vertexPos.push_back(glm::vec3(origin.x, origin.y+1, origin.z));
        vertexPos.push_back(glm::vec3(origin.x, origin.y+1, origin.z+1));
    }

    return vertexPos;

}


void Player::computePhysics(float dT, const Terrain &terrain) {
    m_isGrounded = false;

    float mu = 0.2; // friction coeff
    glm::vec3 fricForce = mu * m_velocity * 100.0f;
    m_acceleration -= fricForce;

    m_acceleration[0] = abs(m_acceleration[0]) < 0.1 ? 0 : m_acceleration[0];
    m_acceleration[1] = abs(m_acceleration[1]) < 0.1 ? 0 : m_acceleration[1];
    m_acceleration[2] = abs(m_acceleration[2]) < 0.1 ? 0 : m_acceleration[2];

    m_velocity += m_acceleration * dT;
    m_velocity[0] = abs(m_velocity[0]) < 0.1 ? 0 : m_velocity[0];
    m_velocity[1] = abs(m_velocity[1]) < 0.1 ? 0 : m_velocity[1];
    m_velocity[2] = abs(m_velocity[2]) < 0.1 ? 0 : m_velocity[2];

    glm::vec3 pos_offset = m_velocity * dT;

    float out_dist = 1000.f; // init with large value
    glm::ivec3 out_blockHit;
    glm::vec3 subdirection;
    glm::vec3 posChange = glm::vec3(0.0f);
    glm::vec3 pos_origin = (glm::vec3(m_position.x-0.5, m_position.y, m_position.z-0.5));

    if(m_flyMode){
        posChange = pos_offset;
    } else{
        for(int i=0; i<3; ++i){
            float min_dist = 1000.0f;
            subdirection = glm::vec3(0);
            subdirection[i] = pos_offset[i];
            std::vector<glm::vec3> collisionPoints = getPoints(pos_origin, subdirection);
            for(const auto &point : collisionPoints){
                if(checkCollision(point, subdirection, i, terrain, out_dist, out_blockHit)){
                    m_acceleration[i] = 0;
                    m_velocity[i] = 0;
                    if(i == 1 && subdirection[1] <= 0){
                        m_isGrounded = true;
                        m_acceleration = glm::vec3(0);
                    }else{
//                        std::cout << glm::to_string(pos_origin)  << "  " << glm::to_string(point) << "  "  << glm::to_string(subdirection) << "  "  << out_dist << std::endl;
                    }
                }
                else if(abs(subdirection[1]) > 0.1 ){
//                    std::cout << "----  " << glm::to_string(pos_offset) << std::endl;
                }
                min_dist = glm::min(min_dist, out_dist);
            }
            if(glm::length(subdirection) != 0){
                subdirection = glm::normalize(subdirection);
            }
            posChange += subdirection * min_dist;
       }
    }
    moveAlongVector(posChange);
//    std::cout << glm::to_string(glm::ivec3(glm::vec3(3.999,2,1))) << std::endl;
//    bool collided = checkCollision(glm::vec3(32,129,30.5), glm::vec3(0,-1,0), 1, terrain, out_dist, out_blockHit);
//    std::cout << collided << "  "  << glm::to_string(out_blockHit) << "  "  << out_dist << std::endl;


}


bool checkCollision(glm::vec3 origin, glm::vec3 rayDirection, int axis, const Terrain &terrain, float &out_dist, glm::ivec3 &out_blockHit){
    glm::ivec3 currCell = glm::ivec3(origin);
    float distanceMoved = (origin[axis] - currCell[axis]) * (-1 * glm::sign(rayDirection[axis]));
    float maxDist = abs(rayDirection[axis]);

    while(distanceMoved < maxDist){
        currCell[axis] += glm::sign(rayDirection[axis]);
        if(terrain.getBlockAt(currCell.x, currCell.y, currCell.z) != EMPTY) {
            out_blockHit = currCell;
            out_dist = glm::max(0.f,distanceMoved);
            out_dist = distanceMoved;
            return true;
        }
        distanceMoved += 1;
    }

    out_dist = maxDist;
    return false;

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
    // function just used for debugging
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
    // NOTE: don't optimise this function for each axis. Will not work with place and break cube functions.
    float maxLen = glm::length(rayDirection); // Farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for(int i = 0; i < 3; ++i) { // Iterate over the three axes
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
            if(rayDirection.y != 0){
                *out_dist = glm::min(maxLen, curr_t);
            }
            else{
                *out_dist = glm::min(maxLen, curr_t) - 0.08f; // subracting a small value to simulate that the player is running.
            }
            return true;
        }
    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}


std::vector<glm::vec3> Player::getNeighs(glm::ivec3 inp_pos){
    std::vector<glm::vec3> neighs;
    neighs.push_back(glm::vec3(inp_pos.x, inp_pos.y, inp_pos.z));
    neighs.push_back(glm::vec3(inp_pos.x-1, inp_pos.y, inp_pos.z));
    neighs.push_back(glm::vec3(inp_pos.x+1, inp_pos.y, inp_pos.z));
    neighs.push_back(glm::vec3(inp_pos.x, inp_pos.y+1, inp_pos.z));
    neighs.push_back(glm::vec3(inp_pos.x, inp_pos.y-1, inp_pos.z));
    neighs.push_back(glm::vec3(inp_pos.x, inp_pos.y, inp_pos.z+1));
    neighs.push_back(glm::vec3(inp_pos.x, inp_pos.y, inp_pos.z-1));

    return neighs;
}

void Player::placeBlock(Terrain& terrain){
    float detectLength = 3.0f;
    glm::vec3 rayOrigin = m_camera.mcr_position;
    glm::vec3 rayDirection = m_forward * detectLength; // camera and player has the same look/forward vector

    float out_dist;
    glm::ivec3 out_blockHit;
    float min_dist = 1000.0f;
    glm::ivec3 targetPos;

    if (gridMarch(rayOrigin, rayDirection, terrain, &out_dist, &out_blockHit)){
        std::vector<glm::vec3> neighs = getNeighs(out_blockHit);
        for(const auto &neigh : neighs){
            if (terrain.getBlockAt(neigh.x, neigh.y, neigh.z) == BlockType::EMPTY){
                glm::vec3 pos = glm::vec3(neigh.x, neigh.y, neigh.z);
                float distance = glm::length(pos - rayOrigin);
                if (distance < min_dist){
                    min_dist = distance;
                    targetPos = pos;
                }
            }
        }

        BlockType bt = terrain.getBlockAt(out_blockHit.x, out_blockHit.y, out_blockHit.z);
        terrain.setBlockAt(targetPos.x, targetPos.y, targetPos.z, bt);
    }
}


void Player::breakBlock(Terrain& terrain){
    float detectLength = 3.0f;
    glm::vec3 rayOrigin = m_camera.mcr_position;
    glm::vec3 rayDirection = m_forward * detectLength; // camera and player has the same look/forward vector

    float out_dist;
    glm::ivec3 out_blockHit;
    float min_dist = 1000.0f;
    glm::ivec3 targetPos;

    if (gridMarch(rayOrigin, rayDirection, terrain, &out_dist, &out_blockHit)){
        std::vector<glm::vec3> neighs = getNeighs(out_blockHit);
        for(const auto &neigh : neighs){
            if (terrain.getBlockAt(neigh.x, neigh.y, neigh.z) != BlockType::EMPTY){
                glm::vec3 pos = glm::vec3(neigh.x, neigh.y, neigh.z);
                float distance = glm::length(pos - rayOrigin);
                if (distance < min_dist){
                    min_dist = distance;
                    targetPos = pos;
                }
            }
        }
        terrain.setBlockAt(targetPos.x, targetPos.y, targetPos.z, BlockType::EMPTY);
    }
}
