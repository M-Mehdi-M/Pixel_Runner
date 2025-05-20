/*
 * Pixel Runner Game
 */

#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// --- LCD (I2C) address 0x27, 16x2 ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Pins ---
const int buttonStartPin = 2;    // Start/Jump
const int buttonLeftPin = 3;     // Menu select left
const int buttonRightPin = 4;    // Menu select right
const int buzzerPin = 5;
const int redPin = 6;
const int greenPin = 10;
const int bluePin = 13;

// --- Sleep Mode ---
unsigned long lastActivityTime = 0;
const unsigned long SLEEP_TIMEOUT = 60000; // 1 minute in milliseconds
bool isSleeping = false;

// --- Dino and Cactus bitmaps ---
byte dino1[8] = {
    B00111,
    B00101,
    B00111,
    B10110,
    B11111,
    B11110,
    B00110,
    B00100
};
byte dino2[8] = {
    B00111,
    B00101,
    B00111,
    B10110,
    B11111,
    B11110,
    B00110,
    B00010
};
byte dino_jumping[8] = {
    B00111,
    B00101,
    B00111,
    B00110,
    B11111,
    B11110,
    B10110,
    B00100
};
byte ghost1[8] = {
  B11111,
  B10101,
  B11111,
  B11111,
  B01110,
  B01010,
  B11011,
  B00000
};
byte ghost2[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B01110,
  B01010,
  B11011,
  B00000
};
byte ghost_jumping[8] = {
  B11111,
  B10101,
  B11111,
  B11111,
  B01110,
  B01010,
  B01010,
  B01010
};
byte lGhost1[8] = {
  B00000,
  B01110,
  B10101,
  B11011,
  B01110,
  B01110,
  B00000,
  B00000
};
byte lGhost2[8] = {
  B00000,
  B01110,
  B11111,
  B11011,
  B01110,
  B01110,
  B00000,
  B00000
};
byte lGhostjumping[8] = {
  B01110,
  B10101,
  B11011,
  B01110,
  B01110,
  B01110,
  B01010,
  B01010
};
byte arrow_left[8] = {
    B00010,
    B00110,
    B01110,
    B11110,
    B01110,
    B00110,
    B00010,
    B00000
};
byte arrow_right[8] = {
    B01000,
    B01100,
    B01110,
    B01111,
    B01110,
    B01100,
    B01000,
    B00000
};
byte cactus[8] = {
    B00000,
    B00100,
    B00101,
    B10101,
    B10110,
    B01100,
    B00100,
    B00100
};
byte terrain[8] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B11111
};
byte heart[8] = {
  B00000,
  B01010,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000
};

// --- Game variables ---
enum GameLevel { EASY = 0, MEDIUM = 1, HARD = 2 };
GameLevel gameLevel = EASY;
const char* levelNames[3] = {"Easy", "Medium", "Hard"};
int levelSpeeds[3] = {260, 170, 90}; // Easy, Medium, Hard gameSpeed
int cactusMinGap[3] = {6, 4, 2};     // Minimum gap between cactus (higher is easier)
int cactusMaxGap[3] = {10, 7, 4};    // Maximum gap between cactus (lower is harder)
int cactusMax[3] = {2, 3, 4};        // Number of cacti on screen

// --- Character variables ---
enum CharacterType { DINO = 0, GHOST = 1, LGHOST = 2 };
CharacterType characterType = DINO;
const char* characterNames[3] = {"Dino", "Ghost", "LGhost"};

// --- Player tracking ---
#define MAX_PLAYERS 4
int currentPlayer = 0;
int highScores[MAX_PLAYERS] = {0, 0, 0, 0};
// EEPROM storage: 0-3 = highScores (one int per player)

// --- Game state ---
const int dinoCol = 0;
int dinoRow = 1;
bool isJumping = false;
volatile bool jumpRequested = false;
unsigned long jumpStartTime = 0;
const int jumpDuration = 420;
int lives = 3;

int maxCacti = 3;
int cactusCols[5]; // Max for hard mode
unsigned long lastCactusSpawn = 0;
int cactusGap = 8;

unsigned long lastUpdate;
unsigned long startTime;
int gameSpeed = 250;
int score = 0;
int highScore = 0;

// For menu selection
int menuSelection = 0;
const int MENU_ITEMS = 4; // Start Game, High Scores, Instructions, Settings
const char* mainMenuOptions[MENU_ITEMS] = {"Play", "Scores", "Settings", "Help"};

// --- Chiptune Notes ---
#define NOTE_REST 0
#define NOTE_B4  494
#define NOTE_E5  659
#define NOTE_C5  523
#define NOTE_G5  784
#define NOTE_D5  587
#define NOTE_A4  440
#define NOTE_G4  392
#define NOTE_F5  698
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_A5  880
#define NOTE_GS4 415
#define NOTE_AS4 466

int gameplayMelody[] = {
    NOTE_E5, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_A4, 
    NOTE_C5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_B4, NOTE_C5, NOTE_E5, NOTE_D5,
    NOTE_E5, NOTE_G5, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_B4, NOTE_A4
};
int gameplayMelodyDurations[] = {
    120, 120, 120, 120, 120, 120, 180, 80,
    120, 120, 120, 120, 120, 120, 180, 80,
    120, 120, 120, 120, 120, 120, 120, 180
};
const int gameplayMelodyLength = sizeof(gameplayMelody)/sizeof(gameplayMelody[0]);
int melodyIndex = 0;
unsigned long lastMelodyTime = 0;
bool gameplayMusicEnabled = true;

// --- Start and Game Over Melodies ---
int startMelody[] = { NOTE_E5, NOTE_E5, NOTE_REST, NOTE_E5, NOTE_REST, NOTE_C5, NOTE_E5, NOTE_REST, NOTE_G5, NOTE_REST, NOTE_REST, NOTE_G4, NOTE_REST };
int startDurations[] = { 150,150,150,150,150,150,150,150,300,300,300,300,300 };
int gameOverMelody[] = { NOTE_B4, NOTE_F5, NOTE_F5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_REST, NOTE_G4, NOTE_E4, NOTE_A4, NOTE_B4, NOTE_A4, NOTE_GS4, NOTE_AS4, NOTE_GS4, NOTE_G4, NOTE_D4, NOTE_E4 };
int gameOverDurations[] = { 150,150,150,150,150,150,200,200,150,150,150,150,150,150,150,150,150,250,350 };

// --- RGB LED ---
void setRGB(int r, int g, int b) {
    analogWrite(redPin, 255 - r);
    analogWrite(greenPin, 255 - g);
    analogWrite(bluePin, 255 - b);
}

// --- Sleep Mode Functions ---
void enterSleepMode() {
    if (isSleeping) return; // Prevent multiple entries
    
    isSleeping = true;
    
    // Show sleep message before turning off display
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("SLEEP MODE");
    lcd.setCursor(0, 1);
    lcd.print("Press any button");
    
    delay(1500); // Show message before sleeping
    
    lcd.noBacklight(); // Turn off backlight
    setRGB(0, 0, 0);   // Turn off LED
    noTone(buzzerPin); // Stop any sounds
}

void wakeUp() {
    if (!isSleeping) return; // Prevent multiple wake ups
    
    lcd.display();     // Turn on display
    lcd.backlight();   // Turn on backlight
    
    // Show wakeup message
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("WELCOME!");
    lcd.setCursor(1, 1);
    lcd.print("PIXEL RUNNER");
    
    setRGB(0, 255, 0); // Set LED to green
    
    delay(1000); // Show welcome message
    
    isSleeping = false;
    lastActivityTime = millis(); // Reset activity timer
}

// Check for any button press (direct pin read)
bool anyButtonPressed() {
    return (digitalRead(buttonStartPin) == LOW || 
            digitalRead(buttonLeftPin) == LOW || 
            digitalRead(buttonRightPin) == LOW);
}

// --- Sound Functions ---
// New function to control all sound effects
void playSound(int frequency, int duration) {
    // Only play sound if sound is enabled
    if (gameplayMusicEnabled) {
        tone(buzzerPin, frequency, duration);
    }
}

// Modified to respect sound setting for ALL melodies
void playFullMelody(int pin, int* melody, int* durations, int size) {
    // Don't play any melodies if sound is disabled
    if (!gameplayMusicEnabled) return;
    
    for (int i = 0; i < size; i++) {
        if (melody[i] > 0) tone(pin, melody[i], durations[i] * 0.9);
        delay(durations[i]);
        noTone(pin);
    }
}

void playGameplayMusic() {
    if (!gameplayMusicEnabled) return;
    
    unsigned long now = millis();
    if (now - lastMelodyTime > gameplayMelodyDurations[melodyIndex]) {
        if (gameplayMelody[melodyIndex] != NOTE_REST)
            tone(buzzerPin, gameplayMelody[melodyIndex], gameplayMelodyDurations[melodyIndex] * 0.9);
        lastMelodyTime = now;
        melodyIndex = (melodyIndex + 1) % gameplayMelodyLength;
    }
}

// --- EEPROM High Score Functions ---
void loadHighScores() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // Read 2 bytes (int) for each player
        highScores[i] = (EEPROM.read(i*2) << 8) | EEPROM.read(i*2+1);
        
        // If value is invalid (first time running), initialize to 0
        if (highScores[i] < 0 || highScores[i] > 9999) {
            highScores[i] = 0;
        }
    }
}

void saveHighScore(int player, int score) {
    if (score > highScores[player]) {
        highScores[player] = score;
        // Write 2 bytes (int) for the player
        EEPROM.write(player*2, (score >> 8) & 0xFF);
        EEPROM.write(player*2+1, score & 0xFF);
    }
}

// New function that always writes to EEPROM without checking if score is higher
void forceHighScore(int player, int score) {
    highScores[player] = score;
    // Always write to EEPROM, regardless of score value
    EEPROM.write(player*2, (score >> 8) & 0xFF);
    EEPROM.write(player*2+1, score & 0xFF);
}

// --- Buttons ---
bool buttonPressed(int pin) {
    bool pressed = digitalRead(pin) == LOW;
    if (pressed) {
        // Wake up from sleep if needed
        if (isSleeping) {
            wakeUp();
            delay(300); // Debounce
            return false; // Ignore this button press to prevent unwanted actions
        }
        
        // Update activity timer
        lastActivityTime = millis();
    }
    return pressed;
}

// --- Character Sprites Update ---
void updateCharacterSprites() {
    // Update the character sprites based on selection
    switch (characterType) {
        case DINO:
            lcd.createChar(0, dino2);
            lcd.createChar(3, dino_jumping);
            lcd.createChar(4, dino1);
            break;
        case GHOST:
            lcd.createChar(0, ghost2);
            lcd.createChar(3, ghost_jumping);
            lcd.createChar(4, ghost1);
            break;
        case LGHOST:
            lcd.createChar(0, lGhost2);
            lcd.createChar(3, lGhostjumping);
            lcd.createChar(4, lGhost1);
            break;
    }
    // Make sure to create the heart character for lives display
    lcd.createChar(7, heart);
}

// --- Game Logic ---
void setup() {
    pinMode(buttonStartPin, INPUT_PULLUP);
    pinMode(buttonLeftPin, INPUT_PULLUP);
    pinMode(buttonRightPin, INPUT_PULLUP);
    pinMode(buzzerPin, OUTPUT);
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    setRGB(0, 0, 0);

    lcd.init();
    lcd.backlight();
    lcd.createChar(0, dino2);
    lcd.createChar(1, cactus);
    lcd.createChar(2, terrain);
    lcd.createChar(3, dino_jumping);
    lcd.createChar(4, dino1);
    lcd.createChar(5, arrow_left);
    lcd.createChar(6, arrow_right);
    lcd.createChar(7, heart);

    attachInterrupt(digitalPinToInterrupt(buttonStartPin), onJumpButtonPress, FALLING);

    // Load high scores from EEPROM
    loadHighScores();
    
    // Initialize activity timer
    lastActivityTime = millis();
    
    showMainMenu();
}

void onJumpButtonPress() {
    jumpRequested = true;
    lastActivityTime = millis(); // Update activity timer on jump
    
    // Wake up if sleeping
    if (isSleeping) {
        // Will be processed in main loop
        // Don't call wakeUp() in interrupt handler to avoid LCD issues
    }
}

// --- Horizontal Nokia-style Main Menu ---
void showMainMenu() {
    menuSelection = 0;
    bool menuActive = true;
    
    while (menuActive) {
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("PIXEL RUNNER");
        
        // Display the current menu option horizontally
        lcd.setCursor(0, 1); lcd.write(byte(5)); // left arrow
        lcd.setCursor(15, 1); lcd.write(byte(6)); // right arrow
        
        // Center the current menu option
        int optionLength = strlen(mainMenuOptions[menuSelection]);
        int startPos = (16 - optionLength) / 2;
        lcd.setCursor(startPos, 1);
        lcd.print(mainMenuOptions[menuSelection]);
        
        // Wait for button press and check sleep timeout
        unsigned long waitStart = millis();
        while (!buttonPressed(buttonLeftPin) && !buttonPressed(buttonRightPin) && !buttonPressed(buttonStartPin)) {
            delay(50); // Small delay to avoid CPU hogging
            
            // Check for sleep timeout
            if (!isSleeping && millis() - lastActivityTime > SLEEP_TIMEOUT) {
                enterSleepMode();
            }
            
            // If sleeping, check for button presses to wake up
            if (isSleeping && anyButtonPressed()) {
                // Don't wake up here, let buttonPressed handle it
                break;
            }
        }
        
        // If we just woke up from sleep, continue the menu loop
        if (isSleeping) continue;
        
        // Handle button presses
        if (buttonPressed(buttonLeftPin)) {
            menuSelection = (menuSelection + MENU_ITEMS - 1) % MENU_ITEMS;
            playSound(1200, 30);
            delay(200); // Debounce
        }
        
        if (buttonPressed(buttonRightPin)) {
            menuSelection = (menuSelection + 1) % MENU_ITEMS;
            playSound(1200, 30);
            delay(200); // Debounce
        }
        
        if (buttonPressed(buttonStartPin)) {
            playSound(1500, 50);
            delay(200); // Debounce
            
            // Execute the selected menu option
            switch (menuSelection) {
                case 0: // Start Game
                    if (selectPlayer() && selectLevel() && selectCharacter()) {
                        showInstructions();
                        resetGame();
                        menuActive = false; // Exit menu loop to start game
                    }
                    break;
                    
                case 1: // High Scores
                    showHighScores();
                    break;
                    
                case 2: // Instructions
                    showSettings();
                    break;
                    
                case 3: // Settings
                    showInstructions();
                    break;
            }
        }
    }
}

bool selectPlayer() {
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("SELECT PLAYER");
    
    currentPlayer = 0;
    bool selecting = true;
    
    while (selecting) {
        lcd.setCursor(0, 1); lcd.write(byte(5)); // left arrow
        lcd.setCursor(15, 1); lcd.write(byte(6)); // right arrow
        
        // Clear the center area
        for (int i = 1; i < 15; i++) {
            lcd.setCursor(i, 1);
            lcd.print(" ");
        }
        
        // Two display options: show player or show "Back"
        if (currentPlayer < MAX_PLAYERS) {
            // Show player info
            lcd.setCursor(3, 1);
            lcd.print("P");
            lcd.print(currentPlayer + 1);
            lcd.print(" [");
            lcd.print(highScores[currentPlayer]);
            lcd.print("]");
        } else {
            // Show Back option
            lcd.setCursor(6, 1);
            lcd.print("Back");
        }
        
        // Wait for button press and check sleep timeout
        while (!buttonPressed(buttonLeftPin) && !buttonPressed(buttonRightPin) && !buttonPressed(buttonStartPin)) {
            delay(50);
            
            // Check for sleep timeout
            if (!isSleeping && millis() - lastActivityTime > SLEEP_TIMEOUT) {
                enterSleepMode();
            }
            
            // If sleeping, check for button presses to wake up
            if (isSleeping && anyButtonPressed()) {
                // Don't wake up here, let buttonPressed handle it
                break;
            }
        }
        
        // If we just woke up from sleep, continue the loop
        if (isSleeping) continue;
        
        if (buttonPressed(buttonLeftPin)) {
            currentPlayer = (currentPlayer + MAX_PLAYERS) % (MAX_PLAYERS + 1);
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonRightPin)) {
            currentPlayer = (currentPlayer + 1) % (MAX_PLAYERS + 1);
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonStartPin)) {
            playSound(1500, 50);
            selecting = false;
            delay(200);
            
            // If Back was selected, return to main menu
            if (currentPlayer == MAX_PLAYERS) {
                return false;
            }
        }
    }
    
    // Set current high score
    highScore = highScores[currentPlayer];
    return true;
}

bool selectLevel() {
    int selected = 0; // 0=Easy, 1=Medium, 2=Hard, 3=Back
    bool selecting = true;
    
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("SELECT LEVEL");
    
    while (selecting) {
        lcd.setCursor(0, 1); lcd.write(byte(5)); // left arrow
        lcd.setCursor(15, 1); lcd.write(byte(6)); // right arrow
        
        // Clear the center area
        for (int i = 1; i < 15; i++) {
            lcd.setCursor(i, 1);
            lcd.print(" ");
        }
        
        // Display the currently selected level
        if (selected < 3) {
            // Center the level name
            int textLength = strlen(levelNames[selected]);
            int startPos = (16 - textLength) / 2;
            lcd.setCursor(startPos, 1);
            lcd.print(levelNames[selected]);
        } else {
            // Show Back option
            lcd.setCursor(6, 1);
            lcd.print("Back");
        }
        
        // Wait for button press and check sleep timeout
        while (!buttonPressed(buttonLeftPin) && !buttonPressed(buttonRightPin) && !buttonPressed(buttonStartPin)) {
            delay(50);
            
            // Check for sleep timeout
            if (!isSleeping && millis() - lastActivityTime > SLEEP_TIMEOUT) {
                enterSleepMode();
            }
            
            // If sleeping, check for button presses to wake up
            if (isSleeping && anyButtonPressed()) {
                // Don't wake up here, let buttonPressed handle it
                break;
            }
        }
        
        // If we just woke up from sleep, continue the loop
        if (isSleeping) continue;
        
        if (buttonPressed(buttonLeftPin)) {
            selected = (selected + 3) % 4;
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonRightPin)) {
            selected = (selected + 1) % 4;
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonStartPin)) {
            playSound(1500, 50);
            selecting = false;
            delay(200);
            
            // If Back was selected, return to main menu
            if (selected == 3) {
                return false;
            }
            
            gameLevel = static_cast<GameLevel>(selected);
        }
    }
    
    return true;
}

bool selectCharacter() {
    int selected = 0; // 0=Dino, 1=Ghost, 2=LGhost, 3=Back
    bool selecting = true;
    
    // First, make sure all character sprites are created for display
    lcd.createChar(0, dino1);
    lcd.createChar(1, ghost1);
    lcd.createChar(2, lGhost1);
    
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("SELECT CHARACTER");
    
    while (selecting) {
        lcd.setCursor(0, 1); lcd.write(byte(5)); // left arrow
        lcd.setCursor(15, 1); lcd.write(byte(6)); // right arrow
        
        // Clear the center area
        for (int i = 1; i < 15; i++) {
            lcd.setCursor(i, 1);
            lcd.print(" ");
        }
        
        if (selected < 3) {
            // Show only character icon in the middle
            lcd.setCursor(7, 1); // Center position (16/2)
            lcd.write(byte(selected));
        } else {
            // Show Back option
            lcd.setCursor(6, 1);
            lcd.print("Back");
        }
        
        // Wait for button press and check sleep timeout
        while (!buttonPressed(buttonLeftPin) && !buttonPressed(buttonRightPin) && !buttonPressed(buttonStartPin)) {
            delay(50);
            
            // Check for sleep timeout
            if (!isSleeping && millis() - lastActivityTime > SLEEP_TIMEOUT) {
                enterSleepMode();
            }
            
            // If sleeping, check for button presses to wake up
            if (isSleeping && anyButtonPressed()) {
                // Don't wake up here, let buttonPressed handle it
                break;
            }
        }
        
        // If we just woke up from sleep, continue the loop
        if (isSleeping) continue;
        
        if (buttonPressed(buttonLeftPin)) {
            selected = (selected + 3) % 4;
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonRightPin)) {
            selected = (selected + 1) % 4;
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonStartPin)) {
            playSound(1500, 50);
            selecting = false;
            delay(200);
            
            // If Back was selected, return to main menu
            if (selected == 3) {
                // Restore necessary game characters before returning
                lcd.createChar(1, cactus);
                lcd.createChar(2, terrain);
                return false;
            }
            
            characterType = static_cast<CharacterType>(selected);
        }
    }
    
    // Restore necessary game characters
    lcd.createChar(1, cactus);
    lcd.createChar(2, terrain);
    
    // Update character sprites based on selection
    updateCharacterSprites();
    return true;
}

void showHighScores() {
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("HIGH SCORES");
    
    delay(1000);
    
    bool viewing = true;
    int page = 0;
    
    while (viewing) {
        lcd.clear();
        
        if (page < 2) {
            // Display two players per page
            for (int i = 0; i < 2; i++) {
                int playerIndex = page * 2 + i;
                if (playerIndex < MAX_PLAYERS) {
                    lcd.setCursor(4, i);
                    lcd.print("P");
                    lcd.print(playerIndex + 1);
                    lcd.print(": ");
                    lcd.print(highScores[playerIndex]);
                }
            }
        } else {
            // Back option
            lcd.setCursor(6, 0);
            lcd.print("Back");
        }
        
        lcd.setCursor(0, 1); lcd.write(byte(5)); // left arrow (bottom row for consistency)
        lcd.setCursor(15, 1); lcd.write(byte(6)); // right arrow
        
        // Wait for button press and check sleep timeout
        while (!buttonPressed(buttonLeftPin) && !buttonPressed(buttonRightPin) && !buttonPressed(buttonStartPin)) {
            delay(50);
            
            // Check for sleep timeout
            if (!isSleeping && millis() - lastActivityTime > SLEEP_TIMEOUT) {
                enterSleepMode();
            }
            
            // If sleeping, check for button presses to wake up
            if (isSleeping && anyButtonPressed()) {
                // Don't wake up here, let buttonPressed handle it
                break;
            }
        }
        
        // If we just woke up from sleep, continue the loop
        if (isSleeping) continue;
        
        if (buttonPressed(buttonLeftPin)) {
            page = (page + 2) % 3; // 0, 1, 2 (two score pages + back)
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonRightPin)) {
            page = (page + 1) % 3;
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonStartPin)) {
            playSound(1500, 50);
            
            // If Back page selected, exit
            if (page == 2) {
                viewing = false;
            }
            
            delay(200);
        }
    }
}

bool showInstructions() {
    const char* instructions[] = {
        "Button: JUMP!",
        "Avoid cactus",
        "Lives: 3",
        "<> to scroll",
        "More pt=harder",
        "Back"
    };
    
    int numPages = sizeof(instructions) / sizeof(instructions[0]);
    int currentPage = 0;
    bool viewing = true;
    
    while (viewing) {
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("INSTRUCTIONS");
        
        lcd.setCursor(0, 1); lcd.write(byte(5)); // left arrow
        lcd.setCursor(15, 1); lcd.write(byte(6)); // right arrow
        
        // Clear middle area
        for (int i = 1; i < 15; i++) {
            lcd.setCursor(i, 1);
            lcd.print(" ");
        }
        
        // Center the current instruction
        if (currentPage < numPages - 1) {
            // Regular instruction
            int textLength = strlen(instructions[currentPage]);
            int startPos = (16 - textLength) / 2;
            lcd.setCursor(startPos, 1);
            lcd.print(instructions[currentPage]);
        } else {
            // Back option
            lcd.setCursor(6, 1);
            lcd.print("Back");
        }
        
        // Wait for button press and check sleep timeout
        while (!buttonPressed(buttonLeftPin) && !buttonPressed(buttonRightPin) && !buttonPressed(buttonStartPin)) {
            delay(50);
            
            // Check for sleep timeout
            if (!isSleeping && millis() - lastActivityTime > SLEEP_TIMEOUT) {
                enterSleepMode();
            }
            
            // If sleeping, check for button presses to wake up
            if (isSleeping && anyButtonPressed()) {
                // Don't wake up here, let buttonPressed handle it
                break;
            }
        }
        
        // If we just woke up from sleep, continue the loop
        if (isSleeping) continue;
        
        if (buttonPressed(buttonLeftPin)) {
            currentPage = (currentPage + numPages - 1) % numPages;
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonRightPin)) {
            currentPage = (currentPage + 1) % numPages;
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonStartPin)) {
            playSound(1500, 50);
            
            // If back selected, exit to menu
            if (currentPage == numPages - 1) {
                viewing = false;
                delay(200);
                return false;
            }
            
            delay(200);
            viewing = false; // Continue to game if any instruction page is selected
        }
    }
    
    return true;
}

void showSettings() {
    const char* settings[] = {
        "Sound",
        "Reset Scores",
        "Back"
    };
    
    int settingOption = 0;
    bool inSettings = true;
    
    while (inSettings) {
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("SETTINGS");
        
        lcd.setCursor(0, 1); lcd.write(byte(5)); // left arrow
        lcd.setCursor(15, 1); lcd.write(byte(6)); // right arrow
        
        // Clear middle area
        for (int i = 1; i < 15; i++) {
            lcd.setCursor(i, 1);
            lcd.print(" ");
        }
        
        // Center the current setting
        int textLength = strlen(settings[settingOption]);
        int startPos = (16 - textLength) / 2;
        lcd.setCursor(startPos, 1);
        lcd.print(settings[settingOption]);
        
        // Wait for button press and check sleep timeout
        while (!buttonPressed(buttonLeftPin) && !buttonPressed(buttonRightPin) && !buttonPressed(buttonStartPin)) {
            delay(50);
            
            // Check for sleep timeout
            if (!isSleeping && millis() - lastActivityTime > SLEEP_TIMEOUT) {
                enterSleepMode();
            }
            
            // If sleeping, check for button presses to wake up
            if (isSleeping && anyButtonPressed()) {
                // Don't wake up here, let buttonPressed handle it
                break;
            }
        }
        
        // If we just woke up from sleep, continue the loop
        if (isSleeping) continue;
        
        if (buttonPressed(buttonLeftPin)) {
            settingOption = (settingOption + 2) % 3;
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonRightPin)) {
            settingOption = (settingOption + 1) % 3;
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonStartPin)) {
            playSound(1500, 50);
            
            // Handle setting selection
            switch (settingOption) {
                case 0: // Sound Settings
                    soundSettingsMenu();
                    break;
                    
                case 1: // Reset high scores
                    resetScoresMenu();
                    break;
                    
                case 2: // Back
                    inSettings = false;
                    break;
            }
            
            delay(200);
        }
    }
}

void soundSettingsMenu() {
    const char* options[] = {"ON", "OFF", "Back"};
    int selected = gameplayMusicEnabled ? 0 : 1;
    bool selecting = true;
    
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("SOUND");
    
    while (selecting) {
        lcd.setCursor(0, 1); lcd.write(byte(5)); // left arrow
        lcd.setCursor(15, 1); lcd.write(byte(6)); // right arrow
        
        // Clear middle area
        for (int i = 1; i < 15; i++) {
            lcd.setCursor(i, 1);
            lcd.print(" ");
        }
        
        // Center the current option
        int startPos = (16 - strlen(options[selected])) / 2;
        lcd.setCursor(startPos, 1);
        lcd.print(options[selected]);
        
        // Wait for button press and check sleep timeout
        while (!buttonPressed(buttonLeftPin) && !buttonPressed(buttonRightPin) && !buttonPressed(buttonStartPin)) {
            delay(50);
            
            // Check for sleep timeout
            if (!isSleeping && millis() - lastActivityTime > SLEEP_TIMEOUT) {
                enterSleepMode();
            }
            
            // If sleeping, check for button presses to wake up
            if (isSleeping && anyButtonPressed()) {
                // Don't wake up here, let buttonPressed handle it
                break;
            }
        }
        
        // If we just woke up from sleep, continue the loop
        if (isSleeping) continue;
        
        if (buttonPressed(buttonLeftPin)) {
            selected = (selected + 2) % 3;
            if (gameplayMusicEnabled) {
                playSound(1200, 30);
            }
            delay(200);
        }
        
        if (buttonPressed(buttonRightPin)) {
            selected = (selected + 1) % 3;
            if (gameplayMusicEnabled) {
                playSound(1200, 30);
            }
            delay(200);
        }
        
        if (buttonPressed(buttonStartPin)) {
            if (gameplayMusicEnabled) {
                playSound(1500, 50);
            }
            
            // Handle selection
            switch (selected) {
                case 0: // Sound ON
                    gameplayMusicEnabled = true;
                    break;
                    
                case 1: // Sound OFF
                    gameplayMusicEnabled = false;
                    noTone(buzzerPin); // Stop any currently playing tone
                    break;
                    
                case 2: // Back
                    // Just return to main settings
                    break;
            }
            
            selecting = false;
            delay(200);
        }
    }
}

void resetScoresMenu() {
    const char* options[] = {"Yes", "No", "Back"};
    int selected = 1; // Default to "No"
    bool selecting = true;
    
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("RESET ALL SCORES?");
    
    while (selecting) {
        lcd.setCursor(0, 1); lcd.write(byte(5)); // left arrow
        lcd.setCursor(15, 1); lcd.write(byte(6)); // right arrow
        
        // Clear middle area
        for (int i = 1; i < 15; i++) {
            lcd.setCursor(i, 1);
            lcd.print(" ");
        }
        
        // Center the current option
        int startPos = (16 - strlen(options[selected])) / 2;
        lcd.setCursor(startPos, 1);
        lcd.print(options[selected]);
        
        // Wait for button press and check sleep timeout
        while (!buttonPressed(buttonLeftPin) && !buttonPressed(buttonRightPin) && !buttonPressed(buttonStartPin)) {
            delay(50);
            
            // Check for sleep timeout
            if (!isSleeping && millis() - lastActivityTime > SLEEP_TIMEOUT) {
                enterSleepMode();
            }
            
            // If sleeping, check for button presses to wake up
            if (isSleeping && anyButtonPressed()) {
                // Don't wake up here, let buttonPressed handle it
                break;
            }
        }
        
        // If we just woke up from sleep, continue the loop
        if (isSleeping) continue;
        
        if (buttonPressed(buttonLeftPin)) {
            selected = (selected + 2) % 3;
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonRightPin)) {
            selected = (selected + 1) % 3;
            playSound(1200, 30);
            delay(200);
        }
        
        if (buttonPressed(buttonStartPin)) {
            playSound(1500, 50);
            
            // Handle selection
            switch (selected) {
                case 0: // Yes - Reset scores
                    for (int i = 0; i < MAX_PLAYERS; i++) {
                        // Use forceHighScore instead of saveHighScore to ensure scores are reset
                        forceHighScore(i, 0);
                    }
                    
                    lcd.clear();
                    lcd.setCursor(1, 0);
                    lcd.print("Scores Reset!");
                    delay(1500);
                    break;
                    
                case 1: // No
                case 2: // Back
                    // Do nothing, just return
                    break;
            }
            
            selecting = false;
            delay(200);
        }
    }
}

// --- Main Game Loop ---
void loop() {
    // First check if sleep mode is active
    if (isSleeping) {
        // Check for any button press to wake up
        if (anyButtonPressed()) {
            wakeUp();
            delay(300); // Debounce after wake up
        }
        delay(50); // Save power during sleep
        return;  // Skip the rest of the loop
    }
    
    // Check if we should enter sleep mode
    if (millis() - lastActivityTime > SLEEP_TIMEOUT) {
        enterSleepMode();
        return; // Skip the rest of the loop after entering sleep mode
    }
    
    playGameplayMusic();

    // Handle jump behavior and blue LED through the entire jump duration
    if (!isJumping && jumpRequested) {
        isJumping = true;
        jumpStartTime = millis();
        dinoRow = 0;
        playSound(1200, 60);
        jumpRequested = false;
        lastActivityTime = millis(); // Reset activity timer on jump
    }

    if (isJumping) {
        setRGB(0, 0, 255); // LED blue during the jump
        if (millis() - jumpStartTime > jumpDuration) {
            isJumping = false;
            dinoRow = 1;
            setRGB(0, 255, 0); // Back to green after jump
        }
    } else {
        setRGB(0, 255, 0); // Green when not jumping
    }

    if (millis() - lastUpdate > gameSpeed) {
        lastUpdate = millis();

        // Move cacti left
        for (int i = 0; i < maxCacti; i++) {
            if (cactusCols[i] >= 0) cactusCols[i]--;
            if (cactusCols[i] < 0) cactusCols[i] = -1;
        }

        // --- Advanced cactus arrangement based on level ---
        // Only spawn cactus if last cactus is far enough away (gap)
        int rightmost = -cactusGap;
        for (int i = 0; i < maxCacti; i++) {
            if (cactusCols[i] > rightmost) rightmost = cactusCols[i];
        }
        if (rightmost < 15 - cactusGap) {
            // Place cactus with level-based probability
            int chance = (gameLevel == EASY) ? 22 : (gameLevel == MEDIUM ? 33 : 50);
            if (random(100) < chance) {
                // Find empty slot
                for (int i = 0; i < maxCacti; i++) {
                    if (cactusCols[i] == -1) {
                        cactusCols[i] = 15;
                        break;
                    }
                }
                cactusGap = random(cactusMinGap[gameLevel], cactusMaxGap[gameLevel] + 1);
            }
        }

        score++;
        if (score > highScore) {
            highScore = score;
            saveHighScore(currentPlayer, highScore);
        }

        // Draw screen
        lcd.clear();
        // Display score, high score, and lives
        lcd.setCursor(12, 0); 
        lcd.print(score);
        
        // Display lives as hearts
        for (int i = 0; i < lives; i++) {
            lcd.setCursor(2 + i, 0);
            lcd.write(byte(7)); // Heart character
        }

        // Draw dino: show jumping dino when isJumping
        if (isJumping) {
            lcd.setCursor(dinoCol, 0);
            lcd.write(byte(3)); // dino_jumping or equivalent for selected character
        } else {
            lcd.setCursor(dinoCol, dinoRow);
            if (score % 2 == 0)
                lcd.write(byte(0)); // dino2 or equivalent for selected character
            else
                lcd.write(byte(4)); // dino1 or equivalent for selected character
        }

        // Draw cacti
        for (int i = 0; i < maxCacti; i++) {
            if (cactusCols[i] > 0 && cactusCols[i] < 16) {
                lcd.setCursor(cactusCols[i], 1); lcd.write(byte(1));
            }
        }
        // Draw terrain
        for (int col = 0; col < 16; col++) {
            bool skip = (!isJumping && dinoRow == 1 && col == dinoCol); // Don't overwrite dino on ground
            for (int i = 0; i < maxCacti; i++)
                if (cactusCols[i] == col) skip = true;
            if (!skip) {
                lcd.setCursor(col, 1); lcd.write(byte(2));
            }
        }

        // Check collision
        bool collision = false;
        for (int i = 0; i < maxCacti; i++) {
            if (cactusCols[i] == dinoCol && !isJumping && dinoRow == 1) collision = true;
        }
        if (collision) {
            lives--;
            playSound(300, 200);
            
            if (lives <= 0) {
                setRGB(255, 0, 0);
                noTone(buzzerPin);
                
                // Visual game over notification (no sound if disabled)
                if (gameplayMusicEnabled) {
                    playFullMelody(buzzerPin, gameOverMelody, gameOverDurations, sizeof(gameOverMelody)/sizeof(gameOverMelody[0]));
                } else {
                    // Just a visual indication with no sound
                    delay(1000);
                }
                
                setRGB(0, 0, 0);
                showGameOver();
                showMainMenu();
                resetGame();
            } else {
                // Brief invincibility after hit
                lcd.clear();
                lcd.setCursor(3, 0);
                lcd.print("OUCH! ");
                lcd.print(lives);
                lcd.write(byte(7)); // heart
                lcd.print(" left");
                delay(1000);
            }
        }

        // Speed up in hard mode slightly over time
        if (millis() - startTime > 5000 && gameSpeed > 50) {
            gameSpeed -= 5;
            startTime = millis();
            if (gameSpeed <= 110)
                playSound(1046, 30);
        }
    }
}

void resetGame() {
    dinoRow = 1;
    for (int i = 0; i < 5; i++) cactusCols[i] = -1;
    score = 0;
    gameSpeed = levelSpeeds[gameLevel];
    maxCacti = cactusMax[gameLevel];
    cactusGap = random(cactusMinGap[gameLevel], cactusMaxGap[gameLevel] + 1);
    lastUpdate = millis();
    startTime = millis();
    isJumping = false;
    jumpRequested = false;
    melodyIndex = 0;
    lastMelodyTime = 0;
    lives = 3;
    updateCharacterSprites(); // Ensure correct character sprites are loaded
    lcd.clear();
    setRGB(0, 255, 0);
    
    // Reset activity timer when starting a new game
    lastActivityTime = millis();
}

void showGameOver() {
    lcd.clear();
    lcd.setCursor(3, 0); lcd.print("GAME OVER");
    lcd.setCursor(2, 1); lcd.print("Score: "); lcd.print(score);
    delay(2000);
    
    // Show player info
    lcd.clear();
    lcd.setCursor(0, 0); 
    lcd.print("Player ");
    lcd.print(currentPlayer + 1);
    lcd.setCursor(0, 1);
    lcd.print("HS: ");
    lcd.print(highScores[currentPlayer]);
    delay(2000);
    
    // Reset activity timer at game over
    lastActivityTime = millis();
}
