## RMoxy

### Minipops drummer alternative firmware for Music Thing Modular Radio Music module

- [The repository for the Radio Music module by Tom Whitwell](https://github.com/TomWhitwell/RadioMusic/)
- [Kits for the module available from Thonk](https://www.thonk.co.uk/shop/radio-music-full-diy-kit/)

#### Using it
- Top knob selects drum pattern.
- Bottom knob sets tempo, where full CCW position is external clock enable.
- Button is RUN/STOP. Lit LED indicates RUN state. Sequence resets when stopped.
- Bottom right jack is audio output.
- Bottom left jack is external clock input. Pulse rising edge will advance sequence one step.
- Top left jack is reset input. Pulse rising edge will reset sequence.
- Top right jack is CV control for muting drummer voices. Works in a binary fashion. 
- The four top LEDs indicate position in the sequence in binary, 0 to 15 or 0 to 12 in some patterns.


#### License  
I don't think I used much at all if any of the original Radio Music code, but I'll go with the same license as for the Radio Music project. I might add a panel based on the original one later. Creative Commons license it is. [CC-BY-SA: Attribution / ShareAlike](https://creativecommons.org/licenses/by-sa/3.0/)  
 
This project includes work from the [Teensy](https://www.pjrc.com/teensy/) project, which is not covered by this license. Audio sample content is not covered by this license. [Source for the samples.](http://samples.kb6.de) Patterns for the drummer are borrowed from [Janost O2](https://github.com/hexagon5un/jan_ostmans_synths).
