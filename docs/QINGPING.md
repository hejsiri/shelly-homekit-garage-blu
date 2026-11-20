# Qingping — obsługa od 2.16.1_blu.58

Obsługa jest dostępna od wydania 2.16.1_blu.58.

| Model | Wybór w panelu | Obsługiwane dane |
| --- | --- | --- |
| CGG1 | Temperatura i wilgotność wewnątrz lub na zewnątrz → Qingping CGG1 | Temperatura, wilgotność, bateria |
| CGH1 | Typ czujnika zamknięcia lub otwarcia → Qingping CGH1 | Stan kontaktronu, bateria |
| CGPR1 | Typ czujnika ruchu → Qingping CGPR1 | Ruch, brak ruchu, jasność w luksach, bateria |

Wybierz model i naciśnij **Sparuj**. Wywołaj odczyt: dla CGH1 przesuń magnes, a dla CGPR1 przejdź przed czujnikiem. Panel rozpoznaje natywne pakiety Qingping (`0xFDCD`) i pakiety Xiaomi MiBeacon (`0xFE95`). Gdy wykryty pakiet MiBeacon jest szyfrowany, pojawi się pole na 32-znakowy **Bindkey BLE**. Klucz jest sprawdzany na odebranym pakiecie przed zapisaniem. Nie jest pobierany automatycznie z konta Xiaomi.

Rozpoznawane identyfikatory MiBeacon: CGG1 `0x0347` i `0x0B48`, CGH1 `0x03D6`, CGPR1 `0x0A83`. W natywnym formacie Qingping rozpoznawane są CGG1 `0x01`, `0x07`, `0x16`, CGH1 `0x04` i CGPR1 `0x12`. Warianty Thread/HomeKit, np. CGPR1T, nie są objęte tym dekoderem.

Nieszyfrowane przypisania są zapisywane jako protokół 6, wraz z dokładnym MAC i identyfikatorem modelu; klucz nie jest wymagany. Nie zapewniają kryptograficznego uwierzytelnienia beaconów. Szyfrowane przypisania używają protokołu 7, weryfikacji AES-CCM/MIC i trwałego licznika chroniącego przed ponownym użyciem pakietu. Nie są automatycznie obniżane do odbioru nieszyfrowanego. Status zgłasza odpowiednio `qingping` / `qingping_bindkey` i `secure: false` / `true`. Przycisk **Usuń** czyści zapis i ostatnie odczyty, pomijając nieużywane powiązania transportowe BLE.

CGH1 jako krańcówka bramy potwierdza osiągnięcie położenia tylko wtedy, gdy magnes jest obecny. Otwarty kontaktron lub zgłoszenie pozostawienia otwartego oznacza brak potwierdzenia krańcówki. Nadal obowiązuje dotychczasowy limit świeżości; po jego przekroczeniu lub przy sprzecznych krańcówkach sterowanie pozostaje zablokowane. Sam pakiet baterii nie odświeża stanu kontaktronu.

CGPR1 używa natywnego stanu ruchu lub obiektów MiBeacon `0x000F` / `0x1017`. Czas bez ruchu większy niż 30 sekund w `0x1017` oznacza brak ruchu. Pakiet baterii lub samej jasności nie przedłuża ważności stanu ruchu. W natywnych pakietach zdarzeń nie interpretuje się końcowych bajtów pola ruchu jako jasności — zapobiega to fałszywym skokom luksów. Dla świeżych odczytów jasności wykorzystywana jest istniejąca usługa Light sensor w HomeKit.

Temperatura i wilgotność CGG1 pozostają ważne przez godzinę, niezależnie dla każdego pomiaru. Brakująca temperatura nie jest uzupełniana pakietem wilgotności ani baterii.

Weryfikacja obejmuje dekodery obu formatów, publiczny szyfrowany wektor CGG1, niezależnie wygenerowane pakiety AES-CCM CGH1 i CGPR1, faktyczny kod skanera/parowania i zapisu NVS, izolację MAC/modelu, świeżość stanów, formularz PL, zdjęcia oraz wybór klucza po rozpoznaniu szyfrowania. Potwierdzenie na fizycznych czujnikach pozostaje do wykonania.

Referencje protokołów i wektory: [Bluetooth-Devices/xiaomi-ble](https://github.com/Bluetooth-Devices/xiaomi-ble) i [Bluetooth-Devices/qingping-ble](https://github.com/Bluetooth-Devices/qingping-ble), projekty na licencji MIT. Zdjęcia i ich źródła: `BLU_DEVICE_PHOTOS.json`.
