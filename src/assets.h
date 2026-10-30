#pragma once

// Else I would have to change every path to account linux / windows
#if defined(_WIN32)
    #define ASSETS ""
#else
    #define ASSETS "../"
#endif
