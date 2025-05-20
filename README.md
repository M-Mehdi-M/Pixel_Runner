# Pixel Runner Game
A simple "endless runner" arcade game for Arduino, inspired by the Chrome Dino game.  
Uses an I2C LCD display, RGB LED, buzzer, and multiple buttons for controls.  
Supports multiple characters, levels, and persistent high scores.

## Hardware Requirements
- Arduino Uno or compatible board
- 16x2 LCD with I2C backpack (address 0x27)
- 3 momentary push buttons (Start/Jump, Left, Right)
- RGB LED (common anode/cathode, PWM pins)
- Buzzer (digital PWM pin)
- EEPROM (onboard)
- Resistors as needed

## Features
- **Multiple players**: Up to 4 user profiles with separate high scores
- **3 Characters**: Dino, Ghost, LGhost (each animated)
- **3 Levels**: Easy, Medium, Hard (affects speed and cactus frequency)
- **EEPROM High Scores**: Persistent storage for each player
- **Menus**: Horizontal navigation (Play, Scores, Settings, Help)
- **Sleep Mode**: Auto sleep for power saving, wakes on any button
- **Sound and RGB feedback**: Chiptune melodies, jump/collision sounds, dynamic LED color
- **Instructions and Settings**: In-game help, sound on/off, reset high scores

## How to Play
1. Power on the Arduino and LCD.
2. Use Left/Right buttons to navigate menus.
3. Select "Play" to start:
    - Choose player, level, character.
    - Press Start/Jump button to jump over cacti.
    - Survive as long as possible, avoid obstacles, and earn a high score!
    - Game ends when you lose all lives.
4. Check "Scores" to see best runs for each player.
5. "Settings" allows toggling sounds and resetting high scores.
6. "Help" shows instructions.

## Software Design Overview
- **Development Environment**: Arduino IDE  
  Chosen for its ease of use, built-in libraries, and wide hardware support.

- **Libraries Used**:
    - `LiquidCrystal_I2C` for I2C LCD control
    - `EEPROM` for persistent score storage

- **Core Functions**:
    - **Main Menu System**: Handles navigation and option selection.
    - **selectPlayer/Level/Character**: User selects their profile, game difficulty, and avatar.
    - **Game Loop**: Manages animation, obstacle generation, collision detection, scoring, and level logic.
    - **EEPROM Management**: Reads and writes high scores for all players.
    - **Sound/LED Effects**: Provides audio/visual feedback for events (jump, game over, etc.).
    - **Sleep Mode**: Saves power, wakes on any button.

- **Button Handling**:  
  All buttons use internal pull-ups; jump button also uses interrupts for fast reaction.

- **Animation**:  
  Animated characters and obstacles use custom LCD characters, updated per frame.
