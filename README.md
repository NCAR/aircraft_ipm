# Model iPM - Intelligent Power Monitor control software
EOL/RAF code to control the NAI iPM and send UDP packets to nidas. Main program can be forked by nidas dsm process or run standalone.

## Running the code
This code can be run four different ways: from nidas, or from the command line with a menu, specifying an address and command, or free running as would be called from nidas.

### From nidas
To run from nidas, use the ipm.xml file in this directory as a template to add the iPM to the project xml files. Download and install this software to the DSM where the iPM is mounted, eg `git clone this_repo`, `scons`, and `scons install`. Then when dsm_server is started, it will launch this code, initialize the iPM and send commands as configured in the XML.

### From the command line
To run from the command line, login to the DSM where the iPM is mounted, and enter one of the following command line patterns:

```
> ipm_ctrl -m <measurerate> -r <recordperiod> -b <baudrate> -n <num_addresses> -0 <addr, procqueries, port> -D <ipm device>"
```
will loop over command as specified in procqueries at the rates specified in measurerate and recordperiod

```
> ipm_ctrl -b <baudrate> -D <ipm device> -i"
```
 to run in interactive mode and print a menu.

```
> ipm_ctrl -b <baudrate> -D <ipm device> -i -a <address> -c <command>"
```
 to send a single command to given address

## Building the software
`scons` will build ipm_ctrl

## Developmemnt

### Running with the emulator
The required python environment is captured in the ipmenv YAML file. To activate the environment:

```
> conda activate mtp
```

To run with the emulator, run

```
python3 emulate.py
```

In a separate window run one of the `ipm_ctrl` commands above and append `-e` to the command. The emulator responds more slowly than the iPM. The -e increases the timeout period.

### Unit tests
This software uses googletest for unit testing.

`scons tests` will build the tests.

`tests/g_test` will run the tests.

### Deployment
To deploy the code via a dpkg, run `./build_dpkg`
