# NES Tetris Clone using SDL2

This project is a clone of the classic **NES Tetris**, developed using **C++** and **SDL2**. It is designed to demonstrate game development skills and proficiency in **C++** and **SDL2**, making it a valuable addition to my portfolio.

![Tetris Gameplay](recording_2025-03-19_14.35.00(1).gif)


## Features
- Classic Tetris gameplay
- Pixel-perfect rendering using SDL2
- Smooth piece movement and rotation
- Collision detection and line clearing
- Score tracking system
- Configurable controls

## Requirements
To build and run this project, you need:
- C++ compiler (GCC, Clang, or MSVC)
- SDL2 library
- SDL2_ttf (for rendering text, if applicable)
  
## Installation
### Clone the repository
```sh
git clone https://github.com/yourusername/NES-Tetris-Clone.git
cd NES-Tetris-Clone
```

### Build Instructions (Linux/macOS)
```sh
mkdir build
cd build
cmake ..
make
./tetris
```

### Build Instructions (Windows)
Use MinGW or Visual Studio to compile the project. If using MinGW:
```sh
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
./tetris.exe
```

## Controls
- **Arrow Keys**: Move left/right
- **Down Arrow**: Soft drop
- **Up Arrow**: Rotate piece
- **Space**: Hard drop
- **Escape**: Pause/Exit

## Future Improvements
- Implement sound effects and music
- Add a high score system
- Implement different Tetris modes
- Improve animations and visual effects

