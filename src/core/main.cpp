
#define GL3_PROTOTYPES 1
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <concurrentqueue.h>
#include <lua.hpp>

#include <iostream>

#include "window/window.h"
#include "util/telemetry.h"
#include "util/config.h"
#include "util/logging.h"

int main(int argc, char *argv[])
{
    YAML::Node config = YAML::LoadFile("config.yml");
    Logging::init(config);

    {
        Window window("Bento");
        window.open(config);

        // Destroy the YAML configuration data
        config.reset();

        window.run();
    }
    Logging::term();
    return 0;
}

