from threading import Thread, Lock, Semaphore
import argparse
import socket
import queue
import sys
import select

class Job:
    stdin = False
    client = False
    accept = False
    quit = False

    name = None
    fd = None
    addr = None

    command = None

    def __init__(self, stdin, client, accept, quit, name, fd, addr, command):
        self.stdin = stdin
        self.client = client
        self.accept = accept
        self.quit = quit
        self.name = name
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
clientCommandSemaphore = Semaphore(0)
helpMessage = """HELP DIALOGUE
/help                   Prints this help message
/users                  Dumps list of currently logged in users to stdout
/shutdown               Shuts down server gracefully"""

verbose = False
STANDARD_COLOR = '\x1b[1;34m'
ERROR_COLOR = '\x1b[1;31m'

def parseArgs():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', help='Verbose print all incoming and outgoing protocol verbs & content.', action='store_true')
    parser.add_argument('PORT_NUMBER', help='Port number to listen on.')
    parser.add_argument('NUM_WORKERS', help='Number of workers to spawn.')
    parser.add_argument('MOTD', help='Message to display to the client when they connect.')
    args = parser.parse_args()

    if not (args.PORT_NUMBER or args.NUM_WORKERS or args.MOTD):
        parser.error("No port number, number of workers, or message of the day was supplied.")

    if args.v:
        global verbose
        verbose = True

    return int(args.PORT_NUMBER), int(args.NUM_WORKERS), args.MOTD

#Helper method to read until \r\n\r\n
def readSocket(fd):
    message = ''
    while True:
        message += (fd.recv(1)).decode('ascii')
        if message == '':
            return None
        elif len(message) > 5:
            size = len(message)
            if message[size-1] is '\n' and message[size-2] is '\r' and message[size-3] is '\n'\
                    and message[size-4] is '\r':
                message = message.replace('\r', '')
                message = message.replace('\n', '')
                printMessage(STANDARD_COLOR, message)
                return message

def searchForClient(name):
    for c in clientList:
        if c.name == name:
            return c
    return None

def searchByFd(fd):
    for x in clientList:
        if x.fd == fd:
            return x
    return None

def removeByFd(fd):
    c = searchByFd(fd)
    clientList.remove(c)
    return

def printMessage(color, message):
    if verbose:
        print(color + "VERBOSE: " + message + '\x1b[0m')

#Go through login procedure and if successful add to clientList the new client
def loginClient(fd, addr):
    message = readSocket(fd)
    if message != "ME2U":
        print("ME2U was not sent by the client now closing connection with this client.")
        removeByFd(fd)
        fd.close()
        return
    fd.send(str.encode("U2EM\r\n\r\n"))
    printMessage(STANDARD_COLOR, "U2EM\r\n\r\n")
    message = readSocket(fd)
    if message[:3] != "IAM":
        print("IAM was not sent by client closing client connection")
        removeByFd(fd)
        fd.close()
        return
    name = message[4:]
    c = searchForClient(name)
    if c is not None:
        print("Name already taken, closing connection after sending ETAKEN")
        fd.send(str.encode("ETAKEN\r\n\r\n"))
        printMessage(STANDARD_COLOR, "ETAKEN\r\n\r\n")
        removeByFd(fd)
        fd.close()
        return

    #send MAI and MOTD and then add this client to the client list
    fd.send(str.encode("MAI\r\n\r\nMOTD " + motd + "\r\n\r\n"))
    printMessage(STANDARD_COLOR, "MAI\r\n\r\n")
    printMessage(STANDARD_COLOR, "MOTD " + motd + "\r\n\r\n")
    searchByFd(fd).name = name
    return

def clientCommands(clientSocket, name, addr):
    message = readSocket(clientSocket)
    clientCommandSemaphore.release()

    #client closed socket suddenly so time to clean it up
    if message == None:
        c.fd.close()
        return

    elif message == "BYE":
        #send EYB to client
        clientSocket.send(str.encode("EYB\r\n\r\n"))
        printMessage(STANDARD_COLOR, "EYB\r\n\r\n")
        clientSocket.close()
        clientList.remove(searchByFd(clientSocket))
        for client in clientList:
            client.fd.send(str.encode("UOFF " + name + "\r\n\r\n"))
            printMessage(STANDARD_COLOR, "UOFF " + name + "\r\n\r\n")

    elif message == "LISTU":
        temp = "UTSIL"
        for client in clientList:
            temp += (" " + client.name)
        temp +=  "\r\n\r\n"
        clientSocket.send(str.encode(temp))
        printMessage(STANDARD_COLOR, temp)

    elif message[:2] == "TO":
        #get the contents of the string
        toArgs = message.split(maxsplit = 2)
        #first check if correct # of args
        if len(toArgs) != 3:
            printMessage(ERROR_COLOR, "Received garbage client command from " + searchByFd(clientSocket).name)
            return
        #check if target exists
        x = searchForClient(toArgs[1])
        if x is not None:
            toArgs[0] = "FROM"
            temp2 = toArgs[1]
            toArgs[1] = searchByFd(clientSocket).name
            toSend = str.join(' ', toArgs) + "\r\n\r\n"
            x.fd.send(str.encode(toSend))
            printMessage(STANDARD_COLOR, toSend)
            #now wait for MORF
            temp = readSocket(x.fd)
            if temp != ("MORF " + toArgs[1]):
                printMessage(ERROR_COLOR, "uh oh")
            clientSocket.send(str.encode("OT " + temp2 + "\r\n\r\n"))
            printMessage(STANDARD_COLOR, "OT " + temp2 + "\r\n\r\n")

        else:
            clientSocket.send(str.encode("EDNE\r\n\r\n"))
            printMessage(STANDARD_COLOR, "EDNE\r\n\r\n")


    else:
        printMessage(ERROR_COLOR, "Received garbage client command from " + searchByFd(clientSocket).name)
        clientSocket.close()

def worker():
    while True:
        #Grab the lock first before accessing the queue
        jobQueueLock.acquire()
        job = jobQueue.get(block=True,timeout=None)
        if job.quit:
            jobQueue.put(job)
            jobQueueLock.release()
            sys.exit()
        jobQueueLock.release()

        if job.accept:
            loginClient(job.fd, job.addr)

        elif job.client:
            clientCommands(job.fd, job.name, job.addr)

        elif job.stdin:
            print(job.command)
            if job.command == "/help\n":
                print (helpMessage)
            elif job.command == "/users\n":
                if not clientList:
                    print ("No one is currently online")
                for x in clientList:
                    print (x.name)
            elif job.command == "/shutdown\n":
                # for x in clientList:
                #    x.fd.close()
                # jobQueueLock.acquire()
                # for y in jobQueue.items:
                #    y.fd.close()
                # jobQueueLock.release()
                # for t in threadList:
                #     t.exit()
                sys.exit()
            else:
                print ("Unrecognizable command " + job.command)


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
    serverSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    serverSocket.bind((host, port))
    serverSocket.listen()

    while True:
        inputs = [sys.stdin, serverSocket]
        for c in clientList:
            inputs.append(c.fd)
        readable, writable, exceptional = select.select(inputs, [], inputs)

        for r in readable:
            #time to accept a connection
            if r is serverSocket:
                socket, addr = serverSocket.accept()
                clientList.append(Client(None, socket, addr))
                jobQueue.put(Job(False, False, True, False, None, socket, addr, None))

            #stdin command
            elif r is sys.stdin:
                peekCommand = sys.stdin.readline()
                if peekCommand == "/shutdown\n":
                    #place special quit job here
                    jobQueue.put(Job(False, False, False, True, None, None, None, None))
                    for t in threadList:
                        t.join()
                    serverSocket.close()
                    sys.exit()
                else:
                    jobQueue.put(Job(True, False, False, False, None, None, None, peekCommand))

            #else it must be a client socket, use semaphore to ensure the same exact job isn't being repeated across all threads
            else:
                client = searchByFd(r)
                jobQueue.put(Job(False, True, False, False, client.name, r, client.addr, None))
                clientCommandSemaphore.acquire(blocking=True)







