# Model iPM - Intelligent Power Monitor control software
EOL/RAF code to control the NAI iPM and send UDP packets to nidas. Main program can be forked by nidas dsm process or run standalone.

## Building the software
"scons" will build ipm_ctrl


## Unit tests
This software uses googletest for unit testing.

"scons tests" will build the tests.
"tests/g_test" will run the tests.
