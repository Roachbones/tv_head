# TV Head
Arduino code for [my TV head costume project](https://rose.systems/tv_head). Responsible for interpreting keyboard input and playing animations on the LED matrix.

This code only needs to run on one handmade device, so I hardcoded it to fit my screen resolution and processor speed. So, the animations use `sleep` where they should probably use `millis` if you wanted them to have a reliable framerate. This is my first Arduino project, and some of the code is ugly, but the animations look great!
