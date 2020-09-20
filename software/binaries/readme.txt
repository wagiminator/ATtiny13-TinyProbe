avrdude -c usbasp -p t13 -U hfuse:w:0xfb:m -U lfuse:w:0x2a:m -U flash:w:TinyProbe.hex
