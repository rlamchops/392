Nauman Shahzad and Raymond Lam

### Client
./client [-hv] NAME SERVER_IP SERVER_PORT
-h                         Displays this help menu, and returns EXIT_SUCCESS.
-v                         Verbose print all incoming and outgoing protocol verbs & content.
NAME                       This is the username to display when chatting.
SERVER_IP                  The ip address of the server to connect to.
SERVER_PORT                The port to connect to.

client commands
- /help
- /logout
- /listu
- /chat <to>

### Server
./server [-hv] PORT_NUMBER NUM_WORKERS MOTD
-h            Displays help menu & returns EXIT_SUCCESS.
-v            Verbose print all incoming and outgoing protocol verbs & content.
PORT_NUMBER   Port number to listen on.
NUM_WORKERS   Number of workers to spawn.
MOTD          Message to display to the client when they connect.
