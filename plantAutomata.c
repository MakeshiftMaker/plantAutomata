#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <mcp3004.h>
#include <string.h>
#include "../DHT11/DHT11.h"

#define MCP3004_BASE 100 //virtuelle channel-vergabe für den AD-Wandler fängt bei 100 an zu zählen
#define SPI_CHAN 0	//SPICE0: chip select gpio port
#define AD_CHAN 0	//chan-0 auf AD (den verwenden wir)

#define DHTPIN 29

double senseLightLevel(int mcp_base, int ad_channel){
    double value = analogRead(mcp_base + ad_channel);
    double light = value/1023*100;
    return light;
}

double* senseGroundMoisture(int mcp_base, int ad_channel){
    double value = analogRead(mcp_base + ad_channel);
    double* moisture = malloc(sizeof(int));
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
    
    for(int i = 0 ; i < 100 ; i++){
        delay(1000); //delay 2 sec for DHT "cooldown"
        double* moisture = senseGroundMoisture(MCP3004_BASE, AD_CHAN);
        int* dht_data = readDHT(DHTPIN);

        printf("Dirt Moisture:\t%.2f%%\n", *moisture); //im just gonna pull some numbers out my ass and say above 70% is the optimal watering level
        printDHTData(dht_data);

        double lightVal = senseLightLevel(MCP3004_BASE, 1);
        printf("Light:\t\t%.2f%%\n", lightVal);
        
        printf("------------------------\n");
    }
    return 0;
}