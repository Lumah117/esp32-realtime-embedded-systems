// ========= Pin Definitions
// Define GPIO pins for the ESP32 board
#define OUTPUT_ENABLE_PIN  2  // Push Button 1 (PB1) - Enables/Disables Data Output
#define OUTPUT_SELECT_PIN  4  // Push Button 2 (PB2) - Switches between Normal & Alternate Waveforms
#define DATA_PIN           32  // Data Signal (Signal A) Output pin
#define SYNC_PIN           25  // Sync Signal (Signal B) Output pin

// ========= Timing Parameters
// Timing Parameters (calculated based on surname M, I, T, C)
#define TON1        1300  // First pulse on-time (in microseconds)
#define TOFF        900  // Pulse off-time (in microseconds)
#define NUM_PULSES  11  // Number of pulses in the DATA waveform cycle
#define TIDLE       1500  // Idle time before the SYNC pulse (in microseconds)
#define TSYNC_ON    50  // SYNC pulse on-time (in microseconds)

// ========= Constants
// Declaration of constant variables to replace floating/magic numbers
#define TON_INCREMENT 50  // Increment value for each subsequent pulse (normal wave)
#define ALTERNATE_PULSE_REDUCTION 3  // Number of pulses to be removed (alternate wave)
#define DEBUG_TIMING_FACTOR 1000  // Timing factor for debug
#define DEFAULT_TIMING_FACTOR 1  // Timing factor for production
#define DEBOUNCE_DELAY_MS 500  // Debounce time (milliseconds)
#define MICROSECOND_DELAY_LIMIT 16383  // ESP32 limit for delayMicroseconds()  

// ========= Compile-Time Configuration
// Compile-time configuration
// If DEBUG is enabled, slow down timing by a factor of 1000 for LED observation
#ifdef DEBUG
  #define TIMING_FACTOR DEBUG_TIMING_FACTOR  // Slow down for visual debugging (LED observation)
#else
  #define TIMING_FACTOR DEFAULT_TIMING_FACTOR  // Use the actual Production timing
#endif

// ========= Global Varaible Declarations
// Global Variables
bool outputEnabled = false;  // Tracks if DATA output is enabled
bool normalWaveform = true;  // Tracks the waveform type (Normal/Alternative)

// ========= Function Prototypes
// Function Prototypes
void generateNormalWaveform();  // Function to generate the Normal DATA waveform
void generateAlternateWaveform();  // Function to generate the Alternate DATA waveform
void generateSyncPulse();  // Function to generate the SYNC pulse
void handleButtonPresses();  // Function to handle the button state changes

// ========= Setup Function
void setup() {
  // Pin Configurations
  // Set button pins as inputs with internal pull-down resistors
  pinMode(OUTPUT_ENABLE_PIN, INPUT_PULLDOWN);  // Push Button 1
  pinMode(OUTPUT_SELECT_PIN, INPUT_PULLDOWN);  // Push Button 2

  //Set output pins for DATA and SYNC signals
  pinMode(DATA_PIN, OUTPUT);  // DATA output
  pinMode(SYNC_PIN, OUTPUT);  // SYNC output

  // Initialize Outputs
  // Ensure outputs start LOW (off)
  digitalWrite(DATA_PIN, LOW);
  digitalWrite(SYNC_PIN, LOW);
}

// ========= Main Loop
void loop() {
  // Handle button presses
  handleButtonPresses();

  // If output is enabled, generate the waveform
  if (outputEnabled) {
    if (normalWaveform) {
      generateNormalWaveform();  // Generate the Normal Waveform
    } else {
      generateAlternateWaveform();  // Generate the Alternative Waveform
    }
    generateSyncPulse();  // Generate the SYNC pulse
  }
}

// ========= Generate Normal DATA Waveform
// Function to Generate the Normal Waveform
void generateNormalWaveform() {
  for (int index = 1; index <= NUM_PULSES; index++) {
    // Calculate pulse ON time for the current pulse
    int pulseOnTime = TON1 + (index * TON_INCREMENT);  // Calculate TON(n)
    pulseOnTime *= TIMING_FACTOR;  // Adjust timing for DEBUG mode if enabled

    // Turn DATA signal ON
    digitalWrite(DATA_PIN, HIGH);
    delayMicroseconds(pulseOnTime);

    // Turn DATA signal OFF
    digitalWrite(DATA_PIN, LOW);
    delayMicroseconds(TOFF * TIMING_FACTOR);
  }
  delayMicroseconds(TIDLE);
}

// ========= Generate Alternate DATA Waveform
// Function to Generate the Alternative Waveform with last 3 pulses removed
void generateAlternateWaveform() {
  int reducedPulses = NUM_PULSES - ALTERNATE_PULSE_REDUCTION;  // Removes the last 3 pulses

  for (int index = 1; index <= reducedPulses; index++) {
    // Calculate pulse ON time for the current pulse
    int pulseOnTime = TON1 + (index * TON_INCREMENT);  // Calculate the TON(n)
    pulseOnTime *= TIMING_FACTOR;  // Adjust timing for DEBUG mode if enabled

    // Turn DATA signal ON
    digitalWrite(DATA_PIN, HIGH);
    delayMicroseconds(pulseOnTime);

    // Turn DATA signal OFF
    digitalWrite(DATA_PIN, LOW);
    delayMicroseconds(TOFF * TIMING_FACTOR);
  }
  delayMicroseconds(TIDLE);
}

// ========= Generate SYNC Pulse
// Function to Generate the SYNC Pulse after DATA waveform completes
void generateSyncPulse() {
  // If the idle time before SYNC is too long, use delay() instead of delayMicroseconds()
  if (TIDLE * TIMING_FACTOR > MICROSECOND_DELAY_LIMIT) {
    delay((TIDLE * TIMING_FACTOR) / 1000);
  } else{
    delayMicroseconds(TIDLE * TIMING_FACTOR);  // Idle time
  }
  
  // Generate SYNC pulse (HIGH for TSYNC_ON duration)
  digitalWrite(SYNC_PIN, HIGH);
  delayMicroseconds(TSYNC_ON * TIMING_FACTOR);
  digitalWrite(SYNC_PIN, LOW);
}

// ========= Handling Button Presse (with Debounce)
// Function to Handle Button Presses with Debounce (chacks state of buttons and toggles accordingly)
void handleButtonPresses() {
  static unsigned long lastDebounceTime = 0;  // Stores the last time a button was pressed

  // Read the current state of the push buttons
  bool currentEnableState = digitalRead(OUTPUT_ENABLE_PIN);
  bool currentSelectState = digitalRead(OUTPUT_SELECT_PIN);

  // Only executes if debounce delay has passed (preventing multiple triggers)
  if (millis() - lastDebounceTime > DEBOUNCE_DELAY_MS) {

    // If Push Button 1 is pressed, toggle DATA output enable state
    if (currentEnableState == HIGH) {
      outputEnabled = ! outputEnabled;
      lastDebounceTime = millis();  // Reset debounce timer
    }

    // If Push Button 2 is pressed, toggle waveform mode (Normal/Alternate)
    if (currentSelectState == HIGH) {
      normalWaveform = ! normalWaveform;
      lastDebounceTime = millis();  // Reset debounce timer
    }
  }
}
