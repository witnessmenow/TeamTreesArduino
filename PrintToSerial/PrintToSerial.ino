/*******************************************************************
    Reading the live #TeamTrees count directly from the website
    and displaying it on the serial monitor using an ESP8266

    Updates every minute

    Parts:
    D1 Mini ESP8266 * - http://s.click.aliexpress.com/e/uzFUnIe

 *  * = Affilate

    If you find what I do usefuland would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/


    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/


// ----------------------------
// Standard Libraries
// ----------------------------

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

//------- Replace the following! ------
char ssid[] = "ssid";       // your network SSID (name)
char password[] = "password";  // your network key

// For Non-HTTPS requests
// WiFiClient client;

// For HTTPS requests
WiFiClientSecure client;


// Just the base of the URL you want to connect to
#define TEST_HOST "teamtrees.org"

unsigned long requestDue = 0;
int delayBetweenRequests = 60 * 1000; // 60 seconds


void setup() {

  Serial.begin(115200);

  // Connect to the WiFI
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
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

  //--------

  // We don't need to check the fingerprint
  client.setInsecure();
}

void makeHTTPRequest() {

  // Opening connection to server (Use 80 as port if HTTP)
  if (!client.connect(TEST_HOST, 443))
  {
    Serial.println(F("Connection failed"));
    return;
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
  client.println(TEST_HOST);

  client.println(F("Cache-Control: no-cache"));

  if (client.println() == 0)
  {
    Serial.println(F("Failed to send request"));
    return;
  }
  //delay(100);
  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0)
  {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders))
  {
    Serial.println(F("Invalid response"));
    return;
  }

  char treeData[32] = {0};

  // The count for the Tree is a property of the Div that eventually displays the count.
  // the innerHTML of the div starts as 0

  // Skip to the text that comes after "data-count=\""
  char treeString[] = "data-count=\"";
  if (!client.find(treeString))
  {
    Serial.println(F("Found no trees"));
    return;
  } else {
    // copy the data from the stream til the next "
    // thats out tree data
    client.readBytesUntil('\"', treeData, sizeof(treeData));
    Serial.print("#TeamTrees:");
    Serial.println(treeData);
    
  }
}

void loop() {
  
  unsigned long now = millis();
  if(requestDue < now){
    makeHTTPRequest();
    requestDue = now + delayBetweenRequests;
  }
}
