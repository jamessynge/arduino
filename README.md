# arduino
Experiments with Arduino

Sorry about the terrible repo name, but works well for sync'ing with the repo on my laptop, where the Arduino development environment expects the directory name to be Arduino.

First step: I've hooked an Adafruit Trinket Pro (5V) to a NeoPixel Ring 24, and am experimenting with how fast updates can happen.  Looks like 800us between calls to Adafruit_NeoPixel::show(), of which around 50us is an enforced wait (see Adafruit_NeoPixel::canShow() which forces that wait).

Ambition: adapt some of the USB-V variants so that the trinket appears to be a USB HID device (e.g. keyboard or mouse), but with only output support, so that the host computer can send commands to the sketch (e.g. the build for my project has failed, switch the dominant color from green to red).
