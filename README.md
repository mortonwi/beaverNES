# beaverNES
Oregon State Capstone (CS 46X) project revolving around NES Emulation.

This branch covers CPU implementation.

To test CPU with nestest.nes, run the following commands:

    make cpu_test
    ./cpu_test rom/nestest.nes > output.txt

This should create an output.txt file in your directory.
You can now compare and contrast this .txt file with the offical nestest.log file located in rom/
