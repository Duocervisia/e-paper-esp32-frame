try:
    import usocket as socket
except ImportError:
    import socket

try:
    import ubinascii as ubinascii
except ImportError:
    import binascii as ubinascii


class MQTTException(Exception):
    pass


class MQTTClient:
    def __init__(self, client_id, server, port=0, user=None, password=None, keepalive=0, ssl=False, ssl_params=None):
        self.client_id = client_id if isinstance(client_id, bytes) else str(client_id).encode()
        self.server = server
        self.port = port or 1883
        self.user = user
        self.password = password
        self.keepalive = keepalive
        self.ssl = ssl
        self.ssl_params = ssl_params or {}
        self.sock = None
        self.pid = 0

    def _encode_str(self, value):
        if isinstance(value, bytes):
            payload = value
        else:
            payload = str(value).encode()
        return bytes([len(payload) >> 8, len(payload) & 0xFF]) + payload

    def _encode_len(self, length):
        encoded = bytearray()
        while True:
            digit = length % 128
            length //= 128
            if length > 0:
                digit |= 0x80
            encoded.append(digit)
            if length == 0:
                break
        return encoded

    def _send(self, payload):
        sent = 0
        while sent < len(payload):
            written = self.sock.send(payload[sent:])
            if not written:
                raise MQTTException("MQTT socket closed while sending")
            sent += written

    def _read_exact(self, length):
        data = bytearray()
        while len(data) < length:
            chunk = self.sock.recv(length - len(data))
            if not chunk:
                raise MQTTException("MQTT socket closed while receiving")
            data.extend(chunk)
        return data

    def connect(self, clean_session=True):
        addr = socket.getaddrinfo(self.server, self.port)[0][-1]
        self.sock = socket.socket()
        self.sock.connect(addr)
        self.sock.settimeout(10)

        flags = 0x02 if clean_session else 0x00
        if self.user is not None:
            flags |= 0x80
        if self.password is not None:
            flags |= 0x40

        variable_header = bytearray()
        variable_header.extend(self._encode_str("MQTT"))
        variable_header.append(0x04)
        variable_header.append(flags)
        variable_header.append((self.keepalive >> 8) & 0xFF)
        variable_header.append(self.keepalive & 0xFF)

        payload = bytearray()
        payload.extend(self._encode_str(self.client_id))
        if self.user is not None:
            payload.extend(self._encode_str(self.user))
        if self.password is not None:
            payload.extend(self._encode_str(self.password))

        remaining_length = len(variable_header) + len(payload)
        packet = bytearray(b"\x10")
        packet.extend(self._encode_len(remaining_length))
        packet.extend(variable_header)
        packet.extend(payload)
        self._send(packet)

        response = self._read_exact(4)
        if len(response) < 4 or response[0] != 0x20 or response[1] != 0x02 or response[3] != 0x00:
            raise MQTTException("MQTT connection failed")

    def publish(self, topic, msg, retain=False, qos=0):
        if self.sock is None:
            raise MQTTException("MQTT client not connected")
        if qos not in (0, 1):
            raise MQTTException("Only QoS 0 and QoS 1 are supported")

        topic = topic if isinstance(topic, bytes) else str(topic).encode()
        msg = msg if isinstance(msg, bytes) else str(msg).encode()
        header = 0x30 | (qos << 1) | (0x01 if retain else 0x00)
        payload = bytearray()
        payload.extend(self._encode_str(topic))
        packet_id = 0
        if qos == 1:
            self.pid = (self.pid % 65535) + 1
            packet_id = self.pid
            payload.append(packet_id >> 8)
            payload.append(packet_id & 0xFF)
        payload.extend(msg)
        packet = bytearray([header])
        packet.extend(self._encode_len(len(payload)))
        packet.extend(payload)
        self._send(packet)

        if qos == 1:
            response = self._read_exact(4)
            received_id = (response[2] << 8) | response[3]
            if response[0] != 0x40 or response[1] != 0x02 or received_id != packet_id:
                raise MQTTException("Invalid MQTT PUBACK")

    def disconnect(self):
        if self.sock is not None:
            try:
                self._send(b"\xE0\x00")
            except Exception:
                pass
            try:
                self.sock.close()
            finally:
                self.sock = None
