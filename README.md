# OrbitScope: An Interactive 3D Solar System Simulator with Live NASA Data

OrbitScope is a desktop application built using C++ and OpenGL for exploring the solar system in real time. It renders the Sun and all eight planets in an interactive 3D scene, tracks real near-Earth asteroids currently approaching each planet using live NASA and JPL data, and persists planetary data and saved sessions through MongoDB. This README provides documentation for building and running the OrbitScope codebase.

## Key Features

- **3D Solar System Rendering**: The Sun and all eight planets are rendered as lit, textured spheres orbiting at their real relative speeds, using a custom OpenGL rendering pipeline.
- **Free-Flight Camera**: Fly anywhere in the scene with WASD movement, mouse look, sprinting, and scroll-wheel zoom.
- **Simulation Controls**: Pause, resume, speed up, slow down, or fully reset simulated time independently of real time.
- **Live Asteroid Tracking**: Pulls real close-approach data from JPL's Small-Body Database for asteroids currently approaching any planet, not just Earth.
- **Filterable Asteroid List**: An in-engine selectable UI panel lets you filter approaching asteroids by planet and browse them with the keyboard.
- **Camera Focus Mode**: Select an asteroid from the list and the camera flies in for a close-up view scaled to its real reported approach distance.
- **NASA Picture of the Day**: Fetches and logs NASA's daily Astronomy Picture of the Day API on startup.
- **MongoDB-Backed Persistence**: Planetary data is loaded from MongoDB on startup, with automatic seeding of default data if the database is empty.
- **Save/Load Configurations**: Save your current simulation speed, orbit line visibility, scale mode, and selected planet to MongoDB, and reload it later.
- **Graceful Fallback**: The application runs correctly with built-in default data even with no MongoDB connection or NASA API key configured.

## Technologies Used

- **C++17**: Core application language.
- **OpenGL 3.3 (Core Profile)**: Rendering.
- **GLFW**: Windowing, input, and OpenGL context creation.
- **GLAD**: OpenGL function loader.
- **GLM**: Vector and matrix math for 3D transforms.
- **MongoDB C++ Driver (mongocxx / bsoncxx)**: Official native MongoDB driver, compiled from source via vcpkg.
- **nlohmann/json**: Parsing NASA and JPL API responses.
- **stb_truetype**: In-engine bitmap font text rendering for the UI.
- **CMake**: Build system, with GLFW, GLM, and nlohmann/json fetched automatically via FetchContent.
- **WinHTTP**: Native Windows HTTPS client used for all outbound API requests.
- **NASA APOD API** and **JPL Small-Body Database Close Approach API**: External live data sources.

## Prerequisites

- A C++17 compiler. This project was built and tested using a portable MinGW GCC distribution on Windows.
- CMake 3.20 or later.
- [vcpkg](https://github.com/microsoft/vcpkg), used specifically to build the MongoDB C++ driver.
- A MongoDB Atlas cluster (or any reachable MongoDB instance), if you want persistence and saved configurations. Free tier is sufficient.
- A free NASA API key from [api.nasa.gov](https://api.nasa.gov), if you want the Astronomy Picture of the Day feature.
- A TrueType font file, placed at `resources/fonts/arial.ttf`, for in-engine text rendering.
- The `stb_truetype.h` single-header library, placed at `external/stb/stb_truetype.h`.

## Installation

**Clone the repository:**

**Install a C++ toolchain:**
Download a portable MinGW build (for example, the WinLibs UCRT distribution) and a portable CMake distribution. Neither requires administrator access.

**Set up vcpkg and the MongoDB driver:**

**Add the remaining dependencies:**
Download `stb_truetype.h` and place it at `external/stb/stb_truetype.h`. Copy a TrueType font (for example, `arial.ttf` from your system fonts) to `resources/fonts/arial.ttf`.

**Configure MongoDB (optional but recommended):**
Create a free MongoDB Atlas cluster and copy its connection string. This enables planetary data persistence and saved configurations.

**Configure NASA API access (optional):**
Request a free API key from [api.nasa.gov](https://api.nasa.gov). This enables the Astronomy Picture of the Day feature on startup.

**Build the project:**

**Set your environment variables and run:**

Run the executable from the project's root directory, not from inside `build/`, since shaders and other resources are loaded using paths relative to the project root.

## Usage Examples

**Flying through the solar system**
On launch, use WASD to move and the mouse to look around. Hold Left Shift to sprint. Scroll to zoom. Press Space to pause the orbits, and `+`/`-` to change simulation speed.

**Tracking a real asteroid**
Press Tab to cycle the asteroid filter between "All" and each individual planet. Use the Up and Down arrow keys to move the highlighted selection in the side panel. Press Enter to fly the camera in for a close look at that asteroid relative to its target planet, using its real reported approach distance. Press Backspace to return to free flight.

**Saving your session**
Press K at any time to save your current simulation speed, orbit line visibility, scale mode, and selected planet to MongoDB under a quicksave slot. Press L to reload it later, even after restarting the application.

## Controls Reference

| Key | Action |
|---|---|
| W A S D | Move camera |
| Left Shift | Sprint |
| Mouse | Look around |
| Scroll | Zoom |
| Space | Pause / resume |
| + / - | Increase / decrease speed |
| R | Reset simulation |
| O | Toggle orbit lines |
| V | Toggle visual scale mode |
| 0-8 | Select Sun / a planet |
| K | Save configuration |
| L | Load configuration |
| Up / Down | Move asteroid list selection |
| Tab | Cycle asteroid planet filter |
| Enter | Focus camera on selected asteroid |
| Backspace | Exit focus mode |
| H | Print controls to console |
| Escape | Quit |

## Project Structure

The project is organized into the following directories:

- **src/application**: The main `Application` class, owns the window, the game loop, and wires every other system together.
- **src/rendering**: `Renderer`, `Shader`, `Camera`, and `TextRenderer`, everything responsible for drawing to the screen.
- **src/models**: Plain data types for planets and asteroids (`Planet`, `SolarSystem`, `PlanetRecord`, `AsteroidRecord`).
- **src/simulation**: Time-based logic that isn't rendering or data access, orbital math, the asteroid list state, config structs.
- **src/data**: Everything that talks to the outside world, MongoDB access, NASA's API, JPL's API, and the low-level HTTP client.
- **external**: Third-party single-header libraries (GLAD, stb_truetype).
- **resources**: Shaders and font files loaded at runtime.

## Known Limitations

- Planetary orbital distances and sizes are scaled for visibility rather than astronomically accurate, since real distances and sizes can't both be shown clearly in one scene.
- Asteroid marker positions near each planet use a fixed offset and a direction derived from the asteroid's identifier rather than its true 3D trajectory, since full orbital elements for every close-approaching object were outside this project's scope.
- The camera focus view scales distance proportionally to real approach distance in astronomical units, so relative ordering is accurate, but the scaling itself is compressed for visualization rather than a literal 1:1 conversion.
