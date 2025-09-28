cmake_minimum_required(VERSION 3.16)
project(tetris_neat CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Headless Trainer
add_executable(train
    src/train.cpp
    src/game/Board.cpp
    src/game/Tetrimino.cpp
)
target_include_directories(train PRIVATE src)
target_link_libraries(train PRIVATE sfml-graphics sfml-system)

# Final Visualization Demo
add_executable(visual
    src/visual.cpp
    src/game/Board.cpp
    src/game/Tetrimino.cpp
)
target_include_directories(visual PRIVATE src)
target_link_libraries(visual PRIVATE sfml-graphics sfml-window sfml-system)

# Live Training Visualizer
add_executable(visual_train
    src/visual_train.cpp
    src/game/Board.cpp
    src/game/Tetrimino.cpp
)
target_include_directories(visual_train PRIVATE src)
target_link_libraries(visual_train PRIVATE sfml-graphics sfml-window sfml-system)

# single header NEAT library
target_sources(train PRIVATE src/neat/NEAT.h)
target_sources(visual PRIVATE src/neat/NEAT.h)
target_sources(visual_train PRIVATE src/neat/NEAT.h)