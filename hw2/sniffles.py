import argparse
import socket
import sys
import struct
import hexdump
import signal
import fcntl
import packet as p

TIMEOUT = -1
FILTER = None
OUTPUT = None
INTERFACE = None


def sigHandler(signum, frame):
    raise Exception("Finished sniffing on " + INTERFACE + " for " + str(TIMEOUT) + " seconds. Closing... \n")


def argParse():
    parser = argparse.ArgumentParser()
    parser.add_argument('INTERFACE', help="Interface to listen for traffic on.")
    parser.add_argument('-o', '--output', help='File name to output to')
    parser.add_argument('-t', '--timeout', help='Amount of time to capture for. If not set ^C must be sent to close program')
    parser.add_argument('-x', '--hexdump', help='Print hexdump to stdout', action='store_true')
    parser.add_argument('-f', '--filter', metavar='{UDP, Ethernet, DNS, IP, TCP, ONE_MORE_OF_YOUR_CHOOSING}', help='Filter for one specified protocol')
    args = parser.parse_args()

    return args.INTERFACE, args.output, args.timeout, args.filter, args.hexdump


def sniffle(sniffler, toHex, filter):
    while True:
        packet = sniffler.recvfrom(65565)
        packet = packet[0]
        if toHex:
            hexdump.hexdump(packet)
        p.parsePacket(packet, filter)


if __name__ == "__main__":
    #First grab the command line arguments
    interface, outputFile, timeout, filter, toHex = argParse()

    #Set the INTERFACE to listen on and create a raw socket
    INTERFACE = interface
    sniffler = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.ntohs(0x0003))

    try:
        sniffler.bind((interface, 0))
    except (OSError, IOError) as e:
        print("Did not find the IP for given interface " + interface + ". Closing...\n")
        sys.exit()

    if timeout:
        TIMEOUT = int(timeout)
        signal.signal(signal.SIGALRM, sigHandler)
        signal.alarm(TIMEOUT)

    try:
        sniffle(sniffler, toHex, filter)
    except KeyboardInterrupt:
        print("\nReceived Ctrl-C. Exiting...")
    except Exception as e:
        print(e)

