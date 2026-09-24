cmake_minimum_required(VERSION 3.20)
project(OrbitScope CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw glad)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

find_package(bsoncxx CONFIG REQUIRED)
find_package(mongocxx CONFIG REQUIRED)

add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

add_executable(OrbitScope
    src/main.cpp
    src/application/Application.cpp
    src/rendering/Renderer.cpp
    src/rendering/Shader.cpp
    src/rendering/Camera.cpp
    src/rendering/ImGuiLayer.cpp
    src/rendering/Texture.cpp
    src/rendering/StbImageImpl.cpp
    src/models/Planet.cpp
    src/models/SolarSystem.cpp
    src/simulation/Simulation.cpp
    src/simulation/OrbitCalculator.cpp
    src/simulation/AsteroidFilter.cpp
    src/simulation/AsteroidPositioner.cpp
    src/simulation/AsteroidListState.cpp
    src/simulation/KeplerOrbitCalculator.cpp
    src/data/Http.cpp
    src/data/MongoEnvironment.cpp
    src/data/MongoRepository.cpp
    src/data/PlanetRepository.cpp
    src/data/ConfigRepository.cpp
    src/data/AsteroidClient.cpp
    src/data/SBDBClient.cpp
)

target_include_directories(OrbitScope PRIVATE src external/stb)

target_link_libraries(OrbitScope PRIVATE glfw glad glm::glm nlohmann_json::nlohmann_json imgui Threads::Threads)
target_link_libraries(OrbitScope PRIVATE $<IF:$<TARGET_EXISTS:mongo::bsoncxx_static>,mongo::bsoncxx_static,mongo::bsoncxx_shared>)
target_link_libraries(OrbitScope PRIVATE $<IF:$<TARGET_EXISTS:mongo::mongocxx_static>,mongo::mongocxx_static,mongo::mongocxx_shared>)

if (WIN32)
    target_link_libraries(OrbitScope PRIVATE opengl32 winhttp)
endif()