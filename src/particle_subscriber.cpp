
#include <Particle.h>
#include <Adafruit_SSD1306_RK.h>
#include <Adafruit_GFX_RK.h>


// forward declarations
void evt_doorbell_handler(const char *event, const char *data);
int render_string(String command);
void blank_screen();
void oneshot_timer_cb();

// -----------------------------------
// Controlling Displays over the Internet
// -----------------------------------
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

static bool _clear_screen = false;
static Timer _cleanup_screen_timer(30000, oneshot_timer_cb, true);

// SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SystemSleepConfiguration _sleep_config;

// Use primary serial over USB interface for logging output
SerialLogHandler logHandler(LOG_LEVEL_WARN, { // Logging level for non-application messages
    { "app", LOG_LEVEL_INFO } // Logging level for application messages
});

void play_bell_tone() {
    for (int count= 0; count < 5; count++) {
        tone(D3, 800, 125);
        delay(125);
        noTone(D3);
        delay(125);
        tone(D3, 700, 500);
        delay(500);
    }
    noTone(D3);
}

// callback from webhook
void evt_doorbell_handler(const char *event, const char *data) {
    Log.info(event);
    play_bell_tone();
    render_string("DOOR");
    _cleanup_screen_timer.reset();
    _cleanup_screen_timer.start();
}

void oneshot_timer_cb() {
    _clear_screen = true;
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


void setup()
{
    Particle.syncTime();
    Particle.function("render",render_string);
    Particle.subscribe("evt_door", evt_doorbell_handler);
    
    Log.info("My device ID: %s", (const char*)System.deviceID());

    pinMode(D3, OUTPUT);// PWM tone output pin

    display_setup();

    // Time.zone(-7.0);
    play_bell_tone();
}

void loop() {

    //allow time for OTA response or firmware updates
    delay(5000);
    Particle.process();

    if (_clear_screen) {
        blank_screen();
        _clear_screen = false;
    }
}






