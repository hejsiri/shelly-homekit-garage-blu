/* Browser-local UI language. Device configuration, RPC payloads and user names
 * always retain their original values. The same file is bundled into both pages. */
(function () {
  'use strict';
  const translations = {
    'This device requires Wi-Fi power saving mode 1 or 2 while Bluetooth is enabled.':'To urządzenie wymaga trybu oszczędzania Wi-Fi 1 lub 2 przy włączonym Bluetooth.',
    'Show in dashboard':'Pokazuj w dashboard',
    'Temperature':'Temperatura','Humidity':'Wilgotność','Garage doors':'Drzwi garażu',
    'Contact sensor':'Czujnik kontaktowy','Light':'Światło','Smoke':'Dym',
    'Contacts open':'Otwarte','Contacts closed':'Zamknięte','Obstacles':'Przeszkody',
    'Bluetooth enabled':'Bluetooth włączony','Bluetooth starting':'Bluetooth uruchamia się',
    'Disconnected':'Rozłączono','Update available':'Dostępna aktualizacja','Overheating':'Przegrzanie','Failsafe mode':'Tryb awaryjny',
    'Password:':'Hasło:','Forgot password?':'Nie pamiętasz hasła?','Log In':'Zaloguj','Log Out':'Wyloguj',
    'General Settings':'Ustawienia ogólne','Name:':'Nazwa:','Mode:':'Tryb:','Save':'Zapisz','System':'System','Language:':'Język:',
    'Device ID:':'Identyfikator urządzenia:','Model:':'Model:','Version:':'Wersja:','Build:':'Kompilacja:','Uptime:':'Czas pracy:',
    'HomeKit Settings':'Ustawienia HomeKit','Setup':'Konfiguracja','Paired:':'Sparowano:','Connections:':'Połączenia:',
    'Reset':'Resetuj','Reboot':'Uruchom ponownie','WiFi Settings':'Ustawienia Wi-Fi','Network:':'Sieć:','Enable:':'Włącz:',
    'Static IP:':'Stały adres IP:','Gateway:':'Brama sieciowa:','Netmask:':'Maska podsieci:','Namesever:':'Serwer DNS:',
    'Power saving:':'Oszczędzanie energii:','Security Settings':'Ustawienia zabezpieczeń','Old Password:':'Obecne hasło:',
    'New Password:':'Nowe hasło:','Confirm:':'Potwierdź:','Firmware':'Oprogramowanie','Update:':'Aktualizacja:',
    'Read temperature and humidity now':'Odczytaj teraz temperaturę i wilgotność','Reading thermometer...':'Odczytywanie termometru…','Check':'Sprawdź','Install':'Zainstaluj','Update from file:':'Aktualizacja z pliku:','Upload':'Wgraj',
    'Revert to Stock:':'Przywróć fabryczne oprogramowanie:','Revert':'Przywróć','Debug Log:':'Dziennik diagnostyczny:',
    'Enable permanently:':'Włącz na stałe:','Log':'Dziennik','Device:':'MAC:','Device':'Urządzenie','Sensor':'Czujnik',
    'Status:':'Stan:','Value:':'Wartość:','Temperature:':'Temperatura:','Power:':'Moc:','Offset:':'Korekta:',
    'Calibration:':'Kalibracja:','Calibrate':'Kalibruj','Update Interval:':'Odstęp aktualizacji:',
    'Garage Door':'Brama garażowa','Garage Door Opener':'Sterownik bramy garażowej','Garage dashboard':'Panel garażu',
    'Garage door controls':'Sterowanie bramą garażową','Garage with an animated door':'Garaż z animowaną bramą',
    'Garage — Tesla Model S 2023':'Garaż — Tesla Model S 2023','Sensor readings':'Odczyty czujników','Sensors':'Czujniki',
    'Bluetooth gate sensors':'Czujniki bramy Bluetooth','BLU sensors:':'Czujniki BLU:','BLU sensors':'Czujniki BLU','Closing sensor:':'Czujnik zamknięcia:',
    'Opening sensor:':'Czujnik otwarcia:','Motion sensor:':'Czujnik ruchu:','Indoor H&T:':'Temperatura i wilgotność wewnątrz:',
    'Outdoor H&T:':'Temperatura i wilgotność na zewnątrz:','Car name:':'Nazwa auta:','Car name':'Nazwa auta',
    'Car':'Auto','Car in the garage':'Auto w garażu','Name of car':'Nazwa auta','Sensor timeout:':'Limit ważności odczytu:',
    'Remove':'Usuń','Pair':'Sparuj','Pair device':'Parowanie urządzenia','Start pairing':'Rozpocznij parowanie','Cancel':'Anuluj','Close':'Zamknij',
    'Current device PIN:':'Obecny PIN urządzenia:','Current device PIN':'Obecny PIN urządzenia','Encryption PIN:':'PIN szyfrowania:',
    'Encryption PIN':'PIN szyfrowania','Device already has a PIN?':'Urządzenie ma już PIN?','6 digits':'6 cyfr',
    'Existing 6-digit PIN':'Obecny 6-cyfrowy PIN','(optional)':'(opcjonalnie)','(empty)':'(puste)',
    'Indoor thermometer type':'Typ termometru wewnętrznego','Outdoor thermometer type':'Typ termometru zewnętrznego',
    'Closed device name':'Nazwa czujnika zamknięcia','Open device name':'Nazwa czujnika otwarcia',
    'Motion device name':'Nazwa czujnika ruchu','Indoor device name':'Nazwa czujnika wewnętrznego',
    'Outdoor device name':'Nazwa czujnika zewnętrznego','Button device name':'Nazwa przycisku',
    'Place the activated Xiaomi clock near Shelly and enter its 32-character BLE bindkey.':'Umieść aktywowany zegar Xiaomi blisko Shelly i wpisz jego 32-znakowy klucz BLE (bindkey).',
    'Xiaomi E-Ink Clock (BHR5435GL) · without key':'Zegar Xiaomi E-Ink (BHR5435GL) · bez klucza',
    'Xiaomi E-Ink Clock (BHR5435GL) · BLE bindkey':'Zegar Xiaomi E-Ink (BHR5435GL) · klucz BLE',
    'Place the clock near Shelly and close any Bluetooth app connected to it. Shelly saves the clock after receiving a temperature and humidity reading.':'Umieść zegar blisko Shelly i zamknij aplikacje połączone z nim przez Bluetooth. Shelly zapisze zegar po odebraniu temperatury i wilgotności.',
    'Clock read timed out; close other Bluetooth apps and keep it near Shelly':'Zegar nie przesłał pomiaru w wyznaczonym czasie. Umieść go blisko Shelly i zamknij aplikacje połączone z nim przez Bluetooth.',
    'Xiaomi E-Ink Clock (BHR5435GL)':'Zegar Xiaomi E-Ink (BHR5435GL)',
    'Xiaomi LYWSD03MMC (experimental)':'Xiaomi LYWSD03MMC (eksperymentalnie)',
    'Continue previous direction after stop:':'Kontynuuj poprzedni kierunek po zatrzymaniu:',
    'Continue previous direction after stop':'Kontynuuj poprzedni kierunek po zatrzymaniu',
    "Match this setting to how your garage door opener responds to a pulse. This option does not change the opener's programming.":'Dopasuj ustawienie do reakcji napędu na impuls. Ta opcja nie zmienia programu napędu.',
    'Pair each BLU Door/Window sensor here. Encryption and beacon mode are configured automatically. A magnet at either endpoint means that position has been reached.':'Sparuj tutaj oba czujniki BLU Door/Window. Szyfrowanie i nadawanie beaconów zostaną skonfigurowane automatycznie. Wykrycie magnesu przez czujnik krańcowy potwierdza osiągnięcie danego położenia.',
    'Presence beacon: the button is within BLE range. Keep it in your car. Single press: open / stop / close. Beacons never trigger the garage door.':'Beacon obecności oznacza, że przycisk jest w zasięgu BLE. Przechowuj go w aucie. Pojedynczy nacisk: otwórz / stop / zamknij. Beacony nigdy nie uruchamiają bramy.',
    'Not paired':'Nie sparowano','Waiting for thermometer reading':'Oczekiwanie na odczyt termometru',
    'Waiting for beacon':'Oczekiwanie na beacon','Waiting for encrypted beacon':'Oczekiwanie na zaszyfrowany beacon',
    'Closed':'Zamknięta','Open':'Otwarta','Opening…':'Otwieranie…','Closing…':'Zamykanie…','Stopped':'Zatrzymana',
    'No data':'Brak danych','Unknown state':'Nieznany stan','Obstruction':'Przeszkoda','Loading…':'Wczytywanie…',
    'In range':'W zasięgu','Out of range':'Poza zasięgiem','Ready':'Gotowy','No recent beacon':'Brak świeżego beacona','Pressed':'Naciśnięto',
    'Motion':'Ruch','No motion':'Brak ruchu','Motion detected':'Wykryto ruch','Detected':'Wykryto','Not detected':'Nie wykryto',
    'Day':'Dzień','Night':'Noc','Appearance':'Wygląd','Show car':'Pokaż auto','Hide car':'Ukryj auto',
    'Open garage door':'Otwórz bramę','Close garage door':'Zamknij bramę','Stop garage door':'Zatrzymaj bramę',
    'Garage with a Tesla Model S':'Garaż z Teslą Model S','Empty garage':'Pusty garaż',
    'BLU button detected within range — this does not confirm the parking location':'Przycisk BLU wykryty w zasięgu — nie potwierdza to miejsca parkowania',
    'Temperature and humidity':'Temperatura i wilgotność','Motion sensor':'Czujnik ruchu','Sensor contact':'Kontaktron',
    'Battery':'Bateria','Signal unknown':'Brak danych o sygnale',
    'Wired sensors selected':'Wybrano czujniki przewodowe','BLU sensors ready':'Czujniki BLU gotowe',
    'BLU sensors missing, stale or conflicting — controls blocked':'Brak aktualnych lub spójnych odczytów BLU — sterowanie zablokowane',
    'Choose a device':'Wybierz urządzenie','Current PIN required':'Wymagany obecny PIN','Step 1 of 2 · Find device':'Krok 1 z 2 · Wyszukaj urządzenie',
    'Step 2 of 2 · Set up device':'Krok 2 z 2 · Skonfiguruj urządzenie','Pairing complete':'Parowanie zakończone','Pairing stopped':'Parowanie zatrzymane',
    'This device needs a six-digit PIN to enable encrypted readings. Enter a new PIN to finish setup.':'To urządzenie wymaga 6-cyfrowego PIN-u do włączenia szyfrowanych odczytów. Wprowadź nowy PIN, aby zakończyć konfigurację.',
    'This device requires its existing PIN to connect. Enter it once to continue.':'Połączenie z tym urządzeniem wymaga jego obecnego PIN-u. Wprowadź go, aby kontynuować.',
    'More than one matching device is nearby. Select the one you want to pair.':'W pobliżu jest kilka pasujących urządzeń. Wybierz urządzenie do sparowania.',
    'Keep the Xiaomi LYWSD03MMC powered on near Shelly. Encrypted broadcasts require a BLE bindkey; otherwise Shelly will try a direct reading. Close other Bluetooth apps. Direct reading compatibility is experimental.':'Umieść włączony Xiaomi LYWSD03MMC blisko Shelly. Szyfrowane beacony wymagają klucza Bindkey BLE; w przeciwnym razie Shelly spróbuje bezpośredniego odczytu. Zamknij inne aplikacje Bluetooth. Bezpośredni odczyt jest eksperymentalny.',
    'Keep the Xiaomi LYWSDCGQ/01ZM powered on near Shelly. It broadcasts automatically; no pairing mode or PIN is needed. One detected device is added automatically. If several match, select yours.':'Umieść włączony Xiaomi LYWSDCGQ/01ZM blisko Shelly. Termometr nadaje automatycznie — nie wymaga trybu parowania ani PIN-u. Jedno wykryte urządzenie zostanie dodane automatycznie. Jeśli jest ich kilka, wybierz swoje.',
    'Hold the device button for more than 10 seconds, then release it. Keep it near Shelly. Pairing continues automatically when one matching device is detected.':'Przytrzymaj przycisk urządzenia ponad 10 sekund i puść. Trzymaj je blisko Shelly. Parowanie będzie kontynuowane automatycznie po wykryciu jednego pasującego urządzenia.',
    "Hold the same device's button for more than 10 seconds, then release it. Pairing resumes automatically when it is detected again.":'Ponownie przytrzymaj przycisk tego samego urządzenia ponad 10 sekund i puść. Parowanie wznowi się automatycznie po jego wykryciu.',
    'Keep the device near Shelly. The connection will resume automatically if the device is still in pairing mode.':'Trzymaj urządzenie blisko Shelly. Połączenie wznowi się automatycznie, jeśli urządzenie nadal jest w trybie parowania.',
    'Keep the device near Shelly. Pairing will continue when it is detected again.':'Trzymaj urządzenie blisko Shelly. Parowanie będzie kontynuowane po jego ponownym wykryciu.',
    'Wake the device or trigger a reading to confirm that encrypted data is received.':'Wybudź urządzenie lub wywołaj odczyt, aby potwierdzić odbiór szyfrowanych danych.',
    'Reading temperature and humidity directly. Keep the thermometer near Shelly and close other apps connected to it. No PIN or key is needed.':'Trwa bezpośredni odczyt temperatury i wilgotności. Trzymaj termometr blisko Shelly i zamknij inne połączone aplikacje. PIN ani klucz nie są potrzebne.',
    'Keep the thermometer near Shelly while temperature and humidity readings arrive. No button press or PIN is needed.':'Trzymaj termometr blisko Shelly do czasu odebrania temperatury i wilgotności. Nie musisz naciskać przycisku ani podawać PIN-u.',
    'The device is ready to use. You can close this window.':'Urządzenie jest gotowe. Możesz zamknąć to okno.',
    'Recent encrypted readings are available from this device using its saved key. This new pairing attempt did not finish. Close this window and check the device status before retrying.':'Zapisany klucz pozwala odbierać świeże szyfrowane odczyty z tego urządzenia. Nowa próba parowania nie została zakończona. Zamknij okno i sprawdź stan urządzenia przed kolejną próbą.',
    'Pairing did not finish. Close this window before starting a new attempt.':'Parowanie nie zostało zakończone. Zamknij okno przed rozpoczęciem kolejnej próby.',
    'Keep the device near Shelly while pairing finishes.':'Trzymaj urządzenie blisko Shelly do zakończenia parowania.',
    'Stopping the pairing attempt…':'Zatrzymywanie próby parowania…','Starting discovery…':'Rozpoczynanie wyszukiwania…',
    'Enter the current device PIN to continue.':'Wprowadź obecny PIN urządzenia, aby kontynuować.',
    'Choose a device to continue.':'Wybierz urządzenie, aby kontynuować.','Searching for your device…':'Wyszukiwanie urządzenia…',
    'Reading thermometer…':'Odczytywanie termometru…','Connecting to device…':'Łączenie z urządzeniem…','Securing the connection…':'Zabezpieczanie połączenia…',
    'Checking device…':'Sprawdzanie urządzenia…','Configuring device…':'Konfigurowanie urządzenia…','Configuring encryption…':'Konfigurowanie szyfrowania…',
    'Configuring encryption automatically…':'Automatyczne konfigurowanie szyfrowania…','Checking existing encrypted readings…':'Sprawdzanie zapisanych szyfrowanych odczytów…',
    'Encryption PIN required by this device.':'Urządzenie wymaga PIN-u szyfrowania.','Waiting for the encryption key…':'Oczekiwanie na klucz szyfrowania…',
    'Saving the encryption key…':'Zapisywanie klucza szyfrowania…','Restoring the connection…':'Przywracanie połączenia…',
    'Device closed the connection. Waiting to reconnect automatically…':'Urządzenie zamknęło połączenie. Oczekiwanie na automatyczne wznowienie…',
    'Hold the device button again for more than 10 seconds, then release it.':'Ponownie przytrzymaj przycisk urządzenia ponad 10 sekund i puść.',
    'Waiting for device…':'Oczekiwanie na urządzenie…','Waiting for temperature and humidity':'Oczekiwanie na temperaturę i wilgotność',
    'Device paired successfully.':'Urządzenie zostało sparowane.','Cancelling…':'Anulowanie…','Enable encryption':'Włącz szyfrowanie',
    'Continue pairing':'Kontynuuj parowanie','Pair selected device':'Sparuj wybrane urządzenie','Retry cancel':'Ponów anulowanie',
    'Downloading...':'Pobieranie…','Authenticating update...':'Uwierzytelnianie aktualizacji…','Uploading...':'Wgrywanie…',
    'Firmware uploaded; waiting for restart...':'Oprogramowanie wgrane; oczekiwanie na restart…','Up to date':'Oprogramowanie jest aktualne',
    'Update request timed out. Check the device version and update progress before retrying.':'Przekroczono czas aktualizacji. Sprawdź wersję urządzenia i postęp aktualizacji przed ponowną próbą.',
    'firmware downloads':'stronę pobierania oprogramowania','release notes':'opis wydania','Failed, check':'Niepowodzenie, sprawdź',
    'See':'Zobacz','Reboot the device?':'Uruchomić urządzenie ponownie?','Revert to stock firmware?':'Przywrócić fabryczne oprogramowanie?',
    'System is rebooting and will reconnect when ready.':'Urządzenie uruchamia się ponownie. Połączenie zostanie wznowione, gdy będzie gotowe.',
    'Device configuration has been reset, it will reboot in AP mode.':'Konfiguracja została zresetowana. Urządzenie uruchomi się ponownie w trybie punktu dostępowego.',
    'No files selected':'Nie wybrano pliku','Name must not be empty':'Nazwa nie może być pusta',
    'Device name is too long. Please use a shorter name.':'Nazwa urządzenia jest za długa. Użyj krótszej nazwy.',
    'Car name is too long. Please use a shorter name.':'Nazwa auta jest za długa. Użyj krótszej nazwy.',
    'Enter two distinct BLU sensor MAC addresses':'Wprowadź dwa różne adresy MAC czujników BLU',
    'Sensor timeout must be 30-3600 seconds':'Limit ważności odczytu musi wynosić od 30 do 3600 sekund',
    'Set a web panel password in Security Settings before pairing devices.':'Przed parowaniem ustaw hasło panelu WWW w Ustawieniach zabezpieczeń.',
    'Invalid old password!':'Nieprawidłowe obecne hasło!','No unicode in passwords please':'W haśle użyj tylko znaków ASCII',
    "Name must be between 1 and 63 characters and consist of letters, numbers or dashes ('-')":"Nazwa musi mieć od 1 do 63 znaków i zawierać tylko litery, cyfry lub myślniki ('-')",
    'Close Sensor:':'Czujnik zamknięcia:','Open Sensor:':'Czujnik otwarcia:','Movement Time:':'Czas ruchu:',
    'Pulse Time:':'Czas impulsu:','Output Mode:':'Tryb wyjścia:','Input Mode:':'Tryb wejścia:',
    'Inverted Input:':'Odwrócone wejście:','Inverted Output:':'Odwrócone wyjście:',
    'Swap Inputs (Sensors):':'Zamień wejścia (czujniki):','Swap Inputs:':'Zamień wejścia:','Swap Outputs:':'Zamień wyjścia:',
    'Normally Closed':'Normalnie zamknięty','Normally Open':'Normalnie otwarty','Disabled':'Wyłączone',
    'Single (out 1)':'Pojedyncze (wyjście 1)','Single (out 2)':'Pojedyncze (wyjście 2)','Single':'Pojedyncze','Dual':'Podwójne',
    'Momentary':'Chwilowy','Toggle':'Przełącz','Edge':'Zmiana stanu','Detached':'Odłączony','Activation':'Aktywacja',
    'Activation (both inputs)':'Aktywacja (oba wejścia)','Edge (both inputs)':'Zmiana stanu (oba wejścia)',
    'Separate - momentary':'Oddzielne — chwilowe','Separate - toggle':'Oddzielne — przełączane',
    'Toggle, on = off = single press':'Przełącznik: włączenie i wyłączenie = pojedynczy nacisk',
    'Toggle, on = single, off = double':'Przełącznik: włączenie = pojedynczy, wyłączenie = podwójny nacisk',
    'HAP Service Hidden:':'Ukryj usługę HAP:','HAP State Inverted:':'Odwróć stan HAP:','HAP Service Type:':'Typ usługi HAP:',
    'HAP Type:':'Typ HAP:','Initial state:':'Stan początkowy:','Off':'Wyłączone','On':'Włączone','Last':'Ostatni',
    'Input':'Wejście','Auto off:':'Automatyczne wyłączenie:','Auto Off:':'Automatyczne wyłączenie:',
    'Auto off delay:':'Czas do automatycznego wyłączenia:','Auto Off Delay:':'Czas do automatycznego wyłączenia:',
    'State LED:':'Dioda stanu:','Idle Time:':'Czas bezczynności:','Display as:':'Wyświetl jako:',
    'Switch':'Przełącznik','Outlet':'Gniazdko','Lock':'Zamek','Stateless Switch':'Przycisk bezstanowy',
    'Roller Shutter':'Roleta','Window':'Okno','Doorbell':'Dzwonek','Disabled Input':'Wyłączone wejście',
    'Motion Sensor':'Czujnik ruchu','Occupancy Sensor':'Czujnik obecności','Contact Sensor':'Kontaktron',
    'Temperature Sensor':'Czujnik temperatury','Leak Sensor':'Czujnik zalania','Smoke Sensor':'Czujnik dymu',
    'Carbon Monoxide Sensor':'Czujnik czadu','Carbon Dioxide Sensor':'Czujnik dwutlenku węgla',
    'Generic Valve':'Zawór ogólny','Valve':'Zawór','Irrigation':'Nawadnianie','Pulse':'Impuls','Level':'Poziom',
    'Color:':'Kolor:','Hue:':'Odcień:','Saturation:':'Nasycenie:','Color Temperature:':'Temperatura barwowa:',
    'Brightness:':'Jasność:','Transition Time:':'Czas przejścia:','White Mode':'Tryb bieli','Last Event:':'Ostatnie zdarzenie:',
    'seconds':'sekund','on GitHub':'na GitHubie','Use':'Użyj',
    'Please consider reporting missing features':'Zgłoś brakujące funkcje',
    'to report bugs and request features.':'do zgłaszania błędów i propozycji funkcji.',
    'Battery unknown':'Poziom baterii nieznany','Select language':'Wybierz język',
    'yes':'tak','no':'nie','closed':'Zamknięta','open':'Otwarta','opening':'Otwieranie','closing':'Zamykanie','stopped':'Zatrzymana',
    'unavailable':'Niedostępne','on':'Włączone','off':'Wyłączone','server not running':'serwer nie działa',
    'Closing sensor':'Czujnik zamknięcia','Opening sensor':'Czujnik otwarcia','Indoor H&T':'Temperatura i wilgotność wewnątrz','Outdoor H&T':'Temperatura i wilgotność na zewnątrz',
    'Press the smoke sensor test button and enter its 32-character BLE bindkey. A test-button packet verifies the key without confirming a smoke alarm or all-clear.':'Naciśnij przycisk testowy czujnika dymu i wprowadź jego 32-znakowy klucz Bindkey BLE. Pakiet przycisku potwierdza klucz, ale nie oznacza alarmu ani braku dymu.',
    'Waiting for smoke status':'Oczekiwanie na stan czujnika dymu',
    'Smoke sensor':'Czujnik dymu','Pair smoke sensor':'Parowanie czujnika dymu','Smoke device name':'Nazwa czujnika dymu','Smoke detected':'Wykryto dym','No smoke detected':'Nie wykryto dymu',
    'Indoor':'Wewnątrz','Outdoor':'Na zewnątrz','Garage door':'Brama garażowa','Button':'Przycisk','Car presence':'Obecność auta',
    'Garage BLU on GitHub':'Garage BLU na GitHubie','GitHub Issues':'Zgłoszenia na GitHubie',
    'This will erase all pairings and clear setup code. Are you sure?':'Wszystkie powiązania HomeKit i kod konfiguracji zostaną usunięte. Kontynuować?',
    'Device configuration will be wiped and return to AP mode. Are you sure?':'Konfiguracja urządzenia zostanie usunięta. Urządzenie wróci do trybu punktu dostępowego. Kontynuować?',
    'Invalid firmware download; try the ZIP file manually':'Nieprawidłowy plik aktualizacji. Spróbuj ręcznie wgrać ZIP.',
    'Missing update authentication challenge':'Brak wyzwania uwierzytelniania aktualizacji',
    'Name must be between 1 and 64 characters':'Nazwa musi mieć od 1 do 64 znaków',
    'Auto off delay must follow 24 hour format D:HH:MM:SS.sss with a value between 10ms and 24 days.':'Czas automatycznego wyłączenia musi mieć format D:HH:MM:SS.sss (zegar 24-godzinny), od 10 ms do 24 dni.',

    "Device closed the connection. Waiting to reconnect…":"Urządzenie zamknęło połączenie. Oczekiwanie na ponowne połączenie…",
    "Waiting for the same device…":"Oczekiwanie na to samo urządzenie…",
    "Enter a six-digit encryption PIN from 100000 to 999999.":"Wprowadź 6-cyfrowy PIN szyfrowania od 100000 do 999999.",
    "Enter the current six-digit device PIN.":"Wprowadź obecny 6-cyfrowy PIN urządzenia.",
    "Pairing cancelled":"Parowanie anulowane",
    "Pairing timed out; wake the device and retry":"Upłynął czas parowania. Wybudź urządzenie i spróbuj ponownie.",
    "Pairing already active":"Parowanie już trwa",
    "Bluetooth is not running":"Bluetooth nie jest uruchomiony",
    "Unsupported thermometer model":"Nieobsługiwany model termometru",
    "Select a discovered device and role":"Wybierz wykryte urządzenie i jego funkcję",
    "Invalid encryption PIN":"Nieprawidłowy PIN szyfrowania",
    "Invalid current PIN":"Nieprawidłowy obecny PIN",
    "Device expired; scan and wake it again":"Urządzenie nie jest już widoczne. Wybudź je i wyszukaj ponownie.",
    "Select the correct device model for this role":"Wybierz właściwy model urządzenia dla tej funkcji",
    "Device is already assigned to another role":"Urządzenie jest już przypisane do innej funkcji",
    "Enter the current PIN of this secured device, or factory-reset it first":"Wprowadź obecny PIN zabezpieczonego urządzenia lub najpierw przywróć jego ustawienia fabryczne",
    "Choose a different new PIN to rotate the encryption key":"Wybierz nowy PIN inny niż obecny, aby zmienić klucz szyfrowania",
    "No device pairing is waiting for confirmation":"Żadne parowanie nie oczekuje na potwierdzenie",
    "No device is waiting for an encryption PIN":"Żadne urządzenie nie oczekuje na PIN szyfrowania",
    "Enter a six-digit PIN different from the current PIN":"Wprowadź 6-cyfrowy PIN inny niż obecny",
    "Cannot select BLE address":"Nie można wybrać adresu BLE",
    "BLE connection failed":"Nie udało się połączyć przez BLE",
    "BLE link does not satisfy the required bonding/authentication":"Połączenie BLE nie spełnia wymagań parowania i uwierzytelnienia",
    "Cannot initiate BLE security":"Nie można rozpocząć zabezpieczania połączenia BLE",
    "Cannot renew the selected device's BLE bond":"Nie można odnowić powiązania BLE wybranego urządzenia",
    "Device has no PIN-based encryption setup":"Urządzenie nie obsługuje konfiguracji szyfrowania przez PIN",
    "Device rejected encryption setup; no new key was saved":"Urządzenie odrzuciło konfigurację szyfrowania. Nowy klucz nie został zapisany.",
    "Device did not provide an encryption key":"Urządzenie nie udostępniło klucza szyfrowania",
    "Device has no supported encryption setup":"Urządzenie nie obsługuje tej konfiguracji szyfrowania",
    "Cannot save device key":"Nie można zapisać klucza urządzenia",
    "Cannot save device assignment; pair again":"Nie można zapisać przypisania urządzenia. Sparuj je ponownie.",
    "Device lacks required secure BLU characteristics":"Urządzenie nie udostępnia wymaganych zabezpieczonych charakterystyk BLU",
    "Characteristic discovery failed":"Nie udało się wykryć charakterystyk urządzenia",
    "Shelly service not found":"Nie znaleziono usługi Shelly",
    "Connection failed; put the device in pairing mode and retry":"Połączenie nie powiodło się. Włącz tryb parowania urządzenia i spróbuj ponownie.",
    "Device did not complete renewed BLE pairing; wake it and start a new attempt":"Urządzenie nie zakończyło ponownego parowania BLE. Wybudź je i rozpocznij nową próbę.",
    "Cannot save thermometer assignment":"Nie można zapisać przypisania termometru",
    "Cannot save thermometer assignment; add it again":"Nie można zapisać przypisania termometru. Dodaj go ponownie.",
    "Cannot save thermometer assignment; previous assignment retained":"Nie można zapisać przypisania termometru. Zachowano poprzednie przypisanie.",
    "Wait until the garage stops, then retry adding the thermometer":"Poczekaj, aż brama się zatrzyma, i ponów dodawanie termometru",
    "LYWSD03MMC read timed out; close other Bluetooth apps and keep it near Shelly":"Upłynął czas odczytu LYWSD03MMC. Zamknij inne aplikacje Bluetooth i umieść termometr blisko Shelly.",
    "Read cancelled":"Odczyt anulowany",
    "Read interrupted while the garage is moving; retry when stopped":"Odczyt przerwany podczas ruchu bramy. Ponów po jej zatrzymaniu.",
    "Invalid device role":"Nieprawidłowa funkcja urządzenia",
    "Close the pairing window first":"Najpierw zamknij okno parowania",
    "BLE bond removal timed out; device assignment was preserved":"Przekroczono czas usuwania powiązania BLE. Przypisanie urządzenia zostało zachowane.",
    "Cannot start BLE bond removal; try again":"Nie można rozpocząć usuwania powiązania BLE. Spróbuj ponownie.",
    "Cannot delete device BLE bond":"Nie można usunąć powiązania BLE urządzenia",
    "Cannot delete device key":"Nie można usunąć klucza urządzenia",
    "Cannot save device removal; retry":"Nie można zapisać usunięcia urządzenia. Spróbuj ponownie.",
    "Cannot set device PIN":"Nie można ustawić PIN-u urządzenia",
    "Cannot configure device encryption":"Nie można skonfigurować szyfrowania urządzenia",
    "Cannot reconnect to authenticate the device key":"Nie można ponownie połączyć się w celu uwierzytelnienia klucza urządzenia",
    "Cannot request encryption key":"Nie można zażądać klucza szyfrowania",
    "Cannot enable device beacons":"Nie można włączyć beaconów urządzenia",
    "Cannot discover device characteristics":"Nie można wykryć charakterystyk urządzenia",
    "Cannot discover Shelly service":"Nie można wykryć usługi Shelly",
    "BLE bond could not be renewed; wake the device and retry":"Nie udało się odnowić powiązania BLE. Wybudź urządzenie i spróbuj ponownie.",
    "Bluetooth is not ready":"Bluetooth nie jest gotowy",
    'Bluetooth is not running; enable it and restart Shelly':'Bluetooth nie jest uruchomiony. Włącz go i uruchom Shelly ponownie.',
    'Nearby devices':'Urządzenia w pobliżu',
    'Bluetooth button':'Przycisk Bluetooth',
    'Door and window sensor':'Czujnik otwarcia drzwi i okien',
    'Unknown Bluetooth device':'Nieznane urządzenie Bluetooth',
    'Model not identified':'Nie rozpoznano modelu',
    'Pair closing sensor':'Parowanie czujnika zamknięcia',
    'Pair opening sensor':'Parowanie czujnika otwarcia',
    'Pair motion sensor':'Parowanie czujnika ruchu',
    'Pair indoor H&T':'Parowanie czujnika temperatury wewnątrz',
    'Pair outdoor H&T':'Parowanie czujnika temperatury na zewnątrz',
    'Pair BLU Button1':'Parowanie przycisku BLU Button1',
    "Device type:":"Model:",
    "32 hexadecimal characters":"32 znaki szesnastkowe",
    "Verify bindkey and add":"Sprawdź klucz i dodaj",
    "Enter a 32-character hexadecimal bindkey.":"Wprowadź 32-znakowy klucz Bindkey BLE (cyfry i litery A–F).",
    "Wake the Xiaomi device, select it and paste its 32-character BLE bindkey. For a door sensor, move the magnet to send a new reading. The key is verified before saving.":"Wybudź urządzenie Xiaomi, wybierz je i wklej jego 32-znakowy Bindkey BLE. W czujniku otwarcia przesuń magnes, aby wysłać nowy odczyt. Klucz zostanie sprawdzony przed zapisaniem.",
    "Select a Xiaomi device and enter a 32-character hexadecimal bindkey":"Wybierz urządzenie Xiaomi i wprowadź 32-znakowy klucz szesnastkowy Bindkey BLE",
    "Wake the selected Xiaomi device and try again":"Wybudź wybrane urządzenie Xiaomi i spróbuj ponownie",
    "Bindkey verification failed; check the key and trigger a new reading":"Weryfikacja Bindkey BLE nie powiodła się. Sprawdź klucz i wywołaj nowy odczyt.",
    "This beacon was already received; trigger a new reading":"Ten beacon został już odebrany. Wywołaj nowy odczyt.",
    'Motion and illuminance sensor':'Czujnik ruchu i jasności',
    'This device requires a BLE bindkey':'To urządzenie wymaga klucza Bindkey BLE',
    'This Qingping device broadcasts encrypted data. Paste its 32-character BLE bindkey to verify and add it.':'Ten czujnik Qingping nadaje szyfrowane dane. Wklej jego 32-znakowy Bindkey BLE, aby sprawdzić klucz i dodać urządzenie.',
    'Keep the Qingping device near Shelly and trigger a reading. Move the magnet for CGH1 or walk in front of CGPR1. If encryption is detected, the panel will request a BLE bindkey.':'Umieść czujnik Qingping blisko Shelly i wywołaj odczyt. W CGH1 przesuń magnes, a w CGPR1 przejdź przed czujnikiem. Jeśli wykryte zostanie szyfrowanie, panel poprosi o Bindkey BLE.',
  };
  const key = 'garage-ui-language';
  let language = 'en';
  try { if (localStorage.getItem(key) === 'pl') language = 'pl'; } catch (_) {}
  const textCache = new WeakMap(), attrCache = new WeakMap();
  const normalize = s => String(s).replace(/\s+/g, ' ').trim();
  function t(value) {
    const text = normalize(value);
    if (language !== 'pl') return String(value);
    if (Object.prototype.hasOwnProperty.call(translations, text)) return translations[text];
    // Preserve diagnostic codes while translating the user-facing error.
    const diagnostic = text.match(/^(.*?) (\[.*\])$/);
    if (diagnostic) return t(diagnostic[1]) + ' ' + diagnostic[2];
    const patterns = [
      [/^BLE connection unavailable before security (.+)$/, 'Połączenie BLE niedostępne przed zabezpieczeniem $1'],
      [/^Device rejected beacon configuration (.+)$/, 'Urządzenie odrzuciło konfigurację beaconów $1'],
      [/^Device rejected PIN configuration (.+)$/, 'Urządzenie odrzuciło konfigurację PIN-u $1'],
      [/^Cannot read encryption key (.+); device PIN may already be set$/, 'Nie można odczytać klucza szyfrowania $1. Urządzenie może mieć już ustawiony PIN.'],
      [/^BLE security failed (.+)$/, 'Nie udało się zabezpieczyć połączenia BLE $1'],
      [/^Device disconnected during (\S+) (\(BLE reason .+\)); wake it and retry$/, 'Urządzenie rozłączyło się $2. Wybudź je i spróbuj ponownie.'],
      [/^LYWSD03MMC connection failed (.+); close other Bluetooth apps and retry$/, 'Nie udało się połączyć z LYWSD03MMC $1. Zamknij inne aplikacje Bluetooth i spróbuj ponownie.'],
      [/^LYWSD03MMC disconnected (.+); close other Bluetooth apps and retry$/, 'LYWSD03MMC rozłączył się $1. Zamknij inne aplikacje Bluetooth i spróbuj ponownie.'],
      [/^Cannot connect to LYWSD03MMC (.+)$/, 'Nie można połączyć się z LYWSD03MMC $1'],
      [/^Cannot verify LYWSD03MMC model (.+); expected LYWSD03MMC$/, 'Nie można potwierdzić modelu termometru $1. Oczekiwano LYWSD03MMC.'],
      [/^LYWSD03MMC direct read failed (.+); no key or PIN was requested$/, 'Bezpośredni odczyt LYWSD03MMC nie powiódł się $1. Klucz ani PIN nie były wymagane.'],
      [/^Cannot discover LYWSD03MMC characteristics (.+)$/, 'Nie można wykryć charakterystyk LYWSD03MMC $1'],
      [/^Cannot start LYWSD03MMC characteristic discovery (.+)$/, 'Nie można rozpocząć wykrywania charakterystyk LYWSD03MMC $1'],
      [/^Device detected: (.+)$/, 'Wykryto urządzenie: $1'],
      [/^Invalid movement time (.+)$/, 'Nieprawidłowy czas ruchu: $1'],
      [/^Invalid pulse time (.+)$/, 'Nieprawidłowy czas impulsu: $1'],
      [/^Error downloading: (.+)$/, 'Błąd pobierania: $1'],
      [/^Error uploading: (.+)$/, 'Błąd wgrywania: $1'],
      [/^Update authentication failed \(HTTP (\d+)\)$/, 'Uwierzytelnienie aktualizacji nie powiodło się (HTTP $1)'],
      [/^Version (.+) is available\.$/, 'Dostępna jest wersja $1.'],
      [/^Signal (\d) of 4 bars$/, 'Sygnał: $1 z 4 kresek'],
      [/^Battery (.+)$/, 'Bateria: $1'],
      [/^(\d+) pending$/, '$1 oczekujących'],
      [/^(\d+) active$/, '$1 aktywnych'],
      [/^(\d+) max$/, '$1 maks.'],
      [/^Estimated from battery voltage \((.+) mV\)$/, 'Szacunek na podstawie napięcia baterii ($1 mV)'],
    ];
    for (const [pattern, replacement] of patterns) if (pattern.test(text)) return text.replace(pattern, replacement);
    if (text.endsWith(' · stale')) { const prefix = text.slice(0, -8); const translated = t(prefix); return translated + ' · nieaktualne'; }
    if (text.endsWith(' — stale reading')) return t(text.slice(0, -16)) + ' — nieaktualny odczyt';
    return String(value);
  }
  // These nodes contain user-provided names or technical data, never UI copy.
  const skip = 'script,style,textarea,input,pre,code,[data-i18n-ignore],#head,#device_name,.tile-name,.sensor-label';
  function translateText(node) {
    if (!node.parentElement || node.parentElement.closest(skip)) return;
    const value = node.nodeValue;
    let entry = textCache.get(node);
    if (!entry || value !== entry.rendered) entry = {source: value};
    const leading = entry.source.match(/^\s*/)[0], trailing = entry.source.match(/\s*$/)[0];
    const core = normalize(entry.source);
    if (!core) return;
    const translated = t(core);
    entry.rendered = leading + translated + trailing;
    textCache.set(node, entry);
    if (value !== entry.rendered) node.nodeValue = entry.rendered;
  }
  function translateAttributes(element) {
    if (element.closest('[data-i18n-ignore]')) return;
    let values = attrCache.get(element);
    if (!values) { values = {}; attrCache.set(element, values); }
    for (const attr of ['title', 'aria-label', 'placeholder']) {
      if (!element.hasAttribute(attr)) continue;
      const value = element.getAttribute(attr);
      let entry = values[attr];
      if (!entry || value !== entry.rendered) entry = {source: value};
      entry.rendered = t(entry.source); values[attr] = entry;
      if (value !== entry.rendered) element.setAttribute(attr, entry.rendered);
    }
  }
  let observer, scheduled = false;
  function apply() {
    scheduled = false;
    if (!document.body) return;
    if (observer) observer.disconnect();
    document.documentElement.lang = language;
    const walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT);
    while (walker.nextNode()) translateText(walker.currentNode);
    document.querySelectorAll('[title],[aria-label],[placeholder]').forEach(translateAttributes);
    document.querySelectorAll('[data-ui-language-select]').forEach(select => { select.value = language; });
    if (observer) observer.observe(document.body, {subtree: true, childList: true, characterData: true, attributes: true, attributeFilter: ['title','aria-label','placeholder']});
  }
  function setLanguage(value, persist) {
    if (!['en', 'pl'].includes(value)) return;
    language = value;
    if (persist) { try { localStorage.setItem(key, value); } catch (_) {} }
    apply();
    document.querySelectorAll('iframe').forEach(frame => {
      if (frame.contentWindow) frame.contentWindow.postMessage({garageUILanguage: value}, location.origin);
    });
    document.dispatchEvent(new CustomEvent('garage-language-change', {detail: value}));
  }
  function init() {
    if (window.parent === window) {
      const bar = document.createElement('select'); bar.className = 'ui-language-select';
      bar.id = 'ui_language'; bar.setAttribute('data-ui-language-select', '');
      bar.setAttribute('aria-label', 'Select language');
      for (const [lang, name] of [['en', 'English'], ['pl', 'Polski']]) {
        const option = document.createElement('option'); option.value = lang; option.textContent = name;
        option.setAttribute('data-i18n-ignore', ''); bar.append(option);
      }
      bar.value = language; bar.addEventListener('change', () => setLanguage(bar.value, true));
      const mount = document.getElementById('ui_language_mount');
      if (mount) mount.append(bar); else document.body.prepend(bar);
    }
    observer = new MutationObserver(() => {
      if (!scheduled) { scheduled = true; requestAnimationFrame(apply); }
    });
    apply();
    if (window.parent !== window) window.parent.postMessage({garageUILanguageRequest: true}, location.origin);
  }
  window.GarageI18n = {t, setLanguage: value => setLanguage(value, true), getLanguage: () => language, apply};
  window.addEventListener('storage', event => { if (event.key === key) setLanguage(event.newValue || 'en', false); });
  window.addEventListener('message', event => {
    if (event.origin !== location.origin || !event.data) return;
    if (window.parent !== window && event.source === window.parent && event.data.garageUILanguage) {
      setLanguage(event.data.garageUILanguage, false);
    } else if (event.data.garageUILanguageRequest &&
        Array.from(document.querySelectorAll('iframe')).some(frame => frame.contentWindow === event.source)) {
      event.source.postMessage({garageUILanguage: language}, location.origin);
    }
  });
  for (const name of ['alert','confirm']) {
    const original = window[name].bind(window);
    window[name] = message => original(t(message));
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init, {once: true}); else init();
})();
