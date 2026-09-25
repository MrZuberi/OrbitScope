import os
import glob
import re

def get_comment_prefix(file_path):
    if file_path.endswith('.cpp') or file_path.endswith('.h') or file_path.endswith('.glsl'):
        return '// '
    elif os.path.basename(file_path) == 'CMakeLists.txt':
        return '# '
    else:
        # Default to // for safety
        return '// '

def describe_file(file_path, content):
    # Remove existing comment lines at the top for analysis? We'll just analyze the whole content.
    # We'll look for class definitions, function definitions, etc.
    lines = content.split('\n')

    # Join lines for easier regex
    full_content = content

    # Look for class definition
    class_match = re.search(r'class\s+(\w+)', full_content)
    if class_match:
        class_name = class_match.group(1)
        # Try to find what the class does by looking at its methods or comments
        # We'll look for a summary comment above the class
        # But we are going to ignore existing comments for the description? We'll try to find a meaningful description.
        # For simplicity, we'll say "Implements the <classname> class"
        # and then try to add more from the file's purpose.
        # We'll look for keywords in the file.
        if 'Application' in class_name:
            return f"Implements the {class_name} class that initializes the window, handles input, manages the simulation, rendering, and UI"
        elif 'Renderer' in class_name:
            return f"Implements the {class_name} class responsible for rendering 3D objects using OpenGL"
        elif 'Camera' in class_name:
            return f"Implements the {class_name} class that handles view transformations and user input for navigation"
        elif 'Simulation' in class_name:
            return f"Implements the {class_name} class that manages the simulation state and time progression"
        elif 'SolarSystem' in class_name:
            return f"Implements the {class_name} class that manages planetary data and their positions"
        elif 'Planet' in class_name:
            return f"Implements the {class_name} class representing a celestial body with orbital properties"
        elif 'Asteroid' in class_name:
            return f"Implements the {class_name} class representing an asteroid with orbital elements"
        elif 'OrbitCalculator' in class_name or 'KeplerOrbitCalculator' in class_name:
            return f"Implements the {class_name} class that calculates orbital positions using Kepler's equation"
        elif 'AsteroidPositioner' in class_name:
            return f"Implements the {class_name} class that calculates marker positions for asteroids relative to planets"
        elif 'AsteroidFilter' in class_name:
            return f"Implements the {class_name} class that filters asteroid data based on distance and time"
        elif 'AsteroidListState' in class_name:
            return f"Implements the {class_name} class that manages the state of the asteroid list UI"
        elif 'ImGuiLayer' in class_name:
            return f"Implements the {class_name} class that integrates Dear ImGui for rendering the user interface"
        elif 'Shader' in class_name:
            return f"Implements the {class_name} class that manages OpenGL shader programs"
        elif 'Texture' in class_name:
            return f"Implements the {class_name} class that loads and manages texture images"
        elif 'MongoRepository' in class_name:
            return f"Implements the {class_name} class that interacts with MongoDB for data storage"
        elif 'MongoEnvironment' in class_name:
            return f"Implements the {class_name} class that manages the MongoDB environment"
        elif 'PlanetRepository' in class_name:
            return f"Implements the {class_name} class that handles retrieval and storage of planet data"
        elif 'ConfigRepository' in class_name:
            return f"Implements the {class_name} class that handles application configuration"
        elif 'Http' in class_name:
            return f"Implements the {class_name} class that provides HTTP client functionality"
        elif 'AsteroidClient' in class_name:
            return f"Implements the {class_name} class that fetches asteroid close-approach data from NASA's API"
        elif 'SBDBClient' in class_name:
            return f"Implements the {class_name} class that fetches orbital elements from NASA's Small-Body Database"
        elif 'KeplerOrbitCalculator' in class_name:
            return f"Implements the {class_name} class that solves Kepler's equation for orbital positions"
        else:
            return f"Implements the {class_name} class"

    # Look for function definitions (if no class)
    function_match = re.search(r'(\w+\s+)\w+\s*\(', full_content)
    if function_match:
        # We'll look for main or specific functions
        if 'int main' in full_content or 'void main' in full_content:
            return "Contains the main application entry point"
        # Look for specific function names
        if 'LoadPlanetTextures' in full_content:
            return "Contains functions for loading planetary textures and managing rendering resources"
        if 'EnableAsteroids' in full_content:
            return "Contains functions for enabling and managing asteroid data loading and visualization"
        if 'RenderUI' in full_content:
            return "Contains functions for rendering the user interface using Dear ImGui"
        if 'ProcessFrame' in full_content:
            return "Contains the main frame processing loop for the application"
        if 'Initialize' in full_content:
            return "Contains initialization functions for setting up the application"
        if 'Shutdown' in full_content:
            return "Contains cleanup functions for shutting down the application"
        if 'DrawPlanetWithRing' in full_content:
            return "Contains functions for rendering planets with optional rings"
        if 'RenderAsteroidMarkers' in full_content:
            return "Contains functions for rendering asteroid markers and their orbit lines"
        if 'RecenterCamera' in full_content:
            return "Contains functions for recentering the camera on selected objects"
        if 'PollAsteroidLoad' in full_content:
            return "Contains functions for polling asynchronous asteroid data loading"
        if 'AsteroidLoadWorker' in full_content:
            return "Contains the worker thread function for loading asteroid data"
        if 'FindPlanetByName' in full_content:
            return "Contains helper functions for finding planets by name"
        if 'PrintControls' in full_content:
            return "Contains functions for printing application controls to the console"
        if 'MouseCallback' in full_content or 'ScrollCallback' in full_content or 'KeyCallback' in full_content:
            return "Contains input callback functions for mouse, scroll, and keyboard events"
        if 'FramebufferSizeCallback' in full_content:
            return "Contains the callback function for handling window resize events"

    # If it's a header file, we can describe what it declares
    if file_path.endswith('.h'):
        # Look for struct or enum declarations
        struct_match = re.search(r'struct\s+(\w+)', full_content)
        if struct_match:
            struct_name = struct_match.group(1)
            return f"Declares the {struct_name} struct and related functions"
        enum_match = re.search(r'enum\s+(\w+)', full_content)
        if enum_match:
            enum_name = enum_match.group(1)
            return f"Declares the {enum_name} enumeration and related functions"
        # If we found a class earlier, we would have caught it, but just in case
        if class_match:
            return f"Declares the {class_name} class and related functions"
        return f"Declares interfaces and data structures used throughout the application"

    # If it's a shader file
    if file_path.endswith('.glsl'):
        if 'vertex' in file_path.lower():
            return "Implements the vertex shader that transforms 3D coordinates to 2D screen space"
        elif 'fragment' in file_path.lower():
            return "Implements the fragment shader that calculates the color of each pixel"
        else:
            return "Implements a GLSL shader for rendering"

    # If it's CMakeLists.txt
    if os.path.basename(file_path) == 'CMakeLists.txt':
        return "Configures the build process using CMake to compile the OrbitScope application"

    # Fallback
    return f"Contains source code for the {os.path.basename(file_path)} file"

def main():
    # Define the patterns for files to process
    patterns = [
        'src/application/**/*.cpp',
        'src/application/**/*.h',
        'src/rendering/**/*.cpp',
        'src/rendering/**/*.h',
        'src/models/**/*.cpp',
        'src/models/**/*.h',
        'src/simulation/**/*.cpp',
        'src/simulation/**/*.h',
        'src/data/**/*.cpp',
        'src/data/**/*.h',
        'resources/shaders/**/*.glsl',
        'CMakeLists.txt'
    ]

    files = []
    for pattern in patterns:
        files.extend(glob.glob(pattern, recursive=True))

    # Sort files for consistent processing
    files.sort()

    changed_files = []

    for file_path in files:
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()

            # Generate description
            description = describe_file(file_path, content)

            # Get comment prefix
            prefix = get_comment_prefix(file_path)

            # Create the comment line
            comment_line = prefix + description

            # Prepend the comment line to the content
            new_content = comment_line + '\n' + content

            # Write back to file
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(new_content)

            changed_files.append(file_path)
            print(f"Processed: {file_path}")
        except Exception as e:
            print(f"Error processing {file_path}: {e}")

    print(f"\nChanged {len(changed_files)} files:")
    for f in changed_files:
        print(f)

if __name__ == '__main__':
    main()