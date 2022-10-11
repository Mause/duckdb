import gzip
import io
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from typing import BinaryIO, Union, IO
from urllib.error import HTTPError
from urllib.parse import urlparse
from urllib.request import urlopen, Request

CSV_RESPONSE = "hello,world\nhello,world\n".encode()
EXTENSION_RESPONSE = gzip.compress(b"this isn't a real extension", compresslevel=9)


class FakeProxyHandler(SimpleHTTPRequestHandler):
    def send_head(self) -> Union[io.BytesIO, BinaryIO, IO[bytes], None]:
        headers = self.headers
        print(self.command, self.path, headers)

        try:
            response = urlopen(Request(self.path, headers=headers))
        except HTTPError as e:
            self.send_response(e.status, e.reason)
            if self.command == 'HEAD':
                print(e.fp.read().decode())
                return None
            return e.fp

        self.send_response(HTTPStatus.OK)
        self.send_header("Content-type", self.guess_type(path))
        self.send_header("Content-Length", str(len(response)))
        self.send_header("Last-Modified", self.date_time_string())
        self.end_headers()
        return io.BytesIO(response)


def main():
    server = ThreadingHTTPServer(("localhost", 8888), FakeProxyHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        server.server_close()
        server.shutdown()


if __name__ == '__main__':
    main()
