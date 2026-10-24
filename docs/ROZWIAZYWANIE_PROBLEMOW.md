# Rozwiązywanie problemów Garage BLU

Dla **2.16.1-blu46 / Shelly Plus 1 i Shelly 1 Gen3**, stan: **2026-10-24**.

[Instalacja i aktualizacje](AKTUALIZACJA.md) ·
[Konfiguracja i obsługa](KONFIGURACJA.md) · [Historia zmian](HISTORIA_ZMIAN.md)

## Najpierw sprawdź wersje i odczyty

Version w Firmware dotyczy sterownika Garage BLU. Wersja BLU, np. 1.0.23,
dotyczy osobnego czujnika lub przycisku. Przy diagnozie podawaj obie osobno.

Sprawdź MAC wybranej roli, status, baterię, RSSI oraz liczbę sekund od odczytu.
Nowy odczyt z bieżącym stanem i małym wiekiem danych jest mocniejszym potwierdzeniem
niż sam fakt, że adres MAC jest zapisany w panelu.

## Parowanie BLU

### Rozłączenie 0x208 lub 0x213

Są to komunikaty zakończonej sesji BLE. Sam kod nie rozstrzyga, czy problemem jest
uśpienie, okno parowania, powiązanie BLE lub inne zachowanie urządzenia.

1. Zbliż BLU do sterownika i sprawdź baterię.
2. Czytaj etap modala. Gdy prosi o ponowne wejście w tryb parowania, przytrzymaj przycisk ponad 10 s i puść.
3. Czekaj na automatyczne wykrycie tego samego urządzenia. Nie uruchamiaj równolegle parowania w aplikacji Shelly.
4. Gdy próba zakończy się Pairing stopped, zamknij modal i sprawdź świeże odczyty przed ponowieniem.

Ponowne wejście w tryb parowania nie jest obowiązkowym drugim krokiem każdej próby.
Sterownik prosi o nie wtedy, gdy poprzednia sesja lub okno parowania już wygasły.

### BLE security failed, np. status 0x505

Ten komunikat opisuje nieudane zabezpieczenie nowego połączenia BLE. Nie dowodzi
sam w sobie utraty wcześniej zapisanego klucza BTHome ani nie wskazuje jednoznacznie
błędnego PIN-u.

Jeżeli panel informuje o Recent encrypted readings, sprawdź zwykłe odczyty urządzenia.
Mogą działać z zapisanym kluczem mimo błędu nowej próby parowania. Nie nazywaj tej
nowej próby udaną; osobno potwierdź poprawny stan i świeżość danych.
Jeśli odczytów brak, ponów jedną próbę z urządzeniem w trybie parowania i zachowaj
pełny komunikat diagnostyczny. Początek logu security_start=0x0 oznacza rozpoczęcie
procedury, a nie jej pomyślne zakończenie.

### Dlaczego modal pyta o PIN

PIN nie jest stałym pierwszym krokiem. Pole istniejącego PIN-u pojawia się dla
urządzenia wymagającego takiego uwierzytelnienia. Encryption PIN pojawia się wtedy,
gdy urządzenie potrzebuje PIN-u do aktywacji szyfrowania zamiast bezpośredniego zapisu
klucza. Użyj PIN-u wskazanego przez modal; nowy PIN ma sześć cyfr, od 100000 do 999999.

Jeżeli poprzednia próba już zmieniła PIN urządzenia, jego aktualnym PIN-em może być
właśnie ten ostatnio ustawiony. PIN-u nie należy zgadywać ani publikować w zgłoszeniu.
Sterownik nie przechowuje go jako pola konfiguracji do późniejszego odczytania.

### Waiting for encrypted beacon

Żółty badge oznacza zapisane zabezpieczone przypisanie bez potwierdzonego świeżego
odczytu w bieżącej sesji. Obudź BLU lub wywołaj normalny odczyt; w Door/Window można
zmienić stan magnesu. Długie przytrzymanie stosuj wtedy, gdy modal wyraźnie prosi
o tryb parowania. Samo połączenie GATT nie kończy parowania.

### Reset fabryczny urządzenia BLU jako ostatnia opcja

Reset usuwa jego ustawienia i parowania, więc wymaga ponownej konfiguracji.
Producent opisuje reset po ponownym włożeniu baterii i przytrzymaniu przycisku
ponad 30 s. Przed wykonaniem sprawdź [instrukcję właściwego modelu i wspólne funkcje BLU](https://shelly-api-docs.shelly.cloud/docs-ble/common/#factory-reset).
Nie myl tego z wejściem w tryb parowania ani z resetem całego sterownika.

Reset zewnętrznego BLU nie usuwa automatycznie przypisania z Shelly. Ponowna próba
Garage BLU dotyczy wybranej roli i adresu; nie wymaga kasowania wszystkich urządzeń.

## Czujniki otwarcia i zamknięcia

### MAC jest widoczny, ale odczyt nie wraca

Sprawdź wiek danych, baterię i status bezpiecznego przypisania. Sama wymiana baterii
nie gwarantuje nowego poprawnego zaszyfrowanego pakietu. Wywołaj zmianę kontaktronu,
zbliż czujnik i sprawdź, czy pojawia się nowy odczyt i maleje jego wiek. Dopiero przy braku działających
odczytów przejdź do ponownego Pair i diagnostyki modala.

### Opening sensor pokazuje Closed

Closed opisuje obecność magnesu w kontaktronie. Dla Opening sensor oznacza to
osiągnięcie krańca pełnego otwarcia, a nie zamknięcie bramy.
Sprawdź [przypisanie ról](KONFIGURACJA.md#dwie-krańcówki-doorwindow).

### Sterowanie jest zablokowane lub widzę No data

Przy włączonym BLU sensors wymagane są dwa różne, zabezpieczone przypisania
Closing sensor / Opening sensor i świeże odczyty. Sprawdź, czy oba kontaktrony
nie są aktywne jednocześnie oraz czy Sensor timeout nie został przekroczony.
Aktywne parowanie także wstrzymuje gotowość krańcówek BLU.

### Obstruction mimo dojścia do krańcówki

W blu38 świeże, zgodne wskazanie którejkolwiek krańcówki usuwa timeout i przeszkodę
w HomeKit. Jeśli status pozostaje, sprawdź oba odczyty: właściwa krańcówka musi być
aktywna, druga nieaktywna, a oba odczyty świeże. Brama nie odzyska wiarygodnego stanu
z samego MAC-a ani z nieaktualnego pakietu. Przy Obstruction grafika 50% jest umowna.

Jeżeli panel i aplikacja Dom nadal pokazują różne stany, zapisz wersję, oba odczyty,
ich wiek i zrzut statusu przed następną próbą.

## Button1 i obraz auta

### Auto nie odpowiada ręcznemu przełącznikowi

Nowy zaszyfrowany pakiet Button1 nadpisuje ręczny wybór. To zamierzone działanie.
Przy regularnych beaconach auto pojawi się ponownie. Bez nowych pakietów przez 45 s
obecność znika. Odbiór w zasięgu BLE nie dowodzi dokładnego miejsca postoju auta.

### Button1 nie steruje bramą

Obsługiwany jest świeży pojedynczy nacisk. Zwykły beacon obecności nie steruje
przekaźnikiem. Sprawdź przypisanie Button1, nowy odczyt i gotowość krańcówek;
przy niepewnym stanie bramy polecenie może zostać zablokowane. Podwójny i długi
klik wysyłają zdarzenia HomeKit, bez bezpośredniego sterowania przekaźnikiem.

### Nie widać przycisku lub obecności w aplikacji Dom

1. Zainstaluj blu42 lub nowszy i sprawdź bezpieczne przypisanie Motion i Button1 w panelu. W blu40 występował mieszany układ usług i akcesoriów mostkowanych; w logach Domu po blu41 wykryto obecność przypisaną do identyfikatora ruchu. Blu42 zastępuje te trzy usługi nowymi identyfikatorami, zachowując samą bramę i parowania.
2. Po aktualizacji uruchom sterownik ponownie i pozwól aplikacji Dom odświeżyć konfigurację.
   Już sparowanego Button1 nie trzeba parować ponownie. Nie resetuj HomeKit.
3. Otwórz ustawienia akcesorium Shelly i jego listę Akcesoria. Czujnik obecności używa Car name, a przycisk Name pod Button1; bez własnych nazw
   szukaj Car presence i Button. Dom może zachować wcześniej nadane własne nazwy.
4. Po migracji sprawdź automatyzacje ruchu i obecności. W ustawieniach przycisku przypisz akcje do pojedynczego, podwójnego i długiego
   naciśnięcia. Zdarzenia HomeKit nie wymagają gotowości krańcówek; bezpośrednie sterowanie
   bramą pojedynczym kliknięciem nadal jej wymaga.
5. Brak obecności po 45 sekundach bez beacona jest prawidłowy. Ręczne pokazanie auta
   na grafice nie zmienia stanu czujnika w Domu. Brak Bluetooth lub nasłuchu BLE daje błąd odczytu.

Jeśli usługi nadal są niedostępne, nie resetuj HomeKit. Uwierzytelnione
`Shelly.GetDebugInfo` zawiera od blu42 sekcję HAP database z rzeczywistymi usługami
uruchomionymi na sterowniku. Dla AID 1 oczekiwane są: Motion Sensor — usługa 6144,
odczyt 6146; Occupancy Sensor — usługa 6400, odczyt 6402; Button — usługa 6656,
zdarzenie 6658. Zestaw to z wersją, gotowością BLU i dokładnym komunikatem z Domu.

Powtórzenia, niepoprawne pakiety, beacony bez zdarzenia i naciśnięcia podczas
parowania nie uruchamiają akcji przycisku w HomeKit.

### Brak odczytu Light sensor

Od blu43 usługa korzysta tylko ze sparowanego BLU Motion. Po restarcie poczekaj
na jego nowy szyfrowany pakiet z LUX; nie trzeba ponownie parować urządzenia.
Brakujący lub starszy niż Sensor timeout pomiar daje Status Fault i nieaktywny status.
Pakiet bez LUX nie odświeża wieku poprzedniego pomiaru. W Shelly.GetDebugInfo
sekcja BLU Motion light podaje gotowość i lux100 (odczyt podzielony przez 100 daje lx).
Usługa światła ma IID 6912, a jej odczyt IID 6914 i typ 6B.

## Aktualizacja zatrzymana na Uploading

1. Poczekaj na odpowiedź lub komunikat błędu. W blu38 wysłanie ma limit 120 s i panel nie ponawia go automatycznie.
2. Sprawdź, czy Shelly nadal odpowiada, czy trwa aktualizacja i jaka jest Version po ponownym otwarciu strony. Timeout może wystąpić już po rozpoczęciu zapisu.
3. Po zakończeniu zapisu i restarcie odśwież stronę. Jeśli nowa wersja jest obecna, nie wysyłaj paczki ponownie.
4. Gdy wersja się nie zmieniła, zapisz błąd. Sprawdź ZIP dla swojego modelu, zalogowanie i połączenie; możesz użyć Update from file → Upload.

W starszym panelu sam napis Uploading... mógł pozostać po nieudanym żądaniu.
Zawsze weryfikuj rzeczywistą Version i stan urządzenia, zamiast opierać się na spinnerze.
Przy błędzie pobierania sprawdź [ZIP i sumę SHA-256](AKTUALIZACJA.md#pobieranie).
Nie odłączaj zasilania w trakcie zapisu. System → Reset i Revert to Stock nie są
krokami zwykłej aktualizacji.

## Wygląd, nazwy i logowanie

- Nazwa bramy pochodzi z Name w Garage Door, nie z Name w General Settings.
- Nazwy urządzeń BLU zapisuje Save w sekcji bramy; własne polskie nazwy nie są automatycznie tłumaczone.
- Krańcówki nie są pokazywane nad garażem. Ich statusy są w ustawieniach.
- Przy haśle dashboard ładuje się po zalogowaniu; podczas aktualizacji jest ukryty.
- Jeśli widzisz dawny dom, auto lub polskie statusy systemowe, sprawdź zainstalowaną wersję i odśwież stronę po OTA. Blu38 nie ładuje starszych grafik startowych.
- Język natywnego okna wyboru pliku zależy od przeglądarki i systemu.

## Co dołączyć do zgłoszenia

Użyj [Issues repozytorium Garage BLU](https://github.com/hejsiri/shelly-homekit-garage-blu/issues).
Podaj model sterownika, Version i Build, rolę i model BLU, wersję jego firmware,
etap modala, dokładny błąd oraz świeżość, baterię i RSSI odczytów. Wyjaśnij, czy
urządzenie działało wcześniej i czy problem dotyczy ponownego parowania.

Nie publikuj haseł, PIN-ów, kluczy BTHome, kodu HomeKit ani kopii konfiguracji
z takimi danymi. MAC można zamaskować, zachowując możliwość rozróżnienia urządzeń.

## Xiaomi LYWSDCGQ/01ZM nie pojawia się lub brakuje pomiarów

- Zainstaluj co najmniej blu45 i wybierz **Xiaomi LYWSDCGQ/01ZM** przy właściwym
  **Indoor H&T** albo **Outdoor H&T**, zanim klikniesz **Pair**.
- Włącz czujnik i zbliż go do Shelly. Odbiór jest pasywny: nie uruchamiaj trybu
  parowania i nie wpisuj PIN-u. Poczekaj na zwykłe beacony z pomiarami.
- Przy kilku czujnikach wybierz właściwy MAC. LYWSD03MMC wymaga osobnej opcji **Xiaomi LYWSD03MMC (stock)**; modele Zigbee/Mesh
  nie są obsługiwane przez te opcje.
- Sprawdź baterię i czy inna aplikacja nie utrzymuje aktywnego połączenia z czujnikiem;
  zamknij podgląd czujnika w aplikacji Xiaomi na czas próby.
- **Waiting for temperature and humidity** wymaga obu pomiarów, nie samej baterii.
  Pakiet baterii nie odświeża ważności temperatury ani wilgotności.
- W zgłoszeniu podaj Version, rolę Indoor/Outdoor i tekst modalu. Debug Info zawiera
  dla obu ról stan przypisania i osobną gotowość temperatury/wilgotności.

## Xiaomi LYWSD03MMC (stock): oczekiwanie albo błąd bindkey

- Wybierz **Xiaomi LYWSD03MMC (stock)**, nie LYWSDCGQ. Potrzebny jest aktualny
  32-znakowy bindkey urządzenia aktywowanego w Xiaomi Home.
- Odszyfrowanie weryfikuje MAC i klucz. **Cannot decrypt Xiaomi readings** zwykle
  oznacza bindkey innego urządzenia lub klucz zmieniony po ponownym dodaniu do Xiaomi.
- Możesz wpisać MAC ręcznie, aby pominąć wykrywanie. Nadal trzeba poczekać na
  zaszyfrowane pomiary: okno wynosi 15 minut, pakiety mogą być oddalone o kilka minut.
- Zbliż czujnik, sprawdź baterię i zamknij aktywne połączenie z nim w aplikacji Xiaomi.
  Nie resetuj czujnika tylko po to, aby rozpocząć odbiór na Shelly.
- Po wymianie baterii lub resecie licznika stare pakiety z tym samym kluczem mogą
  być odrzucane przez ochronę przed powtórzeniami. Wtedy potrzebna może być
  ponowna aktywacja z nowym bindkey. Samo ponowne Pair nie zeruje ochrony.
- PVVX/ATC, pakiety nieszyfrowane i inne modele nie są obsługiwane przez opcję stock.
- Do zgłoszenia dołącz komunikat oraz wersję firmware, bez bindkey, haseł i PIN-ów.
