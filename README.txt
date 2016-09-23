
This repository contains sketch code to drive an Arduino micro-controller
board attached to LED's to "blink" in different colors for different holidays.

Its currently coded to work with an Arduino connected to the WS-2801 LED 
light strips that can be purchased from Adafruit.  Several different colors  
are used, depending on the "Holiday" or test mode that is selected.  For
example, Halloween uses white, yellow, and orange colors.  Pressing a button
wired up to the Arduino will cause the program to switch to another holiday
or test mode.  The program also supports a connected LCD character display
that shows status message and messages concerning the current holiday or
 test mode that is running.  I also recently added code that monitors a
 Potentiometer and converts its analog value into digital "dimming value"
number that can be used to adjust the brightness of the LED's in the
 strip.  

This is a work in progress.  The plan is to add more code in the future as I
try out different projects and set them up.

Please see my website at www.huffaker.net for more detailed instructions on
how to build a full holiday lights project with an Arduino board and LED's.

Note the instructions are also a work in progress and are not complete yet, 
and getting this full project working requires working with electronics,
 electricity, and soldering, so a basic understanding of all those skills
is recommended before trying to do this project.  

Also note that although I am sharing this code as Open Source, please 
respect the BSD license terms, as stated in the LICENSE.txt file.

