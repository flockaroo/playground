// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 28.08.2015
// -------------------------------------------------------------------------------------------------

#include "Logging.hpp"

#include <cstdarg>
#include <cstdio>

// -------------------------------------------------------------------------------------------------
Logging::Logging()
{
}

// -------------------------------------------------------------------------------------------------
Logging::~Logging()
{
}

// -------------------------------------------------------------------------------------------------
void Logging::debug(const char *format, ...)
{
    char buffer[1024];

    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    fprintf(stdout, "%s\n", buffer);
}
