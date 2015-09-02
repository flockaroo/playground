// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include "Common.hpp"

struct StateDb;
struct Renderer;
struct Assets;

// -------------------------------------------------------------------------------------------------
/// @brief Platform abstraction
struct Platform
{
    Platform(
        StateDb &stateDbInit,
        Assets &assetsInit,
        Renderer &rendererInit);
    virtual ~Platform();

public:
    StateDb &stateDb;
    Assets &assets;
    Renderer &renderer;

private:
    COMMON_DISABLE_COPY(Platform);
};

#endif
