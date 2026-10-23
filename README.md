[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

# Shelly HomeKit Garage BLU

Shelly HomeKit Garage BLU to firmware do sterowania bramą garażową przez Apple HomeKit
na **Shelly 1 Gen3**. Bazuje na otwartym projekcie
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

## Wspierany sprzęt

| Urządzenie | Rola w Garage BLU |
| --- | --- |
| **Shelly 1 Gen3 (S1G3)** | Sterownik bramy, przekaźnik, panel WWW i połączenie z Apple HomeKit przez Wi-Fi. |
| **Shelly BLU Door/Window (SBDW-002C) — 2 sztuki** | Czujniki pełnego zamknięcia i pełnego otwarcia bramy; komunikacja Bluetooth, fabryczny firmware czujników. |

Paczka OTA jest przeznaczona wyłącznie dla Shelly 1 Gen3.
Kompilacja i testy lokalne przeszły; instalacja, parowanie BLE i działanie
na fizycznym sprzęcie pozostają do sprawdzenia.

**Czujników Shelly BLU Door/Window nie trzeba aktualizować ani zmieniać ich
firmware. Mają działać na oryginalnym firmware Shelly — wystarczy sparować je
z modułem w panelu WWW Garage BLU.** Naszą paczkę firmware instaluje się tylko
na sterowniku Shelly 1 Gen3. Działanie parowania na sprzęcie wymaga jeszcze weryfikacji.

## Funkcje Garage BLU

| Funkcja | Zaimplementowane działanie |
| --- | --- |
| Brama w Apple HomeKit | Sterowanie i odczyt stanu w aplikacji Dom, bez dodatkowego mostka. |
| Dwie krańcówki BLE | Osobne czujniki **Closing sensor** i **Opening sensor**. |
| Parowanie przez WWW | Wyszukiwanie czujników, ustawienie PIN-u i konfiguracja bez aplikacji Shelly. |
| Szyfrowanie BTHome | Weryfikacja AES-CCM i ochrona przed ponownym użyciem przechwyconych pakietów. |
| Kontrola wiarygodności stanu | Blokowanie nowych poleceń przy braku, przedawnieniu lub sprzecznych odczytach oraz podczas parowania. |
| Aktualizacje OTA | Pobieranie wydań Garage BLU z hejsiri.pl lub aktualizacja z pliku ZIP. |

Opisane funkcje wymagają weryfikacji na sprzęcie. Blokowanie poleceń nie
zatrzymuje fizycznie bramy będącej już w ruchu i nie zastępuje zabezpieczeń napędu.

## Wkrótce

Planuję dodać obsługę **Shelly BLU H&T** (temperatura i wilgotność)
oraz przycisku **Shelly BLU Button** i czujnika ruchu **Shelly BLU Motion**. Te urządzenia nie są jeszcze obsługiwane
w aktualnym wydaniu firmware.

## Szybki start

Dotyczy wyłącznie **Shelly 1 Gen3 (S1G3)**. Wydanie eksperymentalne;
instalacja OTA i BLE na fizycznym urządzeniu wymagają weryfikacji.

- [Paczki Garage BLU i instrukcja online](https://hejsiri.pl/firmware/garage-blu/)
- [Pełna instrukcja instalacji i aktualizacji](docs/AKTUALIZACJA.md)
- [Pobierz firmware 2.16.1-blu1 (ZIP)](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-Shelly1Gen3-2.16.1-blu1.zip)
- [Suma SHA-256 paczki](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-Shelly1Gen3-2.16.1-blu1.zip.sha256)

### Pierwsza instalacja z fabrycznego firmware Gen3

Otwórz lokalny panel urządzenia i postępuj zgodnie z
[instrukcją dla firmware fabrycznego](docs/AKTUALIZACJA.md#pierwsza-instalacja-z-fabrycznego-firmware-gen3).
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
