# Chat Room Starter — C++ / Qt 6

A real TCP server and a separate black, terminal-style desktop window.
The client is a Qt Widgets GUI: it is not your shell and does not execute shell commands.

## Build on Fedora

Install the compiler, CMake, and Qt 6 development packages:

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel
```

In the extracted `chatroom-starter` folder:

```bash
cmake -S . -B build
cmake --build build -j
```

Alternatively, open `CMakeLists.txt` in Qt Creator, select a Qt 6 desktop kit,
configure, and build. There are two executable targets: `chat_server` and
`chat_client`. Run the server first, then the client.

## Try it on one computer

Start the server in your normal terminal:

```bash
./build/chat_server
```

In another terminal, start the client:

```bash
./build/chat_client
```

A separate black desktop window opens. In that window, enter:

```text
/connect 127.0.0.1 5050 Ryan
```

You should see `TCP connected`, followed by `You are in the server (lobby)`
and an online-user list. Only the server's welcome response marks you ONLINE.

Run `./build/chat_client` again to open a second window, then enter:

```text
/connect 127.0.0.1 5050 Alex
```

Both windows get an updated online list. Type ordinary text and press Enter
to chat. Close a window or use `/disconnect` to leave; the server logs the
change and the other client gets an updated list. Ctrl+C stops the server.
Starting the client does not automatically start a server.

## Commands in the client window

| Command | What it does |
| --- | --- |
| `/connect HOST PORT NAME` | Connect and join as a display name |
| `/users` | Ask the server for everyone online |
| `/disconnect` | Leave or cancel a connection; the window stays open |
| `/clear` | Clear local output |
| `/help` | Show help |
| `/quit` | Close this client |
| Any other ordinary text | Send a lobby message |

Names allow 1–24 ASCII letters, digits, underscores, or hyphens.
Names are unique while online, ignoring case. A rejected name requires a
new `/connect` command using another name. Each process can open its own client.

## Server controls and local-network use

Default: `./build/chat_server 5050 127.0.0.1` (this computer only).
To listen on all IPv4 interfaces on a trusted LAN:

```bash
./build/chat_server 5050 0.0.0.0
```

On another computer, use `/connect SERVER_LAN_IP 5050 Alex`.
`127.0.0.1` always means the computer running the client. Use the server's actual
LAN address, not `0.0.0.0`, in the client. If Fedora's firewall blocks inbound
connections, allow TCP 5050 in the appropriate trusted network zone.

The server prints JOIN/LEAVE events, peer addresses on join, and a full
`Online (N): ...` list after every membership change. It runs an event loop
and handles multiple sockets without a thread per client.

## How the source fits together

- `src/server.cpp`: accepts TCP connections, validates names, owns the user
  roster, broadcasts messages and presence changes.
- `src/client.cpp`: black window, scrollback, command entry, socket events,
  connection timeout, and online/offline status.
- `CMakeLists.txt`: builds both programs with Qt Widgets and Qt Network.
- `tests/smoke.py`: real TCP test of the compiled server.

Transport is newline-delimited UTF-8 JSON. Each complete JSON object ends in
`\n`; TCP read boundaries are not treated as message boundaries. Receivers
retain incomplete lines until more bytes arrive. Frames are capped at 8192
bytes, chat text at 2000 characters, clients at 100, and pending output is
bounded. Unjoined sockets time out after ten seconds. The GUI keeps 2000
scrollback blocks. User messages are displayed as plain text, not HTML.

Client messages:

```json
{"type":"join","name":"Ryan"}
{"type":"users"}
{"type":"chat","text":"Hello!"}
```

Server replies/events use `welcome`, `users`, `chat`, `notice`, and `error`.
The server supplies the sender name; clients cannot choose someone else's
sender name by adding a field to a chat message.

## Next integration point

This is an in-memory lobby with display names, not account authentication.
In `ChatServer::handle`, the `join` branch is marked for your partner's account
service. Later, validate credentials there and associate sockets with stable
user IDs. Groups, invitations, database persistence, history, and TLS are not
implemented. The current TCP traffic is plaintext; use this starter locally
or on a trusted development LAN, not as a public authenticated chat service.

Qt API references: https://doc.qt.io/qt-6/qtcpsocket.html and
https://doc.qt.io/qt-6/qtcpserver.html

## Validation

After building, from the project folder:

```bash
python3 tests/smoke.py
```

This starts an isolated server, connects two users, checks presence and chat,
rejects duplicate names, checks split/combined TCP frames and malformed/oversized
input, then verifies departure cleanup. It shuts down its own test server.

Manual GUI checks: open two clients; join as different names; send text; close
one window; stop the server; reconnect after restarting it; try a nonexistent
server and an already-used name.

Delivery validation: Python test syntax and archive integrity checked. The
C++ build, network test, and visual GUI run were not executed in the authoring
environment because Qt/CMake were unavailable and dependency installation
failed. Run the build and test above on your Fedora computer.
