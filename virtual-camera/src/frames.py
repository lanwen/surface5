"""Connection-local framing: a disconnected writer cannot poison its successor."""
FRAME_SIZE = 1280 * 720 * 2


def frames(connection, size=FRAME_SIZE):
    buffer = bytearray()
    while True:
        chunk = connection.recv(size - len(buffer))
        if not chunk:
            return  # Discard an incomplete final frame on disconnect.
        buffer.extend(chunk)
        if len(buffer) == size:
            yield buffer
            buffer = bytearray()
