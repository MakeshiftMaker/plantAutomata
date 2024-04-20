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

int main()
{
    wiringPiSetup();
    mcp3004Setup(MCP3004_BASE, SPI_CHAN);
    
    // while(1){
    //     int value = analogRead(MCP3004_BASE + AD_CHAN);
    //     printf("%d\n", value);
    //     delay(100);
    // }
    int errors = 0;
    for(int i = 0 ; i < 100 ; i++){
        
        delay(1000); //delay 2 sec for DHT "cooldown"
        int* dht_data = readDHT(DHTPIN);
        //printf("%p\n", (void*)dht_data);
        if(dht_data == NULL){
            printf("%d: Checksum Error\n", i);
            errors++;
        }
        else{
            printf("%d: Humidity: %d.%d%%\nTemperature: %d.%dC\n", i, dht_data[0], dht_data[1], dht_data[2], dht_data[3]); 
        }
    }
    printf("%d/100 errors", errors);
    return 0;
}
