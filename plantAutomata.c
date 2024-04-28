#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <mcp3004.h>
#include <string.h>
#include "../DHT11/DHT11.h"
#include "../i2c_lcd/i2c_lcd.h"

#define MCP3004_BASE 100 //virtuelle channel-vergabe für den AD-Wandler fängt bei 100 an zu zählen
#define SPI_CHAN 0	//SPICE0: chip select gpio port
#define AD_CHAN 0	//chan-0 on AD

#define DHTPIN 29

// Define some device parameters
#define I2C_ADDR   0x27 // I2C device address

#define LINE1  0x80 // 1st line
#define LINE2  0xC0 // 2nd line

#define BUTTON_PIN 28


double senseLightLevel(int mcp_base, int ad_channel){
    double value = analogRead(mcp_base + ad_channel);
    double light = value/1023*100;
    return light;
}

float* senseGroundMoisture(int mcp_base, int ad_channel){
    float value = analogRead(mcp_base + ad_channel);
    float* moisture = malloc(sizeof(int));
    *moisture = (1-value/1023)*100;

    return moisture;
}

void printDHTData(int* dht_data){
    if(dht_data == NULL){
            printf("Checksum Error\n");
        }
        else{
            printf("Humidity:\t%d.%d%%\nTemperature:\t%d.%dC\n", dht_data[0], dht_data[1], dht_data[2], dht_data[3]); 
        }
}

int main()
{
    wiringPiSetup();
    mcp3004Setup(MCP3004_BASE, SPI_CHAN);
    int fd = wiringPiI2CSetup(I2C_ADDR);
    lcd_init(fd);
    
    pinMode(BUTTON_PIN, INPUT);

    lcdLoc(fd, LINE1);
    typeln(fd, "Plant Automota");
    lcdLoc(fd, LINE2);
    typeln(fd, "by: Maker");

    delay(2500);
    ClrLcd(fd);
    switchBacklight(fd, 0);

    for(int i = 0 ; i < 100 ; i++){
        delay(2000); //delay 2 sec for DHT "cooldown"
        switchBacklight(fd, 1);
        // if(digitalRead(BUTTON_PIN) == HIGH){
        //     printf("pressed\n");
        // }

        float* moisture = senseGroundMoisture(MCP3004_BASE, AD_CHAN);
        int* dht_data = readDHT(DHTPIN);

        printf("Dirt Moisture:\t%.2f%%\n", *moisture); //im just gonna pull some numbers out my ass and say above 70% is the optimal watering level
        lcdLoc(fd, LINE1);
        typeln(fd, "Water: ");
        typeFloat(fd, *moisture);
        typeln(fd, "%");

        printDHTData(dht_data);
        if(dht_data != NULL){
            lcdLoc(fd, LINE2);
            typeln(fd, "HMT: ");
            typeInt(fd, dht_data[0]);
            typeln(fd, ".");
            typeInt(fd, dht_data[1]);
            typeln(fd, "%");
        }
        
        double lightVal = senseLightLevel(MCP3004_BASE, 1);
        printf("Light:\t\t%.2f%%\n", lightVal);

        printf("------------------------\n");
    }
    return 0;
}
