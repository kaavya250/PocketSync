#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// PocketSync - ESP32 Offline Music Player
// OLED layout corrected to prevent overlapping/clipped text.
// No Node.js code, shuffle/repeat, battery code, currentTime(),
// or totalTime() calls are used.
// ============================================================

// ---------------- OLED ----------------
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS  0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------------- DFPlayer Mini ----------------
HardwareSerial dfSerial(2);
DFRobotDFPlayerMini player;

#define DF_RX 16   // ESP32 RX2  <- DFPlayer TX
#define DF_TX 17   // ESP32 TX2  -> DFPlayer RX

// ---------------- Buttons (active LOW) ----------------
#define PLAY_BUTTON      25
#define NEXT_BUTTON      26
#define PREV_BUTTON      27
#define VOL_UP_BUTTON   32
#define VOL_DOWN_BUTTON 33

// ---------------- Player state ----------------
int currentSong = 1;
int selectedSong = 1;
int totalSongs = 0;
int volume = 18;

bool isPlaying = false;
bool playerReady = false;
bool libraryMode = false;

// ---------------- Progress display ----------------
// This is an estimated progress bar because the installed
// DFRobotDFPlayerMini library does not provide currentTime()
// or totalTime() in this setup.
const int DEFAULT_TRACK_DURATION = 180;
int trackPosition = 0;
int trackDuration = DEFAULT_TRACK_DURATION;

unsigned long trackStartMillis = 0;
unsigned long elapsedBeforePause = 0;
unsigned long lastDisplayUpdate = 0;

// ---------------- Button handling ----------------
struct Button {
  int pin;
  bool lastReading;
  bool stableState;
  unsigned long lastChange;
  unsigned long pressStart;
};

Button buttons[5] = {
  {PLAY_BUTTON, HIGH, HIGH, 0, 0},
  {NEXT_BUTTON, HIGH, HIGH, 0, 0},
  {PREV_BUTTON, HIGH, HIGH, 0, 0},
  {VOL_UP_BUTTON, HIGH, HIGH, 0, 0},
  {VOL_DOWN_BUTTON, HIGH, HIGH, 0, 0}
};

const unsigned long DEBOUNCE_TIME = 35;
const unsigned long LONG_PRESS_TIME = 700;

// ============================================================
// SMALL DISPLAY HELPERS
// ============================================================

void printTrackNumber(int song) {
  display.print("TRACK ");
  if (song < 10) display.print('0');
  display.print(song);
}

void showMessage(const char *line1, const char *line2 = "") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 22);
  display.println(line1);
  if (line2[0] != '\0') {
    display.setCursor(0, 36);
    display.println(line2);
  }
  display.display();
}

// ============================================================
// STARTUP SCREEN
// ============================================================

void startupAnimation() {
  for (int i = 0; i < 4; i++) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // "PocketSync" is kept within the 128-pixel display width.
    display.setTextSize(2);
    display.setCursor(4, 14);
    display.print("PocketSync");

    display.drawRoundRect(5, 39, 118, 12, 4, SSD1306_WHITE);
    int fillWidth = 4 + (i * 28);
    display.fillRect(8, 42, fillWidth, 6, SSD1306_WHITE);

    display.display();
    delay(250);
  }

  showMessage("POCKETSYNC", "Loading music...");
  delay(450);
}

// ============================================================
// NOW PLAYING SCREEN
// ============================================================

void showNowPlaying() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Header: separate positions, both fully inside the OLED.
  display.setTextSize(1);
  display.setCursor(3, 2);
  display.print("POCKETSYNC");
  display.setCursor(98, 2);
  display.print("MUSIC");
  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  // Track label.
  display.setCursor(3, 16);
  display.print("NOW PLAYING");

  // Large track number on its own line; no status text beside it.
  display.setTextSize(2);
  display.setCursor(3, 26);
  printTrackNumber(currentSong);

  // Playback state and volume share a separate, non-overlapping row.
  display.setTextSize(1);
  display.setCursor(3, 43);
  display.print(isPlaying ? "PLAYING" : "PAUSED");

  display.setCursor(82, 43);
  display.print("VOL ");
  display.print(volume);

  // Progress bar.
  display.drawRoundRect(3, 54, 122, 7, 2, SSD1306_WHITE);

  if (trackDuration > 0) {
    int progressWidth = map(
      constrain(trackPosition, 0, trackDuration),
      0, trackDuration,
      0, 116
    );

    if (progressWidth > 0) {
      display.fillRect(6, 56, progressWidth, 3, SSD1306_WHITE);
    }
  }

  display.display();
}

// ============================================================
// MUSIC LIBRARY SCREEN
// ============================================================

void showLibrary() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(3, 3);
  display.print("MUSIC LIBRARY");
  display.drawLine(0, 14, 127, 14, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(3, 21);
  printTrackNumber(selectedSong);

  display.setTextSize(1);
  display.setCursor(3, 43);
  display.print("OF ");
  display.print(totalSongs);
  display.print(" TRACKS");

  display.setCursor(3, 55);
  display.print("PLAY: SELECT");

  display.display();
}

// ============================================================
// SCREEN REFRESH
// ============================================================

void refreshScreen() {
  if (libraryMode) {
    showLibrary();
  } else {
    showNowPlaying();
  }
}

// ============================================================
// PLAY SONG
// ============================================================

void playSong(int song) {
  if (!playerReady || totalSongs < 1) return;
  if (song < 1 || song > totalSongs) return;

  currentSong = song;
  selectedSong = song;

  trackPosition = 0;
  trackDuration = DEFAULT_TRACK_DURATION;
  elapsedBeforePause = 0;
  trackStartMillis = millis();

  player.play(currentSong);

  isPlaying = true;
  libraryMode = false;

  refreshScreen();

  Serial.print("Playing track: ");
  Serial.println(currentSong);
}

// ============================================================
// NEXT / PREVIOUS SONG
// ============================================================

void nextSong() {
  if (!playerReady || totalSongs < 1) return;

  int next = currentSong + 1;
  if (next > totalSongs) next = 1;

  playSong(next);
}

void previousSong() {
  if (!playerReady || totalSongs < 1) return;

  int previous = currentSong - 1;
  if (previous < 1) previous = totalSongs;

  playSong(previous);
}

// ============================================================
// PLAY / PAUSE
// ============================================================

void togglePlayPause() {
  if (!playerReady || totalSongs < 1) return;

  if (isPlaying) {
    elapsedBeforePause += (millis() - trackStartMillis) / 1000UL;
    player.pause();
    isPlaying = false;
  } else {
    player.start();
    trackStartMillis = millis();
    isPlaying = true;
  }

  refreshScreen();
}

// ============================================================
// VOLUME
// ============================================================

void volumeUp() {
  if (!playerReady) return;

  if (volume < 30) {
    volume++;
    player.volume(volume);

    Serial.print("Volume: ");
    Serial.println(volume);

    refreshScreen();
  }
}

void volumeDown() {
  if (!playerReady) return;

  if (volume > 0) {
    volume--;
    player.volume(volume);

    Serial.print("Volume: ");
    Serial.println(volume);

    refreshScreen();
  }
}

// ============================================================
// LIBRARY CONTROLS
// ============================================================

void toggleLibrary() {
  if (!playerReady || totalSongs < 1) return;

  libraryMode = !libraryMode;
  selectedSong = currentSong;
  refreshScreen();
}

void libraryNext() {
  if (totalSongs < 1) return;

  selectedSong++;
  if (selectedSong > totalSongs) selectedSong = 1;

  refreshScreen();
}

void libraryPrevious() {
  if (totalSongs < 1) return;

  selectedSong--;
  if (selectedSong < 1) selectedSong = totalSongs;

  refreshScreen();
}

// ============================================================
// BUTTON ACTIONS
// ============================================================

void handleButton(int index, bool longPress) {
  // PLAY: short press = play/pause or select from library.
  if (index == 0) {
    if (!longPress) {
      if (libraryMode) {
        playSong(selectedSong);
      } else {
        togglePlayPause();
      }
    }
  }

  // NEXT: long press = enter/exit library; short = next track.
  else if (index == 1) {
    if (longPress) {
      toggleLibrary();
    } else if (libraryMode) {
      libraryNext();
    } else {
      nextSong();
    }
  }

  // PREVIOUS: short press = previous track/library item.
  else if (index == 2) {
    if (!longPress) {
      if (libraryMode) {
        libraryPrevious();
      } else {
        previousSong();
      }
    }
  }

  // Volume buttons: short press only.
  else if (index == 3) {
    if (!longPress) volumeUp();
  }
  else if (index == 4) {
    if (!longPress) volumeDown();
  }
}

// ============================================================
// BUTTON SCANNER
// ============================================================

void checkButtons() {
  for (int i = 0; i < 5; i++) {
    bool reading = digitalRead(buttons[i].pin);

    if (reading != buttons[i].lastReading) {
      buttons[i].lastChange = millis();
      buttons[i].lastReading = reading;
    }

    if (millis() - buttons[i].lastChange > DEBOUNCE_TIME) {
      if (reading != buttons[i].stableState) {
        buttons[i].stableState = reading;

        if (reading == LOW) {
          buttons[i].pressStart = millis();
        } else {
          unsigned long duration = millis() - buttons[i].pressStart;
          bool longPress = duration >= LONG_PRESS_TIME;
          handleButton(i, longPress);
        }
      }
    }
  }
}

// ============================================================
// ESTIMATED PROGRESS BAR
// ============================================================

void updateProgress() {
  if (!playerReady || !isPlaying || libraryMode) return;
  if (millis() - lastDisplayUpdate < 700) return;

  lastDisplayUpdate = millis();

  unsigned long elapsed =
    elapsedBeforePause + (millis() - trackStartMillis) / 1000UL;

  trackPosition = min((int)elapsed, trackDuration);
  refreshScreen();
}

// ============================================================
// DFPLAYER EVENTS
// ============================================================

void checkPlayerEvents() {
  if (!player.available()) return;

  uint8_t type = player.readType();
  uint16_t value = player.read();

  if (type == DFPlayerPlayFinished) {
    Serial.print("Finished track: ");
    Serial.println(value);
    nextSong();
  }
  else if (type == DFPlayerError) {
    Serial.print("DFPlayer error: ");
    Serial.println(value);
  }
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);

  // Buttons use internal pull-up resistors.
  for (int i = 0; i < 5; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }

  // ESP32 default I2C pins: SDA 21, SCL 22.
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED ERROR!");
    while (true) delay(10);
  }

  display.clearDisplay();
  display.display();
  startupAnimation();

  // DFPlayer serial connection.
  dfSerial.begin(9600, SERIAL_8N1, DF_RX, DF_TX);

  if (!player.begin(dfSerial)) {
    Serial.println("DFPLAYER ERROR!");
    showMessage("DFPLAYER ERROR", "Check wiring/SD");
    while (true) delay(10);
  }

  playerReady = true;
  delay(1000);

  totalSongs = player.readFileCounts();

  if (totalSongs <= 0) {
    Serial.println("No tracks detected!");
    showMessage("NO MUSIC FOUND", "Check SD card");
    playerReady = false;
    return;
  }

  Serial.print("Tracks detected: ");
  Serial.println(totalSongs);

  player.volume(volume);
  playSong(1);
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
  if (!playerReady) return;

  checkButtons();
  checkPlayerEvents();
  updateProgress();
}
