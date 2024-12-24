
#include <SPIFFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

String getServerVersion();

// Wi-Fi credentials
// const char *ssid = "TESTING 2.4";
// const char *password = "Virus1234";
const char *ssid = "Nadim";
const char *password = "zzzzxxxx";

// Firmware URL
const char *firmwareURL = "http://192.168.60.192:3000/firmware.bin";
// Version file URL
const char *versionURL = "http://192.168.60.192:3000/version.txt"; // Replace with your server version file URL

// String updatedVersion = "";
String currentVersion = "1.0.0";

void checkForOTAUpdate();
String readVersionFromSPIFFS();
void saveVersionToSPIFFS(String version);

void setup()
{
  Serial.begin(115200);
  pinMode(32, OUTPUT);
  delay(2000);

  // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  else
  {
    Serial.println("SPIFFS Mounted Successfully");
  }
  if (!SPIFFS.exists("/version.txt"))
  {
    Serial.println("Version file does not exist. Creating one...");
    saveVersionToSPIFFS("1.0.0");
  }

  // Read the version from SPIFFS
  currentVersion = readVersionFromSPIFFS();
  Serial.print("Current Version: ");
  Serial.println(currentVersion);

  // Perform OTA update if a new version is available
  checkForOTAUpdate();
}

void loop()
{
  digitalWrite(32, HIGH); // Turn the LED on
  delay(100);            // Wait for 1 second
  digitalWrite(32, LOW);  // Turn the LED off
  delay(100);

  // Periodically check for OTA updates
  // checkForOTAUpdate();
  // delay(15000);  // Check every 15 seconds
}

void checkForOTAUpdate()
{
  Serial.println("Checking for OTA update...");

  if (WiFi.status() == WL_CONNECTED)
  {
    // Check the version from the server
    String serverVersion = getServerVersion();

    Serial.print("\ncurrentVersion:: ");
    Serial.println(currentVersion);
    Serial.print("serverVersion :: ");
    Serial.println(serverVersion);

    currentVersion = currentVersion.substring(0, 5);
    serverVersion = serverVersion.substring(0, 5);

    int i1 = currentVersion.length();
    int i2 = serverVersion.length();

    Serial.println("Size of currentVersion):: " + String(i1));
    Serial.println("Size of serverVersion) :: " + String(i2));

    // for (int i = 0; i < currentVersion.length(); i++) {
    //   char c = currentVersion[i];  // Get each character from the string
    //   Serial.print("Character ");
    //   Serial.print(i);
    //   Serial.print(": ");
    //   Serial.println(c);  // Print the character
    // }
    // for (int i = 0; i < serverVersion.length(); i++) {
    //   char c = serverVersion[i];  // Get each character from the string
    //   Serial.print("Character ");
    //   Serial.print(i);
    //   Serial.print(": ");
    //   Serial.println(c);  // Print the character
    // }

    if (currentVersion != serverVersion)
    {

      // Write the firmware version to SPIFFS
      saveVersionToSPIFFS(serverVersion); // Example version

      Serial.println("New version available! Updating...");

      // Perform the OTA update
      HTTPClient http;
      http.begin(firmwareURL); // Start the request for the firmware
      int httpCode = http.GET();

      if (httpCode == HTTP_CODE_OK)
      {
        int contentLength = http.getSize();

        if (contentLength > 0)
        {
          bool canBegin = Update.begin(contentLength);

          if (canBegin)
          {
            Serial.println("Starting OTA update...");
            WiFiClient &client = http.getStream();

            size_t written = 0;
            int lastProgress = -1; // Initialize the last progress value

            while (written < contentLength)
            {
              size_t chunkSize = Update.writeStream(client);
              written += chunkSize;

              int progress = (written * 100) / contentLength;
              delay(300);

              if (progress != lastProgress)
              {
                Serial.printf("Download Progress: %d%%\n", progress);
                lastProgress = progress;
              }
            }

            if (written == contentLength)
            {
              Serial.println("OTA update complete!");
            }
            else
            {
              Serial.printf("OTA update failed. Written: %d/%d bytes\n", written, contentLength);
            }

            if (Update.end())
            {
              if (Update.isFinished())
              {
                Serial.println("Update successfully completed. Restarting...");
                ESP.restart();
              }
              else
              {
                Serial.println("Update not finished. Something went wrong.");
              }
            }
            else
            {
              Serial.printf("Update failed. Error #%d\n", Update.getError());
            }
          }
          else
          {
            Serial.println("Not enough space to begin OTA.");
          }
        }
        else
        {
          Serial.println("Content length is invalid or zero.");
        }
      }
      else
      {
        Serial.printf("Failed to fetch firmware. HTTP code: %d\n", httpCode);
      }

      http.end();
    }
    else
    {
      Serial.println("No update needed. Current version is up-to-date.");
    }
  }
  else
  {
    Serial.println("Wi-Fi not connected.");
  }
}

String getServerVersion()
{
  HTTPClient http;
  String serverVersion = "";

  http.begin(versionURL); // Start the request for the version file
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    serverVersion = http.getString();

    // Ensure the string is null-terminated (though it typically is)
    serverVersion.trim();  // Removes any trailing newline or space characters
    serverVersion += '\0'; // Ensure null termination

    Serial.printf("Server version: %s\n", serverVersion.c_str());
  }
  else
  {
    Serial.printf("Failed to fetch version. HTTP code: %d\n", httpCode);
  }

  http.end();
  return serverVersion;
}

void saveVersionToSPIFFS(String version)
{
  File versionFile = SPIFFS.open("/version.txt", FILE_WRITE);

  if (!versionFile)
  {
    Serial.println("Failed to open version file for writing");
    return;
  }

  // version.trim();   // Removes any trailing newline or space characters
  // version += '\0';  // Ensure null termination

  versionFile.println(version); // Write the version string
  versionFile.close();
  Serial.println("Version saved to SPIFFS");
}

String readVersionFromSPIFFS()
{
  File versionFile = SPIFFS.open("/version.txt", FILE_READ);

  if (!versionFile)
  {
    Serial.println("Failed to open version file for reading");
    return "";
  }

  String version = versionFile.readString(); // Read the content
  versionFile.close();

  // Ensure the string is null-terminated (though it typically is)
  version.trim();  // Removes any trailing newline or space characters
  version += '\0'; // Ensure null termination
  return version;
}
