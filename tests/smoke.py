#!/usr/bin/env python3
"""Exercise the compiled server over real TCP. No third-party Python packages."""
import json
import socket
import subprocess
import sys
import time
from pathlib import Path

class Peer:
    def __init__(self, port):
        self.socket = socket.create_connection(('127.0.0.1', port), timeout=2)
        self.file = self.socket.makefile('rb')
    def send(self, message):
        self.socket.sendall(json.dumps(message).encode() + b'\n')
    def until(self, predicate):
        for _ in range(30):
            line = self.file.readline()
            if not line:
                raise AssertionError('Unexpected disconnect')
            msg = json.loads(line)
            if predicate(msg):
                return msg
        raise AssertionError('Expected response not received')
    def kind(self, kind):
        return self.until(lambda m: m['type'] == kind)
    def close(self):
        self.file.close()
        self.socket.close()

def main():
    executable = Path(sys.argv[1] if len(sys.argv) > 1 else 'build/chat_server').resolve()
    with socket.socket() as probe:
        probe.bind(('127.0.0.1', 0))
        port = probe.getsockname()[1]
    server = subprocess.Popen([str(executable), str(port)], stdout=subprocess.DEVNULL)
    peers = []
    try:
        for _ in range(50):
            if server.poll() is not None:
                raise RuntimeError('Server failed to start')
            try:
                a = Peer(port)
                peers.append(a)
                break
            except OSError:
                time.sleep(.05)
        else:
            raise RuntimeError('Server did not become ready')
        # A join split across TCP writes must still work.
        a.socket.sendall(b'{"type":"join",')
        a.socket.sendall(b'"name":"Ryan"}\n')
        assert a.kind('welcome')['name'] == 'Ryan'
        assert a.kind('users')['names'] == ['Ryan']
        b = Peer(port); peers.append(b)
        b.send({'type': 'join', 'name': 'Alex'})
        b.kind('welcome')
        assert b.kind('users')['names'] == ['Alex', 'Ryan']
        assert a.kind('users')['names'] == ['Alex', 'Ryan']
        b.send({'type': 'chat', 'text': 'Hello Ryan'})
        assert a.kind('chat') == {'type': 'chat', 'name': 'Alex', 'text': 'Hello Ryan'}
        duplicate = Peer(port); peers.append(duplicate)
        duplicate.send({'type': 'join', 'name': 'ryan'})
        assert 'already online' in duplicate.kind('error')['text']
        # Combined frames, malformed JSON, and an oversized frame.
        a.socket.sendall(b'not-json\n{"type":"users"}\n')
        a.kind('error')
        assert a.kind('users')['names'] == ['Alex', 'Ryan']
        bad = Peer(port); peers.append(bad)
        bad.socket.sendall(b'x' * 8192)
        assert 'large' in bad.kind('error')['text']
        b.close(); peers.remove(b)
        a.until(lambda m: m['type'] == 'notice' and 'left' in m['text'])
        assert a.kind('users')['names'] == ['Ryan']
        a.send({'type': 'users'})
        assert a.kind('users')['names'] == ['Ryan']
        print('PASS: join, roster, chat, duplicate name, framing, invalid input, disconnect')
    finally:
        for peer in peers:
            peer.close()
        server.terminate()
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill(); server.wait()

if __name__ == '__main__':
    main()
