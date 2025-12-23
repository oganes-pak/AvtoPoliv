Вот текст на русском языке с сохранением оформления:

Автоматическая Система Полива и Освещения


  Описание


Данное устройство представляет собой автономную станцию по уходу за растениями на базе микроконтроллера Arduino Nano.

Главная цель проекта — исключить человеческий фактор в уходе за растениями:
  1.	Забывчивость: Растение не засохнет, если вы забудете его полить.
  2.	Перелив: Система не зальет корни, так как полив включается только при реальном высыхании почвы.
  3.	Недостаток света: Фито-лампа автоматически компенсирует нехватку солнечного света в пасмурные дни или вечернее время.

Ключевые функции и возможности. Система работает по принципу «Установил и забыл», выполняя следующий функционал:

- 	🌱 Адаптивный автополив: Насос включается не по таймеру, а по датчику влажности почвы. Используется алгоритм гистерезиса (включение при <60%, выключение при >70%), что предотвращает "болото" в горшке.

- 	💡 Умное освещение: Фоторезистор следит за уровнем света в комнате. Если стало слишком темно, система автоматически включает фито-подсветку.

- 	🛡️ Защита оборудования:

- 	От сухого хода: Если вода в баке кончилась, насос принудительно отключается, чтобы не сгореть.

- 	От помех: Программные фильтры и таймеры задержки (5-10 сек) предотвращают ложные срабатывания и "дребезг" реле.

- 	🖥️ Удобный интерфейс: Все данные (влажность, уровень воды, освещенность) выводятся на OLED-дисплей. Управление настройками и ручное включение приборов осуществляется через удобный поворотный энкодер (крутилку).

- 	🔌 Энергонезависимость: При отключении электричества настройки сохраняются в коде, и система продолжает работу сразу после включения питания.

Список Компонентов

1.	Контроллер: Arduino Nano.
2.	Дисплей: OLED 0.96.
3.	Датчики:
    - 	Влажности почвы: YL-69 (FC-28) + компаратор.
  	
    - 	Уровня воды.
  	
    - 	Освещенности: Фоторезистор  + Резистор 10 кОм.
  	
4.	Управление: Энкодер поворотный KY-040.
5.	Исполнительные устройства:

    - 	Модуль Реле (2 канала, 5V, Low Trigger).
  	
    - 	Насос погружной (5V DC).
  	
    - 	Лампа/Лента фито-света (5V DC).
  	
6.	Питание:

    - 	Блок питания 5V 3A (можно 2).
  	
    - 	Конденсатор электролитический 1000 мкФ 16В (фильтр питания).
  	
 ПИНЫ
 
<img width="467" height="641" alt="image" src="https://github.com/user-attachments/assets/ef3ad3ae-5f79-4bc3-8d11-779cec0d8e9f" />

Код программы

Необходимые библиотеки
Для компиляции кода необходимо установить следующие библиотеки через Менеджер Библиотек Arduino IDE (Sketch -> Include Library -> Manage Libraries...):

  1.	Adafruit SSD1306 (автор: Adafruit) — для работы с OLED дисплеем.
  2.	Adafruit GFX Library (автор: Adafruit) — графическое ядро для дисплея (шрифты, линии, примитивы).
  3.	Wire — (встроена в Arduino IDE) для общения по протоколу I2C.

Прошивка построена по модульному принципу, где каждый блок отвечает за свою задачу. Главный цикл loop() не блокируется задержками delay(), что позволяет системе реагировать на кнопки и датчики мгновенно.

  1. Настройки Пользователя (Константы). В начале кода задаются параметры, которые пользователь может изменить под свои растения без переписывания логики:

- 	SOIL_TARGET_MIN / MAX — границы влажности (60-70%) для включения/выключения полива.

- 	LIGHT_AUTO_THRESHOLD — порог освещенности, ниже которого включается лампа.

- 	DEBOUNCE_TIME — время задержки (5 и 10 секунд), чтобы предотвратить ложные срабатывания при случайных скачках показаний.
  
  2. Блок readSensors() — Чтение данных. Считывает показания с аналоговых входов:
     
- 	Датчик почвы: Перед измерением на пин D12 подается питание, затем делается замер, и питание отключается. Это предотвращает электролиз (гниение) электродов датчика.

- 	Усреднение: Делается 10-20 замеров подряд и вычисляется среднее значение, чтобы убрать "шум" и помехи.
  
  3. Блок runAutoLogic() — Автоматическое управление. Принимает решения на основе данных:

- 	Полив: Работает по принципу гистерезиса. Включает насос, только если влажность упала ниже минимума, и выключает, когда она достигла максимума. Это защищает насос от частого дерганья.

- 	Свет: Использует таймер. Если стало темно, система ждет 10 секунд, чтобы убедиться, что это не тень от прошедшего человека, и только потом включает свет.

- 	Защита: Если датчик воды показывает «Пусто», насос блокируется программно.
    
  4. Блок Интерфейса (Энкодер и Дисплей). Обрабатывает вращение ручки энкодера для навигации по меню:
     
- 	SCREEN_MAIN: Главное меню.

- 	SCREEN_SENSORS: Экран мониторинга (живые данные с датчиков).

- 	SCREEN_MODE: Экран настроек (включение ручного режима, тест насоса).

- 	Реализована защита кнопок от "дребезга" и обработка двойных кликов.
    
  5.	Таймеры и Многозадачность.
- 	Ручной режим (отключается сам через 5 сек).

- 	Обновление экрана, не останавливая проверку датчиков.

- 	Знак тревоги! при нехватке воды.



Примерное подключение


  
  <img width="396" height="308" alt="image" src="https://github.com/user-attachments/assets/e7de54a5-3bd3-46bc-817b-58a2378c9826" />


Ссылки


1. [https://www.ozon.ru/product/adapter-pitaniya-blok-pitaniya-palmexx-5v-4a-hch005-5v-4a-5-5h2-5-1-5-metra-1630652912/](https://www.ozon.ru/product/adapter-pitaniya-blok-pitaniya-palmexx-5v-4a-hch005-5v-4a-5-5h2-5-1-5-metra-1630652912/)
2. [https://www.ozon.ru/product/oled-displey-0-96-128x64-i2c-belyy-945285571/](https://www.ozon.ru/product/oled-displey-0-96-128x64-i2c-belyy-945285571/)
3. [https://www.ozon.ru/product/rezistivnyy-datchik-vlazhnosti-pochvy-yl-69-fc-28-933032880/?at=gpt4Z8MNPCzxEyVQsv3qq2KI0ZEV9LhAgMm2BhjMXQ98](https://www.ozon.ru/product/rezistivnyy-datchik-vlazhnosti-pochvy-yl-69-fc-28-933032880/?at=gpt4Z8MNPCzxEyVQsv3qq2KI0ZEV9LhAgMm2BhjMXQ98)
4. [https://www.ozon.ru/product/datchik-urovnya-nalichiya-vody-466547777/?at=QktJj709Bc99K6jFPy2qQrsN5QA9XtDDBgyxs8Ljm96](https://www.ozon.ru/product/datchik-urovnya-nalichiya-vody-466547777/?at=QktJj709Bc99K6jFPy2qQrsN5QA9XtDDBgyxs8Ljm96)
5. [https://www.ozon.ru/product/fotorezistor-ldr-gl5528-datchik-sveta-10sht-911619604/?at=J8tgExNyphmLE0P5uzj4J6Ju0R1g46t3BwwzYILBqM1z](https://www.ozon.ru/product/fotorezistor-ldr-gl5528-datchik-sveta-10sht-911619604/?at=J8tgExNyphmLE0P5uzj4J6Ju0R1g46t3BwwzYILBqM1z)
6. [https://www.ozon.ru/product/modul-povorotnogo-enkodera-bez-rezby-hw-040-ky-040-5v-30-shagov-na-oborot-dlya-arduino-2147108762/?at=jYtZQgWAlU9KK35lu4vLOy5C4APw0kf4mAllgfYPvgRv](https://www.ozon.ru/product/modul-povorotnogo-enkodera-bez-rezby-hw-040-ky-040-5v-30-shagov-na-oborot-dlya-arduino-2147108762/?at=jYtZQgWAlU9KK35lu4vLOy5C4APw0kf4mAllgfYPvgRv)
7. [https://www.ozon.ru/product/releynyy-modul-2-kanala-s-opticheskoy-razvyazkoy-220v-10a-dlya-arduino-modul-rele-s-optorazvyazkoy-1046491403/](https://www.ozon.ru/product/releynyy-modul-2-kanala-s-opticheskoy-razvyazkoy-220v-10a-dlya-arduino-modul-rele-s-optorazvyazkoy-1046491403/)
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Automatic Watering and Lighting System


  Description


This device is an autonomous plant care station based on the Arduino Nano microcontroller.

The main goal of the project is to eliminate the human factor in plant care:
  1.	Forgetfulness: The plant won't dry out if you forget to water it.
  2.	Overwatering: The system won't flood the roots since watering only turns on when the soil is actually dry.
  3.	Lack of light: The grow light automatically compensates for the lack of sunlight on cloudy days or in the evening.

Key Features and Capabilities. The system operates on a "Set and Forget" principle, performing the following functions:

- 	🌱 Adaptive Auto-Watering: The pump is triggered not by a timer, but by a soil moisture sensor. A hysteresis algorithm is used (ON at <60%, OFF at >70%), which prevents "swamp" conditions in the pot.

- 	💡 Smart Lighting: A photoresistor monitors the light level in the room. If it gets too dark, the system automatically turns on the grow light.

- 	🛡️ Equipment Protection:

- 	Dry Run Protection: If the tank runs out of water, the pump is forcibly disabled to prevent burnout.

- 	Interference Protection: Software filters and delay timers (5-10 sec) prevent false triggering and relay "chatter".

- 	🖥️ User-Friendly Interface: All data (humidity, water level, light) is displayed on an OLED screen. Settings management and manual control are done via a convenient rotary encoder (knob).

- 	🔌 Power Independence: In case of a power outage, settings are saved in the code, and the system resumes operation immediately after power is restored.

Component List

1.	Controller: Arduino Nano.
2.	Display: OLED 0.96.
3.	Sensors:
    - 	Soil Moisture: YL-69 (FC-28) + comparator.
  	
    - 	Water Level.
  	
    - 	Light: Photoresistor + 10 kΩ Resistor.
  	
4.	Control: Rotary Encoder KY-040.
5.	Actuators:

    - 	Relay Module (2 channels, 5V, Low Trigger).
  	
    - 	Submersible Pump (5V DC).
  	
    - 	Grow Lamp/Strip (5V DC).
  	
6.	Power:

    - 	Power Supply 5V 3A (can use 2).
  	
    - 	Electrolytic Capacitor 1000 µF 16V (power filter).
  	
 PINS
 
<img width="467" height="641" alt="image" src="https://github.com/user-attachments/assets/ef3ad3ae-5f79-4bc3-8d11-779cec0d8e9f" />

Program Code

Required Libraries
To compile the code, you need to install the following libraries via the Arduino IDE Library Manager (Sketch -> Include Library -> Manage Libraries...):

  1.	Adafruit SSD1306 (author: Adafruit) — for OLED display operation.
  2.	Adafruit GFX Library (author: Adafruit) — graphics core for the display (fonts, lines, primitives).
  3.	Wire — (built-in to Arduino IDE) for I2C communication.

The firmware is built on a modular principle, where each block is responsible for its own task. The main loop() is not blocked by delay() functions, allowing the system to react to buttons and sensors instantly.

  1. User Settings (Constants). Parameters are defined at the beginning of the code, allowing the user to change them for their plants without rewriting the logic:

- 	SOIL_TARGET_MIN / MAX — humidity limits (60-70%) for turning watering ON/OFF.

- 	LIGHT_AUTO_THRESHOLD — light threshold below which the lamp turns on.

- 	DEBOUNCE_TIME — delay time (5 and 10 seconds) to prevent false positives during random reading spikes.
    
  2. readSensors() Block — Data Reading. Reads readings from analog inputs:
   
- 	Soil Sensor: Power is applied to pin D12 before measurement, then the measurement is taken, and power is turned off. This prevents electrolysis (corrosion) of the sensor electrodes.

- 	Averaging: 10-20 consecutive measurements are taken and the average is calculated to remove "noise" and interference.
  
  3. runAutoLogic() Block — Automatic Control. Makes decisions based on data:

- 	Watering: Works on the hysteresis principle. Turns on the pump only if humidity drops below minimum, and turns off when it reaches maximum. This protects the pump from frequent toggling.

- 	Light: Uses a timer. If it gets dark, the system waits 10 seconds to ensure it's not a shadow from a passing person, and only then turns on the light.

- 	Protection: If the water sensor shows "Empty", the pump is software-locked.
    
  4. Interface Block (Encoder and Display). Processes encoder knob rotation for menu navigation:
     
- 	SCREEN_MAIN: Main menu.

- 	SCREEN_SENSORS: Monitoring screen (live sensor data).

- 	SCREEN_MODE: Settings screen (manual mode toggle, pump test).

- 	Button debounce protection and double-click handling are implemented.
    
  5.	Timers and Multitasking.
- 	Manual mode (turns off automatically after 5 sec).

- 	Screen update without stopping sensor checks.

- 	Alarm sign! when low on water.



Wiring Diagram (Approximate)


  
  <img width="396" height="308" alt="image" src="https://github.com/user-attachments/assets/e7de54a5-3bd3-46bc-817b-58a2378c9826" />


Links


1. [https://www.ozon.ru/product/adapter-pitaniya-blok-pitaniya-palmexx-5v-4a-hch005-5v-4a-5-5h2-5-1-5-metra-1630652912/](https://www.ozon.ru/product/adapter-pitaniya-blok-pitaniya-palmexx-5v-4a-hch005-5v-4a-5-5h2-5-1-5-metra-1630652912/)
2. [https://www.ozon.ru/product/oled-displey-0-96-128x64-i2c-belyy-945285571/](https://www.ozon.ru/product/oled-displey-0-96-128x64-i2c-belyy-945285571/)
3. [https://www.ozon.ru/product/rezistivnyy-datchik-vlazhnosti-pochvy-yl-69-fc-28-933032880/?at=gpt4Z8MNPCzxEyVQsv3qq2KI0ZEV9LhAgMm2BhjMXQ98](https://www.ozon.ru/product/rezistivnyy-datchik-vlazhnosti-pochvy-yl-69-fc-28-933032880/?at=gpt4Z8MNPCzxEyVQsv3qq2KI0ZEV9LhAgMm2BhjMXQ98)
4. [https://www.ozon.ru/product/datchik-urovnya-nalichiya-vody-466547777/?at=QktJj709Bc99K6jFPy2qQrsN5QA9XtDDBgyxs8Ljm96](https://www.ozon.ru/product/datchik-urovnya-nalichiya-vody-466547777/?at=QktJj709Bc99K6jFPy2qQrsN5QA9XtDDBgyxs8Ljm96)
5. [https://www.ozon.ru/product/fotorezistor-ldr-gl5528-datchik-sveta-10sht-911619604/?at=J8tgExNyphmLE0P5uzj4J6Ju0R1g46t3BwwzYILBqM1z](https://www.ozon.ru/product/fotorezistor-ldr-gl5528-datchik-sveta-10sht-911619604/?at=J8tgExNyphmLE0P5uzj4J6Ju0R1g46t3BwwzYILBqM1z)
6. [https://www.ozon.ru/product/modul-povorotnogo-enkodera-bez-rezby-hw-040-ky-040-5v-30-shagov-na-oborot-dlya-arduino-2147108762/?at=jYtZQgWAlU9KK35lu4vLOy5C4APw0kf4mAllgfYPvgRv](https://www.ozon.ru/product/modul-povorotnogo-enkodera-bez-rezby-hw-040-ky-040-5v-30-shagov-na-oborot-dlya-arduino-2147108762/?at=jYtZQgWAlU9KK35lu4vLOy5C4APw0kf4mAllgfYPvgRv)
7. [https://www.ozon.ru/product/releynyy-modul-2-kanala-s-opticheskoy-razvyazkoy-220v-10a-dlya-arduino-modul-rele-s-optorazvyazkoy-1046491403/](https://www.ozon.ru/product/releynyy-modul-2-kanala-s-opticheskoy-razvyazkoy-220v-10a-dlya-arduino-modul-rele-s-optorazvyazkoy-1046491403/)




