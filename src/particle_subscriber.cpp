
#include <Particle.h>
#include <Adafruit_SSD1306_RK.h>
#include <Adafruit_GFX_RK.h>


// forward declarations
void evt_doorbell_handler(const char *event, const char *data);
int render_string(String command);
int tone_test(String name);
void blank_screen();
void oneshot_timer_cb();

// OLED screen definitions
const uint8_t SCREEN_WIDTH = 128; // width, in pixels
const uint8_t SCREEN_HEIGHT = 64; // height, in pixels

// PWM audio output pin
const uint16_t SPKR_PIN = D3;
// approximate tone frequencies:
const unsigned int TONE_A4 = 440;
const unsigned int TONE_B4 = 494;
const unsigned int TONE_C4 = 262;
const unsigned int TONE_D4 = 294;
const unsigned int TONE_E4 = 330;
const unsigned int TONE_F4 = 349;
const unsigned int TONE_G4 = 392;

const int NOTE_DURATION_MS = 500;

// static globals
static bool g_clear_screen = false;
static bool g_playing_tones = false;
static Timer g_screen_cleanup_timer(30000, oneshot_timer_cb, true);

// SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
SystemSleepConfiguration g_sleep_config;

// Use primary serial over USB interface for logging output
SerialLogHandler logHandler(LOG_LEVEL_WARN, { // Logging level for non-application messages
    { "app", LOG_LEVEL_INFO } // Logging level for application messages
});

// play a tone and wait for it to complete before moving on
void sync_play_tone(unsigned int frequency, int milliseconds) {
    if (frequency != 0) {
        tone(SPKR_PIN, frequency, milliseconds);
    }
    else {
        noTone(SPKR_PIN);
    }
    Particle.process();
    delay(milliseconds);
}

void play_bell_tone() {
    if (!g_playing_tones) {
        g_playing_tones = true;
        for (int count= 0; count < 2; count++) {
            // doorbell close encounters
            sync_play_tone(2*TONE_D4, NOTE_DURATION_MS);
            sync_play_tone(2*TONE_E4, NOTE_DURATION_MS);
            sync_play_tone(2*TONE_C4, NOTE_DURATION_MS);
            sync_play_tone(TONE_C4, NOTE_DURATION_MS);
            sync_play_tone(TONE_G4, 2*NOTE_DURATION_MS);
            sync_play_tone(0, 2*NOTE_DURATION_MS);
        }
        noTone(SPKR_PIN);
        g_playing_tones = false;
    }
}

void play_wakeup_tones() {
    sync_play_tone(TONE_C4, NOTE_DURATION_MS);
    sync_play_tone((2*TONE_C4), NOTE_DURATION_MS);
    sync_play_tone(TONE_C4, NOTE_DURATION_MS);
    sync_play_tone((2*TONE_C4), NOTE_DURATION_MS);
    noTone(SPKR_PIN);
}

// callback from webhook
void evt_doorbell_handler(const char *event, const char *data) {
    Log.info(event);
    render_string("DOOR");
    play_bell_tone();
    g_screen_cleanup_timer.reset();
    g_screen_cleanup_timer.start();
}

// called by the screen cleanup timer
void oneshot_timer_cb() {
    g_clear_screen = true;
}

// clear the OLED display
void blank_screen() {
    display.clearDisplay();
    display.display();
}

// remotely display a string
int render_string(String command) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(4, 4);             
    display.setTextSize(4);            
    display.println(command);
    display.display();
    
    //ensure we write to the display
    delay(5000);
    return 0;
}

// Remote test tones
int tone_test(String name) {
    if (name.equalsIgnoreCase("bell")) {
        play_bell_tone();
    }
    else if (name.equalsIgnoreCase("wakeup")) {
        play_wakeup_tones();
    }
    return 0;
}

// configure the display driver
void display_setup() { 
  delay(250);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32 // Address 0x3D for 128x64
    Log.error("SSD1306 allocation failed");
  }

  // Clear the buffer
  display.clearDisplay();
  display.display();
  delay(2000);
}

void setup() {
    pinMode(SPKR_PIN, OUTPUT);// PWM tone output pin

    Particle.syncTime();
    Particle.function("render",render_string);
    Particle.function("tone_test",tone_test);
    Particle.subscribe("household/frontdoor/bell01", evt_doorbell_handler);
    Log.info("My device ID: %s", (const char*)System.deviceID());

    // Publish vitals periodically, indefinitely
    Particle.publishVitals(120);  

    display_setup();
    play_wakeup_tones();
}

void loop() {
    //allow time for OTA response or firmware updates
    delay(5000);
    Particle.process();

    if (g_clear_screen) {
        g_clear_screen = false;
        blank_screen();
    }
}






