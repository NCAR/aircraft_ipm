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

# Some sample data - to be refined as we go
TEST_DATA = b'0123456789abcdefghijklmn'  # sample 24 byte data
MEASURE_DATA = b'0123456789abcdefghijklmnopqrstuvwx'  # 34 byte data
STATUS_DATA = b'0123456789abc'  # 12 byte data
RECORD_DATA = b'xxxxxxx'  # 64 byte data

# Tuple to store iPM commands and responses
Command = namedtuple('Command', ['msg', 'response', 'bytes'])

sequence = [
    Command('OFF', 'OK\n', ''),           # Turn Device OFF
    Command('RESET', 'OK\n', ''),         # Turn Device ON (reset)
    Command('SERNO?', '203456-7\n', ''),  # Query Serial number
    Command('VER?', 'VER 004 2022-11-21\n', ''),  # Query Firmware Ver
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
