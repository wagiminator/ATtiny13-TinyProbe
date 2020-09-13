avrdude -c usbasp -p t13 -U flash:w:TinyProbe_v1.0.hex
avrdude -c usbasp -p t13 -U hfuse:w:0xfb:m -U lfuse:w:0x2a:m
