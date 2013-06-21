# Led lights and formal grammars

I use scons to upload my program to my ardunino. Thanks [scons]! :)

![Photo](https://github.com/Tarrasch/motion-serving/raw/master/photo.png
"Plugged up Arduino One")

## Arduino setup

I know almost nothing about electronics, however following
[these](http://wiring.org.co/learning/basics/rgbled.html)
[two](http://arduino.cc/en/tutorial/button) manuals and doing some manual
tweaking I got the hardware to work the way I wanted.

## Uploading program

These two commands worked for me, after plugging in my Arduino One to my Ubuntu
machine (worked on both 12.04 and 13.04)

    scons
    scons ARDUINO_PORT=/dev/ttyACM0 upload

## More info

For more info, including how I generated most of the c source code. See this
[similar repository](https://github.com/Tarrasch/motion-serving) by me.

### Video

I made a 7 minute [video] of this for people attending a particular [conference]. 

[scons]: https://github.com/suapapa/arscons/
[video]: https://vimeo.com/68833696
[conference]: http://verifiablerobotics.com/RSS13/index.html
