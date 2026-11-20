#!/usr/bin/env python3
"""Apply Garage BLU's bounded KVS persistence fix to prepared build sources.

Only the known upstream Save implementation is accepted. No device files or
credentials are accessed. Run after mos --build-dry-run fetches dependencies.
"""
import argparse
import hashlib
from pathlib import Path

START = 'HAPError KVStore::Save() const {'
END = '\n// static\n'
UPSTREAM_SHA256 = '5419bfe89e85a9868851a53f8909c073d3ac16e72ed7606aa530cd747a2e1e26'
REPLACEMENT = r'''HAPError KVStore::Save() const {
    // Garage BLU: never replace a valid store with a failed/short write.
    // Frozen's FILE printer returns a byte count, not a negative I/O error;
    // stdio can also defer ENOSPC until fflush/fclose.
    const std::string tmpFileName = fileName_ + ".tmp";
    FILE* fp = fopen(tmpFileName.c_str(), "w");
    if (fp == nullptr) {
        LOG(LL_ERROR, ("Failed to open %s for writing", tmpFileName.c_str()));
        return kHAPError_Unknown;
    }
    struct CheckedFile {
        FILE* stream;
        bool complete;
    } file = {fp, true};
    struct json_out out = {};
    out.u.data = &file;
    out.printer = [](struct json_out* output, const char* data, size_t size) -> int {
        auto* target = static_cast<CheckedFile*>(output->u.data);
        if (!target->complete) return 0;
        const size_t written = fwrite(data, 1, size, target->stream);
        if (written != size) target->complete = false;
        return static_cast<int>(written);
    };
    int num = 0;
    json_printf(&out, "{");
    for (const Item* itm = items_; itm != nullptr; itm = itm->next, num++) {
        if (num != 0) json_printf(&out, ",");
        const uint16_t fkey = (static_cast<uint16_t>(itm->dom) << 8) | itm->key;
        json_printf(&out, "\n  \"%u\": %V", fkey, &itm->data[0], itm->len);
    }
    json_printf(&out, "\n}\n");
    bool complete = file.complete && !ferror(fp);
    if (fflush(fp) != 0) complete = false;
    if (fclose(fp) != 0) complete = false;
    if (!complete) {
        LOG(LL_ERROR, ("Failed to persist %s; previous store retained", fileName_.c_str()));
        // A partial first save must not be recovered by Load() as a valid .tmp.
        if (remove(tmpFileName.c_str()) != 0) {
            LOG(LL_ERROR, ("Failed to remove incomplete %s", tmpFileName.c_str()));
        }
        return kHAPError_Unknown;
    }
    // Both controller profiles use LittleFS, which supports replacing a file
    // with rename. Do not unlink the previous store before this operation.
    if (rename(tmpFileName.c_str(), fileName_.c_str()) != 0) {
        LOG(LL_ERROR, ("Failed to replace %s; previous store retained", fileName_.c_str()));
        if (remove(tmpFileName.c_str()) != 0) {
            LOG(LL_ERROR, ("Failed to remove pending %s", tmpFileName.c_str()));
        }
        return kHAPError_Unknown;
    }
    LOG(LL_DEBUG, ("Saved %d keys to %s", num, fileName_.c_str()));
    return kHAPError_None;
}
'''


def patch_source(source: str) -> str:
    if source.count(START) != 1:
        raise ValueError('Expected exactly one HomeKit KVS Save implementation')
    start = source.index(START)
    end = source.index(END, start)
    current = source[start:end]
    if current == REPLACEMENT:
        return source
    if hashlib.sha256(current.encode()).hexdigest() != UPSTREAM_SHA256:
        raise ValueError('Unknown HomeKit KVS Save implementation; review upstream before building')
    return source[:start] + REPLACEMENT + source[end:]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path, help='Prepared dependency HAPPlatformKeyValueStore.cpp')
    args = parser.parse_args()
    original = args.source.read_text()
    patched = patch_source(original)
    if patched != original:
        args.source.write_text(patched)
    print('HomeKit KVS persistence patch verified')


if __name__ == '__main__':
    main()
