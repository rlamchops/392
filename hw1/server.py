from threading import Thread, Lock
import argparse
import socket
import queue
import sys
import select

class Job:
    stdin = False
    client = False
    accept = False

    fd = None
    addr = None

    command = None

    def __init__(self, stdin, client, accept, fd, addr, command):
        self.stdin = stdin
        self.client = client
        self.accept = accept
        self.fd = fd
        self.addr = addr
        self.command = command

class Client:
    fd = None
    addr = None

    def __init__(self, fd, addr):
        self.fd = fd
        self.addr = addr

jobQueue = queue.Queue()
jobQueueLock = Lock()

clientList = []

def parseArgs():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', help='Verbose print all incoming and outgoing protocol verbs & content.')
    parser.add_argument('PORT_NUMBER', help='Port number to listen on.')
    parser.add_argument('NUM_WORKERS', help='Number of workers to spawn.')
    parser.add_argument('MOTD', help='Message to display to the client when they connect.')
    args = parser.parse_args()

    if not (args.PORT_NUMBER or args.NUM_WORKERS or args.MOTD):
        parser.error("No port number, number of workers, or message of the day was supplied.")

    return int(args.PORT_NUMBER), int(args.NUM_WORKERS), args.MOTD

#Go through login procedure and if successful add to clientList the new client
def loginClient(fd):
    return


def worker():
    while True:
        #Grab the lock first before accessing the queue
        jobQueueLock.acquire()
        job = jobQueue.get(block=True,timeout=None)
        jobQueueLock.release()

        if job.accept:
            loginClient(job.fd)


if __name__ == "__main__":
    port, numwWorkers, motd = parseArgs()

    #Spawn the worker threads
    threadList = [None] * numwWorkers
    for i in range(0, numwWorkers):
        t = Thread(target=worker)
        threadList[i] = t

    # set up listen socket
    serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    host = socket.gethostname()
    serverSocket.bind((host, port))
    serverSocket.listen()

    # empty list to store client sockets

    while True:
        inputs = [sys.stdin, serverSocket]
        for c in clientList:
            inputs.append(c.fd)
        readable, writable, exceptional = select.select(inputs, [], inputs)

        for r in readable:
            #time to accept a connection
            if r is serverSocket:
                socket, addr = serverSocket.accept()
                jobQueue.put(Job(False, False, True, socket, addr, None))

            #stdin command
            elif r is sys.stdin:
                jobQueue.put(Job(True, False, False, None, None, sys.stdin.readline()))







