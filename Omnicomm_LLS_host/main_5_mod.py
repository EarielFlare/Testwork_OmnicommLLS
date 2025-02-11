from PyQt5 import QtWidgets, uic
from PyQt5.QtSerialPort import QSerialPort, QSerialPortInfo
from PyQt5.QtCore import QIODevice
app = QtWidgets.QApplication([])
ui = uic.loadUi("design.ui")
ui.setWindowTitle("SerialGUI")

serial = QSerialPort()
serial.setBaudRate(19200)  # Измененная скорость порта
serial.setDataBits(QSerialPort.Data8)
serial.setStopBits(QSerialPort.OneStop)
serial.setParity(QSerialPort.NoParity)

# Получение списка доступных портов
port_list = [port.portName() for port in QSerialPortInfo().availablePorts()]
ui.comList.addItems(port_list)

rx_buffer = bytearray()  # Буфер для входящих данных


def on_read():
    while serial.bytesAvailable() >= 9:  # Ждем, пока не придет 9 байт
        data = serial.read(9)  # Читаем 9 байт сразу
        
        # Разбираем данные
        byte1, byte2, byte3, byte4 = data[:4]  # Первые 4 байта
        num1 = int.from_bytes(data[4:6], byteorder='little', signed=False)  # 5-6 байты (2-байтовое число)
        num2 = int.from_bytes(data[6:8], byteorder='little', signed=False)  # 7-8 байты (2-байтовое число)
        crc = data[8]  # 9-й байт (CRC)

        # Выводим в Label
        ui.label_1.setText(f"{byte1:02X}")
        ui.label_2.setText(f"{byte2:02X}")
        ui.label_3.setText(f"{byte3:02X}")
        ui.label_4.setText(f"{byte4:02X}")
        ui.label_5.setText(f"{num1}")
        ui.label_6.setText(f"{num2}")
        ui.label_7.setText(f"{crc:02X}")


def on_send():
    """Функция отправки заранее заданного сообщения (4 байта)."""
    if serial.isOpen():
        message = bytes([0x31, 0xA5, 0x06, 0xCC])
        serial.write(message)
        print(f"Отправлено: {message.hex().upper()}")


def on_open():
    """Открытие COM-порта."""
    serial.setPortName(ui.comList.currentText())
    if serial.open(QIODevice.ReadWrite):  # Чтение + запись
        print("Порт открыт:", serial.portName())
    else:
        print("Ошибка открытия порта")


def on_close():
    """Закрытие COM-порта."""
    serial.close()
    print("Порт закрыт")


# Подключаем обработчики событий
serial.readyRead.connect(on_read)
ui.openButton.clicked.connect(on_open)
ui.closeButton.clicked.connect(on_close)
ui.sendButton.clicked.connect(on_send)  # Добавлена кнопка отправки

ui.show()
app.exec()
