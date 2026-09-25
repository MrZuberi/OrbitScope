# OrbitScope: An Interactive 3D Solar System Simulator with Live NASA Data

OrbitScope is a desktop application built using C++ and OpenGL for exploring the solar system in real time. It opens on a clean, generously spaced view of the Sun and all eight planets with their orbit paths, then lets you switch into a focused asteroid mode that isolates a single planet and shows real, live near-Earth asteroids currently approaching it, using data from NASA and JPL. Planetary data persists through MongoDB.

## Key Features

- **Clean Overview**: The Sun and all eight planets, plus their orbit paths, always visible on launch, spaced out for clarity rather than jammed together.
- **Textured, Self-Rotating Planets**: Earth, Mars, Jupiter, and every other planet render with real surface imagery and spin on their own axis, including Venus and Uranus spinning in their real retrograde direction. Saturn renders with a separate, textured, tilted ring.
- **Fullscreen by Default**: Launches directly into your monitor's native fullscreen resolution.
- **Free-Flight Camera**: WASD movement, mouse look, sprinting, and scroll-wheel zoom, always active.
- **Asteroid Mode**: Press T to isolate a single planet, starting with Earth. Only that planet, its real position, and the real orbital paths of every asteroid currently approaching it are shown, each asteroid rendered as a unique, randomly-shaped rocky mesh rather than a smooth sphere. Switch planets with Tab. Selecting one asteroid from the list hides every other asteroid and orbit line, leaving only that asteroid, its orbit path, and the planet it orbits, and centers the camera on it, after which you fly freely as normal.
- **Never Lose the Planet**: When you're zoomed into a single asteroid at true scale, an on-screen arrow points toward the planet whenever it drifts out of view, so you can always find your way back.
- **Live Data, Loaded in the Background**: Fetching real close-approach and orbital data from JPL never freezes the app, it loads on a background thread while you keep flying around.
- **MongoDB-Backed Persistence**: Planetary data is loaded from MongoDB on startup, with automatic seeding of default data if the database is empty.
- **Graceful Fallback Everywhere**: The application runs correctly with built-in default data and flat-colored planets even with no MongoDB connection and no texture files present.

## Technologies Used

- **C++17**
- **OpenGL 3.3 (Core Profile)** with multisampled anti-aliasing
- **GLFW**: Windowing, fullscreen, input, OpenGL context
- **GLAD**: OpenGL function loader
- **GLM**: Vector and matrix math
- **Dear ImGui**: All on-screen panels and buttons
- **stb_image**: Planet, ring, and asteroid texture loading
- **MongoDB C++ Driver (mongocxx / bsoncxx)**: Compiled from source via vcpkg
- **nlohmann/json**: Parsing JPL API responses
- **CMake**: Build system, with dependencies fetched automatically via FetchContent
- **WinHTTP**: Native Windows HTTPS client
- **JPL Small-Body Database Close Approach API** and **Small-Body Database API**: Live asteroid data sources
- **C++ standard threading**: Background loading of asteroid data so the app never freezes

## A note on scale

Planet sizes and orbital distances in the main view are deliberately not astronomically exact, real distances between planets are so vast that an exact model makes every planet an invisible dot. Instead, planets are spaced out generously and readably, the same tradeoff most solar system illustrations make. Asteroid mode, by contrast, positions the featured planet and its real asteroids using genuine AU-based coordinates from JPL, so that a real close approach is shown at its real relative distance from the planet.

## Prerequisites

- A C++17 compiler (portable MinGW GCC works, no admin rights required)
- CMake 3.20 or later
- [vcpkg](https://github.com/microsoft/vcpkg), used to build the MongoDB C++ driver
- A MongoDB Atlas cluster (optional, enables persistence)
- Planet, ring, and asteroid texture images in `resources/textures/` (optional, falls back to flat colors)

## Installation

**Clone the repository:**

**Install a C++ toolchain and vcpkg, then the MongoDB driver:**

git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install mongo-cxx-driver:x64-mingw-dynamic --host-triplet=x64-mingw-dynamic
cd ..

**Configure MongoDB:**
Create a free MongoDB Atlas cluster and copy its connection string

**Build:**

cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic -DVCPKG_HOST_TRIPLET=x64-mingw-dynamic
cmake --build build


Run from the project root, not from inside `build/`, since shaders and textures load using relative paths.

## Controls Reference

| Key | Action |
|---|---|
| W A S D | Fly the camera (always active) |
| Left Shift | Sprint |
| Mouse | Look around |
| Scroll | Zoom |
| Left Alt | Free the mouse cursor to click on-screen buttons |
| Space | Pause / resume time |
| + / - | Increase / decrease speed |
| R | Reset simulation |
| O | Toggle orbit paths |
| T | Enter or exit asteroid mode |
| Tab | (Asteroid mode) switch which planet you're viewing |
| Up / Down | (Asteroid mode) pick an asteroid; camera centers on it |
| H | Print controls to console |
| Escape | Quit |

## How It Works

This section walks through the codebase in the order it was actually built, each link points to the file in the repository so you can read the real implementation, in the order it makes sense to learn it.

1. [`src/main.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/main.cpp) creates the single `Application` object and starts its run loop. This is the entire entry point of the program.
2. [`src/application/Application.h`](https://github.com/MrZuberi/OrbitScope/blob/main/src/application/Application.h) declares the `Application` class, the central hub that owns the window, every subsystem, and the main loop, and defines the shape of the whole program.
3. [`src/rendering/Shader.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/rendering/Shader.cpp) compiles and links the GLSL shader programs the graphics card runs to turn 3D points into colored pixels.
4. [`src/rendering/Renderer.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/rendering/Renderer.cpp), see `BuildSphereMesh` near the top, builds the reusable sphere shape every planet and asteroid marker is drawn from, and the functions below it (`BuildOrbitMesh`, `BuildStarMesh`, `BuildRingMesh`, `BuildAsteroidMeshes`) build every other shape used in the scene.
5. [`src/models/Planet.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/models/Planet.cpp) defines what a single planet knows about itself, its size, color, texture, and how to turn that into a position and a model matrix at a given moment in time.
6. [`src/models/SolarSystem.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/models/SolarSystem.cpp), see `GetDefaultRecords`, is where every planet's actual numbers live, radius, curated overview distance, real AU distance, texture path, and rotation period.
7. [`src/rendering/Camera.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/rendering/Camera.cpp) is the free-flight camera, tracking a position and facing direction from raw mouse and keyboard input, plus `SetPositionAndTarget`, the one-time snap used whenever the app centers on a planet or asteroid.
8. [`src/simulation/Simulation.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/simulation/Simulation.cpp) tracks simulated elapsed time independently of real time, this is the one number that pause, speed, and reset all change.
9. [`src/simulation/OrbitCalculator.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/simulation/OrbitCalculator.cpp) turns a planet's distance, period, and the simulation's elapsed time into an actual circular position, the core orbital math for the overview.
10. [`src/data/MongoEnvironment.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/data/MongoEnvironment.cpp) and [`src/data/MongoRepository.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/data/MongoRepository.cpp) are the only files in the project that talk to MongoDB directly, everything else asks these to do it on their behalf.
11. [`src/data/PlanetRepository.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/data/PlanetRepository.cpp) knows how to turn a `Planet`'s data into a MongoDB document and back again, and seeds the database with defaults the first time it runs.
12. [`resources/shaders/fragment.glsl`](https://github.com/MrZuberi/OrbitScope/blob/main/resources/shaders/fragment.glsl) is where the actual lighting math lives, ambient, diffuse, specular highlight, and rim light, computed per pixel on the graphics card.
13. [`src/models/AsteroidRecord.h`](https://github.com/MrZuberi/OrbitScope/blob/main/src/models/AsteroidRecord.h) and [`src/models/AsteroidOrbitalElements.h`](https://github.com/MrZuberi/OrbitScope/blob/main/src/models/AsteroidOrbitalElements.h) define the plain data an asteroid carries, including its real orbital elements when available.
14. [`src/data/AsteroidClient.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/data/AsteroidClient.cpp) fetches the live list of asteroids currently making a close approach to any planet from JPL's Close Approach API.
15. [`src/data/SBDBClient.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/data/SBDBClient.cpp) fetches a specific asteroid's real orbital elements, semi-major axis, eccentricity, inclination, from JPL's Small-Body Database API.
16. [`src/simulation/KeplerOrbitCalculator.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/simulation/KeplerOrbitCalculator.cpp) solves Kepler's equation to turn a real asteroid's orbital elements into an actual 3D position and a full elliptical path at any point in time.
17. [`src/simulation/AsteroidFilter.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/simulation/AsteroidFilter.cpp) and [`src/simulation/AsteroidListState.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/simulation/AsteroidListState.cpp) narrow the full asteroid list down to one planet and track which one is currently selected.
18. [`src/simulation/AsteroidPositioner.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/simulation/AsteroidPositioner.cpp) places asteroids that have no published orbital elements at a plausible, consistent marker position near their target planet instead.
19. [`src/rendering/ImGuiLayer.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/rendering/ImGuiLayer.cpp) sets up Dear ImGui and chains its input handling alongside the app's own camera controls, this is what makes every on-screen panel and button actually work.
20. [`src/rendering/Texture.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/rendering/Texture.cpp) loads an image file from disk onto the graphics card as a texture, with a graceful fallback if the file is missing.
21. [`src/application/Application.cpp`](https://github.com/MrZuberi/OrbitScope/blob/main/src/application/Application.cpp), see `AsteroidLoadWorker` and `PollAsteroidLoad`, runs the live asteroid fetch on a background thread so the app never freezes while waiting on the network, and `RenderUI` at the bottom draws every panel, the asteroid list, and the off-screen planet arrow.

## Project Structure

- **src/application**: `Application`, owns the window, the game loop, and mode switching between overview and asteroid mode
- **src/rendering**: `Renderer`, `Shader`, `Camera`, `Texture`, `ImGuiLayer`
- **src/models**: `Planet`, `SolarSystem`, `PlanetRecord`, `AsteroidRecord`, `AsteroidOrbitalElements`
- **src/simulation**: Orbital math (curated overview and real Keplerian asteroid math), asteroid list state
- **src/data**: MongoDB access, JPL's two APIs, the low-level HTTP client
- **external**: GLAD, stb_image
- **resources**: Shaders and textures

## Known Limitations

- Overview spacing is illustrative, not astronomically exact.
- Asteroids without published orbital elements use an approximate marker position near their target planet rather than a true trajectory.
- Asteroid shapes are procedurally generated rocky variants, not models of the actual physical shape of each real asteroid, since real shape data isn't available for most catalogued objects.
- All UI interaction happens through the keyboard by default; Left Alt frees the mouse for clicking buttons.

## License

MIT License.

## Acknowledgments

- NASA and the Jet Propulsion Laboratory, for the Small-Body Database Close Approach API and Small-Body Database API
- MongoDB, for the official mongocxx and bsoncxx drivers
- Omar Cornut and contributors, for Dear ImGui
- Sean Barrett, for stb_image
- Solar System Scope, for the free, Creative Commons licensed planet textures
- The GLFW, GLAD, GLM, and nlohmann/json project maintainers