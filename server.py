from http.server import SimpleHTTPRequestHandler, HTTPServer
import os

class OTAUpdateHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        try:
            # Si la solicitud comienza con '/update', sirve el archivo binario
            if self.path.startswith('/update'):
                firmware_version = "2.1"  # Cambia la versión del firmware según sea necesario
                firmware_path = 'esp32_ota.ino.bin'  # Ruta del archivo binario
                
                if os.path.exists(firmware_path):
                    with open(firmware_path, 'rb') as firmware_file:
                        firmware_data = firmware_file.read()

                    self.send_response(200)
                    self.send_header('Content-Length', str(len(firmware_data)))
                    self.send_header('x-ESP32-http-Update-Version', firmware_version)
                    self.end_headers()
                    self.wfile.write(firmware_data)
                    print(f'Se ha enviado el firmware de la versión {firmware_version}')
                    return
                else:
                    self.send_response(404)
                    self.send_header('Content-Type', 'text/plain')
                    self.end_headers()
                    self.wfile.write(b'File not found')
                    print('Archivo binario no encontrado')
                    return

            # Si la solicitud no comienza con "/update", sirve el contenido del directorio
            else:
                super().do_GET()

        except Exception as e:
            print(f'Error en la solicitud GET: {str(e)}')

def run_server():
    host = '172.30.10.69'
    port = 8080

    try:
        server_address = (host, port)
        httpd = HTTPServer(server_address, OTAUpdateHandler)

        print(f'Servidor OTA iniciado en http://{host}:{port}')
        httpd.serve_forever()

    except Exception as e:
        print(f'Error al iniciar el servidor: {str(e)}')

if __name__ == '__main__':
    run_server()
