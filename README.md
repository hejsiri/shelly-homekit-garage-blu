[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

# Shelly HomeKit Garage BLU

Shelly HomeKit Garage BLU to firmware do sterowania bramą garażową przez Apple HomeKit
na **Shelly 1 Gen3** i **Shelly Plus 1**. Bazuje na otwartym projekcie
[mongoose-os-apps/shelly-homekit](https://github.com/mongoose-os-apps/shelly-homekit),
który udostępnia urządzenia Shelly w aplikacji Dom bez dodatkowego mostka.

Dodano dwa czujniki **Shelly BLU Door/Window** jako potwierdzenie pełnego zamknięcia
i otwarcia bramy, parowanie z panelu WWW bez aplikacji Shelly, szyfrowane komunikaty
BTHome z ochroną przed ponownym użyciem pakietów, blokowanie poleceń przy braku
wiarygodnego stanu czujników oraz kanał OTA na hejsiri.pl. HomeKit działa przez Wi-Fi,
a czujniki komunikują się z modułem przez Bluetooth. Obsługa BLE wymaga jeszcze
weryfikacji na fizycznym sprzęcie.

[Instrukcja instalacji i aktualizacji](docs/AKTUALIZACJA.md) ·
[Paczki OTA na hejsiri.pl](https://hejsiri.pl/firmware/garage-blu/) ·
[Konfiguracja BLE i ograniczenia](README-BLU.md).

## Idea projektu

Chciałem stworzyć prosty sposób na dodanie bramy garażowej do Apple HomeKit
**bez plątaniny kabli prowadzących do czujników**. Bezprzewodowe czujniki BLU
mają ułatwić i przyspieszyć montaż, szczególnie przy istniejącym napędzie,
gdzie prowadzenie dodatkowego okablowania bywa kłopotliwe.

Kluczowe są **dwa czujniki — po jednym dla każdego położenia krańcowego**.
Pierwszy potwierdza, że brama jest całkowicie zamknięta, a drugi, że jest
całkowicie otwarta. W rozwiązaniu z jednym czujnikiem zamknięcia jego
zwolnienie oznacza tylko „brama nie jest zamknięta”; nie potwierdza pełnego
otwarcia. Brama może przecież być w ruchu albo zatrzymać się w połowie.

Zależało mi na tym, by potwierdzenie obu pozycji pochodziło z czujników,
niezależnie od tego, czy bramę uruchomiono z aplikacji Dom, przyciskiem
na ścianie czy zwykłym pilotem radiowym. Dzięki temu naprzemienne korzystanie
z HomeKit i pilota nie wymaga zgadywania położenia na podstawie ostatniej
komendy z aplikacji. **Do takiego potwierdzania obu stanów projekt wymaga
dwóch czujników.** Nie jest to ciągły pomiar położenia bramy między krańcówkami.

W założeniach projektu ważna jest również możliwość montażu czujników
na zewnątrz. Kierunkiem rozwoju jest obsługa
[Shelly BLU Door/Window Outdoor ZB](https://www.shelly.com/pl/products/shelly-blu-door-window-outdoor-zb-ivory),
który według producenta ma obudowę IP55 oraz łączność Bluetooth i Zigbee.
To model **SBDW-302C**, inny niż obecnie zaimplementowany SBDW-002C;
jego parowanie i zgodność z Garage BLU wymagają osobnego dostosowania i testów.
Nie jest jeszcze obsługiwany w aktualnym wydaniu.

Projekt ma być też podstawą do dalszej rozbudowy garażu o inne czujniki
i akcesoria **Shelly BLU**. Dodano obsługę BLU Motion i dwóch BLU H&T
(wewnętrznego i zewnętrznego), z konfiguracją w tym samym panelu WWW.
Obsługiwany jest również BLU Button1: pojedynczy klik steruje bramą, a beacony
pokazują obecność przycisku przechowywanego w aucie. Obsługa czujnika wycieku
i modelu Outdoor ZB pozostaje w planach.

## Wspierany sprzęt

| Urządzenie | Rola w Garage BLU |
| --- | --- |
| **Shelly 1 Gen3 (S1G3)** | Sterownik bramy, przekaźnik, panel WWW i połączenie z Apple HomeKit przez Wi-Fi. |
| **Shelly Plus 1 (Plus1)** | Sterownik bramy na ESP32; osobna paczka firmware, ten sam panel i obsługa BLE. |
| **Shelly BLU Door/Window (SBDW-002C) — 2 sztuki** | Czujniki pełnego zamknięcia i pełnego otwarcia bramy; komunikacja Bluetooth, fabryczny firmware czujników. |
| **Shelly BLU Motion (SBMO-003Z) — 1 sztuka** | Osobny szyfrowany czujnik ruchu w panelu WWW i usługa Motion Sensor w Apple Home; weryfikacja na sprzęcie w toku. |
| **Shelly BLU H&T (SBHT-003C) — do 2 sztuk** | Osobne przypisania Indoor i Outdoor, temperatura oraz wilgotność w WWW i Apple Home; weryfikacja na sprzęcie w toku. |
| **Shelly BLU Button1 — 1 sztuka** | Pojedynczy klik: otwórz / stop / zamknij. Beacon: obecność przycisku w zasięgu BLE i widoczność auta na wizualizacji. |

Każdy sterownik ma własną paczkę OTA. Paczki S1G3 i Plus1 nie są zamienne.
Na Shelly Plus 1 potwierdzono instalację OTA oraz sparowanie dwóch BLU
Door/Window z odbiorem zaszyfrowanych stanów. Sterowanie rzeczywistą bramą,
nowe czujniki i działanie na Shelly 1 Gen3 wymagają dalszych testów sprzętowych.

**Czujników Shelly BLU Door/Window nie trzeba aktualizować ani zmieniać ich
firmware. Mają działać na oryginalnym firmware Shelly — wystarczy sparować je
z modułem w panelu WWW Garage BLU.** Naszą paczkę firmware instaluje się tylko
na sterowniku Shelly 1 Gen3 lub Shelly Plus 1 — wybierz odpowiednią paczkę. Podgląd sparowanych czujników działa także przed włączeniem ich jako krańcówek bramy.

Czujniki Motion i H&T pojawiają się w Apple Home dopiero po bezpiecznym
sparowaniu. Pierwszy potwierdzony zaszyfrowany odczyt odświeża listę usług
HomeKit bez ponownego dodawania sterownika. Sparowany czujnik, który chwilowo
nie odpowiada, pozostaje w HomeKit z informacją o niedostępności.

## Funkcje Garage BLU

| Funkcja | Zaimplementowane działanie |
| --- | --- |
| Brama w Apple HomeKit | Sterowanie i odczyt stanu w aplikacji Dom, bez dodatkowego mostka. |
| Dwie krańcówki BLE | Osobne czujniki **Closing sensor** i **Opening sensor**. |
| Parowanie przez WWW | Automatyczne wykrywanie i konfiguracja; PIN tylko wtedy, gdy wymaga go czujnik. |
| Szyfrowanie BTHome | Weryfikacja AES-CCM i ochrona przed ponownym użyciem przechwyconych pakietów. |
| Kontrola wiarygodności stanu | Blokowanie nowych poleceń przy braku, przedawnieniu lub sprzecznych odczytach oraz podczas parowania. |
| Wizualizacja garażu | Widok dnia/nocy, animacja położenia i kafelki odczytów, dostępne po zalogowaniu; ukryte podczas OTA. |
| Własne nazwy | Pole **Name** pod przyciskiem **Pair** dla każdego sparowanego urządzenia. Zapisane nazwy pojawiają się na wizualizacji; nazwa bramy pochodzi z sekcji Garage Door. |
| Auto na wizualizacji | Nowy zaszyfrowany beacon Button1 pokazuje auto. Brak beaconów przez 45 s je ukrywa. Ręczny przełącznik działa do następnego beacona lub zmiany obecności. |
| Zgodność stanu z HomeKit | Panel otrzymuje ten sam stan i flagę przeszkody co HomeKit. Potwierdzone pełne otwarcie lub zamknięcie kasuje przekroczenie czasu ruchu. |
| Aktualizacje OTA | Pobieranie wydań Garage BLU z hejsiri.pl lub aktualizacja z pliku ZIP. |

Beacony potwierdzają obecność przycisku w zasięgu Bluetooth, a nie dokładne miejsce
zaparkowania. Nie uruchamiają przekaźnika. Animacja pomiędzy krańcówkami szacuje
położenie na podstawie czasu ruchu; potwierdzone pozycje pochodzą z czujników.
Przy statusie **Przeszkody** animacja pokazuje umowne **50%**. Scena pojawia się
dopiero po przygotowaniu kompletu aktualnych grafik wybranego motywu.

Nowe wydanie wymaga sprawdzenia po instalacji na urządzeniu. Blokowanie poleceń nie
zatrzymuje fizycznie bramy będącej już w ruchu i nie zastępuje zabezpieczeń napędu.

## Wkrótce

Planuję dodać obsługę czujnika wycieku Shelly BLU oraz modelu Door/Window Outdoor ZB.
BLU Button1 jest już obsługiwany.

## Szybki start

Bieżące paczki i testy dotyczą **Shelly Plus 1 (Plus1)**. Kod obsługi
**Shelly 1 Gen3** jest zachowany i rozwijany, ale jego kompilacje i testy
są na razie wstrzymane. Nowe funkcje wymagają weryfikacji na sprzęcie.

- [Paczki Garage BLU i instrukcja online](https://hejsiri.pl/firmware/garage-blu/)
- [Pełna instrukcja instalacji i aktualizacji](docs/AKTUALIZACJA.md)
- [Pobierz firmware 2.16.1-blu38 dla Shelly Plus 1 (ZIP)](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-ShellyPlus1-2.16.1-blu38.zip)
- [Suma SHA-256 paczki Plus 1](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-ShellyPlus1-2.16.1-blu38.zip.sha256)

### Pierwsza instalacja z fabrycznego firmware Gen3 / Plus

Otwórz lokalny panel urządzenia i postępuj zgodnie z
[instrukcją dla firmware fabrycznego](docs/AKTUALIZACJA.md#pierwsza-instalacja-z-fabrycznego-firmware-gen3--plus).
Użyj naszej paczki ZIP lub fabrycznego RPC `Shelly.Update` z adresem paczki.
Instrukcja zawiera gotowy adres RPC do uzupełnienia adresem IP urządzenia.
Nie mamy jeszcze potwierdzenia przejścia z fabrycznego 2.1.0 na fizycznym sprzęcie.

### Aktualizacja z istniejącego Shelly-HomeKit

Pobierz powyższy ZIP bez rozpakowywania. W panelu wybierz
**Firmware → Update from file → Upload**, poczekaj na restart i sprawdź wersję.

### Kolejne aktualizacje Garage BLU

W panelu wybierz **Firmware → Update → Check**, a po wykryciu nowszego
wydania — **Install**. Panel korzysta z
[kanału OTA Garage BLU](https://hejsiri.pl/firmware/garage-blu/update.json).
Po restarcie sprawdź wersję i stany obu czujników.

## Dokumentacja i rozwój

- [Instalacja i aktualizacje](docs/AKTUALIZACJA.md)
- [Konfiguracja czujników, bezpieczeństwo, budowanie i testy](README-BLU.md)
- [Kod projektu Garage BLU](https://github.com/hejsiri/shelly-homekit-garage-blu)
- [Bazowy projekt Shelly-HomeKit](https://github.com/mongoose-os-apps/shelly-homekit)

## Autorzy i licencja

Projekt bazuje na pracy autorów Shelly-HomeKit wymienionych w [AUTHORS.md](AUTHORS.md).
Kod jest udostępniany na licencji [Apache 2.0](LICENSE).

H&T Gen3 z ekranem e-ink (S3SN-0U12A) korzysta z Wi-Fi. Nie jest czujnikiem BLU i nie można sparować go powyższym przyciskiem BLE; jego integracja sieciowa wymaga osobnego uruchomienia.
