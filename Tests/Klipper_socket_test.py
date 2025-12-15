import socket
import json
import time
import math

KLIPPER_SOCKET = "/tmp/printer"

class KlipperSocket:
    def __init__(self, socket_path):
        self.socket_path = socket_path
        self.sock = None

    def connect(self):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.settimeout(2.0)
        try:
            self.sock.connect(self.socket_path)
            print(f"Connected to {self.socket_path}")
            # Читаем приветственное сообщение
            self.receive() 
        except Exception as e:
            print(f"Error connecting: {e}")
            self.sock = None

    def send_gcode(self, gcode):
        if not self.sock:
            return
        # Формат JSON-RPC для Klipper
        payload = {
            "id": int(time.time() * 1000),
            "method": "run_gcode/script",
            "params": {"script": gcode}
        }
        data = json.dumps(payload) + "\x03" # \x03 - разделитель команд
        try:
            self.sock.sendall(data.encode())
            # В реальном PID-цикле ответ можно не ждать для скорости, 
            # но для теста лучше читать
            return self.receive()
        except Exception as e:
            print(f"Send error: {e}")

    def receive(self):
        if not self.sock:
            return None
        try:
            # Читаем чанками. В идеале нужен буфер, но для теста хватит так
            data = self.sock.recv(4096)
            return data.decode(errors='ignore')
        except socket.timeout:
            return None

    def close(self):
        if self.sock:
            self.sock.close()

def generate_circle_path(center_x, center_y, radius, points=50):
    path = []
    for i in range(points + 1):
        angle = 2 * math.pi * i / points
        x = center_x + radius * math.cos(angle)
        y = center_y + radius * math.sin(angle)
        path.append((x, y))
    return path

def main():
    klipper = KlipperSocket(KLIPPER_SOCKET)
    klipper.connect()

    if not klipper.sock:
        return

    print("--- Start H-BOT Test ---")
    
    # 1. Базовые настройки
    # G28 - Домой
    # G90 - Абсолютные координаты
    # G1 F... - Скорость (например, 12000 мм/мин = 200 мм/с)
    init_cmds = [
        "G28",
        "G90",
        "G1 F12000" 
    ]

    for cmd in init_cmds:
        print(f"Sending: {cmd}")
        klipper.send_gcode(cmd)
        # Даем время на парковку
        if "G28" in cmd:
            time.sleep(5) 
        else:
            time.sleep(0.1)

    # 2. Тест движения по кругу (имитация плавного наведения)
    # Центр стола (подставьте свои значения, допустим стол 200x200)
    cx, cy = 60, 60 
    radius = 25
    
    path = generate_circle_path(cx, cy, radius, points=30)

    print("Executing circle motion...")
    start_time = time.time()
    
    for x, y in path:
        # Формируем команду движения
        # ВАЖНО: Для PID регулятора вам придется играть с сегментацией отрезков
        cmd = f"G1 X{x:.2f} Y{y:.2f}"
        klipper.send_gcode(cmd)
        
        # Klipper имеет буфер планировщика (queue).
        # Если слать слишком быстро без M400, он заполнит буфер и будет движение "в будущем".
        # Если слать с M400 - будет дергаться (stop-and-go).
        # Для теста просто шлем поток:
        time.sleep(0.01) 

    # 3. Возврат в центр
    klipper.send_gcode(f"G1 X{cx} Y{cy} F6000")
    
    print(f"Finished. Loop time: {time.time() - start_time:.4f}s")
    klipper.close()

if __name__ == "__main__":
    main()