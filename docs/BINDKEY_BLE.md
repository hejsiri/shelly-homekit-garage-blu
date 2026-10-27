# Xiaomi Bindkey BLE

Funkcja jest dostępna od 2.16.1-blu56. Dotyczy oryginalnych, szyfrowanych beaconów MiBeacon v4/v5:

| Model | Miejsce w panelu | Dane |
| --- | --- | --- |
| Xiaomi MCCGQ02HL, identyfikator `0x098B` | Czujnik zamknięcia lub otwarcia | Otwarty/zamknięty, bateria |
| Xiaomi Motion Sensor 2S, identyfikatory `0x3531` i `0x4C60` | Czujnik ruchu | Ruch, jasność, bateria |
| Xiaomi LYWSD03MMC, identyfikator `0x055B` | Temperatura i wilgotność wewnątrz lub na zewnątrz | Temperatura, wilgotność, bateria |

1. Dla ruchu wybierz **Xiaomi Motion Sensor 2S · Bindkey BLE**. Dla krańcówki wybierz **Xiaomi Door & Window Sensor 2 · Bindkey BLE** lub **Xiaomi LYWSD03MMC** i naciśnij **Sparuj**. Dla LYWSD03MMC formularz klucza pojawi się po rozpoznaniu szyfrowanego beacona.
2. Wybudź czujnik. Dla MCCGQ02HL przysuń lub odsuń magnes, aby nadał aktualny stan kontaktronu.
3. Wybierz wykryte urządzenie i wklej jego 32-znakowy klucz szesnastkowy z konta Xiaomi, np. pobrany przez lokalny Xiaomi Cloud Tokens Extractor.
4. Naciśnij **Sprawdź klucz i dodaj**. Nie jest wymagany PIN Shelly ani połączenie GATT z czujnikiem. Konto Xiaomi nie jest obsługiwane przez sam sterownik.

Przed zapisem odbiornik sprawdza autentyczność beaconu AES-CCM z 4-bajtowym MIC i dekoduje obsługiwany pomiar. Nieprawidłowy klucz, inny MAC/model, nieaktualny kandydat lub już odebrany pakiet nie dają potwierdzenia dodania. Po błędzie wywołaj nowy odczyt i rozpocznij próbę ponownie.

Klucz i ostatni zaakceptowany 32-bitowy licznik są zapisywane w NVS, oddzielnie dla każdego przypisania. Duplikaty i starsze liczniki są odrzucane również po restarcie. Licznik jest utrwalany przed udostępnieniem odczytu. Nie ma automatycznej akceptacji zawinięcia lub resetu licznika dla tego samego klucza. Wycofane wpisy protokołów 2/3 z blu46 nie są wczytywane jako nowe przypisania.

Status urządzenia pokazuje protokół `xiaomi_bindkey`, bez klucza. Panel wysyła klucz jedynie w chronionym poleceniu `Shelly.BLU.Bind`; nie zapisuje go w localStorage i usuwa zawartość pola po wysłaniu lub zamknięciu okna. Log wysyłanych poleceń maskuje klucz. Przycisk **Usuń** kasuje zapis NVS, przypisanie i odczyty.

MCCGQ02HL może nadawać rzadko, dlatego sterowanie bramą pozostaje zablokowane po przekroczeniu istniejącego limitu świeżości krańcówki. Pakiet baterii nie potwierdza położenia magnesu i nie przedłuża ważności tego stanu. Sprzeczne stany obu krańcówek nadal blokują sterowanie. Dla temperatury i wilgotności obowiązuje oddzielna ważność po jednej godzinie.

Zakres nie obejmuje innych modeli Xiaomi, urządzeń BLE Mesh/Zigbee ani starszego szyfrowania MiBeacon v2/v3 bez weryfikowanego MIC. Dla LYWSD03MMC zachowano eksperymentalny odczyt GATT bez klucza w ramach tej samej pozycji modelu.

Weryfikacja: publiczne próbki MCCGQ02HL i LYWSD03MMC, rzeczywisty AES-CCM, kontrola MIC/MAC/modelu, błędny klucz, ucięte pakiety, duplikaty po restarcie, błędy zapisu i cofanie przypisania, świeżość kontaktronu oraz formularz PL dla czterech ról. Próba z fizycznymi czujnikami pozostaje do wykonania.

Format i publiczne wektory testowe porównano z projektem [Bluetooth-Devices/xiaomi-ble](https://github.com/Bluetooth-Devices/xiaomi-ble), udostępnianym na [licencji MIT](https://github.com/Bluetooth-Devices/xiaomi-ble/blob/main/LICENSE). Referencje: funkcja `_decrypt_mibeacon_v4_v5`, obiekt `0x1019`, testy `test_Xiaomi_MCCGQ02HL` i `test_Xiaomi_LYWSD03MMC_encrypted`. Klucze w testach są publicznymi danymi tych wektorów, nie danymi użytkownika.

Xiaomi LYWSD03MMC ma jedną pozycję na liście modeli. Szyfrowany MiBeacon uruchamia formularz Bindkey BLE; bez niego zachowano eksperymentalny odczyt GATT.

Motion Sensor 2S: dostępny w paczce Plus 1 2.16.1_blu.63. Sprawdzono publiczną próbkę xiaomi-ble z rzeczywistym AES-CCM (51 lx i ruch), ochronę przed powtórzeniem oraz testy obu identyfikatorów. Brak próby na fizycznym czujniku. Źródło formatu: [xiaomi-ble](https://github.com/Bluetooth-Devices/xiaomi-ble).
