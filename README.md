# I3C Saleae Protocol Analyzer

Being interested in the I3C protocol, I ran into the issue that my most favorite engineering tool did not support it.

So I started making my own analyzer plugin for Saleae Logic V2.

This analyzer can be compiled for Mac OSX, Linux, Windows. However, I did only build it for windows and provide the binary here in this repository.

Note: This is a low level analyzer only, similar to the inbuilt i2c analyzer. A High level analyzer is not planned for now.

# Why not use the builtin i2c analyzer?

I3C looks on a first rough view similar to I2C, but has many differences:
- In SDR mode the acknowledgment phase is handled in 2 phases. On Rising and falling edge of the 9th Bit different information can be signalled in both directions (e.g. Ack, Abort, Continue, ...).
- SDR mode can also generate patterns which an I2C analyzer would detect as start or stop condition, allthough they are none.
- Special bus signals are defined in I3C , e..g HDR Exit, HDR Restart or Target reset.

I3C HDR-DDR mode cannot be decoded with the I2C decoder.

This decoder does support right now SDR and HDR-DDR mode. I plan to further extend it to other HDR modes.

Allthough all information is decoded in DDR mode (like preamble, parity, CRC field, ...), I consider it still as a bit basic for now.

# Features

- Supports SDR and HDR-DDR mode
- Supports decoding of ENTDAA phase (in that phase 8 bytes are transmitted without ACK/NAK bit)
- Automatic detection of ENTHDR0..3 to switch automatically to HDR decoding mode (right now only HDR-DDR supported)
- Detection of special I3C bus conditions HDR restart, HDR exit, target reset
- all information is displayed, that means specifically: data payload, parity/preamble bits, abort/continue request from target or controller, ...
- All data is also dumped into the DATA view of the Analyzer.
- IBI/HJ works too, but that's no magic as this looks on the bus like a normal bus transfer.

# How to install

Step #1: Download [i3c_analyzer.dll](i3c_analyzer.dll)
Step #2: Copy it to the Saleae Logic protocol decoder directory: C:\Program Files\Logic2\resources\windows-x64\Analyzers
Step #3: restart or start Logic 2 software

## alternative installation method

Step #1: Download [i3c_analyzer.dll](i3c_analyzer.dll)
Step #2: Copy it to a folder where you want to permanently keep it
Step #3: Open Logic2 software and go to Edit -> Preferences
Step #4: Select in the section "Custom Low Level Analyzers" the directory where the i3c_analyzer.dll file is located.
Step #5: Restart Logic2 software.

# How to use

After installation the analyzer can be used like any other decoder. Just click on Analyzer, then the + button and select the I3C decoder.

# Any issue/comment/question

Post in the Issue or discussion section!

# Some pictures

## SDR mode decoding

<img src="https://raw.githubusercontent.com/xyphro/XyphroLabs-I3C-Saleae-Protocol-Analyzer/master/pictures/decode_ddr.png" width="80%"/>


## HDR-DDR mode decoding

<img src="https://raw.githubusercontent.com/xyphro/XyphroLabs-I3C-Saleae-Protocol-Analyzer/master/pictures/decode_sdr.png" width="80%"/>
