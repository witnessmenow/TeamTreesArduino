/*******************************************************************
    Displaying the live #teamtrees count on a RGB Matrix board

    Fetches the data directly from teamtrees.org
 *                                                                 *
    Built using an ESP32 and using my own ESP32 Matrix Shield
    https://www.tindie.com/products/brianlough/esp32-matrix-shield-mini-32/

    If you find what I do usefuland would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/

    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// ----------------------------
// Standard Libraries - Already Installed if you have ESP32 set up
// ----------------------------

#include <Ticker.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------


#define double_buffer // this must be enabled to stop flickering
#include <PxMatrix.h>
// The library for controlling the LED Matrix
//
// Can be installed from the library manager
//
// https://github.com/2dom/PxMatrix

// Adafruit GFX library is a dependancy for the PxMatrix Library
// Can be installed from the library manager
// https://github.com/adafruit/Adafruit-GFX-Library

#include <TetrisMatrixDraw.h>
// This library draws out characters using a tetris block
// amimation
// Can be installed from the library manager
// https://github.com/toblum/TetrisAnimation


// ----------------------------
// Wiring and Display setup
// ----------------------------

#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 2
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// This defines the 'on' time of the display is us. The larger this number,
// the brighter the display. If too large the ESP will crash
uint8_t display_draw_time = 10; //10-50 is usually fine

PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);

TetrisMatrixDraw tetris(display); // Bottom Text
TetrisMatrixDraw tetrisTwo(display); // Top Counter

// Some standard colors
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 255, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myBLACK = display.color565(0, 0, 0);

Ticker animation_ticker;
bool finishedAnimating = false;
bool finishedAnimatingTwo = false;
unsigned long resetAnimationDue = 0;

WiFiClientSecure client;

//Beacuse this is not an API, there are no terms of service how often
// it can be hit, once a minute is probably ok though.
unsigned long timeBetweenRequests = 60000; // 1 minute
unsigned long apiRequestDue = 0; // When API request is next Due

// ----------------------------
// Config - Replace These
// ----------------------------

char ssid[] = "SSID";       // your network SSID (name)
char password[] = "password";  // your network key

// Website we are connecting to
#define HOST_WEBSITE "teamtrees.org"

void IRAM_ATTR display_updater() {
  display.display(display_draw_time);
}


void display_update_enable(bool is_enable)
{
  if (is_enable)
  {
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 2000, true);
    timerAlarmEnable(timer);
  }
  else
  {
    timerDetachInterrupt(timer);
    timerAlarmDisable(timer);
  }
}

// Will center the given text
void displayText(String text, int yPos) {
  int16_t  x1, y1;
  uint16_t w, h;
  display.setTextSize(2);
  char charBuf[text.length() + 1];
  text.toCharArray(charBuf, text.length() + 1);
  display.setTextSize(1);
  display.getTextBounds(charBuf, 0, yPos, &x1, &y1, &w, &h);
  int startingX = 33 - (w / 2);
  display.setTextSize(1);
  display.setCursor(startingX, yPos);
  Serial.println(startingX);
  Serial.println(yPos);
  display.print(text);
}

void drawStuff()
{
  // Not clearing the display and redrawing it when you
  // dont need to improves how the refresh rate appears
  if (!finishedAnimating) {

    // Step 1: Clear the display
    display.clearDisplay();

    // Step 2: draw tetris
    if (tetris.drawText(1, 30)) {
      if (tetrisTwo.drawText(4, 16)) {
        finishedAnimatingTwo = true;
      }
    }

    // Step 4: Display the buffer
    display.showBuffer();
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to Wifi before displaying to the screen

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);

  /* Explicitly set the ESP32 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  // Define your display layout here, e.g. 1/8 step
  display.begin(16);

  // Define your scan pattern here {LINE, ZIGZAG, ZAGGIZ, WZAGZIG, VZAG} (default is LINE)
  //display.setScanPattern(LINE);

  // Define multiplex implemention here {BINARY, STRAIGHT} (default is BINARY)
  //display.setMuxPattern(BINARY);

  display.setFastUpdate(true);
  display.clearDisplay();
  display_update_enable(true);
  display.showBuffer();
  int tree = getTreeCount();
  if (tree > 0) {
    String numberstring = String(tree);
    tetrisTwo.setText(numberstring);
  }
  tetris.setText("TEAMTREES");
  animation_ticker.attach(0.10, drawStuff);
}

String getCommas(String number) {
  int commaCount = (number.length() - 1) / 3;
  String numberWithCommas = "";
  numberWithCommas.reserve(50);
  int commaOffsetIndex = 0;
  for (int j = 0; j < commaCount; j++) {
    commaOffsetIndex = number.length() - 3;
    numberWithCommas = "," + number.substring(commaOffsetIndex) + numberWithCommas;
    number.remove(commaOffsetIndex);
  }

  numberWithCommas = number + numberWithCommas;

  return numberWithCommas;
}

bool timerSet = false;

int getTreeCount() {

  if (client.connected())
  {
    client.stop();
  }

  int treeCount = 0;

  // Opening connection to server (Use 80 as port if HTTP)
  if (!client.connect(HOST_WEBSITE, 443))
  {
    Serial.println(F("Connection failed"));
    return 0;
  }

  // give the esp a breather
  yield();

  // Send HTTP request
  client.print(F("GET "));
  // This is the second half of a request (everything that comes after the base URL)
  client.print("/");
  client.println(F(" HTTP/1.1"));

  //Headers
  client.print(F("Host: "));
  client.println(HOST_WEBSITE);

  client.println(F("Cache-Control: no-cache"));

  if (client.println() == 0)
  {
    Serial.println(F("Failed to send request"));
    return 0;
  }
  //delay(100);
  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0)
  {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return 0;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders))
  {
    Serial.println(F("Invalid response"));
    return 0;
  }

  char treeData[32] = {0};

  // The count for the Tree is a property of the Div that eventually displays the count.
  // the innerHTML of the div starts as 0

  // Skip to the text that comes after "data-count=\""
  char treeString[] = "data-count=\"";
  if (!client.find(treeString))
  {
    Serial.println(F("Found no trees"));
    return 0;
  } else {
    // copy the data from the stream til the next "
    // thats out tree data
    client.readBytesUntil('\"', treeData, sizeof(treeData));
    Serial.print("#TeamTrees:");
    Serial.println(treeData);

    sscanf(treeData, "%d", &treeCount);

  }

  return treeCount;
}

void loop() {

  if (finishedAnimatingTwo && !timerSet)
  {
    apiRequestDue = millis() + timeBetweenRequests;
    timerSet = true;
  }

  if (finishedAnimatingTwo && millis() > apiRequestDue) {
    int tree = getTreeCount();
    if (tree > 0) {
      String numberstring = String(tree);
      tetrisTwo.setText(numberstring);
      finishedAnimating = false;
      finishedAnimatingTwo = false;
    }
    timerSet = false;
  }
}
