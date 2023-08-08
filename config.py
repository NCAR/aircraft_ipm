"""
##########################################################################
# This file contains variables that hold configuration details for the iPM
# including commands and responses, the format of data included in the
# device response, and sample data (for testing).
#
# Written in Python 3
#
# COPYRIGHT:   University Corporation for Atmospheric Research, 2022
##########################################################################
"""
from collections import namedtuple
import numpy

# I am playing with using a namedtuple to hold the components of
# the reported response. Still not sure this is better than a
# dictionary...

# Some sample data from instrument runs in the DSM lab, with a few changes
# since the Python to C link is causing issues with some non-printing chars.
TEST_DATA = '\0\0?\0?\0\0\0\0\0P\0\0\0\0\0#\0?\0?\0L\x02\n'  # 24 bytes
MEASURE_DATA = '\x58\x02\x00\x00\x05\x02\x8b\x02\x8b\x02\x00\x00\x06\x06' \
               '\xFC\x06\x00\x00\x1B\x00\x1B\x00\x09\x00\xC9\x0D\xC8\x06' \
               '\x06\x06\x1B\x1B\x01\x01\n'  # 34 bytes
STATUS_DATA = '\0\0\0\0\0\0\0\0\0\0\0\0\n'  # 12 bytes
RECORD_DATA = '\x00\x02\x63\x00\x00\x00\x8B\x44\x69\x05\x00\x00\x00\x00\x00' \
              '\x00\x00\x00\xD1\x00\x9B\x05\xD1\x00\x9B\x05\x00\x00\x00\x00' \
              '\x45\x02\x58\x02\x00\x00\x5E\x00\x00\x00\x55\x00\x00\x00\x0d' \
              '\x00\x1B\x71\x1B\x71\x01\x01\x05\x06\x5A\x06\x05\x06\x53\x06' \
              '\x00\x00\x18\x00\x0d\x1B\x7C\x08\n'  # 68 bytes

# Tuple to store iPM commands and responses
Command = namedtuple('Command', ['msg', 'response', 'bytes'])

sequence = [
    Command('OFF', 'OK\n', ''),           # Turn Device OFF
    Command('RESET', 'OK\n', ''),         # Turn Device ON (reset)
    Command('SERNO?', '200728\n', ''),  # Query Serial number
    Command('VER?', 'VER A022(L) 2018-11-13\n', ''),  # Query Firmware Ver
    Command('TEST', 'OK\n', ''),          # Execute build-in self test
    Command('BITRESULT?', '24\n', TEST_DATA),  # Query self test result
    Command('ADR', '', ''),               # Device Address Selection
    Command('MEASURE?', '34\n', MEASURE_DATA),  # Device Measurement
    Command('STATUS?', '12\n', STATUS_DATA),    # Device Status
    Command('RECORD?', '68\n', RECORD_DATA),    # Device Statistics
]

# Tuple to store byte responses to queries
Data = namedtuple('Data', "len varName description units scale val")

measure = [
    Data(2, 'FREQ', 'AC Power Frequency', 'Hz', '0.1', numpy.nan),
    Data(2, 'reserved', 'unused', '', '1', numpy.nan),
    Data(2, 'T', 'Temperature', 'C', '0.1', numpy.nan),
    Data(2, 'VRMSA', 'AC Voltage RMS Phase A', 'V', '0.1', numpy.nan),
    Data(2, 'VRMSB', 'AC Voltage RMS Phase B', 'V', '0.1', numpy.nan),
    Data(2, 'VRMSC', 'AC Voltage RMS Phase C', 'V', '0.1', numpy.nan),
    Data(2, 'VPKA', 'AC Voltage Peak Phase A', 'V', '0.001', numpy.nan)
]
