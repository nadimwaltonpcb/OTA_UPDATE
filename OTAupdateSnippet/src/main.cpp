
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
const char *firmwareURL = "http://192.168.120.192:3000/firmware.bin";
// Version file URL
const char *versionURL = "http://192.168.120.192:3000/version.txt"; // Replace with your server version file URL

// String updatedVersion = "";
String currentVersion = "1.0.0";

void checkForOTAUpdate();
String readVersionFromSPIFFS();
void saveVersionToSPIFFS(String version);
void printProgressBar(int progress);

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
  delay(1000);             // Wait for 1 second
  digitalWrite(32, LOW);  // Turn the LED off
  delay(1000);
}

void printProgressBar(int progress)
{
  const int barWidth = 50; // Width of the progress bar
  String bar = "";

  // Calculate the filled width of the progress bar
  int filledWidth = (progress * barWidth) / 100;

  for (int i = 0; i < barWidth; i++)
  {
    bar += (i < filledWidth) ? '=' : ' ';
  }

  // Display the progress bar
  Serial.printf("\rDownload Progress: [%s] %3d%%", bar.c_str(), progress);
  Serial.flush(); // Ensure output is sent immediately
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

    if (currentVersion != serverVersion)
    {
      // Write the firmware version to SPIFFS
      saveVersionToSPIFFS(serverVersion); // Example version
      Serial.println("New version available! Starting update...");

      HTTPClient http;
      http.begin(firmwareURL);
      int httpCode = http.GET();

      if (httpCode == HTTP_CODE_OK)
      {
        int contentLength = http.getSize();
        WiFiClient *stream = http.getStreamPtr();

        if (contentLength > 0)
        {
          if (Update.begin(contentLength))
          {
            Serial.println("Beginning OTA update...");
            size_t written = 0;
            int lastProgress = -1;

            while (written < contentLength)
            {
              // Read data from the HTTP stream
              uint8_t buffer[128];
              size_t bytesRead = stream->readBytes(buffer, sizeof(buffer));

              if (bytesRead > 0)
              {
                // Write data to the update stream
                written += Update.write(buffer, bytesRead);

                // Calculate progress
                int progress = (written * 100) / contentLength;

                // Update progress bar if progress changes
                if (progress != lastProgress)
                {
                  printProgressBar(progress);
                  lastProgress = progress;
                }
              }
              else
              {
                Serial.println("\nError: No data received. Update aborted.");
                break;
              }
            }

            Serial.println(); // Move to the next line after the progress bar

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
          Serial.println("Invalid content length or zero.");
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
