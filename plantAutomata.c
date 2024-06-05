#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <mcp3004.h>
#include <string.h>
#include "../DHT11/DHT11.h"
#include "../i2c_lcd/i2c_lcd.h"
#include <time.h>
#include <jansson.h>

#define MCP3004_BASE 100
#define SPI_CHAN 0 // SPICE0: chip select gpio port
#define AD_CHAN 0  // chan-0 on AD

#define DHTPIN 29
#define BUTTON_PIN 28
#define PUMP_PIN 27

// Define some device parameters
#define I2C_ADDR 0x27 // I2C device address
#define LINE1 0x80    // 1st line
#define LINE2 0xC0    // 2nd line

#define POWER_SAVE_TIME 10 // 10x2=20 seconds to power save mode
#define WATER_COOLDOWN 10  // 1 in x times to water

typedef struct PlantData
{
    int *dht_data;
    float soil_moisture;
    float light_level;
    float pump_threshhold;

} PlantData;

float senseLightLevel(int mcp_base, int ad_channel)
{
    float value = analogRead(mcp_base + ad_channel);
    float light = value / 1023 * 100;
    return light;
}

float senseSoilMoisture(int mcp_base, int ad_channel)
{
    float value = analogRead(mcp_base + ad_channel);
    float moisture = (1 - value / 1023) * 100;

    return moisture;
}

float sensePumpThreshhold(int mcp_base, int ad_channel)
{
    float value = analogRead(mcp_base + ad_channel);
    float thresh = value / 1023 * 100;
    return thresh;
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
    case 0: // Soil and Temperature
        lcdLoc(fd, LINE1);
        typeln(fd, "Soil: ");
        typeFloat(fd, plantData->soil_moisture);
        typeln(fd, "%");
        if (plantData->dht_data != NULL)
        {
            lcdLoc(fd, LINE2);
            typeln(fd, "TMP: ");
            typeInt(fd, plantData->dht_data[2]);
            typeln(fd, ".");
            typeInt(fd, plantData->dht_data[3]);
            typeln(fd, "C");
        }
        break;
    case 1: // Temperature and Humidity
        if (plantData->dht_data != NULL)
        {
            lcdLoc(fd, LINE1);
            typeln(fd, "TMP: ");
            typeInt(fd, plantData->dht_data[2]);
            typeln(fd, ".");
            typeInt(fd, plantData->dht_data[3]);
            typeln(fd, "C");

            lcdLoc(fd, LINE2);
            typeln(fd, "HMT: ");
            typeInt(fd, plantData->dht_data[0]);
            typeln(fd, ".");
            typeInt(fd, plantData->dht_data[1]);
            typeln(fd, "%");
        }
        break;
    case 2: // Humidity and light
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
    case 3: // light and pump threshhold
        lcdLoc(fd, LINE1);
        typeln(fd, "Light: ");
        typeFloat(fd, plantData->light_level);
        typeln(fd, "%");

        lcdLoc(fd, LINE2);
        typeln(fd, "Pump: ");
        typeFloat(fd, plantData->pump_threshhold);
        typeln(fd, "%");
        break;

    case 4: // pump threshhold and soil
        lcdLoc(fd, LINE1);
        typeln(fd, "Pump: ");
        typeFloat(fd, plantData->pump_threshhold);
        typeln(fd, "%");

        lcdLoc(fd, LINE2);
        typeln(fd, "Soil: ");
        typeFloat(fd, plantData->soil_moisture);
        typeln(fd, "%");
        break;
    }
}

void water()
{
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, HIGH);
    delay(2000);
    digitalWrite(PUMP_PIN, LOW);
    pinMode(PUMP_PIN, INPUT);
}

int main()
{
    int fd = wiringPiI2CSetup(I2C_ADDR);
    int menu = 0;
    int powerSaveTimer = 0;
    int powerSaveFlag = 0;
    int waterCounter = 0;
    PlantData *myPlantData = malloc(sizeof(PlantData));
    // FILE *dataFile = fopen("plantData.json", "a");
    // if (dataFile == NULL)
    // {
    //     printf("Error opening file\n");
    // }
    json_t *array = json_array();

    wiringPiSetup();
    mcp3004Setup(MCP3004_BASE, SPI_CHAN);
    lcd_init(fd);

    pinMode(BUTTON_PIN, INPUT);

    // printf("printing...");

    lcdLoc(fd, LINE1);
    typeln(fd, "Plant Automota");
    lcdLoc(fd, LINE2);
    typeln(fd, "by: Maker");

    delay(1500);
    ClrLcd(fd);

    while (1)
    {
        if (millis() % 2000 == 0)
        {
            // printf("hello");
            myPlantData->soil_moisture = senseSoilMoisture(MCP3004_BASE, AD_CHAN); // get da data
            // printf("world");
            myPlantData->dht_data = readDHT(DHTPIN);
            myPlantData->light_level = senseLightLevel(MCP3004_BASE, 1);
            myPlantData->pump_threshhold = sensePumpThreshhold(MCP3004_BASE, 2);

            if (powerSaveTimer <= POWER_SAVE_TIME) // no need to display the data if its in power-save mode
            {
                displayPlantData(fd, myPlantData, menu);
                powerSaveFlag = 0;
                powerSaveTimer++;
            }
            else if (!powerSaveFlag) // if the powersave time-limit is reached clear lcd and turnr off the backlight
            {
                ClrLcd(fd);
                switchBacklight(fd, 0);
                powerSaveFlag = 1;
            }

            time_t now = time(NULL);
            char buffer[20]; // Buffer to hold the date/time string
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));

            if (myPlantData->dht_data != NULL)
            {
                // fprintf(dataFile, "\n{\n\"date\": \"%s\",\n\"time\": \"%s\",\n\"temp\": \"%d.%d\",\n\"light\": \"%f\",\n\"moisture\": \"%f\",\n\"humidity\": \"%d.%d\"\n},",
                //         buffer, buffer + 11, myPlantData->dht_data[2], myPlantData->dht_data[3], myPlantData->light_level, myPlantData->soil_moisture, myPlantData->dht_data[0], myPlantData->dht_data[1]);
                // // fprintf(dataFile, "%s %d.%d %f %f %d.%d %f\n", buffer, myPlantData->dht_data[2], myPlantData->dht_data[3], myPlantData->light_level, myPlantData->soil_moisture, myPlantData->dht_data[0], myPlantData->dht_data[1], myPlantData->pump_threshhold);
                // fflush(dataFile);

                json_t *new_data = json_object();
                json_object_set_new(new_data, "date", json_string(buffer));
                json_object_set_new(new_data, "time", json_string(buffer+11));
                json_object_set_new(new_data, "temp", json_real(myPlantData->dht_data[0]));
                json_object_set_new(new_data, "light", json_real(myPlantData->light_level));
                json_object_set_new(new_data, "moisture", json_real(myPlantData->soil_moisture));
                json_object_set_new(new_data, "humidity", json_real(myPlantData->dht_data[0]));

                json_array_append_new(array, new_data);

                json_dump_file(array, "./plantData.json", 0);

                
            }

            if (myPlantData->soil_moisture < 50.00)
            {
                if (waterCounter == 9)
                {
                    ClrLcd(fd);
                    lcdLoc(fd, LINE1);
                    typeln(fd, "Watering...");
                    water();
                }
                waterCounter = (waterCounter + 1) % WATER_COOLDOWN; // loop from 0-9
            }
            else
            {
                waterCounter = 0;
            }
            printf("Soil Moisture:\t%.1f%%\n", myPlantData->soil_moisture);
            printDHTData(myPlantData->dht_data);
            printf("Light:\t\t%.1f%%\n", myPlantData->light_level);
            printf("Pump:\t\t%.1f%%\n", myPlantData->pump_threshhold);
            printf("Menu: %d\n", menu);
            printf("Water in %d\n", 10 - waterCounter);
            printf("------------------------\n");
        }

        if (digitalRead(BUTTON_PIN) == HIGH) // if button pushed
        {
            powerSaveTimer = 0; // reset powersave time
            if (powerSaveFlag)
            { // if its already in power save mode, just switch on the light
                switchBacklight(fd, 1);
            }
            else
            {                          // otherwise switch through menus
                menu = (menu + 1) % 5; // loop from 0-4
            }

            ClrLcd(fd);                              // clear lcd for next set of data
            displayPlantData(fd, myPlantData, menu); // display next set of data

            delay(200); // delay to avoid button-bounce
        }
    }

    return 0;
}
