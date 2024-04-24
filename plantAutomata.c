#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <mcp3004.h>
#include <string.h>
#include "../DHT11/DHT11.h"

#define MCP3004_BASE 100 //virtuelle channel-vergabe für den AD-Wandler fängt bei 100 an zu zählen
#define SPI_CHAN 0	//SPICE0: chip select gpio port
#define AD_CHAN 0	//chan-0 auf AD (den verwenden wir)

#define DHTPIN 29

// Define some device parameters
#define I2C_ADDR   0x27 // I2C device address

// Define some device constants
#define LCD_CHR  1 // Mode - Sending data
#define LCD_CMD  0 // Mode - Sending command

#define LINE1  0x80 // 1st line
#define LINE2  0xC0 // 2nd line

#define LCD_BACKLIGHT   0x08  // On
// LCD_BACKLIGHT = 0x00  # Off

#define ENABLE  0b00000100 // Enable bit

int fd;

#define BUTTONPIN 28

void lcd_init(void);
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);

// added by Lewis
void typeInt(int i);
void typeFloat(float myFloat);
void lcdLoc(int line); //move cursor
void ClrLcd(void); // clr LCD return home
void typeln(const char *s);
void typeChar(char val);
int fd;  // seen by all subroutines

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

// float to string
void typeFloat(float myFloat)   {
  char buffer[20];
  sprintf(buffer, "%3.1f",  myFloat);
  typeln(buffer);
}

// int to string
void typeInt(int i)   {
  char array1[20];
  sprintf(array1, "%d",  i);
  typeln(array1);
}

// clr lcd go home loc 0x80
void ClrLcd(void)   {
  lcd_byte(0x01, LCD_CMD);
  lcd_byte(0x02, LCD_CMD);
}

// go to location on LCD
void lcdLoc(int line)   {
  lcd_byte(line, LCD_CMD);
}

// out char to LCD at current position
void typeChar(char val)   {

  lcd_byte(val, LCD_CHR);
}


// this allows use of any size string
void typeln(const char *s)   {

  while ( *s ) lcd_byte(*(s++), LCD_CHR);

}

void lcd_byte(int bits, int mode)   {

  //Send byte to data pins
  // bits = the data
  // mode = 1 for data, 0 for command
  int bits_high;
  int bits_low;
  // uses the two half byte writes to LCD
  bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT ;
  bits_low = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT ;

  // High bits
  wiringPiI2CReadReg8(fd, bits_high);
  lcd_toggle_enable(bits_high);

  // Low bits
  wiringPiI2CReadReg8(fd, bits_low);
  lcd_toggle_enable(bits_low);
}

void lcd_toggle_enable(int bits)   {
  // Toggle enable pin on LCD display
  delayMicroseconds(500);
  wiringPiI2CReadReg8(fd, (bits | ENABLE));
  delayMicroseconds(500);
  wiringPiI2CReadReg8(fd, (bits & ~ENABLE));
  delayMicroseconds(500);
}


void lcd_init()   {
  // Initialise display
  lcd_byte(0x33, LCD_CMD); // Initialise
  lcd_byte(0x32, LCD_CMD); // Initialise
  lcd_byte(0x06, LCD_CMD); // Cursor move direction
  lcd_byte(0x0C, LCD_CMD); // 0x0F On, Blink Off
  lcd_byte(0x28, LCD_CMD); // Data length, number of lines, font size
  lcd_byte(0x01, LCD_CMD); // Clear display
  delayMicroseconds(500);
}

int main()
{
    wiringPiSetup();
    mcp3004Setup(MCP3004_BASE, SPI_CHAN);
    fd = wiringPiI2CSetup(I2C_ADDR);
    lcd_init();

    lcdLoc(LINE1);
    typeln("Plant Automota");
    lcdLoc(LINE2);
    typeln("by: Maker");

    delay(2500);
    ClrLcd();

    for(int i = 0 ; i < 100 ; i++){
        delay(2000); //delay 2 sec for DHT "cooldown"

        float* moisture = senseGroundMoisture(MCP3004_BASE, AD_CHAN);
        int* dht_data = readDHT(DHTPIN);

        printf("Dirt Moisture:\t%.2f%%\n", *moisture); //im just gonna pull some numbers out my ass and say above 70% is the optimal watering level
        lcdLoc(LINE1);
        typeln("Water: ");
        typeFloat(*moisture);
        typeln("%");

        printDHTData(dht_data);
        if(dht_data != NULL){
            lcdLoc(LINE2);
            typeln("HMT: ");
            typeInt(dht_data[0]);
            typeln(".");
            typeInt(dht_data[1]);
            typeln("%");
        }
        
        double lightVal = senseLightLevel(MCP3004_BASE, 1);
        printf("Light:\t\t%.2f%%\n", lightVal);

        
        printf("------------------------\n");


    }
    return 0;
}
