avrdude -c usbasp -pt13 -u -Ulfuse:w:0x7A:m -Uhfuse:w:0xFF:m -Uflash:w:MoodLamp/Release/MoodLamp.hex:a


