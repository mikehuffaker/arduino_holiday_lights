// This is an Arduino sketch to run a holiday lights program that will  
// cause WS2801 LED's to blink different colors and patterns depending
// on the holiday that is selected.
//
// NOTE: The Adafruit_WS2801 Arduino library is used by this sketch and 
//       must be downloaded and installed before this sketch will compile 
//       or run.  Please obtain this code from Adafruit per the link below:
//       https://github.com/adafruit/Adafruit-WS2801-Library

// Tested with Arduino Uno R3, other versions of Arduino may not work.
// 
// Written by Mike H Huffaker
// Distributed under the BSD license, per the terms below.

// Copyright (c) 2013, 2014, 2015, Mike H Huffaker ( thecodingguy )

// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// 
// * Redistributions in binary form must reproduce the above copyright notice, this
//  list of conditions and the following disclaimer in the documentation and/or
//  other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Includes for 3rd party libraries 
#include <Adafruit_WS2801.h>
#include <SPI.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

// Note - Pins 3,5,10,11,12,13 used by wi-fi card, so using 
// 4 and 6 to communicate with the LED strand and 2 for button
const int dataPin = 4;
const int clockPin = 6;
const int buttonApin = 2;
const int brightnessPin = A2;
 
// LEDMode starts at mode 1
volatile int LEDMode = 9;
const int LEDModeMax = 11;

// EEPROM save values, setting flag to true will save LEDMode to EEPROM to
// preserve the last Mode when powered off.  Note the EEPROM eventually wears
// out after 100K cycles per the docs I've read, so setting this to true may
// "in theory" eventually burn out the EEPROM at the first addres and require
// changing the save address to another cell.
const boolean LEDModeSave = true;
const int LEDModeSaveAdr = 0;

//  Brightness reduction factor, controlled by variable resister knob
// int dimmingLevel = 0;

// timestamp for button push timer
volatile unsigned long btnPrevTS = 0;
unsigned long btnDelayMSec = 250;

unsigned long LCDMsgDelay = 750;
unsigned long LCDMsgPrev = 0;

unsigned long startUpMsgDelay = 7000;

volatile boolean breakLEDMode = false;

// Using some analog pins to free up for sensor/Wi-FI
// A0 must be free for future microphone use
LiquidCrystal lcd(7, 8, 9, 17, 18, 19);

// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
Adafruit_WS2801 strip = Adafruit_WS2801(100 , dataPin, clockPin);
  
void setup()
{
    // If flag is set, then try to get last LED Mode value from EEPROM
    if ( LEDModeSave )
    {
        int tmp = 0;
        EEPROM.get( LEDModeSaveAdr, tmp );
        if ( tmp > 0 && tmp <= LEDModeMax )
            LEDMode = tmp;
        else
            LEDMode = 1;
    }

    pinMode( buttonApin, INPUT_PULLUP );

    strip.begin();
    // Update LED contents, to start they are all 'off'
    strip.show();
    
    // Interrupt for button push - 0 is pin 2, 1 is pin 3
    attachInterrupt( 0, buttonCheck, LOW );
    
    lcd.begin( 16, 2 );
    // Print a message to the LCD.
    writeLCDMessage( true, "Mike's", "Holiday Lights!" );
    //lcd.setCursor( 0, 0 );
    //lcd.print( "Mike's" );
    //lcd.setCursor( 0, 1 );
    //lcd.print( "Holiday Lights!" );
}

boolean writeLCDMessage( boolean withDelay, const char* line1, const char* line2 )
{
    // Delay a bit if the flag is passed in, to provide a quick transition delay
    // before displaying a new message.  Too keep messages from disappearing 
    // too quickly after a mode change for instance.
    if ( withDelay )
    {
       delay( LCDMsgDelay );      
    }

    lcd.clear();
    if ( strlen( line1 ) > 0 )
    {
        lcd.setCursor( 0, 0 );
        lcd.print( line1 );
    }
    
    if ( strlen( line2 ) > 0 )
    {
        lcd.setCursor( 0, 1 );
        lcd.print( line2 );
    }
    
    return true;
}

boolean writeLCDColorMessage( boolean withDelay, const char* mode, const char* color )
{
    // Delay a bit if the flag is passed in, to provide a quick transition delay
    // before displaying a new message.  Too keep messages from disappearing 
    // too quickly after a mode change for instance.
    if ( withDelay )
    {
       delay( LCDMsgDelay );      
    }

    char line2[20];
    memset( line2, 0, sizeof(line2) );
    int brightnessReduction = checkBrightnessLevel() * -1;
    
    sprintf( line2, "*%s* : %d", color, brightnessReduction );
   
    lcd.clear();
    if ( strlen( mode ) > 0 )
    {
        lcd.setCursor( 0, 0 );
        lcd.print( mode );
    }
    
    if ( strlen( line2 ) > 0 )
    {
        lcd.setCursor( 0, 1 );
        lcd.print( line2 );
    }
    
    return true;
}

// fill the dots one after the other with color
// good for testing purposes
void colorWipe( uint32_t c, uint8_t wait, char oddEvenAll, char beginOrEnd )
{
    int i;
    
    if ( ( !beginOrEnd || beginOrEnd == 'B' ) && !breakLEDMode )
    {
        for ( i=0; i < strip.numPixels(); i++ )
        {
            if ( oddEvenAll == 'A' )
            {
                strip.setPixelColor(i, c);
            }
            else if ( oddEvenAll == 'E' and i % 2 == 0 )
            {
                strip.setPixelColor(i, c);
            }
            else if ( oddEvenAll == 'O' and i % 2 == 1 )
            {
                strip.setPixelColor(i, c);
            }
        
            if ( !breakLEDMode )
            {
                strip.show();
                delay(wait);
            }
        }
    }
    else if ( beginOrEnd == 'E' && !breakLEDMode )
    {
        for ( i = strip.numPixels(); i > -1; i-- )
        {
            if ( oddEvenAll == 'A' )
            {
                strip.setPixelColor(i, c);
            }
            else if ( oddEvenAll == 'E' and i % 2 == 0 )
            {
                strip.setPixelColor(i, c);
            }
            else if ( oddEvenAll == 'O' and i % 2 == 1 )
            {
                strip.setPixelColor(i, c);
            }
        
            if ( !breakLEDMode )
            {
                strip.show();
                delay(wait);
            }
        }
    }
}

// set 2 colors down the strip, alternating
void colorSet2( uint32_t color1, uint32_t color2, int wait ) 
{
    int i;
    
    for ( i=0; i < strip.numPixels(); i++ )
    {
        if ( ( i % 2 ) == 1 )
        {
            strip.setPixelColor(i, color1);            
        }
        else if ( ( i % 2 ) == 0 )
        {
            strip.setPixelColor(i, color2);            
        }
    }
    if ( !breakLEDMode )
    {
        strip.show();
        delay(wait);
    }
}

// set 3 colors down the strip, alternating
void colorSet3( uint32_t color1, uint32_t color2, uint32_t color3, int wait ) 
{
    int i;
    
    for ( i=0; i < strip.numPixels(); i++ )
    {
        //strip.setPixelColor(i, color1);
        if ( ( i % 3 ) == 2 )
        {
            strip.setPixelColor(i, color1);            
        }
        else if ( ( i % 3 ) == 1 )
        {
            strip.setPixelColor(i, color2);            
        }
        else if ( ( i % 3 ) == 0 )
        {
            strip.setPixelColor(i, color3);            
        }
    }
    if ( !breakLEDMode )
    {
        strip.show();
        delay(wait);
    }
}

// set 3 colors down the strip, alternating, 2 pixels per color
void colorSet3_2pixel( uint32_t color1, uint32_t color2, uint32_t color3, int wait ) 
{
    int i;
    
    for ( i=0; i < strip.numPixels(); i++ )
    {
        //strip.setPixelColor(i, color1);
        if ( ( i % 6 ) == 4 || ( i % 6 ) == 5 )
        {
            strip.setPixelColor(i, color1);            
        }
        else if ( ( i % 6 ) == 2 || ( i % 6 ) == 3 )
        {
            strip.setPixelColor(i, color2);            
        }
        else if ( ( i % 6 ) == 0 || ( i % 6 ) == 1 )
        {
            strip.setPixelColor(i, color3);            
        }
    }
    if ( !breakLEDMode )
    {
        strip.show();
        delay(wait);
    }
}


// set 2 colors, strip is first color and then 2nd color "chases" up and down the strip
void colorChase2( uint32_t color1, uint32_t color2, int wait )
{
    int i;
    int j;
    
    //initialize with one color
    for ( i=0; i < strip.numPixels() && !breakLEDMode; i++ )
    {
        if ( i == 0 )
            strip.setPixelColor(i, color2);
        else
            strip.setPixelColor(i, color1);
    }
    
    if ( !breakLEDMode )
    {
        strip.show();
        delay(wait);
    }
    
    // start at beginning
    for ( j=0; j < strip.numPixels() && !breakLEDMode; j++ )
    {
        for ( i=0; i < strip.numPixels() - 1; i++ )
        {
            if ( i == j || i == (j + 1) )
                strip.setPixelColor(i, color2);
            else
                strip.setPixelColor(i, color1);
        }
        
        if ( !breakLEDMode )
        {
            strip.show();
            delay(wait);
        }
    }
    
    // bring it back
    for ( j=strip.numPixels(); j > 1 && !breakLEDMode; j-- )
    {
        for ( i=0; i < strip.numPixels(); i++ )
        {
            if ( i == j || i == (j - 1) )
                strip.setPixelColor(i, color2);
            else
                strip.setPixelColor(i, color1);
        }
        
        if ( !breakLEDMode )
        {
            strip.show();
            delay(wait);
        }
    }
}

// randomly set a LED to one of 2 colors down the strip for a twinkle
// effect
void twinkle2( uint32_t color1, uint32_t color2, int wait, int iterations )
{
    int i;
    int c;
    
    for ( int x = 0; x < iterations && !breakLEDMode; x++ )
    {
        i = random(0, strip.numPixels());
        c = random(0,2);

        if ( c == 1 )
            strip.setPixelColor(i, color2);
        else
            strip.setPixelColor(i, color1);
            
        if ( !breakLEDMode )
        {
            strip.show();
            delay(wait);
        }
    }   
 }

// randomly set a LED to one of 3 colors down the strip for a twinkle
// effect
void twinkle3( uint32_t color1, uint32_t color2, uint32_t color3, int wait, int iterations )
{
    int i;
    int c;
    
    for ( int x = 0; x < iterations && !breakLEDMode; x++ )
    {
        i = random(0, strip.numPixels());
        c = random(0,3);

        if ( c == 2 )
            strip.setPixelColor(i, color3);
        else if ( c == 1 )
            strip.setPixelColor(i, color2);
        else
            strip.setPixelColor(i, color1);
            
        if ( !breakLEDMode )
        {
            strip.show();
            delay(wait);
        }
    }   
 }

/* Helper functions */
// Create a 24 bit color value from R,G,B
uint32_t Color( byte r, byte g, byte b )
{
    uint32_t c;
    c = r;
    c <<= 8;
    c |= g;
    c <<= 8;
    c |= b; 
    return c;
}

// This uses a string for now, at some point I might change it to an color ENUM
// with maybe short ints for each color value, to save a bit of memory.  I could
// also potentially create an array of a struct with each struct holding a value
// to define the color, plus 3 values to hold the RGB values.  
uint32_t getRealColor( String colorName )
{
    // When the color is set, read the analog port to check the variable resistor 
    // and see if the brightness was turned down.
    
    uint32_t dimmingLevel = checkBrightnessLevel();
    
    if ( colorName == "RED" )
    {
        return Color( 255 - dimmingLevel, 0, 0 );
    }
    else if ( colorName == "ORANGE" )
    {
        return Color( 255 - dimmingLevel, 60 - (dimmingLevel / 4), 0) ;
    }
    else if ( colorName == "BLUE" )
    {
        return Color( 0, 0, 255 - dimmingLevel );
    }
    else if ( colorName == "GREEN" )
    {
        return Color( 0, 255 - dimmingLevel, 0 );
    }
    else if ( colorName == "SEAGREEN" )
    {
        return Color( 0, 255 - dimmingLevel, 64 - (dimmingLevel / 4) );
    }
    else if ( colorName == "YELLOW" )
    {
        return Color( 128 - (dimmingLevel / 2), 128 - (dimmingLevel / 2), 0 );
    }
    else if ( colorName == "SILVER" )
    {
        return Color( 180 - dimmingLevel, 190 - dimmingLevel, 190 - dimmingLevel );
    }
    else if ( colorName == "CYAN" )
    {
        return Color( 0, 128 - (dimmingLevel / 2), 128 - (dimmingLevel / 2));
    }
    else if ( colorName == "MAGENTA" )
    {
        return Color( 128 - (dimmingLevel / 2), 0, 128 - (dimmingLevel / 2) );
    }
    else if ( colorName == "PINK" )
    {
        return Color( 255 - dimmingLevel, 100 - (dimmingLevel / 2), 175 - dimmingLevel );
    }
    else if ( colorName == "WHITE" )
    {
        return Color( 200 - dimmingLevel, 200 - dimmingLevel, 200 - dimmingLevel );
    }
}

uint32_t checkBrightnessLevel( )
{
    int analogLevel =  analogRead(brightnessPin);
    int reduction = ( analogLevel + 1 ) / 12 ;
    return reduction;
}

uint32_t getRandomColor( )
{
    int x = random( 1,7 );
    
    if ( x == 1 )
       return getRealColor( "RED" );
    else if ( x == 2 )
        return getRealColor( "BLUE" );
    else if ( x == 3 )
        return getRealColor( "GREEN" );
    else if ( x == 4 )
        return getRealColor( "YELLOW" );
    else if ( x == 5 )
        return getRealColor( "CYAN" );
    else if ( x == 6 )
        return getRealColor( "MAGENTA" );
}

void buttonCheck()
{
    // Added this delay check to make sure 1 button press that "bounced"
    // and triggers multiple interrupts doesn't change the Mode more than
    // once.  
    unsigned long timestamp = millis();

    if ( ( timestamp - btnPrevTS ) < btnDelayMSec )
        return;
    else
        btnPrevTS = timestamp;

    // breakLEDMode tells the current light loop to break and also by checking 
    // for false here, it prevents the mode from skipping ahead until the 
    // current mode is finished and the next one started.
    if ( digitalRead( buttonApin ) == LOW and breakLEDMode == false )
    {
        LEDMode++;
        breakLEDMode = true;
        if ( LEDMode > LEDModeMax )
        {
            LEDMode = 1;
        }
        
        // Save new Mode to EEPROM if flag set
        if ( LEDModeSave )
        {
            EEPROM.put( LEDModeSaveAdr, LEDMode );
        }
        
        lcd.clear();
        lcd.setCursor( 0, 0 );
        lcd.print( "New Blink MODE" );
        
        if ( LEDModeSave )
        {
            lcd.setCursor( 0, 1 );
            lcd.print( "EEPROM Saved" );
        }
    }      
}

// This function is to display all the available colors and will also show 
// the brightness reduction.
void ColorTest1()
{
    breakLEDMode = false;
    
    while ( breakLEDMode != true )
    {
        writeLCDColorMessage( false, "ColorTest1 MODE", "RED" );
        colorWipe( getRealColor( "RED" ), 25, 'A', 'B' );

        writeLCDColorMessage( false, "ColorTest1 MODE", "ORANGE" );
        colorWipe( getRealColor( "ORANGE" ), 25, 'A', 'B' );

        writeLCDColorMessage( false, "ColorTest1 MODE", "BLUE" );
        colorWipe( getRealColor( "BLUE" ), 25, 'A', 'B' );

        writeLCDColorMessage( false, "ColorTest1 MODE", "GREEN" );
        colorWipe( getRealColor( "GREEN" ), 25, 'A', 'B' );
        
        writeLCDColorMessage( false, "ColorTest1 MODE", "SEAGREEN" );
        colorWipe( getRealColor( "SEAGREEN" ), 25, 'A', 'B' );

        writeLCDColorMessage( false, "ColorTest1 MODE", "YELLOW" );
        colorWipe( getRealColor( "YELLOW" ), 25, 'A', 'B' );

        writeLCDColorMessage( false, "ColorTest1 MODE", "SILVER" );
        colorWipe( getRealColor( "SILVER" ), 25, 'A', 'B' );

        writeLCDColorMessage( false, "ColorTest1 MODE", "CYAN" );
        colorWipe( getRealColor( "CYAN" ), 25, 'A', 'B' );

        writeLCDColorMessage( false, "ColorTest1 MODE", "MAGENTA" );
        colorWipe( getRealColor( "MAGENTA" ), 25, 'A', 'B' );
      
        writeLCDColorMessage( false, "ColorTest1 MODE", "PINK" );       
        colorWipe( getRealColor( "PINK" ), 25, 'A', 'B' );

        writeLCDColorMessage( false, "ColorTest1 MODE", "WHITE" );       
        colorWipe( getRealColor( "WHITE" ), 25, 'A', 'B' );
    }
}

void RandomColor1()
{
    breakLEDMode = false;

    writeLCDMessage( true, "RandomColor1", "MODE" );                      

    while ( breakLEDMode != true )
    {
        colorWipe(getRandomColor(), 30, 'O', 'B');
        colorWipe(getRandomColor(), 30, 'E', 'B');
    }
}

void RandomColor2()
{
    breakLEDMode = false;

     writeLCDMessage( true, "RandomColor2", "MODE" );  

    while ( breakLEDMode != true )
    {
        colorWipe(getRandomColor(), 15, 'A', 'B');
    }
}

void TwoColorProgram( const char* mode, const char* color1, const char* color2 )
{
    breakLEDMode = false;
    int x = 0;
    
    writeLCDMessage( true, mode, "MODE" ); 

    while ( breakLEDMode != true )
    {
        for ( x = 0; x < 2 && breakLEDMode != true; x++ )
        {       
            colorWipe(getRealColor( color1 ), 15, 'A', 'B' );
            colorWipe(getRealColor( color2 ), 15, 'A', 'E' );
            colorWipe(getRealColor( color1 ), 15, 'A', 'B' );
            colorWipe(getRealColor( color2 ), 15, 'A', 'E' );
        }
        
        for ( x = 0; x < 4 && breakLEDMode != true; x++ )
        { 
            colorSet2(getRealColor( color1 ), getRealColor( color2 ), 1000);
            colorSet2(getRealColor( color2 ), getRealColor( color1 ), 1000);
        }
        
        colorChase2(getRealColor( color1 ), getRealColor( color2 ), 20 );
        colorChase2(getRealColor( color2 ), getRealColor( color1 ), 20 );
        
        twinkle2(getRealColor( color1 ), getRealColor( color2 ), 50, 400 );
    }
}

// generic 3 color program - runs for colors passed in
// to-do, add control flags to enable/disable certain patterns
void ThreeColorProgram( const char* mode, const char* color1, const char* color2, const char* color3 )
{
    breakLEDMode = false;
    int x = 0;
    
    writeLCDMessage( true, mode, "MODE" );  

    while ( breakLEDMode != true )
    {
        for ( x = 0; x < 2 && breakLEDMode != true; x++ )
        {       
            colorWipe(getRealColor( color1 ), 15, 'A', 'B' );
            colorWipe(getRealColor( color2 ), 15, 'A', 'E' );
            colorWipe(getRealColor( color3 ), 15, 'A', 'B' );
            colorWipe(getRealColor( color1 ), 15, 'A', 'E' );
            colorWipe(getRealColor( color2 ), 15, 'A', 'B' );
            colorWipe(getRealColor( color3 ), 15, 'A', 'E' );
        }
        
        for ( x = 0; x < 8 && breakLEDMode != true; x++ )
        { 
            colorSet3(getRealColor( color1 ), getRealColor( color3 ), getRealColor( color2 ), 1000);
            colorSet3(getRealColor( color3 ), getRealColor( color2 ), getRealColor( color1 ), 1000);
            colorSet3(getRealColor( color2 ), getRealColor( color1 ), getRealColor( color3 ), 1000);
        }
   
        colorChase2(getRealColor( color1 ), getRealColor( color2 ), 20 );
        colorChase2(getRealColor( color2 ), getRealColor( color1 ), 20 );
        colorChase2(getRealColor( color3 ), getRealColor( color1 ), 20 );
        
        twinkle3(getRealColor( color1 ), getRealColor( color3 ), getRealColor( color2 ), 50, 400 );

        // Testing to see if this looks good        
        for ( x = 0; x < 8 && breakLEDMode != true; x++ )
        { 
            colorSet3_2pixel(getRealColor( color1 ), getRealColor( color3 ), getRealColor( color2 ), 1000);
            colorSet3_2pixel(getRealColor( color3 ), getRealColor( color2 ), getRealColor( color1 ), 1000);
            colorSet3_2pixel(getRealColor( color2 ), getRealColor( color1 ), getRealColor( color3 ), 1000);
        }

        // Add "swirl" function, with speed varying, then reverse direction
        
        //unsigned long testvalue = millis() % 500;
        //String temp = String(testvalue);
        //writeLCDMessage( false, "Delay", temp.c_str() );  

        //delay( testvalue );
    }
}

// TODO: 4, 5, 6 color programs, etc.

// Holiday Mode functions start here
void ChristmasRGBMode()
{
    ThreeColorProgram( "Christmas RGB", "RED", "GREEN", "BLUE" );  
}

void ChristmasMGYMode()
{
    ThreeColorProgram( "Christmas MGY", "MAGENTA", "GREEN", "YELLOW" );  
}

void HalloweenOWRMode()
{
    ThreeColorProgram( "Halloween OWR", "ORANGE", "WHITE", "RED" );  
}  

void July4thRWBMode()
{
    ThreeColorProgram( "July 4th RWB", "RED", "WHITE", "BLUE" ); 
}

void ChanukahMode()
{
    TwoColorProgram( "Chanukah", "BLUE", "SILVER" ); 
}

void ValentinesRPWMode()
{
    ThreeColorProgram( "Valentines RPW", "RED", "PINK", "WHITE" ); 
}

void ValentinesRMWMode()
{
    ThreeColorProgram( "Valentines RMW", "RED", "MAGENTA", "WHITE" ); 
}

void StPatricksMode()
{
    TwoColorProgram( "Saint Patricks", "GREEN", "WHITE" ); 
}

void loop ()
{
    restart_loop:
    // testswitch sets the program.  Maybe in the future use a switch on the arduino to control
    // the program and led readout to show the playing program
    // int testSwitch = 1;
    // LEDMode = 3;
    if ( LEDMode == 1)
    {
        ColorTest1();
    }
    if ( LEDMode == 2 )
    {
        RandomColor1();
    }
    else if ( LEDMode == 3 )
    {
        RandomColor2();
    }
    else if ( LEDMode == 4 )
    {
        July4thRWBMode();
    }
    else if ( LEDMode == 5 )
    {
        HalloweenOWRMode();
    }
    else if ( LEDMode == 6 )
    {
        ChanukahMode();
    }
    else if ( LEDMode == 7 )
    {
        ChristmasRGBMode();
    }
    else if ( LEDMode == 8 )
    {
        ChristmasMGYMode();
    }
    else if ( LEDMode == 9 )
    {
        ValentinesRPWMode();
    }
    else if ( LEDMode == 10 )
    {
        ValentinesRMWMode();
    }
    else if ( LEDMode == 11 )
    {
        StPatricksMode();
    }

}

