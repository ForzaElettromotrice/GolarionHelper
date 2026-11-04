import queue
import re
import socket
import struct
import subprocess
import sys
import threading
import time


class Process:
    def __init__(self, *arguments):
        self.process = subprocess.Popen(arguments, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, encoding='utf-8', errors='replace')
        self.lines = queue.Queue()
        self.history = []
        threading.Thread(target=self._read, daemon=True).start()

    def _read(self):
        for line in self.process.stdout:
            self.lines.put(line.rstrip())

    def wait_line(self, predicate, timeout=5):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                line = self.lines.get(timeout=max(deadline - time.monotonic(), 0.01))
            except queue.Empty:
                break
            self.history.append(line)
            if predicate(line):
                return line
        raise AssertionError(f'Expected output not received: {self.history}')

    def stop(self):
        if self.process.poll() is None:
            self.process.kill()
        self.process.wait(timeout=3)


def receive_exact(connection, count):
    result = b''
    while len(result) < count:
        chunk = connection.recv(count - len(result))
        assert chunk, 'Unexpected socket closure'
        result += chunk
    return result


def receive_message(connection):
    size = struct.unpack('!I', receive_exact(connection, 4))[0]
    assert 2 <= size <= 65536
    return receive_exact(connection, size)


def text_field(value):
    encoded = value.encode('utf-8')
    return struct.pack('!I', len(encoded)) + encoded


def snapshot_names(payload):
    assert payload[:2] == bytes([1, 2])
    position = 2

    def integer():
        nonlocal position
        value = struct.unpack_from('!I', payload, position)[0]
        position += 4
        return value

    def text():
        nonlocal position
        size = integer()
        value = payload[position:position + size].decode('utf-8')
        position += size
        return value

    assert text() == 'game'
    assert text() == 'Partita di prova'
    count = integer()
    participants = [(text(), text()) for _ in range(count)]
    assert position == len(payload)
    assert len({identity for identity, _ in participants}) == count
    return sorted(name for _, name in participants)


def main(host_executable, client_executable):
    processes = []
    try:
        host = Process(host_executable, '127.0.0.1', '0')
        processes.append(host)
        listening = host.wait_line(lambda line: line.startswith('In ascolto su '))
        port = re.search(r':(\d+)$', listening).group(1)

        alice = Process(client_executable, '127.0.0.1', port, 'Alice')
        processes.append(alice)
        alice.wait_line(lambda line: line == 'Partecipanti collegati: 1')
        bob = Process(client_executable, '127.0.0.1', port, 'Bob')
        processes.append(bob)
        alice.wait_line(lambda line: line == 'Partecipanti collegati: 2')
        bob.wait_line(lambda line: line == 'Partecipanti collegati: 2')

        with socket.create_connection(('127.0.0.1', int(port)), timeout=3) as fragmented:
            payload = bytes([1, 1]) + text_field('Frammentato')
            frame = struct.pack('!I', len(payload)) + payload
            fragmented.sendall(frame[:3])
            time.sleep(0.05)
            fragmented.sendall(frame[3:7])
            time.sleep(0.05)
            fragmented.sendall(frame[7:])
            assert snapshot_names(receive_message(fragmented)) == ['Alice', 'Bob', 'Frammentato']
            alice.wait_line(lambda line: line == 'Partecipanti collegati: 3')
            bob.wait_line(lambda line: line == 'Partecipanti collegati: 3')

        alice.wait_line(lambda line: line == 'Partecipanti collegati: 2')
        bob.wait_line(lambda line: line == 'Partecipanti collegati: 2')

        with socket.create_connection(('127.0.0.1', int(port)), timeout=3) as invalid:
            invalid.sendall(struct.pack('!I', 2) + bytes([99, 1]))
            assert receive_message(invalid)[:2] == bytes([1, 3])
            assert invalid.recv(1) == b''

        alice.stop()
        bob.wait_line(lambda line: line == 'Partecipanti collegati: 1')
        replacement = Process(client_executable, '127.0.0.1', port, 'Alice')
        processes.append(replacement)
        replacement.wait_line(lambda line: line == 'Partecipanti collegati: 2')
        bob.wait_line(lambda line: line == 'Partecipanti collegati: 2')

        host.stop()
        assert bob.process.wait(timeout=3) == 1
        assert replacement.process.wait(timeout=3) == 1
        print('PASS: C++ host and clients synchronize joins, departures, new connections, fragmented input, malformed input, and host shutdown.')
    finally:
        for process in reversed(processes):
            process.stop()


if __name__ == '__main__':
    main(*sys.argv[1:])
