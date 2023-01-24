#!/usr/bin/env python3
"""
##########################################################################
# Ugly script to test XML parsing of iPM packets. Could use work but it
# gets the job done.
#
# Written in Python 3
#
# COPYRIGHT:   University Corporation for Atmospheric Research, 2023
##########################################################################
"""
import socket
import time
import argparse


def parse_args():
    """ Instantiate a command line argument parser """

    # Define command line arguments which can be provided
    parser = argparse.ArgumentParser(
        description="Script to emulate a UDP packet")
    args = parser.parse_args()

    return args


def main():

    args = parse_args()

    # Instrument emulator
    udp_id = "STATUS"
    udp_id2 = "MEASURE"
    udp_id3 = "RECORD"
    # To send from the aircraft to the ground
    udp_send_port = 30101  # nidas
    udp_send_port2 = 30101  # nidas
    udp_send_port3 = 30103  # nidas
    udp_ip = "172.16.47.154"  # from /etc/dhcp/dhcpd_ac.conf on the aircraft

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    while 1:
        # STATUS
        buffer = "%s,%s,0,0,0\r\n" % \
            (udp_id, time.strftime("%Y%m%dT%H%M%S", time.gmtime()))
        print(buffer)

        # MEASURE 1-phase
        buffer2 = "%s,%s,99,99,99,99,99,99,99\r\n" % \
            (udp_id2, time.strftime("%Y%m%dT%H%M%S", time.gmtime()))
        print(buffer2)

        # RECORD 1-phase
        buffer3 = "%s,%s,99,99,99,99,99,99,99,99,99,99,99,99,99,99\r\n" % \
            (udp_id3, time.strftime("%Y%m%dT%H%M%S", time.gmtime()))
        print(buffer3)

        if sock:
            bytes = sock.sendto(buffer.encode('utf-8'),
                                (udp_ip, udp_send_port))
            if udp_send_port2:
                bytes = sock.sendto(buffer2.encode('utf-8'),
                                    (udp_ip, udp_send_port2))
            if udp_send_port3:
                bytes = sock.sendto(buffer3.encode('utf-8'),
                                    (udp_ip, udp_send_port3))
            # print(bytes)

        time.sleep(1)


if __name__ == "__main__":
    main()
