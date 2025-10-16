### Only tested in Fedora

compile && run
```bash
g++ -std=c++17 core/*.cpp entities/*.cpp entities/opponents/*.cpp main.cpp `pkg-config --cflags --libs sdl3` -lSDL3_image -lSDL3_ttf -o m
./m
```

# NOTES: 
`m_cameraX` is a *horizontal scroll offset* that defines how far the view has panned left or right across the larger game world -- implements a 2D side-scrolling camera that follows the player.
The *camera* is not a separate object -- it’s implemented through the `m_cameraX` offset

**How the camera works**

During rendering, every entity’s world position (x, y) is adjusted by subtracting m_cameraX to convert it into screen space:
```cpp
SDL_FRect renderBounds = m_player->getBounds();
renderBounds.x -= m_cameraX;  // <-- camera transform
m_player->render(m_renderer, &renderBounds);
```
 
This makes it appear as if the screen is following the player as they move left/right.
 
**How `m_cameraX` is updated** 

In `Game::updateCamera()`: 
```cpp
float target = playerBounds.x - w/2.0f;  // center player horizontally
if (target < 0) target = 0;
if (target > m_worldWidth - w) target = m_worldWidth - w;
m_cameraX = target;
```
The camera tries to keep the player centered horizontally.
It is clamped so you never see outside the world bounds (0 to m_worldWidth - screen_width).
     

## questions/TODO

- store m_cameraX in a local const to help the compiler optimize?
- getBounds() is called twice per entity in some places - cache result?
- use `< random >` instead of srand() and rand() 
- getters like getProjectiles() are allowing for external mutation (could enhance with const versions)