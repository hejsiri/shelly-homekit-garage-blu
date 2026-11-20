# Xiaomi Honeywell Smoke Detector — JTYJ-GD-03MI/BB

To czujnik dymu z Bluetooth, rozpoznawany jako MiBeacon `0x0997`. Nie mierzy gazu ani czadu.

W panelu wybierz **Smoke sensor → Pair**, naciśnij przycisk testowy czujnika i wprowadź jego 32-znakowy **Bindkey BLE**. Klucz jest sprawdzany na odebranej szyfrowanej ramce przed zapisaniem. Przycisk **Remove** usuwa klucz i przypisanie.

Dane w panelu i HomeKit:

- alarm dymu oraz potwierdzenie jego ustąpienia,
- poziom baterii, gdy nadejdzie odpowiedni pakiet,
- siła sygnału i czas ostatniego odczytu w panelu,
- usługa **Smoke Sensor** w HomeKit, z sygnalizacją niedostępnego lub nieaktualnego stanu.

Każdy poprawnie odszyfrowany beacon potwierdza łączność i gotowość czujnika. Panel pokazuje wtedy **Gotowy**, baterię i RSSI. Sam beacon życia ani pakiet przycisku/baterii nie oznacza alarmu ani jego ustąpienia: stan zmienia się wyłącznie po odebraniu pola stanu dymu. Brak nowej ramki nie kasuje odebranego alarmu. Zdalne wyciszanie i uruchamianie autotestu nie są obsługiwane.

Po braku świeżego, poprawnie odszyfrowanego beacona HomeKit zgłasza usterkę łączności. Obsługa rozróżnia beacon życia od stanu alarmu; potwierdzenie tego zachowania na fizycznym JTYJ-GD-03MI/BB wymaga sprawdzenia po aktualizacji.

Referencje: [modele xiaomi-ble](https://github.com/Bluetooth-Devices/xiaomi-ble/blob/main/src/xiaomi_ble/devices.py), [testy protokołu](https://github.com/Bluetooth-Devices/xiaomi-ble/blob/main/tests/test_parser.py).
