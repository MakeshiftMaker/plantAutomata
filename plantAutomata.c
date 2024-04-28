#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <mcp3004.h>
#include <string.h>
#include "../DHT11/DHT11.h"
#include "../i2c_lcd/i2c_lcd.h"

#define MCP3004_BASE 100 // virtuelle channel-vergabe für den AD-Wandler fängt bei 100 an zu zählen
#define SPI_CHAN 0       // SPICE0: chip select gpio port
#define AD_CHAN 0        // chan-0 on AD

#define DHTPIN 29

// Define some device parameters
#define I2C_ADDR 0x27 // I2C device address

#define LINE1 0x80 // 1st line
#define LINE2 0xC0 // 2nd line

#define BUTTON_PIN 28

typedef struct PlantData
{
    int *dht_data;
    float ground_moisture;
    float light_level;

} PlantData;

float senseLightLevel(int mcp_base, int ad_channel)
{
    float value = analogRead(mcp_base + ad_channel);
    float light = value / 1023 * 100;
    return light;
}

float senseGroundMoisture(int mcp_base, int ad_channel)
{
    float value = analogRead(mcp_base + ad_channel);
    float moisture = (1 - value / 1023) * 100;

    return moisture;
}

void printDHTData(int *dht_data)
{
    if (dht_data == NULL)
    {
        printf("Checksum Error\n");
    }
    else
    {
        printf("Humidity:\t%d.%d%%\nTemperature:\t%d.%dC\n", dht_data[0], dht_data[1], dht_data[2], dht_data[3]);
    }
}

void displayPlantData(int fd, PlantData *plantData, int menu)
{
    switch (menu)
    {
        case 0: //Soil and Temperature
            lcdLoc(fd, LINE1);
            typeln(fd, "Soil: ");
            typeFloat(fd, plantData->ground_moisture);
            typeln(fd, "%");
            if (plantData->dht_data != NULL)
            {
                lcdLoc(fd, LINE2);
                typeln(fd, "TMP: ");
                typeInt(fd, plantData->dht_data[2]);
                typeln(fd, ".");
                typeInt(fd, plantData->dht_data[3]);
                typeln(fd, "%");
            }
            break;
        case 1: //Temperature and Humidity
            if (plantData->dht_data != NULL)
            {
                lcdLoc(fd, LINE1);
                typeln(fd, "TMP: ");
                typeInt(fd, plantData->dht_data[2]);
                typeln(fd, ".");
                typeInt(fd, plantData->dht_data[3]);
                typeln(fd, "%");

                lcdLoc(fd, LINE2);
                typeln(fd, "HMT: ");
                typeInt(fd, plantData->dht_data[0]);
                typeln(fd, ".");
                typeInt(fd, plantData->dht_data[1]);
                typeln(fd, "%");
            }
            break;
        case 2: //Humidity and light
            if (plantData->dht_data != NULL)
            {
                lcdLoc(fd, LINE1);
                typeln(fd, "HMT: ");
                typeInt(fd, plantData->dht_data[0]);
                typeln(fd, ".");
                typeInt(fd, plantData->dht_data[1]);
                typeln(fd, "%");
            }
            lcdLoc(fd, LINE2);
            typeln(fd, "Light: ");
            typeFloat(fd, plantData->light_level);
            typeln(fd, "%");
            break;
        default:
            printf("Uh Oh\n");
            break;
    }

    printDHTData(plantData->dht_data);
    printf("Dirt Moisture:\t%.1f%%\n", plantData->ground_moisture); // im just gonna pull some numbers out my ass and say above 70% is the optimal watering level
    printf("Light:\t\t%.1f%%\n", plantData->light_level);
    printf("Menu: %d\n", menu);
    printf("------------------------\n");
}

int main()
{
    wiringPiSetup();
    mcp3004Setup(MCP3004_BASE, SPI_CHAN);

    int fd = wiringPiI2CSetup(I2C_ADDR);
    int flag = 0;
    int menu = 0;
    PlantData *myPlantData = malloc(sizeof(PlantData));
    lcd_init(fd);

    pinMode(BUTTON_PIN, INPUT);

    lcdLoc(fd, LINE1);
    typeln(fd, "Plant Automota");
    lcdLoc(fd, LINE2);
    typeln(fd, "by: Maker");

    delay(2500);
    ClrLcd(fd);
    while (1)
    {
        if (millis() % 2000 == 0)
        {
            myPlantData->ground_moisture = senseGroundMoisture(MCP3004_BASE, AD_CHAN);
            myPlantData->dht_data = readDHT(DHTPIN);
            myPlantData->light_level = senseLightLevel(MCP3004_BASE, 1);

            displayPlantData(fd, myPlantData, menu);
        }
        if (digitalRead(BUTTON_PIN) == HIGH)
        {
            if (!flag)
            {
                menu++;
                if(menu > 2){
                    menu = 0;
                }
                ClrLcd(fd);
                displayPlantData(fd, myPlantData, menu);
                flag = 1;
                delay(200);
            }
        }
        else
        {
            flag = 0;
        }
    }
    return 0;
}
