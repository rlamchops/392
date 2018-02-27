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
    name = ''
    fd = None
    addr = None

    def __init__(self, name, fd, addr):
        self.name = name
        self.fd = fd
        self.addr = addr

jobQueue = queue.Queue()
jobQueueLock = Lock()

clientList = []

port = None
numwWorkers = None
motd = None
helpMessage = """HELP DIALOGUE
/help                   Prints this help message
/users                  Dumps list of currently logged in users to stdout
/shutdown               Shuts down server gracefully"""

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

#Helper method to read until \r\n\r\n
def readSocket(fd):
    message = ''
    while True:
        message += (fd.recv(1)).decode('ascii')
        if len(message) > 5:
            size = len(message)
            if message[size-1] is '\n' and message[size-2] is '\r' and message[size-3] is '\n'\
                    and message[size-4] is '\r':
                message = message.replace('\r', '')
                message = message.replace('\n', '')
                return message

def searchForClient(name):
    for c in clientList:
        if c.name == name:
            return c
    return None

#Go through login procedure and if successful add to clientList the new client
def loginClient(fd, addr):
    message = readSocket(fd)
    if message != "ME2U":
        print("ME2U was not sent by the client now closing connection with this client.")
        fd.close()
        return
    fd.send(str.encode("U2EM\r\n\r\n"))
    message = readSocket(fd)
    if message[:3] != "IAM":
        print("IAM was not sent by client closing client connection")
        fd.close()
        return
    name = message[4:]
    c = searchForClient(name)
    if c is not None:
        print("Name already taken, closing connection after sending ETAKEN")
        fd.send(str.encode("ETAKEN\r\n\r\n"))
        fd.close()
        return

    #send MAI and MOTD and then add this client to the client list
    fd.send(str.encode("MAI\r\n\r\nMOTD " + motd + "\r\n\r\n"))
    clientList.append(Client(name, fd, addr))
    return

def clientCommands(clientSocket):
    message = readSocket(clientSocket)

def worker():
    while True:
        #Grab the lock first before accessing the queue
        jobQueueLock.acquire()
        job = jobQueue.get(block=True,timeout=None)
        jobQueueLock.release()

        if job.accept:
            loginClient(job.fd, job.addr)

        elif job.client:
            clientCommands(job.fd)


if __name__ == "__main__":
    port, numwWorkers, motd = parseArgs()

    #Spawn the worker threads
    threadList = [None] * numwWorkers
    for i in range(0, numwWorkers):
        t = Thread(target=worker)
        t.start()
        threadList[i] = t

    # set up listen socket
    serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    host = ''
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
                currentCommand = sys.stdin.readline()
                if currentCommand == "/help\n":
                    print (helpMessage)
                elif currentCommand == "/users\n":
                    if not clientList:
                        print ("No one is currently online")
                    for x in clientList:
                        print (x.name)
                elif currentCommand == "/shutdown\n":
                    # for x in clientList:
                    #    x.fd.close()
                    # jobQueueLock.acquire()
                    # for y in jobQueue.items:
                    #    y.fd.close()
                    # jobQueueLock.release()
                    for t in threadList:
                        t.join()
                    sys.exit()
                else:
                    print ("Unrecognizable command " + currentCommand)
                #jobQueue.put(Job(True, False, False, None, None, sys.stdin.readline()))

            #else it must be a client socket
            else:
                jobQueue.put(Job(False, True, False, r, None, None))







