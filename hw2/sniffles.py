import argparse
import socket
import sys
import struct
# import hexdump
import signal
import fcntl

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

    return args.INTERFACE, args.output, args.timeout, args.filter

# def getInterfaceIP(iname):
#     s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#     try:
#         interfaceIP = socket.inet_ntoa(fcntl.ioctl(s.fileno(), 0x8915, struct.pack('256s', iname[:15].encode('utf-8')))[20:24])
#     except (OSError, IOError) as e:
#         return None
#     return interfaceIP

def sniffle(sniffler):
    while True:
        packet = sniffler.recvfrom(65565)
        print (packet)

if __name__ == "__main__":
    interface, outputFile, timeout, filter = argParse()
    # foundInterface = getInterfaceIP(interface)
    # if not foundInterface:
    #     print ("Did not find the IP for given interface " + interface + ". Closing... \n")
    #     sys.exit()
    INTERFACE = interface
    sniffler = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.ntohs(0x0003))
    try:
        sniffler.bind((interface, 0))
    except (OSError, IOError) as e:
        print ("Did not find the IP for given interface " + interface + ". Closing...\n")
        sys.exit()

    if timeout:
        TIMEOUT = int(timeout)
        signal.signal(signal.SIGALRM, sigHandler)
        signal.alarm(TIMEOUT)

    try:
        sniffle(sniffler)
    except Exception as e:
        print(e)

