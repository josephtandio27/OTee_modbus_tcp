#include <SPI.h>
#include <Ethernet.h>
#include <ModbusServerEthernet.h>

// W5500 Pins
#define W5500_CS 14
#define W5500_RST 9
#define W5500_INT 10
#define W5500_MISO 12
#define W5500_MOSI 11
#define W5500_SCK 13

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// The eModbus server for Ethernet
ModbusServerEthernet mbServer;

// Internal Storage (Registers 100 to 210)
uint16_t registers[111]; // Index 0 = Reg 100, Index 100 = Reg 200

union FloatConverter
{
    float f_val;
    uint16_t reg_val[2];
};

// Callback function to handle requests
ModbusMessage handleData(ModbusMessage request)
{
    uint8_t fc = request.getFunctionCode();
    uint16_t addr, count;
    request.get(2, addr, count);

    // Offset address: if PLC asks for 100, we use index 0
    uint16_t offset = addr - 100;

    // READ Holding Registers (0x03)
    if (fc == READ_HOLD_REGISTER)
    {
        if (addr >= 100 && (offset + count) <= 111)
        {
            ModbusMessage response;
            response.add(request.getServerID(), fc, (uint8_t)(count * 2));
            for (int i = 0; i < count; i++)
            {
                response.add(registers[offset + i]);
            }
            return response;
        }
    }

    // WRITE Single (0x06) or Multiple (0x10 / 16 decimal)
    if (fc == WRITE_HOLD_REGISTER || fc == WRITE_MULT_REGISTERS)
    {
        if (addr >= 100 && (offset + count) <= 111)
        {
            if (fc == WRITE_HOLD_REGISTER)
            {
                request.get(4, registers[offset]);
            }
            else
            {
                // For Multiple Registers, data starts after the byte count (at index 7)
                for (int i = 0; i < count; i++)
                {
                    request.get(7 + (i * 2), registers[offset + i]);
                }
            }
            // Return the standard echo response for writes
            return request;
        }
    }

    // If we get here, the address was wrong
    return ModbusMessage(request.getServerID(), fc + 0x80, ILLEGAL_DATA_ADDRESS);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {
        delay(50); 
    }
    Serial.println("\n\nBOOTING...");
    Serial.println("\n--- Modbus Server Starting ---");

    // Hardware Reset for W5500
    pinMode(W5500_RST, OUTPUT);
    digitalWrite(W5500_RST, LOW);
    delay(100);
    digitalWrite(W5500_RST, HIGH);
    delay(500); // Give it time to wake up

    // Hardware Setup
    SPI.begin(W5500_SCK, W5500_MISO, W5500_MOSI, W5500_CS);
    Ethernet.init(W5500_CS);

    if (Ethernet.begin(mac) == 0)
    {
        Serial.println("Ethernet DHCP Failed");
        while (1)
            ;
    }
    Serial.print("IP Address: ");
    Serial.println(Ethernet.localIP());

    // Define workers for FC 03, 06, and 16 (0x10)
    mbServer.registerWorker(1, READ_HOLD_REGISTER, &handleData);
    mbServer.registerWorker(1, WRITE_HOLD_REGISTER, &handleData);
    mbServer.registerWorker(1, WRITE_MULT_REGISTERS, &handleData);

    // Start server on port 502
    mbServer.start(502, 4, 2000, 0);
}

void loop()
{
    // Update Register 100 with a dummy sensor value
    registers[0] = (uint16_t)random(0, 500);

    // Reconstruct Float from Registers 200 & 201 (indices 100 & 101)
    FloatConverter converter;
    converter.reg_val[0] = registers[100];
    converter.reg_val[1] = registers[101];

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 2000)
    {
        Serial.print("Sensor (Reg 100): ");
        Serial.print(registers[0]);
        Serial.print(" | Motor Speed (Reg 200-201): ");
        Serial.println(converter.f_val);
        lastPrint = millis();
    }
}