// Import WiFi and ESPSupabase Library
#include <WiFi.h>
#include <ESPSupabase.h>

// Add you Wi-Fi credentials
const char* ssid = "MiFibra-EE24";
const char* password = "fFpiEq2R";

// Supabase credentials
const char* supabaseUrl = "https://kduoudmniqjtartllozn.supabase.co";
const char* supabaseKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImtkdW91ZG1uaXFqdGFydGxsb3puIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDM5MzUwNzcsImV4cCI6MjA1OTUxMTA3N30.T1mpTXZaGF8AIzvQ96jeQh95Vo4cfLCiWh0QSH0Ea84";

Supabase supabase;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }
  Serial.println("Wi-Fi connected!");

  // Init Supabase
  supabase.begin(supabaseUrl, supabaseKey);

  // Add the table name here
  String tableName = "locations";
  // change the correct columns names you create in your table
  String jsonData = "{\"name\": \"Madrid\"}";

  // sending data to supabase
  int response = supabase.insert(tableName, jsonData, false);
  if (response == 201) {
    Serial.println("Data inserted successfully!");
  } else {
    Serial.print("Failed to insert data. HTTP response: ");
    Serial.println(response);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
