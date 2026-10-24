# Rozwiązywanie problemów Garage BLU

Dla **2.16.1-blu38 / Shelly Plus 1**, stan: **2026-10-24**.

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
przy niepewnym stanie bramy polecenie może zostać zablokowane. Nie szukaj osobnej
usługi przycisku w HomeKit — w tej wersji Button1 steruje usługą bramy.

## Aktualizacja zatrzymana na Uploading

1. Poczekaj na odpowiedź lub komunikat błędu. W blu38 wysłanie ma limit 120 s i panel nie ponawia go automatycznie.
2. Sprawdź, czy Shelly nadal odpowiada, czy trwa aktualizacja i jaka jest Version po ponownym otwarciu strony. Timeout może wystąpić już po rozpoczęciu zapisu.
3. Po zakończeniu zapisu i restarcie odśwież stronę. Jeśli nowa wersja jest obecna, nie wysyłaj paczki ponownie.
4. Gdy wersja się nie zmieniła, zapisz błąd. Sprawdź ZIP dla Plus1, zalogowanie i połączenie; możesz użyć Update from file → Upload.

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
