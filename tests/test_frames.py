import importlib.util
from pathlib import Path
import socket
import threading
import unittest

spec = importlib.util.spec_from_file_location('frames', Path(__file__).resolve().parents[1] / 'virtual-camera/src/frames.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class FramesTest(unittest.TestCase):
    def read_chunks(self, chunks):
        reader, writer = socket.socketpair()
        def send():
            with writer:
                for chunk in chunks:
                    writer.sendall(chunk)
        thread = threading.Thread(target=send)
        thread.start()
        with reader:
            result = list(module.frames(reader, size=8))
        thread.join()
        return result

    def test_fragmented_frames(self):
        self.assertEqual(self.read_chunks([b'abc', b'defghij', b'klmnop']), [b'abcdefgh', b'ijklmnop'])

    def test_interrupted_stream_does_not_corrupt_next_connection(self):
        self.assertEqual(self.read_chunks([b'abcdefgh', b'partial']), [b'abcdefgh'])
        self.assertEqual(self.read_chunks([b'newframe']), [b'newframe'])
