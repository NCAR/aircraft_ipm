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
