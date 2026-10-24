# Konfiguracja i obsługa Garage BLU

Instrukcja dla **2.16.1-blu45 / Shelly Plus 1 i Shelly 1 Gen3**, stan: **2026-10-24**.
Nazwy pól i komunikatów podano tak, jak wyglądają w angielskim panelu.

[Instalacja i aktualizacje](AKTUALIZACJA.md) ·
[Rozwiązywanie problemów](ROZWIAZYWANIE_PROBLEMOW.md) · [Historia zmian](HISTORIA_ZMIAN.md)

## Pierwsze ustawienia

1. Otwórz lokalny adres Shelly w przeglądarce.
2. W Security Settings ustaw hasło administratora, a następnie zaloguj się. Parowanie BLU wymaga skonfigurowanej autoryzacji.
3. W General Settings wybierz Mode → Garage Door Opener i kliknij Save.
4. W sekcji Garage Door wpisz Name dla bramy. Ta nazwa pojawi się na jej głównym kafelku.

Name w General Settings jest nazwą sterownika. Name w sekcji bramy jest nazwą
akcesorium bramy. Pod przyciskiem Pair każdego sparowanego BLU znajduje się
oddzielne pole Name dla tego urządzenia.

## Ustawienia bramy

| Pole | Znaczenie |
| --- | --- |
| Name | Nazwa bramy używana przez komponent i kafelek wizualizacji. |
| BLU sensors | Włącza dwie krańcówki BLE jako źródło stanu bramy. Wyłączenie wybiera wejścia przewodowe; podgląd już sparowanych BLU nadal może działać. |
| Movement Time | Czas pełnego ruchu w sekundach. Domyślnie 30 s; panel wymaga co najmniej 10 s. Dopasuj go do rzeczywistego czasu napędu. |
| Pulse Time | Długość impulsu wyjściowego w milisekundach. Domyślnie 300 ms; panel wymaga co najmniej 20 ms. Dobierz ją do wejścia sterującego napędu. |
| Continue previous direction after stop | Wyłączony: następne uruchomienie po zatrzymaniu zmienia kierunek. Włączony: kontynuuje poprzedni kierunek. Nie zmienia programu samego napędu. |
| Sensor timeout | Maksymalny wiek odczytu BLU: domyślnie 180 s, zakres 30–3600 s. To inny limit niż 45 s obecności Button1. |
| Save | Zapisuje nazwę, ustawienia i nazwy urządzeń BLU w tej sekcji. |

Przykład: przy wyłączonej kontynuacji kolejne naciśnięcia podczas otwierania dają
**otwieranie → zatrzymanie → zamykanie**. Przy włączonej:
**otwieranie → zatrzymanie → dalsze otwieranie**. Rzeczywista reakcja napędu musi
odpowiadać wybranemu ustawieniu. W trybie BLU ustawienia polaryzacji i zamiany
przewodowych czujników nie zmieniają ról przypisanych do adresów BLU.

## Pair: parowanie urządzenia

1. Kliknij Pair przy właściwej roli. Modal otwiera się i od razu uruchamia wyszukiwanie.
2. Wprowadź BLU w tryb parowania: przytrzymaj przycisk ponad 10 sekund i puść. Trzymaj urządzenie blisko sterownika.
3. Po wykryciu jednego zgodnego urządzenia panel kontynuuje automatycznie. Przy kilku pasujących urządzeniach wybierz właściwy adres MAC.
4. W zwykłym przebiegu nie wpisujesz PIN-u na początku. Jeśli urządzenie wymaga istniejącego PIN-u albo aktywacji szyfrowania przez PIN, modal pokaże odpowiednie pole i przycisk.
5. Poczekaj na Device paired successfully. Dopiero świeży, poprawnie odszyfrowany pakiet kończy parowanie.
6. Zamknij modal. Sprawdź przypisany MAC, status i wiek odczytu, nadaj Name i zapisz Save w sekcji bramy.

Jeżeli parujesz ponownie ten sam adres z zapisanym kluczem, sterownik najpierw
przez krótki czas sprawdza istniejące szyfrowane odczyty. Działające przypisanie
może zostać potwierdzone bez ponownego zestawiania zabezpieczonego połączenia BLE.

### Gdy modal prosi o ponowne obudzenie

- **Hold the device button again for more than 10 seconds, then release it** — ponownie włącz tryb parowania tego samego urządzenia. Wznowienie nastąpi po jego wykryciu, bez dodatkowego Continue.
- **Waiting for encrypted beacon** — sterownik czeka na szyfrowany odczyt. Obudź urządzenie lub wywołaj normalną zmianę odczytu; długie przytrzymanie nie jest automatycznie wymagane na tym etapie.
- **Pairing stopped** — próba się zakończyła. Przeczytaj błąd, zamknij modal i sprawdź odczyty przed kolejną próbą.

Powtórne wejście w tryb parowania nie jest stałym drugim krokiem. Szczegóły
ponawiania, błędów i PIN-u znajdziesz w [diagnostyce](ROZWIAZYWANIE_PROBLEMOW.md#parowanie-blu).

## Dwie krańcówki Door/Window

| Rola | Kiedy ma wykrywać swój magnes |
| --- | --- |
| Closing sensor | Wyłącznie przy pełnym zamknięciu bramy. |
| Opening sensor | Wyłącznie przy pełnym otwarciu bramy. |

Każda rola musi mieć inny adres MAC. Sparuj oba urządzenia, sprawdź zmianę stanu
kontaktronu, włącz BLU sensors i zapisz ustawienia.

**Closed i Open pod czujnikiem opisują jego kontaktron, a nie cały garaż.**
Gdy Opening sensor pokazuje Closed, jego magnes jest obecny: oznacza to osiągnięcie
pełnego otwarcia bramy. Dwa kontaktrony Closed jednocześnie dają sprzeczne
krańcówki i blokują sterowanie. Dwa Open mogą oznaczać położenie pomiędzy końcami.

Przy włączonych krańcówkach BLU brak świeżych danych, sprzeczność lub aktywne
parowanie blokują polecenia. Przed pierwszymi odczytami po restarcie panel może
pokazać No data. Krańcówki są widoczne w ustawieniach; od blu36 nie ma ich
nazw ani statusów nad grafiką garażu.

## Motion, H&T i Button1

| Pole w panelu | Co pojawia się po sparowaniu |
| --- | --- |
| Motion sensor | Status Motion / No motion, kafelek Motion detected / No motion oraz usługi ruchu i oświetlenia w HomeKit. |
| Indoor H&T | Temperatura i wilgotność wewnątrz, odczyt nad garażem i usługi w HomeKit. |
| Outdoor H&T | Temperatura i wilgotność na zewnątrz, odczyt nad garażem i usługi w HomeKit. |
| BLU Button1 | In range / Out of range, kafelek obecności i wskaźnik naciśnięcia. Dwie usługi HomeKit w akcesorium Shelly: czujnik obecności auta i przycisk programowalny. Pojedynczy nacisk steruje także istniejącą bramą. |

Usługi Motion i Shelly BLU H&T są dodawane do HomeKit po bezpiecznym sparowaniu.
Xiaomi dodaje te same usługi temperatury i wilgotności po zapisaniu przypisania i potwierdzeniu pomiarów. Chwilowy brak
odczytów nie usuwa ich z HomeKit; są raportowane jako niedostępne. Po sparowaniu
Button1 pojawiają się usługi obecności auta i przycisku programowalnego w tym samym akcesorium Shelly.

Od blu43 sparowany BLU Motion udostępnia dodatkowo usługę **Light sensor** w tym
samym akcesorium Shelly. Pomiar pochodzi wyłącznie z jego szyfrowanych pakietów,
nie z krańcówek Door/Window. HomeKit podaje natężenie w luksach. Nazwę nowej usługi
możesz zmienić w aplikacji Dom. Aktualizacja odświeża listę usług, zachowując
identyfikatory dotychczasowych usług i parowania.

Oświetlenie ma własny czas ważności równy Sensor timeout. Pakiet bez pomiaru LUX
nie przedłuża tego czasu; brak świeżego pomiaru zgłasza błąd i nieaktywny status.
HomeKit wymaga zakresu 0,0001–100000 lx, więc całkowita ciemność ma wartość minimalną.

## Xiaomi Bluetooth LYWSDCGQ/01ZM

1. Przy **Indoor H&T** lub **Outdoor H&T** wybierz **Xiaomi LYWSDCGQ/01ZM**.
2. Kliknij **Pair**. Zostaw włączony czujnik blisko Shelly; nie przytrzymuj przycisku.
3. Jeden wykryty czujnik jest dodawany automatycznie. Jeśli są dwa lub więcej,
   wybierz właściwy adres MAC. Nie jest potrzebny PIN, konto Xiaomi ani zmiana firmware czujnika.
4. Modal pokaże **Waiting for temperature and humidity**, a po odebraniu obu
   pomiarów zakończy dodawanie. Nazwę wpisz w **Name** i zapisz przez **Save**.

Odczyty pojawiają się nad garażem i jako temperatura oraz wilgotność w HomeKit,
w tym samym akcesorium Shelly. Możesz mieszać Xiaomi i Shelly BLU H&T w dwóch
istniejących rolach. Ten wybór obsługuje tylko LYWSDCGQ/01ZM (MJ_HT_V1), nie dowolny Xiaomi.
Czujnik Xiaomi nadaje bez szyfrowania; **Pair** zapisuje odbiornik wybranego MAC,
nie zestawia połączenia BLE. Nie zmienia to szyfrowania krańcówek ani przycisku.
Temperatura ma rozdzielczość 0,1 °C, wilgotność jest zaokrąglana do całego procenta.
Każdy pomiar ma własny czas ważności; sam beacon baterii nie odświeża pomiarów.

## Własne nazwy

Pole Name urządzenia pojawia się po przypisaniu, w nowym wierszu po
Pair i przed jego statusem. Wpisz nazwę, a następnie użyj Save w sekcji bramy.
Zapis na Shelly przetrwa ponowne otwarcie strony. Zmiana nazwy nie wymaga parowania.

- Motion używa nazwy na swoim kafelku.
- Button1 używa Name na wskaźniku naciśnięcia. Osobne Car name określa nazwę kafelka auta.
  Puste pola pokazują podpowiedzi Button i Name of car. Obie nazwy zapisuje Save;
  wyczyszczenie pól przywraca etykiety Button i Car presence na kafelkach i nazwy
  akcesoriów HomeKit. Zmiana Car name odświeża usługę HomeKit bez usuwania kluczy.
- H&T używają nazw przy odczytach temperatury i wilgotności.
- Nazwy krańcówek pozostają w ustawieniach.
- Kafelek bramy używa pierwszego Name w sekcji Garage Door.

Puste Name urządzenia BLU przywraca angielską nazwę domyślną. Limit wynosi 64 bajty
UTF-8; polskie znaki mogą zajmować więcej niż jeden bajt. Teksty systemowe są po
angielsku, lecz własne nazwy mogą być po polsku. Motion, H&T oraz oba akcesoria Button1
przekazują nazwy także do HomeKit; aplikacja Dom może zachować nazwy nadane wcześniej ręcznie.

## Wizualizacja i statusy

| Element / komunikat | Działanie |
| --- | --- |
| Day / Night / Auto | Motyw dzienny, nocny lub zgodny z ustawieniem przeglądarki/systemu. |
| Ikona auta u góry | Ręczne pokazanie lub ukrycie auta; nie steruje przekaźnikiem. |
| Closed / Open | Potwierdzona pozycja końcowa bramy. |
| Opening… / Closing… | Ruch; procent położenia jest estymacją czasową. |
| Stopped | Zatrzymanie pomiędzy krańcówkami. |
| Obstruction | Błąd/przeszkoda; dla przekroczenia czasu ruchu animacja pokazuje umowne 50%. |
| No data | Brakuje wiarygodnych odczytów do określenia stanu. |
| Disconnected | Panel nie otrzymuje bieżącej telemetrii. |

Po przekroczeniu czasu ruchu prawidłowo potwierdzona krańcówka kasuje błąd timeoutu
i przeszkodę w HomeKit. Nieaktualne lub sprzeczne odczyty tego nie robią. Pozycja 50%
przy Obstruction jest symbolem błędu, a nie pomiarem położenia.

Przy skonfigurowanym haśle wizualizacja ładuje się po zalogowaniu. Wylogowanie lub
utrata połączenia usuwa widok; podczas OTA dashboard jest ukryty. Urządzenie bez
ustawionego hasła może udostępniać pełny status, ale parowanie BLU nadal wymaga hasła.

## Obecność auta i przycisk

Przechowuj Button1 w aucie, jeśli chcesz używać jego beaconów jako wskazówki obecności.
Nowy prawidłowo odszyfrowany pakiet pokazuje auto. Po przekroczeniu **45 sekund** bez
pakietu obecność znika. Sam sygnał BLE potwierdza zasięg przycisku, nie miejsce postoju.

Ręczne pokazanie lub ukrycie auta obowiązuje do następnego pakietu albo zmiany
obecności. Odświeżenie panelu z tym samym pakietem nie cofa od razu ręcznego wyboru.
Od blu44 ikona auta w prawym górnym rogu zmienia jednocześnie grafikę i kafelek
Car presence: ukrycie auta wygasza kafelek i pokazuje Not detected. Nowy beacon
przywraca stan wynikający z BLE. Ten ręczny wybór dotyczy panelu WWW; czujnik
obecności w HomeKit nadal korzysta z rzeczywistych pakietów.
Sensor timeout dotyczy świeżości danych czujników i nie zmienia limitu obecności Button1.

Wskaźnik Button1 miga po nowym pojedynczym naciśnięciu, a nie od zwykłego beacona.
Naciśnięcie może wywołać normalne otwórz / stop / zamknij, jeśli stan krańcówek
pozwala na sterowanie. Beacon nigdy nie uruchamia przekaźnika.

### Obecność i przycisk w aplikacji Dom

W blu42 sparowany Button1 udostępnia dwie usługi w istniejącym akcesorium Shelly,
razem z bramą i pozostałymi czujnikami. Są dostępne przez jego listę Akcesoria:

- **Czujnik obecności auta (Occupancy Sensor)** — nazwa z Car name. Obecność oznacza
  świeży, poprawnie odszyfrowany beacon; po 45 sekundach bez pakietu zgłasza brak
  obecności. To prawidłowy stan, nie błąd czujnika. Brak Bluetooth lub nasłuchu BLE zgłasza błąd
  oraz nieaktywny status, zachowując ostatni odczyt. Ręczny przełącznik grafiki auta nie zmienia HomeKit.
- **Przycisk programowalny (Stateless Programmable Switch)** — nazwa z Name pod
  BLU Button1. W ustawieniach tego akcesorium w aplikacji Dom przypisz akcje do
  pojedynczego, podwójnego i długiego naciśnięcia. Pojedynczy klik nadal bezpośrednio
  steruje bramą, oprócz zdarzenia w HomeKit; podwójny i długi klik wysyłają tylko
  zdarzenie HomeKit. Zwykłe beacony i powtórzenia tego samego pakietu nie wywołują akcji.

Jeśli Button1 jest już bezpiecznie sparowany, wystarczy aktualizacja i restart
sterownika. Nie resetuj HomeKit ani nie paruj BLU ponownie. Nowe akcesoria
powinny pojawić się po odświeżeniu konfiguracji w Domu. Blu42 zastępuje usługi
ruchu, obecności i przycisku nowymi identyfikatorami oraz jednorazowo zwiększa numer
konfiguracji HomeKit. Zachowuje identyfikatory samej bramy i parowania.
Po tej migracji sprawdź i ewentualnie przypisz ponownie akcje przycisku oraz
automatyzacje korzystające z ruchu lub obecności. Na Plus 1 z tym błędem potwierdzono
po migracji widoczność wszystkich czterech pozycji w Domu, gotowość ruchu, odczyt
obecności oraz dostępność trzech rodzajów naciśnięcia przycisku.
Brak beaconów lub zmiana nazwy nie usuwa usług. Dom może zachować własne, ręcznie
nadane nazwy.

## Dodanie do Apple Home

1. Skonfiguruj role krańcówek i potwierdź poprawny stan bramy.
2. W HomeKit Settings wybierz Setup i wyświetl kod QR / kod konfiguracji.
3. W aplikacji Dom wybierz dodanie akcesorium i użyj tego kodu.
4. Sprawdź połączenie i wskazania bramy; opcjonalne Motion, H&T oraz oba akcesoria Button1 pojawiają się po ich bezpiecznym sparowaniu.

Nie używaj HomeKit Reset do zwykłego odświeżenia statusu lub aktualizacji firmware;
usuwa on parowania HomeKit i kod konfiguracji.
