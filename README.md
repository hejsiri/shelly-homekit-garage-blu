[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Gitter](https://badges.gitter.im/shelly-homekit/community.svg)](https://gitter.im/shelly-homekit/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

# Open Source Apple HomeKit Firmware for Shelly Devices

## Shelly HomeKit Garage BLU — instalacja i OTA

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

Instrukcje i tabela poniżej pochodzą z projektu bazowego. Ich paczki z rojer.me
instalują oryginalne Shelly-HomeKit, bez naszych zmian Garage BLU.

This firmware exposes Shelly devices as Apple HomeKit accessories.

Firmware is compatible with stock and can be uploaded via OTA (Watch a 2 minute [video](https://www.youtube.com/watch?v=BZc-kp4dDRw)), for more info take a look at the flashing wiki [here](https://github.com/mongoose-os-apps/shelly-homekit/wiki/Flashing#updating-from-stock-firmware).

Reverting to stock firmware is also possible [see here](https://github.com/mongoose-os-apps/shelly-homekit/wiki/Flashing#reverting-to-stock-firmware).

## Wspierane urządzenia Garage BLU

Paczka Garage BLU jest przygotowana wyłącznie dla **Shelly 1 Gen3 (S1G3)**
z dwoma czujnikami **Shelly BLU Door/Window (SBDW-002C)**, potwierdzającymi
pełne zamknięcie i otwarcie bramy. Kompilacja i testy lokalne przeszły;
instalacja, parowanie i działanie na sprzęcie pozostają do sprawdzenia.

Poniższa tabela Gen3 opisuje funkcje **oryginalnego Shelly-HomeKit**.
Rozszerzenie Garage BLU nie zostało dodane do pozostałych modeli.
Nie wgrywaj paczki S1G3 do innego urządzenia.

## Urządzenia i funkcje projektu bazowego (bez rozszerzenia Garage BLU)

### Gen 3 Devices

|                                            |[1 G3]   |[1PM G3] |[2PM G3] |[I4 G3]  |[Mini1 G3]|[Mini1PM G3]|[PlugS G3]
|-                                           |-        |-        |-        |-        |-         |-           |-   
|Switch & Co.<sup>1</sup>                    |✓        |✓        |✓        |✗        |✓         |✓           |✓
|Stateless Input<sup>2</sup>                 |✓        |✓        |✓        |✓        |✓         |✓           |✗  
|Sensors<sup>3</sup>                         |✓        |✓        |✓        |✓        |✓         |✓           |✗  
|Garage door opener                          |✓        |✓        |✓        |✗        |✓         |✓           |✗  
|Roller shutter mode                         |✗        |✗        |✓        |✗        |✗         |✗           |✗  
|Power measurement                           |✗        |✓        |✓        |✗        |✗         |✓           |✓
|Temperature/Humidity measurement<sup>4</sup>|✓        |✓        |✓        |✓        |✗         |✗           |✗ 

_Notes:_  
_✓: supported_  
_-: possible but not supported yet_  
_✗: not possible_  
_1: includes lock, outlet and valve_  
_2: includes doorbell_  
_3: includes motion, occupancy, contact, smoke, leak_  
_4: with [Sensor AddOn/Shelly Plus AddOn](https://shop.shelly.cloud/temperature-sensor-addon-for-shelly-1-1pm-wifi-smart-home-automation#312)_ and DS18B20 sensor(s) (maximum 5 for Shelly Plus Addon, maximum 3 for Sensor AddOn) or 1 DHT sensor

Features that are not yet supported:
 * Cloud connections: no Shelly Cloud, no MQTT
 * Remote actions (web hooks)
 * Valve with timer support

## Quick Start

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

## Documentation

See our [Wiki](https://github.com/mongoose-os-apps/shelly-homekit/wiki).

## Getting Support

If you'd like to report a bug or a missing feature, please use [GitHub issue tracker](https://github.com/mongoose-os-apps/shelly-homekit/issues).

Some of us can be found in the [Gitter chat room](https://gitter.im/shelly-homekit/community).

## Contributions and Development

Code contributions are welcome! Check out [open issues](https://github.com/mongoose-os-apps/shelly-homekit/issues) and feel free to pick one up.

See [here](https://github.com/mongoose-os-apps/shelly-homekit/wiki/Development) for development environment setup.

## Authors

See [here](AUTHORS.md).

## Support

If you like to show support for the project and support further development, consider a Donation to the current maintainer markib via [Paypal](https://www.paypal.com/donate/?hosted_button_id=RVFA9G5VMXRX8)

## License

This firmware is free software and is distributed under [Apache 2.0 license](LICENSE).

[1]: https://www.shelly.cloud/en/products/shop/1xs1
[+1]: https://www.shelly.cloud/en/products/shop/shelly-plus-1
[+1Mini]: https://www.shelly.cloud/en/products/shop/shelly-plus-1-mini
[+1PMMini]: https://www.shelly.cloud/en/products/shop/shelly-plus-1pm-mini
[+Uni]: https://www.shelly.cloud/en/products/shop/shelly-plus-uni
[Mini1 G3]: https://www.shelly.cloud/en/products/shop/shelly-1-mini-gen-3
[Mini1PM G3]: https://www.shelly.cloud/en/products/shop/shelly-1-pm-mini-gen3
[2PM G3]: https://www.shelly.cloud/en/products/shop/shelly-2pm-gen3
[1PM G3]: https://www.shelly.cloud/en/products/shop/shelly-1pm-gen3
[1 G3]: https://www.shelly.cloud/en/products/shop/shelly-1-gen3
[I4 G3]: https://www.shelly.cloud/en/products/shop/shelly-i4-gen3
[PlugS G3]: https://www.shelly.com/products/shelly-plug-s-gen3
[+1PM]: https://www.shelly.cloud/en/products/shop/shelly-plus-1-pm-2-pack/shelly-plus-1-pm
[+2PM]: https://www.shelly.cloud/en/products/shop/shelly-plus-2-pm
[+RGBWPM]: https://www.shelly.cloud/en/products/shop/shelly-plus-rgbw-pm
[+Plug S]: https://www.shelly.cloud/en/products/shop/shelly-plus-plug-s
[1L]: https://www.shelly.cloud/en/products/shop/shelly-1l
[Plug]: https://www.shelly.cloud/en/products/shop/1xplug
[PlugS]: https://www.shelly.cloud/en/products/shop/shelly-plug-s
[AC]: https://www.shelly.cloud/en-de/products/product-overview/splusi4x1
[DC]: https://www.shelly.cloud/en-de/products/product-overview/shelly-plus-i4-dc
[UNI]: https://www.shelly.cloud/en/products/shop/shelly-uni-1
[RGBW2]: https://www.shelly.cloud/en/products/shop/shelly-rgbw2-1
[Duo RGBW]: https://www.shelly.cloud/en/search?query=%22Shelly+Duo+-+RGBW%22
[Duo]: https://www.shelly.cloud/en/search?query=%22Shelly+Duo%22
[Vintage]: https://www.shelly.cloud/en/search?query=vintage
