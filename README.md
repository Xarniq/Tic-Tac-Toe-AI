# Tic-Tac-Toe AI

A networked Tic-Tac-Toe game where you play against an AI opponent powered by **Monte Carlo Tree Search (MCTS)**. Features a Raylib-based graphical interface and a client-server architecture using a custom binary protocol (PAME).

## Prerequisites

- **GCC** (C99)
- **Git** (for submodules)
- **Linux** environment (uses POSIX threads and sockets)

## Build

```bash
cd tic-tac-toe-ai

# Initialize the Raylib dependency
git submodule update --init --recursive external/raylib

# Build both client and server
make all
```

You can also build them separately:

```bash
make client    # builds bin/ttt_game
make server    # builds bin/ttt_server
```

## How to Play

### 1. Start the Server

Open a terminal and run:

```bash
./bin/ttt_server
```

The server will start listening on port **5556** (configurable in `config/server.ini`).

### 2. Start the Client

Open a second terminal and run:

```bash
./bin/ttt_game
```

An 800×600 game window will open.

### 3. Connect and Play

1. **Enter your username** on the login screen.
2. **Select the AI difficulty** from the dropdown:
   - **Easy** — AI performs 10 MCTS iterations (weak)
   - **Medium** — AI performs 100 MCTS iterations (balanced)
   - **Hard** — AI performs 5000 MCTS iterations (near-optimal)
3. **Click Connect** to join the server.
4. Once the game starts, **click on a cell** to place your mark.
5. The AI will respond with its move automatically.
6. The game ends when someone gets three in a row, or the board is full (draw).
7. After the game ends, you can start a new round.

## Configuration

### Server — `config/server.ini`

```ini
[server]
port = 5556              # TCP port
default_difficulty = 2   # 1 = Easy, 2 = Medium, 3 = Hard
max_clients = 10         # Maximum concurrent connections
```

### Client — `config/client.ini`

```ini
[server]
server_ip = 127.0.0.1   # Server address
server_port = 5556       # Server port
```

To play over a network, change `server_ip` to the server machine's IP address.

## Quick Reference

| Command | Description |
|---------|-------------|
| `make all` | Build client and server |
| `make client` | Build client only |
| `make server` | Build server only |
| `make run` | Build and run the client |
| `make run-server` | Build and run the server |
| `make clean` | Remove build artifacts |
