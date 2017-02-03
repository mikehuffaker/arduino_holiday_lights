
## Arduino_holiday_lights 

Arduino sketch code to drive an Arduino micro-controller
board attached to LED's to "blink" in different colors for different holidays.

## Description

This sketch is currently coded to work with an Arduino connected to the WS-2801 LED 
light strips that can be purchased from Adafruit.  Several different colors  
are used, depending on the "Holiday" or test mode that is selected.  For
example, Halloween uses white, yellow, and orange colors.

Pressing a button
wired up to the Arduino will cause the program to switch to another holiday
or test mode.  The program also supports a connected LCD character display
that shows status message and messages concerning the current holiday or
 test mode that is running.  

I also recently added code that monitors a
 Potentiometer and converts its analog value into digital "dimming value"
number that can be used to adjust the brightness of the LED's in the
 strip.  

## Usage

Please see my website at www.huffaker.net for more detailed instructions on
how to build a full holiday lights project with an Arduino board and LED's.

Getting this full project working requires working with electronics,
 electricity, and soldering, so a basic understanding of all those skills
is recommended before trying to do this project.  

## License
Also note that although I am sharing this code as Open Source, please 
respect the BSD license terms, as stated in the LICENSE.txt file.

## Credits
I would like to get credit to [Adafruit](https://www.adafruit.com) and Lady Ada for the WS2801 arduino library
code that interfaces with the LED strands.  I also bought much of the electronics from Adafruit and
used their excellent tutorials to get this project working.  I also used some electronics from
[Sparkfun](https://www.sparkfun.com).

