#! /bin/env python3
"""
##########################################################################
# Program to emulate an iPM. Given iPM prompts on a serial port, write out
# corresponding responses on stdout.
#
# Written in Python 3
#
# COPYRIGHT:   University Corporation for Atmospheric Research, 2022
##########################################################################
"""
import logging
import argparse
import os
import sys
import time
import shutil
import tempfile
import subprocess as sp

import serial
from config import sequence

logger = logging.getLogger('ipmLogger')


class VirtualPorts():
    """
    Shamelessly stolen from the GNI emulator code. -JAA 12/15/2022

    Setup virtual serial devices.

    This creates two subprocesses: the socat process which manages the pty
    devices for us, and the socat "serial relay" to which the emulator
    process will connect.

    The emulator opens the "instrument port", or instport, while the
    control program opens the "user port", or userport.
    """
    def __init__(self):
        """ Initialize some instance variables """
        self.socat = None
        self.instport = None
        self.userport = None
        self.tmpdir = None

    def get_user_port(self):
        """ Return the user port """
        return self.userport

    def get_instrument_port(self):
        """ Return the instrument port """
        return self.instport

    def start_ports(self):
        """ Start just the ports. The emulator is run separately."""
        self.tmpdir = tempfile.mkdtemp()
        self.userport = os.path.join(self.tmpdir, "userport")
        self.instport = os.path.join(self.tmpdir, "instport")
        cmd = ["socat"]

        cmd.extend(["PTY,echo=0,link=%s" % (self.instport),
                    "PTY,echo=0,link=%s" % (self.userport)])

        # Open ports
        self.socat = sp.Popen(cmd, close_fds=True, shell=False)
        started = time.time()

        found = False
        while time.time() - started < 5 and not found:
            time.sleep(1)
            found = bool(os.path.exists(self.userport) and
                         os.path.exists(self.instport))

        # Error handling
        if not found:
            raise Exception("serial port devices still do not exist "
                            "after 5 seconds")
        return self.instport

    def stop(self):
        """ Shut down socat and clean up """
        logger.info("Stopping...")
        if self.socat:
            logger.debug("killing socat...")
            self.socat.kill()
            self.socat.wait()
            self.socat = None
        if self.tmpdir:
            logger.debug("removing %s", (self.tmpdir))
            shutil.rmtree(self.tmpdir)
            self.tmpdir = None


class IpmEmulator():
    """ Class to emulate an Intelligent Power Monitor """

    def __init__(self, device):

        # Establish serial connection to client
        self.sport = serial.Serial(
            port=device,
            baudrate=115200,
            timeout=1,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS
        )
        self.sport.nonblocking()

    def listen(self):
        """ Loop and wait for commands to arrive """
        while True:
            rdata = self.sport.read(128)
            msg = rdata.decode('utf-8')

            if msg == 'x':
                return

            for cmd in sequence:
                if cmd.msg == msg:
                    logger.debug(cmd.msg)
                    logger.debug(cmd.response)
                    self.sport.write(cmd.response.encode('utf-8'))
                    # If the command has a binary component, send that too
                    if cmd.bytes != '':
                        self.sport.write(cmd.bytes)
                    sys.stdout.flush()


def parse_args():
    """ Instantiate a command line argument parser """

    # Define command line arguments which can be provided by users
    parser = argparse.ArgumentParser(
        description="Script to operate an iPM")
    parser.add_argument(
        '-d', '--debug', dest='loglevel', action='store_const',
        const=logging.DEBUG, default=logging.INFO,
        help="Show debug log messages")

    # Parse the command line arguments
    args = parser.parse_args()

    return args


def main():
    """
    Instantiate ports and iPM emulator and start listening for commands
    from the control program
    """
    # Process command line arguments
    args = parse_args()

    # Set log level
    logging.basicConfig(level=args.loglevel)

    # Instantiate a set of virtual ports for iPM emulator and control code
    # to communicate over. When the control program is manually started, it
    # needs to connect to the userport.
    vports = VirtualPorts()

    instport = vports.start_ports()
    print("Emulator connecting to virtual serial port: %s", (instport))
    print("User clients connect to virtual serial port: %s",
          (vports.get_user_port()))

    # Instantiate iPM emulator and connect to instport.
    ipm = IpmEmulator(instport)

    # Loop and listen for commands from control program
    ipm.listen()

    # Clean up
    vports.stop()
    sys.exit(1)


if __name__ == "__main__":
    main()
