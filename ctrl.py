#! /bin/env python3
"""
##########################################################################
# Program to send commands to an Intelligent Power Monitor (iPM), receive
# returned data and generate a UDP packet to be sent to nidas.
#
# IN DEVELOPMENT:
#     Questions are marked "Question:"
#     Incomplete items are marked "TBD"
#
# Written in Python 3
#
# COPYRIGHT:   University Corporation for Atmospheric Research, 2022
##########################################################################
"""
import argparse
import logging
import sys
import time
import serial
from config import sequence, measure

logger = logging.getLogger('ipmLogger')


class IpmControl():
    """ Class to control iPM interaction """

    def __init__(self, args):
        """ Establish serial connection to iPM and initialize device """
        # Establish serial connection to iPM
        self.ser = serial.Serial(
            port=args.device,
            baudrate=115200,
            timeout=1,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS
        )
        self.ser.nonblocking()

    def init_device(self, addresses):
        """
        Initialize the iPM device. Returns a verified list of device addresses
        that may be shorter than the list passed in if some addresses did not
        pass verification.
        """
        if not self.send_command('OFF'):  # Turn the device OFF
            # Command failed
            sys.exit(1)  # TBD: Is this the correct action

        time.sleep(.11)  # Wait > 100ms

        if not self.send_command('RESET'):  # Turn the device ON
            # Command failed
            sys.exit(1)  # TBD: Is this the correct action

        # Query device serial number at all addresses.
        for address in addresses:
            self.set_addr(address)
            logger.info("Verifying address %s", str(address))

            # Verify serial number at current addr
            if not self.send_command('SERNO?'):
                # Serial number has no response so log an error and do
                # not attempt any more queries to this address
                addresses.remove(address)  # Remove address from list
            else:  # Good response
                # Question: Writeup says record good response in ads file, but
                # where? How?

                # Query device firmware version
                if not self.send_command('VER?'):
                    # TBD: Record error to ads file.
                    print("TBD")

                # Perform built-in self test on all devices
                if not self.send_command('TEST'):
                    # TBD: Record error to ads file.
                    print("TBD")
                else:
                    if not self.send_command('BITRESULT?'):  # get test result
                        # TBD: Record error to ads file.
                        print("TBD")
                    else:
                        data = self.ser.read(24)
                        self.parse_self_test(data)

        return addresses

    def set_addr(self, address):
        """ Set Active Address """
        self.ser.write(b'ADR ' + str(address).encode('utf-8') + b'\n')
        if self.ser.readline() != b'':
            logger.error('Address not set correctly')
            sys.exit(1)

    def parse_self_test(self, bitresult):
        """ Parse the results of a self test and confirm test passed """
        print('TBD:' + str(bitresult))
        # TBD: If first 16 bits are zero, all voltages are within limits
        # Question: Do we need to go any further if that is the case?

    def loop(self, addresses):
        """ Loop over sampling commands per config taken from XML """
        # TBD: Get config from XML and pass into this fn

        # For each address
        for address in addresses:
            self.set_addr(address)
            logger.info("Querying address %s", str(address))

            # Query Device Measurement (TBD: set freq to once a second)
            if self.send_command('MEASURE?'):
                # Read 34 bytes of measure data
                data = self.ser.read(34)
                print(data)
                # Parse measure response
                self.decode_measure(data)

            else:
                print("TBD")
                # TBD: If a query returns the wrong number of bytes, ...

        # TBD: Device Status (once a second)

        # TBD: Device Statistics (once every 10 minutes)

        # TBD: Send data out via UDP to NIDAS

    def send_command(self, msg):
        """ Send command to iPM and verify response """
        expected_response = self.response(msg)
        if self.query(msg.encode('utf-8')) != expected_response:
            logger.error('Device command ' + msg + ' did not return expected' +
                         ' response ' + expected_response)
            return False  # Command failed

        return True  # Command succeeded

    def query(self, cmd):
        """ Query the iPM and return response """
        self.ser.write(cmd)
        response = self.ser.readline()
        while response == b'':
            # TBD: timeout after 5 tries
            print(response)
            response = self.ser.readline()
        print(response)
        return response

    def response(self, msg):
        """ Given a command, find the expected response from the iPM """
        for cmd in sequence:
            if cmd.msg == msg:
                return cmd.response.encode('utf-8')

        logger.error("Command %s not found in msg options in config.py file." +
                     "Error in code.", cmd)
        sys.exit(1)

    def decode_measure(self, data):
        """ Decode components of byte response to Measure inquiry """
        # TBD: Variables should be appended or prepended with a sample
        # name to differentiate between devices (addresses). This will happen
        # in the XML.

        # TBD: Phase in XML indicates if just capture one phase or all three
        # Value of numphases indicates which one: 1 = A, ...

        # Question: procqueries indicates what to include in final data? but
        # everything still goes into raw?

        # TBD: flesh this out. Just a hack to start
        index = 0
        for measure_data in measure:
            data_value = data[index:index+measure_data.len]
            measure_data = measure_data._replace(val=data_value)
            print(measure_data.varName, measure_data.val)
            # Need to convert data from 8 or 16bit uint with given scale factor
            # to a number... TBD  (copy from MTP)
            index = index + measure_data.len


def parse_args():
    """ Instantiate a command line argument parser """

    # Define command line arguments which can be provided by users
    parser = argparse.ArgumentParser(
        description="Script to operate an iPM")
    parser.add_argument(
        '--device', type=str, default='',
        help="Device on which to communicate with the iPM")
    parser.add_argument(
        '-d', '--debug', dest='loglevel', action='store_const',
        const=logging.DEBUG, default=logging.INFO,
        help="Show debug log messages")

    # Parse the command line arguments
    args = parser.parse_args()

    if args.device == "":
        print("Default port not determined yet - waiting for iPM hardware")
        print("Please supply a poet using the --device command line option")
        sys.exit(1)

    return args


def main():
    """
    Process command line args, instantiate an iPM controller and send
    commands to iPM
    """

    # Process command line arguments
    # TBD: Args needs to include ACserverIP, port (to transmit UDP to nidas),
    # valid addresses (in the range 0-7), as well as other iPM config info
    # from the XML file.
    # Question: At the top of the writeup it says the value of address can
    # be 0-255. Under "Queries and Addressing" it says addresses outside
    # 0-7 shall be ignored.
    args = parse_args()
    # For now:
    addresses = [0, 1, 2]  # for testing

    #  Set log level
    logging.basicConfig(level=args.loglevel)

    # Read in XML to get configuration - TBD (what does arinc_enet do?
    #  - look in XML)
    # for example addresses in use, ratre of data collection,
    # phases, and what responses to process

    # Instantiate the iPM comtroller
    ipm = IpmControl(args)

    # Initialize the device
    addresses = ipm.init_device(addresses)

    while True:
        ipm.loop(addresses)
        sys.exit(1)  # Just for testing


if __name__ == "__main__":
    main()
