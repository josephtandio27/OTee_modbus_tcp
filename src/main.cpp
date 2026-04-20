// #include <Arduino.h>
#include <ETH.h>
#include <ModbusIP_ESP8266.h>

ModbusIP mb;

void setup()
{
  Serial.begin(115200);
  
  Serial.println("Connecting to Ethernet...");
  ETH.begin(); // Initialize Ethernet

  // Wait for the IP address to be assigned by DHCP
  int timeout = 0;
  while (ETH.localIP().toString() == "0.0.0.0" && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (ETH.localIP().toString() != "0.0.0.0") {
    Serial.println("\nEthernet Connected!");
    Serial.print("IP Address: ");
    Serial.println(ETH.localIP()); // <--- This prints the IP
  } else {
    Serial.println("\nDHCP Failed. Check cable.");
  }

  mb.server();     
  mb.addHreg(100);
}

void loop()
{
  mb.task(); // Handle Modbus requests

  // Example: Write a sensor value to register 100 for the PLC to read
  uint16_t sensorVal = random(0, 100); // Simulate a sensor value
  mb.Hreg(100, sensorVal);

  delay(100);
}
