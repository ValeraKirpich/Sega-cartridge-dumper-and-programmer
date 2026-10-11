//-------------------------------------------------------------------
// НАСТРОЙКА ПИНОВ 
//-------------------------------------------------------------------

#define PIN_LED PC13

// ФИЗИЧЕСКИЕ ПИНЫ УПРАВЛЕНИЯ ПАМЯТЬЮ
#define PIN_CE  PB12  
#define PIN_OE  PB13  
#define PIN_WE  PB14
#define PIN_POWERSUPPLY  PB11

// СДВИГОВЫЕ РЕГИСТРЫ 74HC595 (УПРАВЛЕНИЕ АДРЕСОМ)
#define LATCH_PIN PA4  
#define CLOCK_PIN PA5  
#define DATA_PIN  PA7

// ШИНА ДАННЫХ (D0 - D15)
const int dataPins[16] = 
{
  PA0, PA1, PA2, PA3, PA6, PA8, PA9, PA10,    // D0 - D7
  PB0, PB1, PB5, PB6, PB7, PB8, PB9, PB10    // D8 - D15
};

//-------------------------------------------------------------------
// ПРОЦЕДУРЫ ДЛЯ РАБОТЫ С ПОРТАМИ 
//-------------------------------------------------------------------

// Управление подачей питания
void setPowerSupply(bool applyPower)
{
  if (applyPower)
  {
    pinMode(PIN_POWERSUPPLY, OUTPUT);
    digitalWrite(PIN_POWERSUPPLY, LOW);
    delay(5); // На всякий случай, чтоб успел проснуться чип и подняться напряжение 12в
  }
  else 
  {
    pinMode(PIN_POWERSUPPLY, INPUT);
  }
}

// НАСТРОЙКА РЕЖИМА ШИНЫ ДАННЫХ (INPUT или OUTPUT)
void setDataBusMode(bool mode) 
{
  for (int i = 0; i < 16; i++) 
  {
      pinMode(dataPins[i], mode);
  }
}

// ЗАПИСЬ СЛОВА НА ФИЗИЧЕСКИЕ ПИНЫ ДАННЫХ
void writeDataBus(uint16_t value) 
{
  for (int i = 0; i < 16; i++) 
  {
    digitalWrite(dataPins[i], (value >> i) & 1);
  }
}

// ЧТЕНИЕ СЛОВА С ФИЗИЧЕСКИХ ПИНОВ ДАННЫХ
uint16_t readDataBus() 
{
  uint16_t value = 0;

  for (int i = 0; i < 16; i++) 
  {
    if (digitalRead(dataPins[i]) == HIGH)
      value |= (1 << i);
  }
  return value;
}

// ВЫСТАВЛЕНИЕ АДРЕСА ЧЕРЕЗ СДВИГОВЫЕ РЕГИСТРЫ 74HC595
void setAddress(uint32_t address) 
{
  byte reg1 = 0; byte reg2 = 0; byte reg3 = 0;
  if (address & (1 << 0)) 
    reg3 |= (1 << 7);
  if (address & (1 << 1)) 
    reg3 |= (1 << 6);
  if (address & (1 << 2)) 
    reg3 |= (1 << 5);
  if (address & (1 << 3)) 
    reg3 |= (1 << 4);
  if (address & (1 << 4)) 
    reg3 |= (1 << 3);
  if (address & (1 << 5)) 
    reg3 |= (1 << 2);
  if (address & (1 << 6)) 
    reg3 |= (1 << 1);
  if (address & (1 << 7)) 
    reg3 |= (1 << 0);

  if (address & (1 << 8))  
    reg2 |= (1 << 7);
  if (address & (1 << 9))  
    reg2 |= (1 << 6);
  if (address & (1 << 10)) 
    reg2 |= (1 << 5);
  if (address & (1 << 11)) 
    reg2 |= (1 << 4);
  if (address & (1 << 12)) 
    reg2 |= (1 << 3);
  if (address & (1 << 13)) 
    reg2 |= (1 << 2);
  if (address & (1 << 14)) 
    reg2 |= (1 << 1);
  if (address & (1 << 15)) 
    reg2 |= (1 << 0);

  if (address & (1 << 16)) 
    reg1 |= (1 << 7);
  if (address & (1 << 17)) 
    reg1 |= (1 << 6);
  if (address & (1 << 18)) 
    reg1 |= (1 << 5);
  if (address & (1 << 19)) 
    reg1 |= (1 << 4);
  if (address & (1 << 20)) 
    reg1 |= (1 << 3);
  if (address & (1 << 21)) 
    reg1 |= (1 << 2);
  if (address & (1 << 22)) 
    reg1 |= (1 << 1);

  digitalWrite(LATCH_PIN, LOW);
  delayMicroseconds(2);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, reg3);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, reg2);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, reg1);
  delayMicroseconds(2);
  digitalWrite(LATCH_PIN, HIGH);
  delayMicroseconds(2);
}

// ПРОЦЕДУРА ПОШАГОВОГО ИНТЕРАКТИВНОГО ТЕСТИРОВАНИЯ ВСЕХ ЛИНИЙ СЛОТА
void flashPinsStepTest()
{
  Serial.println(F("START_PINS_TEST"));

  // 1. Принудительно переводим абсолютно все порты программатора на ВЫХОД
  pinMode(PIN_CE, OUTPUT);
  pinMode(PIN_OE, OUTPUT);
  pinMode(PIN_WE, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  setDataBusMode(OUTPUT);

  // Сбрасываем все выходы в чистый ноль (GND)
  digitalWrite(PIN_CE, LOW);
  digitalWrite(PIN_OE, LOW);
  digitalWrite(PIN_WE, LOW);
  digitalWrite(LATCH_PIN, LOW);
  digitalWrite(CLOCK_PIN, LOW);
  digitalWrite(DATA_PIN, LOW);
  writeDataBus(0x0000);
  setAddress(0x000000);

  // Массив всех физических управляющих и сигнальных пинов STM32, идущих к слоту
  int testControlPins[] = { PIN_CE, PIN_OE, PIN_WE };
  const char* controlNames[] = { "PIN_CE", "PIN_OE", "PIN_WE" };

  // Шаг А: Тестируем прямые линии управления и 595 регистра
  for (int i = 0; i < 3; i++)
  {
    // Включаем 3.3V строго на текущем пине
    digitalWrite(testControlPins[i], HIGH);
    
    // Сообщаем ПК, какой пин сейчас под напряжением
    Serial.print(F("ACTIVE:"));
    Serial.println(controlNames[i]);

    // Ждем от ПК команду 'n' (Next), чтобы включить следующий пин
    while (true)
    {
      if (Serial.available() > 0)
      {
        char c = Serial.read();
        if (c == 'n' || c == 'N') break;
      }
    }

    digitalWrite(testControlPins[i], LOW);
  }

  // Шаг Б: Тестируем 16 линий шины данных D0 - D15
  for (int i = 0; i < 16; i++)
  {
    // Зажигаем строго один i-й бит на шине данных
    uint16_t busValue = (1 << i);
    writeDataBus(busValue);

    Serial.print(F("ACTIVE:D"));
    Serial.print(i);

    while (true)
    {
      if (Serial.available() > 0)
      {
        char c = Serial.read();
        if (c == 'n' || c == 'N') break;
      }
    }

    writeDataBus(0x0000); // Гасим шину в ноль
  }

  // Шаг В: Тестируем 20 линий шины адреса A0 - A19 через сдвиговые регистры 595
  // Напомним, что "A0 флешки -> A1 слота Сеги", но функция setAddress идеально транслирует биты числа!
  for (int i = 0; i < 23; i++)
  {
    // Зажигаем строго один i-й бит адреса (числа 1, 2, 4, 8, 16...)
    uint32_t addressValue = (1UL << i);
    setAddress(addressValue); // Регистры 595 выдадут 3.3В на нужную дорожку

    Serial.print(F("ACTIVE:A"));
    Serial.println(i+1);

    while (true)
    {
      if (Serial.available() > 0)
      {
        char c = Serial.read();
        if (c == 'n' || c == 'N') break;
      }
    }

    setAddress(0x000000); // Гасим адресную шину в ноль после шага
  }

  // Возвращаем программатор в безопасный исходный режим чтения ПЗУ
  setDataBusMode(INPUT);
  digitalWrite(PIN_CE, LOW);
  digitalWrite(PIN_OE, HIGH);
  digitalWrite(PIN_WE, HIGH);
  Serial.println(F("TEST_COMPLETE"));
}

//-------------------------------------------------------------------
// ПРОЦЕДУРЫ РАБОТЫ С ПАМЯТЬЮ 
//-------------------------------------------------------------------

// НИЗКОУРОВНЕВАЯ ЗАПИСЬ КОМАНДЫ (СЛОВА) ВО ФЛЕШ
void flashWriteWord(uint32_t addr, uint16_t data) 
{
  setAddress(addr);
  setDataBusMode(OUTPUT);
  writeDataBus(data);
  delayMicroseconds(2); // Даем адресам и данным устояться
  
  digitalWrite(PIN_WE, LOW);
  delayMicroseconds(2); // Длина импульса записи должна быть не менее 35-50 нс
  digitalWrite(PIN_WE, HIGH);
  delayMicroseconds(2);

  setDataBusMode(INPUT);
}

// НИЗКОУРОВНЕВОЕ ЧТЕНИЕ СЛОВА ИЗ ФЛЕШ
uint16_t flashReadWord(uint32_t addr) 
{
  setAddress(addr);
  delayMicroseconds(2); // Даем адресу на 595 стабилизироваться
  
  digitalWrite(PIN_OE, LOW);
  delayMicroseconds(2); // Даем флешке честные 1000 нс (ей нужно всего 70 нс), чтобы выставить данные
  
  uint16_t data = readDataBus();
  
  digitalWrite(PIN_OE, HIGH);
  delayMicroseconds(2);

  return data;
}

// СБРОС ФЛЕШКИ В РЕЖИМ ЧТЕНИЯ
void flashReset() 
{
  flashWriteWord(0x0000, 0x00F0); // Команда Reset (F0H)
  delay(5);
}

// ФУНКЦИЯ ЧТЕНИЯ мануфактуры (SILICON ID)
void flashReadInfo() 
{
  Serial.println(F("READ_INFO..."));

  digitalWrite(PIN_LED, LOW);

  setPowerSupply(true);
  
  flashWriteWord(0x555, 0x00AA); // 1-й цикл разблокировки
  flashWriteWord(0x2AA, 0x0055); // 2-й цикл разблокировки
  flashWriteWord(0x555, 0x0090); // 3-й цикл: Команда Auto Select (90H)
  delay(5);
  
  // Читаем данные автовыбора
  // В 16-битном режиме адресация идет по словам:
  uint16_t manufCode = flashReadWord(0x0000); // Manufacturer Code (Адрес 0000h)
  uint16_t deviceCode = flashReadWord(0x0001); // Device Code (Адрес 0001h)

  // НОВЫЙ ШАГ: Читаем статус защиты Сектора 4 (SA4, адрес слова 0x08000)
  // По спецификации к адресу сектора подмешиваем A1=1, A0=0, то есть + 0x0002
  uint16_t protectStatus = flashReadWord(0x08000 + 0x0002);
  
  Serial.print(F("Manufacturer code: 0x"));
  if (manufCode < 0x10) 
    Serial.print(F("0"));
  Serial.println(manufCode, HEX);
  
  Serial.print(F("Device code: 0x"));
  Serial.println(deviceCode, HEX);

  // Выводим человеческий статус аппаратной защиты кремния
  Serial.print(F("Sector status: 0x"));
  Serial.println(protectStatus, HEX);

  // Младший бит (Q0) равен 1 - сектор заперт железно, равен 0 - сектор открыт!
  if (protectStatus & 0x0001)
  {
    Serial.println(F("The memory is locked. Writing and erasing are not possible. Most likely, 12V has not been applied to the reset pin."));
  }
  else
  {
    Serial.println(F("Memory unlocked"));
  }

  Serial.println(F("READ_INFO_OK"));
  
  // Выходим из режима автовыбора обратно в режим чтения данных
  flashReset();

  setPowerSupply(false);
  digitalWrite(PIN_LED, HIGH);
}

// ПРОЦЕДУРА СКОРОСТНОГО БИНАРНОГО ЧТЕНИЯ ДАМПА ПАМЯТИ НА ПК
void flashReadBlock()
{
  uint8_t sizeBuf[4];

  digitalWrite(PIN_LED, LOW);

  // Вычитываем 4 байта размера дампа от ПК
  int received = 0;

  while (received < 4)
  {
    if (Serial.available() > 0)
    {
      sizeBuf[received] = Serial.read();
      received++;
    }
  }

  // Склеиваем байты в чистые 32-битные числа
  uint32_t totalBytesToRead = ((uint32_t)sizeBuf[0] << 24) | ((uint32_t)sizeBuf[1] << 16) | ((uint32_t)sizeBuf[2] << 8) | sizeBuf[3];

  // Переводим параметры в масштаб 16-битных слов флешки
  uint32_t totalWords = totalBytesToRead / 2;

  // Жестко переводим шину данных программатора в режим чтения
  setDataBusMode(INPUT);
  setPowerSupply(true); // Гарантируем подачу питания на картридж
  flashReset(); // Сбрасываем логику чипа

  // Сигнализируем ПК, что параметры приняты и сейчас полетит бинарный поток
  Serial.println(F("START_DUMP"));

  // Скоростной выплеск байт в шину USB
  for (uint32_t i = 0; i < totalWords; i++)
  {
    uint16_t wordData = flashReadWord(i);l:\Projects\Схемы\Приставки\sega\Cartridge\Программатор + картридж\firmware — копия\firmware.ino

    // Шлем в исходном Big-Endian формате (сначала старший байт, затем младший)
    uint8_t highByte = (wordData >> 8) & 0xFF;
    uint8_t lowByte = wordData & 0xFF;

    Serial.write(highByte);
    Serial.write(lowByte);
  }

  setPowerSupply(false); // Отключаем питание после операции
  digitalWrite(PIN_LED, HIGH);
}

// Полное стирание 
void flashFullErase() 
{
  Serial.println(F("ERASE"));

  digitalWrite(PIN_LED, LOW);
  setPowerSupply(true);
  
  flashReset(); // Сброс чипа

  // ОТПРАВЛЯЕМ КОМАНДУ ПОЛНОГО bulk-erase
  flashWriteWord(0x555, 0x00AA); 
  flashWriteWord(0x2AA, 0x0055); 
  flashWriteWord(0x555, 0x0080); 
  flashWriteWord(0x555, 0x00AA); 
  flashWriteWord(0x2AA, 0x0055); 
  flashWriteWord(0x555, 0x0010); // Финальный аккорд: стираем ВСЁ и ВЕЗДЕ!
  delay(5);

  uint32_t eraseStartTime = millis();
  uint32_t timeoutLimit = 60000; // Экстренный таймаут 35 секунд (типично трется за 25с)
  
  Serial.println();

  while (true)
  {
    Serial.print(F("."));

    uint16_t currentStatus = flashReadWord(0x00000);

    // Проверяем Бит 7 (Q7) флага готовности JEDEC
    if (currentStatus & (1 << 7)) 
    {
      break; // ПОЛНЫЙ УСПЕХ! Чип рапортует об очистке всей матрицы!
    }

    // Защита от зависания, если 12В не поданы или провод отвалился
    if (millis() - eraseStartTime > timeoutLimit) 
    {
      Serial.println(F("ERASE_INTERRUPTED"));
      break;
    }
    
    delay(10); // Небольшая пауза, чтобы не перегревать шину непрерывными запросами
  }

  flashReset(); // Принудительный финальный сброс для финализации и перевода ног в Z-состояние

  setPowerSupply(false);
  digitalWrite(PIN_LED, HIGH);

  Serial.println();
  Serial.println(F("ERASE_OK"));
}

// Поблочная запись данных
// ПРОЦЕДУРА ПОБАЙТОВОЙ ЗАПИСИ ДАННЫХ ИГРЫ С КОНТРОЛЕМ КАЖДОГО БАЙТА
void flashWriteBlock()
{
  uint8_t usbBuffer[256];
  
  digitalWrite(PIN_LED, LOW);
  // Включаем силовую часть программатора (3.3В + 12В на RESET)
  setPowerSupply(true);
  flashReset(); // Сброс логики флешки перед записью

  // Сигнализируем ПК-приложению, что STM32 на связи и готова к пинг-понгу
  Serial.println(F("READY_TO_RECEIVE"));
  
  // Начинаем писать строго с нулевого адреса слова (корень памяти)
  uint32_t wordAddress = 0x00000; 

  while (true)
  {
    // Ожидаем заголовок пакета от компьютера (команду 'b' или 's')
    while (Serial.available() == 0);
    char packetHeader = Serial.read();
    
    // Если ПК прислал 's' или 'S' (Stop) -> прошивка файла игры завершена
    if (packetHeader == 's' || packetHeader == 'S') 
    {
      break;
    }
    
    // Если ПК прислал 'b' или 'B' (Block) -> начинаем побайтовый прием 256 байт
    if (packetHeader == 'b' || packetHeader == 'B') 
    {
      // Вычитываем ровно 256 байт в режиме жесткого пинг-понга
      for (int bytesReceived = 0; bytesReceived < 256; bytesReceived++)
      {
        // Ждем, пока компьютер физически пришлет один очередной байт
        while (Serial.available() == 0);
        usbBuffer[bytesReceived] = Serial.read();
        
        // КРИТИЧЕСКИЙ ШАГ: Мгновенно отправляем ПК подтверждение '+' (Байт принят!)
        Serial.print('+'); 
      }
      
      // Сюда мы долетаем только тогда, когда ВСЕ 256 байт проверены и лежат в буфере!
      // Запускаем цикл аппаратного прожига 128 слов во флешку
      for (int i = 0; i < 256; i += 2)
      {
        uint16_t wordData = (usbBuffer[i] << 8) | usbBuffer[i + 1];
        
        flashWriteWord(0x555, 0x00AA); // 1 цикл разблокировки
        flashWriteWord(0x2AA, 0x0055); // 2 цикл разблокировки
        flashWriteWord(0x555, 0x00A0); // 3 цикл: Команда записи (A0H)
        
        flashWriteWord(wordAddress, wordData); // 4-й цикл: пишем наше слово
        delayMicroseconds(15); // Время на кристаллизацию ячейки
        
        wordAddress++;
      }
      
      // Отправляем текстовый маркер, что прожиг блока в кремний успешно завершен!
      Serial.println(F("BLOCK_BURN_OK")); 
    }
  }
  
  // Финализируем: сбрасываем флешку и отключаем силовые 12В
  flashReset(); //
  setPowerSupply(false); //
  digitalWrite(PIN_LED, HIGH);
  
  Serial.println(F("FLASH_COMPLETE"));
}

//-------------------------------------------------------------------
// БАЗОВЫЕ ПРОЦЕДУРЫ АРДУИНО 
//-------------------------------------------------------------------

void setup() 
{
  Serial.begin(115200);
  while(!Serial);
  
  // Светодиод
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

  setDataBusMode(INPUT); // Инициализация пинов данных
  
  // Инициализация пинов управления
  pinMode(PIN_CE, OUTPUT);
  pinMode(PIN_OE, OUTPUT);
  pinMode(PIN_WE, OUTPUT);
  
  // Инициализация пинов сдвигового регистра
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  
  // Исходное состояние: чип выбран (CE=LOW), чтение и запись запрещены (OE=HIGH, WE=HIGH)
  digitalWrite(PIN_CE, LOW);
  digitalWrite(PIN_OE, HIGH);
  digitalWrite(PIN_WE, HIGH);
  
  // Исходное состояние сдвиговых регистров
  digitalWrite(LATCH_PIN, LOW);
  digitalWrite(CLOCK_PIN, LOW);
  digitalWrite(DATA_PIN, LOW);
}

void loop() 
{
  // Ждем, пока в буфер COM-порта прилетит хотя бы один байт команды
  if (Serial.available() > 0) 
  {
    char incomingChar = Serial.read();
    
    // Элегантный переключатель команд со стороны ПК приложения
    switch (incomingChar) 
    {
      // Сброс памяти
      case 'T':
      {
        Serial.print(F("SEGA_PROG_OK"));
        break;
      }

      // КОМАНДА 't': Запуск пошагового интерактивного теста пинов слота
      case 't':
      {
        flashPinsStepTest();
        break;
      }

      // Сброс памяти
      case 'P':
      {
        setPowerSupply(true);
        break;
      }

      // Сброс памяти
      case 'p':
      {
        setPowerSupply(false);
        break;
      }

      // Сброс памяти
      case 'r':
      {
        flashReset();
        break;
      }

      // Чтение идентификаторов чипа
      case 'i':
      {
        flashReadInfo();
        break;
      }
      
      // Полное стирание чипа
      case 'e':
      {
        flashFullErase();
        break;
      }
      
      // Запуск поблочной прошивки РОМа игры с ПК
      case 'w':
      {
        flashWriteBlock();
        break;
      }

      // Высокоскоростное бинарное скачивание дампа памяти на ПК
      case 'd':
      {
        flashReadBlock();
        break;
      }

      default:
      {
        // Просто игнорируем или пишем предупреждение (по желанию)
        Serial.println(F("Unknown command: "));
        Serial.print(incomingChar);
        Serial.println(F("'"));
        break;
      }
    }
  }
}
