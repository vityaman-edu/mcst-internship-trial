set(
    CMAKE_CXX_FLAGS_ASAN 
    "-g -fsanitize=address,undefined -fno-sanitize-recover=all"
    CACHE STRING "Compiler flags in ASAN build"
    FORCE
)

set(
    CMAKE_CXX_FLAGS_MSAN
    "-g -fsanitize=memory -fno-omit-frame-pointer \
    -fno-optimize-sibling-calls -fsanitize-memory-track-origins"
    CACHE STRING "Compiler flags in MSAN build"
    FORCE
)
