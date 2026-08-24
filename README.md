# ASCII3D



\# ASCII3D



> A real-time 3D raycasting engine built in C++ and rendered entirely inside the Windows terminal.



!\[ASCII3D V7.1 Color](screenshots/v7-1-color.png)



ASCII3D is a small first-person 3D engine built around one restriction:



\*\*Everything has to run and render inside the terminal.\*\*



No traditional graphics window. No game engine.



Just C++, raycasting, ASCII characters, ANSI colors, and the Windows console.



\---



\# Where This Started



The idea behind ASCII3D actually goes back several years.



When I was around \*\*11 years old\*\*, I was experimenting with Scratch and wanted to figure out how to create a 3D game even though Scratch is primarily a 2D environment.



That led me to \*\*raycasting\*\*.



I created a 2D map and cast rays outward from the player's position. By measuring where those rays collided with walls, I could calculate how tall each wall should appear on screen.



Conceptually, I was taking something like this:



```text

2D MAP



\################

\#..............#

\#.....##.......#

\#........P.....#

\#.........###..#

\#..............#

\################

```



and using math to generate something resembling:



```text

FIRST-PERSON VIEW



&#x20;      ██████████████

&#x20;      █            █

████████            ████████

&#x20;       \\            /

&#x20;        \\          /

&#x20;         \\\_\_\_\_\_\_\_\_/

```



It was one of my earliest experiences with the idea that \*\*a 3D-looking world could be generated entirely from 2D data and mathematics\*\*.



Years later, I wanted to revisit that same idea.



Except this time I wanted to build it with:



\- C++

\- real-time keyboard input

\- DDA raycasting

\- collision detection

\- sprites

\- enemies

\- weapons

\- a minimap

\- ANSI color



And there was one additional restriction:



> \*\*The entire thing had to render inside the Windows terminal.\*\*



That became \*\*ASCII3D\*\*.



\---



\# How ASCII3D Works



Despite looking three-dimensional, the world itself is still just a \*\*2D grid\*\*.



For example:



```text

\##############

\#............#

\#.####.......#

\#.#..#.......#

\#.####.......#

\#............#

\#......P.....#

\#.......#####.

\#.......#...##

\#.......#...##

\#.......#####.

\#............#

\##############

```



The player has:



```text

X position

Y position

viewing angle

```



From that position, the renderer casts rays across the player's field of view.



```text

&#x20;             /

&#x20;           /

&#x20;         /

&#x20;       /

&#x20;     P────────────>

&#x20;       \\

&#x20;         \\

&#x20;           \\

&#x20;             \\

```



Each ray travels through the map until it intersects a wall.



```text

\###############

\#             #

\#        X    #

\#       /     #

\#      /      #

\#     P       #

\#             #

\###############



P = Player

X = Wall intersection

```



The distance between the player and that intersection determines how tall the wall appears.



```text

NEAR WALL                 FAR WALL



████████████                 ████

████████████                 ████

████████████                 ████

████████████                 ████

```



Closer wall:



```text

larger projection

```



Farther wall:



```text

smaller projection

```



Repeating this calculation across the terminal creates the illusion of a 3D environment.



\---



\# DDA Raycasting



Later versions of ASCII3D use \*\*Digital Differential Analyzer (DDA)\*\* raycasting.



A simple ray marcher could move through the world using many tiny steps:



```text

ray += 0.01

ray += 0.01

ray += 0.01

ray += 0.01

...

```



That works, but it performs calculations even when the ray is traveling through empty space.



DDA instead moves between boundaries in the map grid.



```text

PLAYER

&#x20;  ●

&#x20;   \\

&#x20;    \\

──────X──────── Grid boundary

&#x20;      \\

&#x20;       \\

─────────X───── Grid boundary

&#x20;         \\

&#x20;          X   WALL

```



This makes wall intersection testing more efficient and predictable.



The result is:



\- more accurate wall intersections

\- cleaner corners

\- more stable rendering

\- better performance

\- easier texture calculations



\---



\# Rendering Pipeline



Each frame roughly follows this process:



```text

&#x20;       Player Position

&#x20;             +

&#x20;        Viewing Angle

&#x20;              │

&#x20;              ▼

&#x20;       Calculate Rays

&#x20;              │

&#x20;              ▼

&#x20;        DDA Raycasting

&#x20;              │

&#x20;              ▼

&#x20;    Detect Wall Intersection

&#x20;              │

&#x20;              ▼

&#x20;      Calculate Distance

&#x20;              │

&#x20;              ▼

&#x20;  Correct Perspective / Fisheye

&#x20;              │

&#x20;              ▼

&#x20;Calculate Projected Wall Height

&#x20;              │

&#x20;              ▼

&#x20;    Apply Wall Appearance

&#x20;              │

&#x20;              ▼

&#x20;       Render Sprites

&#x20;              │

&#x20;              ▼

&#x20;      Render Weapon/HUD

&#x20;              │

&#x20;              ▼

&#x20;     Build Terminal Frame

&#x20;              │

&#x20;              ▼

&#x20;       ANSI / ASCII Output

&#x20;              │

&#x20;              ▼

&#x20;       Windows Terminal

```



This happens continuously while the player moves through the environment.



\---



\# The Terminal as a Display



A terminal obviously isn't designed to be a graphics API.



ASCII3D essentially treats terminal characters as extremely low-resolution pixels.



Different characters can represent different brightness levels or surfaces:



```text

.

:

x

\#

█

```



The terminal can therefore approximate differences in:



\- distance

\- walls

\- floor

\- geometry

\- objects

\- enemies



Later versions also use \*\*ANSI escape sequences\*\* to add color.



This allowed the engine to preserve the ASCII appearance while making important parts of the scene easier to distinguish.



\---



\# Evolution



ASCII3D wasn't built all at once.



The renderer went through multiple iterations as I experimented with what could actually be displayed clearly inside a terminal.



\## V5.5 — Geometry \& Raycasting



!\[ASCII3D V5.5](screenshots/v5-5-circles.png)



At this stage, the focus was primarily on the rendering system.



Features included:



\- DDA raycasting

\- first-person movement

\- collision detection

\- minimap

\- wall shading

\- floor rendering

\- experiments with more complex geometry



The minimap in the upper-left shows the actual \*\*2D representation of the world\*\* that the raycaster converts into the first-person view.



This was still primarily a rendering experiment rather than a game.



\---



\## V7 — Gameplay



!\[ASCII3D V7 Gameplay](screenshots/v7-gameplay.png)



Once the environment was working, I wanted to see how far I could push it.



So I started adding gameplay.



V7 introduced:



\- enemies

\- sprite projection

\- enemy movement

\- an ASCII first-person weapon

\- shooting

\- ammunition

\- reloading

\- health

\- player damage

\- enemy health

\- kill tracking



At this point, ASCII3D had gone from a graphics experiment to a small playable \*\*terminal FPS\*\*.



The HUD now displayed information such as:



```text

HP 80 | AMMO 12/48 | KILLS 0

```



while the minimap tracked both the player and enemies.



\---



\## V7.1 — ANSI Color



!\[ASCII3D V7.1 Color](screenshots/v7-1-color.png)



The final major iteration added \*\*ANSI color rendering\*\*.



Different parts of the scene could now be visually separated:



```text

Environment → yellow / gray

Enemies     → red

Player      → green

HUD         → cyan

Weapon      → yellow

```



This dramatically improved readability without abandoning the terminal/ASCII aesthetic.



V7.1 became the final major version of the original ASCII3D experiment.



\---



\# Sprites



Walls are only one part of a game world.



Enemies and other objects need to exist \*\*inside the 3D projection\*\* without being part of the map grid.



ASCII3D handles these objects as sprites.



Their position relative to the player is converted from world coordinates into screen coordinates.



Conceptually:



```text

WORLD



&#x20;       Enemy

&#x20;         E



&#x20;            Wall

&#x20;             #



&#x20;    P

&#x20;  Player

```



becomes:



```text

FIRST-PERSON VIEW



&#x20;          /\\

&#x20;         /  \\

&#x20;        / E  \\

&#x20;       /      \\

████████        ████████

```



A depth buffer helps determine whether a sprite should appear in front of or behind walls.



This allows enemies to disappear correctly behind geometry instead of simply being drawn over everything.



\---



\# Turning It Into a Game



Once sprites worked, the renderer could support actual gameplay systems.



ASCII3D eventually gained a basic combat loop:



```text

Player moves

&#x20;    │

&#x20;    ▼

Enemy detected

&#x20;    │

&#x20;    ▼

Player aims

&#x20;    │

&#x20;    ▼

SPACE

&#x20;    │

&#x20;    ▼

Weapon fires

&#x20;    │

&#x20;    ▼

Hit detection

&#x20;    │

&#x20;    ▼

Enemy health decreases

&#x20;    │

&#x20;    ▼

Enemy eliminated

&#x20;    │

&#x20;    ▼

Kill counter increases

```



Enemies can also approach and damage the player.



That required combining the rendering system with:



\- game state

\- entity positions

\- hit detection

\- health systems

\- ammunition

\- enemy behavior

\- frame timing



\---



\# Controls



| Key | Action |

|:---:|---|

| `W` | Move forward |

| `S` | Move backward |

| `A` | Strafe left |

| `D` | Strafe right |

| `←` | Turn left |

| `→` | Turn right |

| `SPACE` | Shoot |

| `R` | Reload |

| `ESC` | Exit |



\---



\# Building ASCII3D



\## Requirements



ASCII3D was developed for Windows using:



\- Windows 10/11

\- C++17

\- Microsoft Visual C++ compiler

\- Windows SDK

\- Visual Studio Developer Command Prompt

\- Windows Terminal / Command Prompt



\---



\## Compile



Open a \*\*Visual Studio Developer Command Prompt\*\*.



Navigate to the repository:



```cmd

cd /d path\\to\\ASCII3D

```



Compile the latest version:



```cmd

cl /EHsc /std:c++17 src\\ascii3d\_v7\_1\_color.cpp user32.lib

```



MSVC will generate:



```text

ascii3d\_v7\_1\_color.exe

```



Then run:



```cmd

ascii3d\_v7\_1\_color.exe

```



\---



\# Repository Structure



```text

ASCII3D/

│

├── README.md

├── .gitignore

│

├── src/

│   ├── ascii3d\_v5\_5\_circles.cpp

│   ├── ascii3d\_v7\_gameplay.cpp

│   └── ascii3d\_v7\_1\_color.cpp

│

├── screenshots/

│   ├── v5-5-circles.png

│   ├── v7-gameplay.png

│   └── v7-1-color.png

│

└── docs/

```



Compiled executables and compiler-generated files are intentionally excluded from the repository.



\---



\# Technical Challenges



Getting something to appear in the terminal wasn't enough.



It had to be understandable while moving.



Some of the biggest challenges were:



\- terminal character aspect ratios

\- fisheye distortion

\- maintaining stable wall geometry

\- depth sorting sprites

\- avoiding terminal scrolling

\- minimizing flicker

\- real-time keyboard input

\- collision detection

\- maintaining usable frame rates

\- making enemies readable against ASCII backgrounds

\- balancing detail with terminal resolution



One of the biggest lessons from the project was:



> \*\*A renderer can be mathematically correct and still be visually terrible.\*\*



A lot of the work involved finding a balance between:



```text

accuracy

&#x20;  ×

resolution

&#x20;  ×

readability

&#x20;  ×

performance

```



\---



\# What I Learned



ASCII3D gave me hands-on experience with concepts including:



\- raycasting

\- DDA algorithms

\- trigonometry

\- perspective projection

\- fisheye correction

\- coordinate systems

\- frame timing

\- game loops

\- collision detection

\- depth buffering

\- sprite projection

\- entity systems

\- basic enemy behavior

\- keyboard input

\- ANSI terminal rendering

\- debugging real-time systems



But one of the most interesting parts was seeing how the same fundamental idea I experimented with in Scratch years earlier could be taken much further with more programming and mathematics.



The core idea was still the same:



```text

2D DATA

&#x20;  +

RAYS

&#x20;  +

DISTANCE

&#x20;  +

PROJECTION

&#x20;  =

3D ILLUSION

```



The implementation had simply grown with me.



\---



\# Then I Had a Question...



After getting a playable FPS running entirely through terminal characters, there was an obvious joke to make:



> \*\*"Okay... but can it run DOOM?"\*\*



So I tried it.



That became a completely separate project:



\## DOOM on CMD



\[View DOOM on CMD](https://github.com/Quackels/Doom-on-cmd-)



Instead of using my custom raycasting engine, DOOM on CMD takes the real DOOM software-rendered framebuffer and translates it into ANSI-colored terminal cells.



The two projects therefore represent two different approaches:



```text

ASCII3D

&#x20;   │

&#x20;   │  Build the 3D renderer myself

&#x20;   │

&#x20;   ▼

Custom Raycasting Engine

&#x20;   │

&#x20;   ├── DDA

&#x20;   ├── Sprites

&#x20;   ├── Combat

&#x20;   └── ANSI Color

&#x20;   │

&#x20;   ▼

"Can it run DOOM?"

&#x20;   │

&#x20;   ▼

DOOM on CMD

&#x20;   │

&#x20;   │  Use DOOM's existing renderer

&#x20;   │

&#x20;   ▼

Framebuffer Conversion

&#x20;   │

&#x20;   ├── Downsampling

&#x20;   ├── RGB Averaging

&#x20;   ├── ANSI Color

&#x20;   └── ASCII Edge Detection

```



What started as a raycasting experiment eventually turned into an exploration of \*\*how far a command-line terminal can be pushed as a real-time graphical display.\*\*



\---



\# AI-Assisted Development



AI tools were used as a programming and debugging assistant during the development of ASCII3D.



They helped with areas such as:



\- debugging C++ issues

\- experimenting with rendering approaches

\- terminal behavior

\- gameplay-system iteration

\- code refinement



The project itself developed through repeated compilation, testing, visual comparison, debugging, and iteration across multiple renderer versions.



\---



\# Project Timeline



```text

Around age 11

&#x20;    │

&#x20;    ▼

Scratch

&#x20;    │

&#x20;    ├── 2D map

&#x20;    ├── raycasting

&#x20;    └── 3D projection

&#x20;    │

&#x20;    │

&#x20;    │  Years later...

&#x20;    ▼

ASCII3D

&#x20;    │

&#x20;    ▼

V5.5

Raycasting + Geometry

&#x20;    │

&#x20;    ▼

V7

Sprites + Combat

&#x20;    │

&#x20;    ▼

V7.1

ANSI Color

&#x20;    │

&#x20;    ▼

Playable Terminal FPS

&#x20;    │

&#x20;    ▼

"Can it run DOOM?"

&#x20;    │

&#x20;    ▼

DOOM on CMD

```



\---



\# Final Thought



ASCII3D started with an idea I first experimented with years ago:



\*\*Can a 2D map be turned into a convincing 3D world using nothing but rays and math?\*\*



Years later, I came back to that question with C++ and pushed it further.



Then I gave it enemies.



Then a gun.



Then color.



And eventually...



I tried DOOM.



Turns out a terminal can do a lot more than print text.



\[View DOOM on CMD](https://github.com/Quackels/Doom-on-cmd-)

