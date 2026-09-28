#pragma once

/*
Where the asset folders sit relative to the working directory.

Visual Studio runs the exe with the project root as working directory, but the
CMake build puts it in build/ and runs it from there, so it has to climb out
first. One definition here beats an #if in every file that opens a file.

Used by string literal concatenation: ASSETS "maps/superslug.json".
*/
#if defined(_WIN32)
    #define ASSETS ""
#else
    #define ASSETS "../"
#endif
