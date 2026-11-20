(function () {
  'use strict';

  function validIP(ip) {
    const parts = ip.split('.');
    return parts.length === 4 && parts.every(p => /^(0|[1-9][0-9]{0,2})$/.test(p) && Number(p) <= 255);
  }

  function verifyDevice(payload, rules) {
    const info = payload && payload.result ? payload.result : payload;
    if (!info || typeof info !== 'object' || Array.isArray(info)) {
      throw new Error('Wklej pełną odpowiedź JSON z informacji urządzenia.');
    }
    const entry = Object.entries(rules).find(([, rule]) =>
      typeof info.id === 'string' && new RegExp('^' + rule.prefix + '-[0-9a-f]{12}$', 'i').test(info.id));
    if (!entry) throw new Error('Nieobsługiwany model. Instalator obsługuje tylko Shelly Plus 1 i Shelly 1 Gen3.');
    const [model, rule] = entry;
    if (info.app !== rule.app || info.gen !== rule.gen || !rule.hardware_models.includes(info.model)) {
      throw new Error('Niezgodna wersja sprzętu: ' + String(info.model || 'brak modelu') +
        ', generacja ' + String(info.gen || '?') + '. Oczekiwano ' + rule.name + ' (' + rule.hardware_models.join(', ') + ').');
    }
    if (info.ver !== rule.stock_version) {
      throw new Error('Firmware ' + String(info.ver || 'nieznany') + ' nie jest zgodny z obecną paczką dla ' +
        rule.name + '. Wymagana wersja fabryczna: ' + rule.stock_version +
        '. Starszy firmware zaktualizuj w panelu Shelly. Nowsza wersja wymaga sprawdzenia zgodności paczki.');
    }
    if (typeof info.fw_id !== 'string' ||
        !new RegExp('^[0-9]{8}-[0-9]{6}/' + rule.stock_version.replace(/\./g, '\\.') + '-g[0-9a-f]+$', 'i').test(info.fw_id)) {
      throw new Error('Nie można potwierdzić oryginalnego firmware. Wklej pełną odpowiedź Shelly.GetDeviceInfo.');
    }
    return { model, rule, info };
  }

  if (typeof module !== 'undefined' && module.exports) {
    module.exports = { validIP, verifyDevice };
    return;
  }

  const form = document.getElementById('installer');
  const ipField = document.getElementById('ip');
  const result = document.getElementById('result');
  const status = document.getElementById('status');
  const fallback = document.getElementById('manual');
  const infoLink = document.getElementById('device-info');
  const manualData = document.getElementById('device-json');
  const button = document.getElementById('check-device');
  const config = window.garageInstallerConfig;
  let generation = 0;
  let activeSocket = null;
  let timer = null;

  function reset() {
    generation++;
    if (timer !== null) clearTimeout(timer);
    timer = null;
    if (activeSocket) activeSocket.close();
    activeSocket = null;
    button.disabled = false;
    result.replaceChildren();
    status.textContent = '';
    fallback.hidden = true;
    infoLink.removeAttribute('href');
  }

  function showFallback(ip) {
    fallback.hidden = false;
    infoLink.href = 'http://' + ip + '/rpc/Shelly.GetDeviceInfo';
    infoLink.textContent = 'Otwórz informacje Shelly pod ' + ip;
  }

  function showVerified(payload, ip, source) {
    result.replaceChildren();
    const { rule, info } = verifyDevice(payload, config.rules);
    const details = document.createElement('p');
    details.textContent = 'Zgodność potwierdzona ' + source + ': ' + rule.name +
      ' · ' + info.model + ' · Gen' + info.gen + ' · oryginalny firmware ' + info.ver + '.';
    const link = document.createElement('a');
    link.href = 'http://' + ip + '/ota?url=' + encodeURIComponent(config.installURL);
    link.textContent = 'Rozpocznij instalację Garage BLU na ' + ip;
    link.className = 'install-link';
    result.append(details, link);
    status.textContent = 'Model, generacja i wersja fabrycznego firmware są zgodne.';
  }

  ipField.addEventListener('input', () => { reset(); manualData.value = ''; });
  manualData.addEventListener('input', () => { result.replaceChildren(); status.textContent = ''; });

  form.addEventListener('submit', function (event) {
    event.preventDefault();
    reset();
    const ip = ipField.value.trim();
    if (!validIP(ip)) {
      status.textContent = 'Wpisz poprawny adres IPv4, np. 192.168.1.198.';
      return;
    }
    const current = generation;
    showFallback(ip);
    status.textContent = 'Odczytuję model i firmware Shelly… Jeśli przeglądarka poprosi o dostęp do sieci lokalnej, zezwól na połączenie z urządzeniem.';
    button.disabled = true;
    let socket;
    let finished = false;
    function finish(error, payload) {
      if (finished || generation !== current) return;
      finished = true;
      if (timer !== null) clearTimeout(timer);
      timer = null;
      button.disabled = false;
      if (socket) socket.close();
      activeSocket = null;
      if (error) {
        status.textContent = error;
        return;
      }
      try {
        showVerified(payload, ip, 'przez połączenie z urządzeniem');
        fallback.hidden = true;
      } catch (error) {
        status.textContent = error.message;
      }
    }
    const blocked = 'Nie udało się odczytać urządzenia. Sprawdź adres IP i połączenie z tą samą siecią. Jeśli przeglądarka blokuje dostęp, użyj poniżej odpowiedzi JSON z Shelly.';
    try {
      socket = new WebSocket('ws://' + ip + '/rpc');
      activeSocket = socket;
      timer = setTimeout(() => finish(blocked), 6000);
      socket.addEventListener('open', () => {
        if (generation === current && !finished) socket.send(JSON.stringify({ id: 1, src: 'garage-blu-installer', method: 'Shelly.GetDeviceInfo' }));
      });
      socket.addEventListener('message', event => {
        try {
          const payload = JSON.parse(event.data);
          if (payload.id !== 1) return;
          if (payload.error) finish('Shelly odmówił odczytu informacji. Użyj odpowiedzi JSON poniżej.');
          else finish(null, payload);
        } catch (_) { finish('Nieprawidłowa odpowiedź Shelly. Użyj odpowiedzi JSON poniżej.'); }
      });
      socket.addEventListener('error', () => finish(blocked));
      socket.addEventListener('close', () => finish(blocked));
    } catch (_) { finish(blocked); }
  });

  document.getElementById('verify-json').addEventListener('click', function () {
    const ip = ipField.value.trim();
    reset();
    if (!validIP(ip)) {
      status.textContent = 'Najpierw wpisz poprawny adres IP Shelly.';
      return;
    }
    showFallback(ip);
    try {
      showVerified(JSON.parse(manualData.value), ip, 'na podstawie wklejonych danych z urządzenia');
    } catch (error) {
      status.textContent = error instanceof SyntaxError ? 'Nieprawidłowy JSON. Skopiuj całą odpowiedź z otwartej strony Shelly.' : error.message;
    }
  });
})();
