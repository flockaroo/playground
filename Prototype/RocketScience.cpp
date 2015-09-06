// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 05.01.2015
// -------------------------------------------------------------------------------------------------

#define GLM_SWIZZLE

#include "RocketScience.hpp"

#include <SDL.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

#include "Math.hpp"

#include "Platform.hpp"
#include "StateDb.hpp"
#include "Assets.hpp"

#include "Renderer.hpp"
#include "Physics.hpp"

const int RocketScience::OCEAN_TILE_VERTEX_COUNT = 32;
const glm::fvec2 RocketScience::OCEAN_TILE_UNIT_SIZE =
    glm::fvec2((OCEAN_TILE_VERTEX_COUNT - 1) * OCEAN_TILE_VERTEX_DIST);
const float RocketScience::OCEAN_TILE_VERTEX_DIST = 1.0f;

const int RocketScience::PLATFORM_SPHERE_COUNT = 4;

// -------------------------------------------------------------------------------------------------
RocketScience::RocketScience()
{
}

// -------------------------------------------------------------------------------------------------
RocketScience::~RocketScience()
{
}

// -------------------------------------------------------------------------------------------------
void RocketScience::registerTypesAndStates(StateDb &stateDb)
{
}

// -------------------------------------------------------------------------------------------------
bool RocketScience::initialize(Platform &platform)
{
    Renderer::Camera::Info *camera = nullptr;
    m_cameraHandle = platform.stateDb.createObjectAndRefState(&camera);
    camera->position = glm::fvec3(20.0f, 20.0f, 20.0f);
    camera->target   = glm::fvec3( 0.0f,  0.0f,  0.0f);

    platform.renderer.activeCameraHandle = m_cameraHandle;

    {
        Renderer::Mesh::Info *mesh = nullptr;
        m_arrowMeshHandle = platform.stateDb.createObjectAndRefState(&mesh);
        mesh->modelAsset = platform.assets.asset("Assets/Arrow.obj");
    }

    m_oceanModelAsset = platform.assets.asset("procedural/ocean",
        Assets::Flag::PROCEDURAL | Assets::Flag::DYNAMIC);

    // Create floating platform
    {
        // Create platform mesh
        Renderer::Mesh::Info *platformMesh = nullptr;
        u64 platformMeshHandle = platform.stateDb.createObjectAndRefState(&platformMesh);
        platformMesh->translation = glm::fvec3(0.0f, 0.0f, 0.0f);
        platformMesh->rotation = glm::dquat(1.0f, 0.0f, 0.0f, 0.0f);
        platformMesh->modelAsset = platform.assets.asset("Assets/Platform.obj");
        // Create platform rigid body
        Physics::RigidBody::Info *platformRigidBody = nullptr;
        u64 platformRigidBodyHandle = platform.stateDb.createObjectAndRefState(&platformRigidBody);
        platformRigidBody->mass = 10.0f;
        platformRigidBody->meshHandle = platformMeshHandle;
        platformRigidBody->collisionGroup = 2;
        platformRigidBody->collisionMask  = 2;
        // Create buoyancy spheres
        const float platformSize = 9.5f;
        for (int col = 0; col < PLATFORM_SPHERE_COUNT; ++col)
        {
            for (int row = 0; row < PLATFORM_SPHERE_COUNT; ++row)
            {
                // Create sphere mesh
                Renderer::Mesh::Info *sphereMesh = nullptr;
                u64 sphereMeshHandle = platform.stateDb.createObjectAndRefState(&sphereMesh);
                sphereMesh->translation = glm::fvec3(
                    -0.5f * platformSize + double(col)
                            / double(PLATFORM_SPHERE_COUNT - 1) * platformSize,
                    -0.5f * platformSize + double(row)
                            / double(PLATFORM_SPHERE_COUNT - 1) * platformSize, -2.0);
                sphereMesh->modelAsset = platform.assets.asset("Assets/Sphere.obj");
                sphereMesh->flags |= Renderer::Mesh::Flag::HIDDEN;
                // Create sphere rigid body
                Physics::RigidBody::Info *sphereRigidBody = nullptr;
                u64 sphereRigidBodyHandle =
                    platform.stateDb.createObjectAndRefState(&sphereRigidBody);
                sphereRigidBody->meshHandle = sphereMeshHandle;
                sphereRigidBody->collisionGroup = 2;
                sphereRigidBody->collisionMask  = 2;
                sphereRigidBody->collisionShapeType =
                    Physics::RigidBody::Info::CollisionShapeType::BOUNDING_SPHERE;
                // Buoyancy affectors
                Physics::Affector::Info *buoyancyAffector = nullptr;
                u64 buoyancyAffectorHandle =
                    platform.stateDb.createObjectAndRefState(&buoyancyAffector);
                buoyancyAffector->rigidBodyHandle = sphereRigidBodyHandle;
                m_buoyancyAffectorHandles.push_back(buoyancyAffectorHandle);
                // Constrain buoyancy sphere to platform
                Physics::Constraint::Info *constraint = nullptr;
                platform.stateDb.createObjectAndRefState(&constraint);
                constraint->rigidBodyAHandle = platformRigidBodyHandle;
                constraint->rigidBodyBHandle = sphereRigidBodyHandle;
                constraint->paramQuatA = glm::fquat(1.0f, 0.0f, 0.0f, 0.0f);
                constraint->paramQuatB = glm::fquat(1.0f, 0.0f, 0.0f, 0.0f);
                constraint->paramVecA = sphereMesh->translation;
            }
        }
    }

    const bool enableRocket = true;
    for (int meshIdx = 0; meshIdx < 128; ++meshIdx)
    {
        Renderer::Mesh::Info *mesh = nullptr;
        u64 meshHandle = platform.stateDb.createObjectAndRefState(&mesh);
        mesh->translation = glm::linearRand(
            glm::fvec3(-10.0f, -10.0f, +20.0f), glm::fvec3(+10.0f, +10.0f, +40.0f));

        if (enableRocket && meshIdx == 0)
        {
            mesh->translation = glm::fvec3(0.0f, 0.0f, 10.0f);
            mesh->rotation = glm::angleAxis(glm::radians(90.0f), glm::fvec3(1.0f, 0.0f, 0.0f));
            mesh->modelAsset = platform.assets.asset("Assets/Pusher.obj");
        }
        else if (rand() % 9 > 5)
        {
            mesh->modelAsset = platform.assets.asset("Assets/MaterialCube.obj");
        }
        else if (rand() % 9 > 2)
        {
            mesh->modelAsset = platform.assets.asset("Assets/Sphere.obj");
        }
        else
        {
            mesh->modelAsset = platform.assets.asset("Assets/Torus.obj");
        }

        m_meshHandles.push_back(meshHandle);
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
void RocketScience::shutdown(Platform &platform)
{
    platform.stateDb.destroyObject(m_arrowMeshHandle);
    for (auto meshHandle : m_oceanMeshHandles)
    {
        platform.stateDb.destroyObject(meshHandle);
    }
    for (auto rigidBodyIter : m_rigidBodyByMeshHandle)
    {
        platform.stateDb.destroyObject(rigidBodyIter.second);
    }
    for (auto meshHandle : m_meshHandles)
    {
        platform.stateDb.destroyObject(meshHandle);
    }
    platform.stateDb.destroyObject(m_cameraHandle);
}

// -------------------------------------------------------------------------------------------------
void RocketScience::update(Platform &platform, double deltaTimeInS)
{
    m_timeInS += deltaTimeInS;

    // Update tileable dynamic ocean model
    {
        glm::fvec2 unitSize = OCEAN_TILE_UNIT_SIZE;
        int vertexCount = OCEAN_TILE_VERTEX_COUNT;
        float vertexDist = OCEAN_TILE_VERTEX_DIST;
        Assets::Model *model = platform.assets.refModel(m_oceanModelAsset);
        if (model->positions.empty())
        {
            // Create ocean tile geometry
            for (int y = 0; y < vertexCount - 1; ++y)
            {
                for (int x = 0; x < vertexCount - 1; ++x)
                {
                    // Clockwise quad starting from bottom left
                    glm::fvec3  bottomLeft((x + 0) * vertexDist, (y + 0) * vertexDist, 0.0f);
                    glm::fvec3     topLeft((x + 0) * vertexDist, (y + 1) * vertexDist, 0.0f);
                    glm::fvec3    topRight((x + 1) * vertexDist, (y + 1) * vertexDist, 0.0f);
                    glm::fvec3 bottomRight((x + 1) * vertexDist, (y + 0) * vertexDist, 0.0f);
                    // Triangles covering quad
                    model->positions.push_back(bottomLeft);
                    model->positions.push_back(topRight);
                    model->positions.push_back(topLeft);
                    model->positions.push_back(bottomLeft);
                    model->positions.push_back(bottomRight);
                    model->positions.push_back(topRight);
                    // Triangle normals
                    glm::fvec3 normal(0.0f, 0.0f, 1.0f);
                    for (int vIdx = 0; vIdx < 6; ++vIdx) model->normals.push_back(normal);
                    // Triangle colors
                    float value = glm::linearRand(0.5f, 1.0f);
                    glm::fvec3 color(value, value, value);
                    for (int vIdx = 0; vIdx < 6; ++vIdx) model->colors.push_back(color);
                }
            }
            // Create ocean tiles
            const int tileCount = 4;
            for (int y = 0; y < tileCount; ++y)
            {
                for (int x = 0; x < tileCount; ++x)
                {
                    Renderer::Mesh::Info *mesh = nullptr;
                    u64 oceanMeshHandle = platform.stateDb.createObjectAndRefState(&mesh);
                    mesh->modelAsset = m_oceanModelAsset;
                    mesh->translation = glm::fvec3(glm::fvec2(float(x), float(y))
                        * unitSize - 0.5f * float(tileCount) * unitSize, 0.0f);
                    m_oceanMeshHandles.push_back(oceanMeshHandle);
                }
            }
        }
        // Update positions according to ocean equation
        for (glm::fvec3 &position : model->positions)
        {
            // Update Z and normal depending on X and Y
            position.z = oceanEquation(position.xy(), m_timeInS);
        }
        // Update normals and colors according to ocean equation
        const int triCount = int(model->normals.size()) / 3;
        for (int triIdx = 0; triIdx < triCount; ++triIdx)
        {
            glm::fvec3 normal = glm::normalize(glm::cross(
                model->positions[triIdx * 3 + 1] - model->positions[triIdx * 3 + 0],
                model->positions[triIdx * 3 + 2] - model->positions[triIdx * 3 + 0]));
            model->normals[triIdx * 3 + 0] = normal;
            model->normals[triIdx * 3 + 1] = normal;
            model->normals[triIdx * 3 + 2] = normal;
        }
    }

    // FIXME(martinmo): Add keyboard input to platform abstraction (remove dependency to SDL)
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    // Update camera
    {
        double horizontalRotationInDeg = 0.0f;
        if (state[SDL_SCANCODE_RIGHT]) horizontalRotationInDeg += deltaTimeInS * 90.0;
        if (state[SDL_SCANCODE_LEFT])  horizontalRotationInDeg -= deltaTimeInS * 90.0;
        double verticalRotationInDeg = 0.0f;
        if (state[SDL_SCANCODE_DOWN]) verticalRotationInDeg += deltaTimeInS * 90.0;
        if (state[SDL_SCANCODE_UP])   verticalRotationInDeg -= deltaTimeInS * 90.0;
        double translationInM = 0.0f;
        if (state[SDL_SCANCODE_W]) translationInM += deltaTimeInS * 20.0;
        if (state[SDL_SCANCODE_S]) translationInM -= deltaTimeInS * 20.0;

        auto camera = platform.stateDb.refState< Renderer::Camera::Info >(m_cameraHandle);

        glm::fvec3 cameraDir = (camera->target - camera->position).xyz();

        // Vertical rotation axis in world space
        // FIXME(martinmo): Correctly handle near +/- 90 deg cases
        glm::fvec3 verticalRotationAxis = cameraDir;
        std::swap(verticalRotationAxis.x, verticalRotationAxis.y);
        verticalRotationAxis.y = -verticalRotationAxis.y;
        verticalRotationAxis.z = 0.0f;
        verticalRotationAxis = glm::normalize(verticalRotationAxis);

        glm::fmat4 xform;
        xform = glm::translate(xform, camera->target.xyz());
        if (glm::abs(horizontalRotationInDeg) > 0.001)
        {
            xform = glm::rotate(xform, glm::radians(
                float(horizontalRotationInDeg)), glm::fvec3(0.0f, 0.0f, 1.0f));
        }
        if (glm::abs(verticalRotationInDeg) > 0.001)
        {
            xform = glm::rotate(xform, glm::radians(
                float(verticalRotationInDeg)), verticalRotationAxis);
        }
        xform = glm::translate(xform, -camera->target.xyz());
        camera->position = glm::fvec3(xform * glm::fvec4(camera->position, 1.0f));
        if (glm::abs(translationInM) > 0.001)
        {
            camera->position += glm::normalize(cameraDir) * float(translationInM);
        }
    }

    // Turn meshes into rigid bodies
    if (state[SDL_SCANCODE_P] && m_rigidBodyByMeshHandle.size() < m_meshHandles.size())
    {
        u64 meshHandle = m_meshHandles[m_rigidBodyByMeshHandle.size()];

        Physics::RigidBody::Info *rigidBodyInfo = nullptr;
        u64 rigidBodyHandle = platform.stateDb.createObjectAndRefState(&rigidBodyInfo);

        rigidBodyInfo->meshHandle = meshHandle;
        rigidBodyInfo->collisionGroup = 1;
        rigidBodyInfo->collisionMask  = 1;

        m_rigidBodyByMeshHandle[meshHandle] = rigidBodyHandle;

        auto mesh = platform.stateDb.refState< Renderer::Mesh::Info >(meshHandle);

        if (mesh->modelAsset == platform.assets.asset("Assets/Pusher.obj"))
        {
            rigidBodyInfo->collisionShapeType =
                //Physics::RigidBody::Info::CollisionShapeType::BOUNDING_BOX;
                Physics::RigidBody::Info::CollisionShapeType::CONVEX_HULL_COMPOUND;
            // Create Pusher rocket motor force
            {
                Physics::Affector::Info *affector = nullptr;
                m_pusherAffectorHandle = platform.stateDb.createObjectAndRefState(&affector);
                affector->rigidBodyHandle = rigidBodyHandle;
                affector->enabled = 1;
            }
        }
        else if (mesh->modelAsset == platform.assets.asset("Assets/Sphere.obj"))
        {
            // Simulate buoyancy for spheres instead of collisions
            rigidBodyInfo->collisionGroup = 2;
            rigidBodyInfo->collisionMask  = 2;
            rigidBodyInfo->collisionShapeType =
                Physics::RigidBody::Info::CollisionShapeType::BOUNDING_SPHERE;
            // Create sphere buoyancy force
            Physics::Affector::Info *affector = nullptr;
            u64 buoyancyAffectorHandle =
                platform.stateDb.createObjectAndRefState(&affector);
            affector->rigidBodyHandle = rigidBodyHandle;
            m_buoyancyAffectorHandles.push_back(buoyancyAffectorHandle);
        }
    }

    // Update rocket force control logic
    if (m_pusherAffectorHandle)
    {
        auto mesh = platform.stateDb.refState< Renderer::Mesh::Info >(m_meshHandles.front());
        auto affector = platform.stateDb.refState<
            Physics::Affector::Info >(m_pusherAffectorHandle);

        glm::fmat3 meshRot = glm::mat3_cast(mesh->rotation);

        /*
        // Top wing position
        forceInfo->forcePosition = meshRot * glm::fvec3(0.00f, -2.64, 2.75f);
        // Head position
        forceInfo->forcePosition = meshRot * glm::fvec3(0.00f,  3.61, 0.00f);
        */

        // Tail/main engine position
        affector->forcePosition = meshRot * glm::fvec3(0.00f, -2.14, 0.00f);

        /*
        // This force applied to top wing makes rocket spin clockwise
        forceInfo->force = modelRot * glm::fvec3(10.0f, 0.0f, 0.0f);
        */

        // Main engine force to keep height of 10 m
        float mainEngineForce = 0.0f;
        if (mesh->translation.z < 10.0)
        {
            // Mass of rocket is 1 kg (hard-coded for all RBs ATM)
            // ==> Force needs to be at least 9.81 x 1 ==> 10
            mainEngineForce = 12.0f;
        }
        else
        {
            mainEngineForce = 9.0f;
        }

        // Rocket's local +Y should be aligned to global +Z
        glm::fvec3 nominalDir = glm::fvec3(0.0, 0.0, 1.0);
        glm::fvec3 actualDir  = meshRot * glm::fvec3(0.0f, 1.0f, 0.0f);

        // Tilt nominal direction to steer rocket towards origin
        nominalDir += glm::normalize(-mesh->translation) / 20.0f;
        nominalDir  = glm::normalize(nominalDir);

        // Thrust vector tries to align rocket with nominal direction
        glm::fvec3 thrustVector = glm::normalize(nominalDir + 1.2f * (actualDir - nominalDir));

        // Add some main engine jitter
        glm::fvec3 maxNoise = glm::fvec3(0.05f, 0.05f, 0.05f);
        thrustVector += glm::linearRand(-maxNoise, maxNoise);

        affector->force = thrustVector * mainEngineForce;

        // Use arrow mesh to display thrust vector
        auto arrowMesh = platform.stateDb.refState< Renderer::Mesh::Info >(m_arrowMeshHandle);
        arrowMesh->translation = mesh->translation + affector->forcePosition;
        arrowMesh->rotation = Math::rotateFromTo(
            glm::fvec3(0.0f, 1.0f, 0.0f), -affector->force);

        // Emergency motors off...
        double errorAngleInDeg = glm::degrees(
            glm::angle(Math::rotateFromTo(nominalDir, actualDir)));
        //Logging::debug("errorAngleInDeg: %5.2f", errorAngleInDeg);
        if (glm::abs(errorAngleInDeg) > 20.0f)
        {
            platform.stateDb.destroyObject(m_pusherAffectorHandle);
            m_pusherAffectorHandle = 0;
        }
    }

    // Update buoyancy forces
    {
        for (auto buoyancyAffectorHandle : m_buoyancyAffectorHandles)
        {
            updateBuoyancyAffector(platform.stateDb, m_timeInS, buoyancyAffectorHandle);
        }
    }
}

// -------------------------------------------------------------------------------------------------
float RocketScience::oceanEquation(const glm::fvec2 &position, double timeInS)
{
    glm::fvec2 unitSize = OCEAN_TILE_UNIT_SIZE;
    float twoPi = 2.0f * glm::pi< float >();
    float result = 0.0f;
    result += 0.50f * sinf((twoPi / (0.500f * unitSize.x)) * (position.x + position.y + timeInS));
    result += 0.50f * sinf((twoPi / (1.000f * unitSize.x)) * (position.x - position.y + timeInS));
    result += 0.10f * sinf((twoPi / (0.250f * unitSize.x)) * (position.x + position.y - timeInS * 0.5f));
    return result - 10.0f;
}

// -------------------------------------------------------------------------------------------------
void RocketScience::updateBuoyancyAffector(
    StateDb &stateDb, double timeInS, u64 affectorHandle)
{
    // TODO(martinmo): Get rid of multi-level indirection by
    // TODO(martinmo): - Storing buoyancy type hint in force info
    // TODO(martinmo): - Keeping a copy of all relevant state in force info
    // TODO(martinmo):
    // TODO(martinmo): ==> Less efficient in space but more efficent in time
    // TODO(martinmo): ==> Flatten only if time-efficiency really is an issue

    auto affector  = stateDb.refState< Physics::Affector::Info  >(affectorHandle);
    auto rigidBody = stateDb.refState< Physics::RigidBody::Info >(affector->rigidBodyHandle);
    auto mesh      = stateDb.refState< Renderer::Mesh::Info     >(rigidBody->meshHandle);

    glm::fvec2 pos2 = mesh->translation.xy();
    glm::fvec3 oceanPt     (pos2 + glm::fvec2( 0.0f,  0.0f), 0.0f);
    glm::fvec3 oceanPtLeft (pos2 + glm::fvec2(-1.0f,  0.0f), 0.0f);
    glm::fvec3 oceanPtRight(pos2 + glm::fvec2(+1.0f,  0.0f), 0.0f);
    glm::fvec3 oceanPtAbove(pos2 + glm::fvec2( 0.0f, +1.0f), 0.0f);
    glm::fvec3 oceanPtBelow(pos2 + glm::fvec2( 0.0f, -1.0f), 0.0f);

    oceanPt.z      = oceanEquation(oceanPt.xy(),      m_timeInS);
    oceanPtLeft.z  = oceanEquation(oceanPtLeft.xy(),  m_timeInS);
    oceanPtRight.z = oceanEquation(oceanPtRight.xy(), m_timeInS);
    oceanPtAbove.z = oceanEquation(oceanPtAbove.xy(), m_timeInS);
    oceanPtBelow.z = oceanEquation(oceanPtBelow.xy(), m_timeInS);

    glm::fvec3 normal = glm::normalize(
        glm::cross(oceanPtRight - oceanPtLeft, oceanPtAbove - oceanPtBelow));

    double sphereRadius = 0.5;
    double sphereDepth = double(oceanPt.z - mesh->translation.z) + sphereRadius;
    double spherePenetrationDepth = glm::clamp(sphereDepth, 0.0, 2.0 * sphereRadius);
    double spherePenetrationVolume = 0.0;
    if (spherePenetrationDepth > 0.0)
    {
        spherePenetrationVolume = (1.0 / 3.0) * glm::pi< double >()
            * glm::pow(spherePenetrationDepth, 2.0)
            * (3.0 * sphereRadius - spherePenetrationDepth);
    }
    // Density of water at 22 degrees is 997.7735 kg/m^3
    // (https://en.wikipedia.org/wiki/Density#Water)
    double waterDensity = 50.0;
    double buoyancy = spherePenetrationVolume * waterDensity;

    if (buoyancy > 0.0)
    {
        rigidBody->linearVelocityLimit.z = 0.8f;

        // Force from buoyancy
        affector->force = normal * float(buoyancy);
        // Force from friction between moving sphere and water
        affector->force += -8.0f * glm::fvec3(rigidBody->linearVelocity.xy(), 0.0f);
        // Force from friction between spinning sphere and water
        glm::fvec3 avFriction = rigidBody->angularVelocity;
        avFriction.z = 0.0f;
        std::swap(avFriction.x, avFriction.y);
        avFriction.y = -avFriction.y;
        affector->force += 0.5f * avFriction;
        // Torque from friction between spinning sphere and water
        affector->torque = -0.03f * rigidBody->angularVelocity;

        affector->enabled = 1;
    }
    else
    {
        rigidBody->linearVelocityLimit.z = 0.0f;
        affector->enabled = 0;
    }
}
