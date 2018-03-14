===============
arduino_gps_gsm
===============

Удаленный запуск подогрева мотора по звонку или SMS и GPS-трекер. К Arduino
подключено реле. Реле подключено к пульту подогревателя eberspacher  параллельно
кнопке влючения. Включается на 20 минут по звонку или на заданное время через
SMS. К Ардуино подключен GPS-модуль для отправки координат на сервер. Сервер
написан на Python (https://github.com/parshin/save_gps_data), данные хранятся в MySQL.
Может быть использован для управления различными устройствами, к примеру на даче.

Обрудование:
1. arduino duemilanove
	https://www.arduino.cc/en/Main/arduinoBoardDuemilanove

2. GSM shield Hilo. 
	https://www.cooking-hacks.com/documentation/tutorials/arduino-gprs-quadband

3. GPS shield with external antenna
	https://www.cooking-hacks.com/gps-module-for-arduino
	https://www.cooking-hacks.com/external-gps-antenna-5877

4. Power supply DC-DC 12v-5v MEAN WELL SKA15A-05
	http://www.meanwell.com/webapp/product/search.aspx?prod=SKA15


Remote start engine preheater and GPS-tracker. It's connected to eberspacher preheater using rely connected to the "start" button. Starts preheater with a mobile or landline phone using phone calls for 20 minutes or defined time through SMS. Also sends GPS-coords to my server. Python script (https://github.com/parshin/save_gps_data) receives data and writes it in MySQL database.

It can be used for remote control any devices, e.g. country house
