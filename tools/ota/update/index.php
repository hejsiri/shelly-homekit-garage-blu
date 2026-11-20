<?php
declare(strict_types=1);

// Stock -> Garage BLU entry point: one package per model, exact compatibility gate.
header('Cache-Control: no-store');
header('X-Content-Type-Options: nosniff');
const DOWNLOAD_BASE = 'https://hejsiri.pl/firmware/garage-blu/';
const INSTALL_URL = DOWNLOAD_BASE . 'update/';

function failUpdate(int $status, string $message): void {
    http_response_code($status);
    header('Content-Type: text/plain; charset=utf-8');
    echo $message . "\nHelp: " . INSTALL_URL . "\n";
    exit;
}

$compatibility = json_decode((string) @file_get_contents(__DIR__ . '/compatibility.json'), true);
if (!is_array($compatibility) || !isset($compatibility['ShellyPlus1'], $compatibility['Shelly1Gen3'])) {
    failUpdate(503, 'Compatibility information is temporarily unavailable.');
}

$deviceID = trim($_SERVER['HTTP_X_MGOS_DEVICE_ID'] ?? '');
if ($deviceID !== '') {
    $device = preg_split('/\s+/', $deviceID)[0];
    if (!preg_match('/^(shellyplus1|shelly1g3)-[0-9a-f]{12}$/iD', $device, $match)) {
        failUpdate(404, 'Unsupported Shelly model. Garage BLU supports only Shelly Plus 1 and Shelly 1 Gen3; Mini, PM and Gen4 are not supported.');
    }
    $model = strtolower($match[1]) === 'shellyplus1' ? 'ShellyPlus1' : 'Shelly1Gen3';
    $rule = $compatibility[$model];
    $firmware = trim($_SERVER['HTTP_X_MGOS_FW_VERSION'] ?? '');
    if (preg_match('/^(esp32|esp32c3)\s+(\S+)(?:\s+.*)?$/D', $firmware, $fwMatch)) {
        $expectedPlatform = $model === 'ShellyPlus1' ? 'esp32' : 'esp32c3';
        if ($fwMatch[1] !== $expectedPlatform) {
            failUpdate(412, 'Incompatible Shelly hardware platform. No firmware will be sent.');
        }
        $version = $fwMatch[2];
    } elseif (preg_match('/^(?:[0-9]{8}-[0-9]{6}\/)?([0-9]+\.[0-9]+\.[0-9]+)(?:-g[0-9a-f]+)?$/iD', $firmware, $fwMatch)) {
        $version = $fwMatch[1];
    } else {
        failUpdate(412, 'Cannot verify the original Shelly firmware version. Required stock firmware for ' . $rule['name'] . ': ' . $rule['stock_version'] . '. No firmware will be sent.');
    }
    if ($version !== $rule['stock_version']) {
        failUpdate(412, 'Incompatible original Shelly firmware ' . $version . '. Required stock firmware for ' . $rule['name'] . ': ' . $rule['stock_version'] . '. Update older stock in the Shelly panel. Newer versions require a compatibility check. No firmware will be sent.');
    }
    $metadata = @file_get_contents(dirname(__DIR__) . '/install.json');
    $releases = $metadata !== false ? json_decode($metadata, true) : null;
    $release = is_array($releases) ? ($releases[$model] ?? null) : null;
    $url = is_array($release) ? ($release['url'] ?? '') : '';
    $prefix = DOWNLOAD_BASE . 'shelly-homekit-garage-blu-' . $model . '-';
    if (!is_string($url) || strpos($url, $prefix) !== 0 || $url !== DOWNLOAD_BASE . basename($url) ||
        !preg_match('/^shelly-homekit-garage-blu-' . $model . '-[0-9A-Za-z._-]+\.zip$/D', basename($url))) {
        failUpdate(503, 'Firmware is temporarily unavailable for this model. Try again later.');
    }
    $file = dirname(__DIR__) . '/' . basename($url);
    if (!is_file($file) || !isset($release['size']) || filesize($file) !== $release['size']) {
        failUpdate(503, 'Firmware is temporarily unavailable for this model. Try again later.');
    }
    header('Location: ' . $url, true, 302);
    exit;
}
if (isset($_SERVER['HTTP_X_MGOS_FW_VERSION'])) {
    failUpdate(400, 'Shelly did not send its model identifier (X-MGOS-Device-ID).');
}
header('Content-Type: text/html; charset=utf-8');
?>
<!doctype html>
<html lang="pl"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Instalacja Garage BLU</title>
<style>body{font:17px/1.6 system-ui,sans-serif;max-width:760px;margin:48px auto;padding:0 24px;color:#172b3a;background:#f6f8fa}a{color:#006da8}code{overflow-wrap:anywhere}label,input,button,textarea{font:inherit}input,textarea{padding:8px;max-width:100%;box-sizing:border-box}button{padding:9px 15px;cursor:pointer}textarea{display:block;width:100%;margin:12px 0;font-family:monospace}#result{display:block;margin-top:16px}.note{padding:16px;background:#fff;border-left:4px solid #0085b2}.install-link{display:inline-block;padding:12px 18px;color:white;background:#006da8;border-radius:6px}#status:empty,#result:empty{display:none}[hidden]{display:none!important}</style>
<h1>Instalacja Garage BLU</h1>
<p>Najpierw sprawdzamy model, generację i oryginalny firmware Shelly. Link instalacji pojawi się dopiero po potwierdzeniu zgodności.</p>
<ol><li>Zaktualizuj <strong>oryginalny firmware Shelly do najnowszej stabilnej wersji</strong> w jego panelu.</li>
<li>Wpisz lokalny adres IP Shelly i sprawdź zgodność. Następnie otwórz link instalacji.</li>
<li>Poczekaj na zakończenie zapisu i restart. Nie odłączaj zasilania.</li></ol>
<p>Obsługiwane zestawy: <strong>Shelly Plus 1 / SNSW-001X16EU / Gen2 / stock 1.7.6</strong> oraz <strong>Shelly 1 Gen3 / S3SW-001X16EU / Gen3 / stock 2.2.0</strong>. Stan sprawdzenia wersji: 11.11.2026. Mini, 1PM i Gen4 nie są obsługiwane. Starsze i nowsze wersje firmware są blokowane do czasu sprawdzenia ich zgodności.</p>
<p class="note">Zgodność układu paczek została sprawdzona offline. Instalacja i działanie blu.79 na fizycznych urządzeniach pozostają do potwierdzenia.</p>
<form id="installer"><label for="ip">Adres IP Shelly</label><br><input id="ip" placeholder="192.168.1.198" required inputmode="decimal" autocomplete="off"> <button id="check-device">Sprawdź zgodność</button></form>
<p id="status" role="status" aria-live="polite"></p>
<section id="manual" hidden><h2>Sprawdzenie przez odpowiedź z urządzenia</h2>
<p>Jeśli przeglądarka blokuje połączenie z siecią lokalną, <a id="device-info" target="_blank" rel="noopener noreferrer">otwórz informacje Shelly</a>, skopiuj całą odpowiedź i wklej ją poniżej. Odczyt informacji nie uruchamia aktualizacji. Dane pozostają w tej przeglądarce.</p>
<label for="device-json">Odpowiedź JSON z Shelly.GetDeviceInfo</label><textarea id="device-json" rows="6" spellcheck="false" autocomplete="off"></textarea>
<button id="verify-json" type="button">Sprawdź wklejone dane</button></section>
<div id="result" aria-live="polite"></div>
<noscript><p>Włącz JavaScript, aby sprawdzić urządzenie i przygotować link instalacji.</p></noscript>
<h2>Jeśli instalacja się nie powiedzie</h2>
<p>Błąd <code>PT update is req'd but unsupported by the new fw</code> / <code>pt check failed</code> oznacza niezgodny układ partycji. Sprawdź, czy fabryczny firmware jest aktualny i czy paczka odpowiada modelowi. Jeśli masz już wskazaną wyżej wersję fabryczną, zachowaj pełny log błędu — nie wymuszaj zapisu ani resetu.</p>
<p>Instalator sprawdza model sprzętowy, generację, aplikację i dokładną wersję fabrycznego firmware. Przy pobieraniu paczki serwer dodatkowo sprawdza model oraz wersję przekazaną przez Shelly. Jego panel może wyświetlić własny komunikat techniczny zamiast treści błędu serwera.</p>
<p>Ten adres służy do konwersji z fabrycznego firmware. W Garage BLU używaj Firmware → Check. Starsze Garage BLU na Gen3 wymagają osobnej procedury przejścia z poprzedniego układu partycji.</p>
<p><a href="../">Pobieranie i pełna instrukcja</a></p>
<script>window.garageInstallerConfig = <?= json_encode(['installURL' => INSTALL_URL, 'rules' => $compatibility], JSON_HEX_TAG | JSON_HEX_AMP | JSON_HEX_APOS | JSON_HEX_QUOT | JSON_UNESCAPED_SLASHES) ?>;</script>
<script src="installer.js?v=2"></script></html>
