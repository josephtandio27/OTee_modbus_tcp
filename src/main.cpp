// #include <Arduino.h>
#include <ETH.h>
#include <ModbusIP_ESP8266.h>

ModbusIP mb;

// A Union allows you to access the same 4 bytes of memory
// as either 1 float OR 2 uint16_t registers.
union FloatConverter
{
  float f_val;
  uint16_t reg_val[2];
};

void setup()
{
  Serial.begin(115200);

  Serial.println("Connecting to Ethernet...");
  ETH.begin(); // Initialize Ethernet

  // Wait for the IP address to be assigned by DHCP
  int timeout = 0;
  while (ETH.localIP().toString() == "0.0.0.0" && timeout < 20)
  {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (ETH.localIP().toString() != "0.0.0.0")
  {
    Serial.println("\nEthernet Connected!");
    Serial.print("IP Address: ");
    Serial.println(ETH.localIP()); // <--- This prints the IP
  }
  else
  {
    Serial.println("\nDHCP Failed. Check cable.");
  }

  mb.server();
  mb.addHreg(100);
  // Register 200 & 201: For receiving a 32-bit Float (Motor Speed) from PLC
  // We add two registers starting at 200
  mb.addHreg(200, 0, 2);
}

void loop()
{
  mb.task(); // Handle Modbus requests

  // Example: Write a sensor value to register 100 for the PLC to read
  uint16_t sensorVal = random(0, 100); // Simulate a sensor value
  mb.Hreg(100, sensorVal);

  // Example: Read a 32-bit float (Motor Speed) from registers 200 & 201
  FloatConverter converter;
  // 1. Grab the two 16-bit registers the PLC wrote to
  converter.reg_val[0] = mb.Hreg(200);
  converter.reg_val[1] = mb.Hreg(201);
  // 2. Now 'f_val' automatically contains the reconstructed float
  float motorSpeed = converter.f_val;

  // Print it so you can verify in the Serial Monitor
  if (motorSpeed > 0) { // Only print if there's a value to avoid spam
      Serial.print("Target Motor Speed from PLC: ");
      Serial.println(motorSpeed);
  }

  delay(100);
}
