# Windows Chrome/Edge Extension Force Install (Policy Registry)

Force-install Google Chrome and Microsoft Edge extensions on **Windows** with **no user interaction** by managing the browser policy registry keys:

- `HKLM\SOFTWARE\Policies\Google\Chrome\ExtensionInstallForcelist`
- `HKLM\SOFTWARE\Policies\Microsoft\Edge\ExtensionInstallForcelist`

This approach uses the official enterprise policy mechanism that Chrome/Edge read on startup.

---

## How it works

Chrome and Edge can be centrally managed using Windows registry policies. When an extension is listed in `ExtensionInstallForcelist`, the browser will automatically download and install it (and keep it installed).

Each entry is a **string value** with:
- **Name**: a unique integer string like `1`, `2`, `3`, ...
- **Data**: `EXTENSION_ID;UPDATE_URL`

Example value data:
- `cjpalhdlnbpafiamejdnhcphjbkeiagm;https://clients2.google.com/service/update2/crx`

Most Chrome Web Store extensions use the same update URL above.

---

## Requirements

- Windows 10/11 or Windows Server
- Administrative privileges (writes to `HKLM`)
- Chrome and/or Edge installed
- Browser restart (or policy refresh) after applying changes

> Note: Policies are typically applied under `HKLM` for device-wide enforcement.

---

## Extension ID

You need the extension ID (32 lowercase letters). You can find it by:
1. Open extension page in Chrome Web Store
2. The URL contains the ID, e.g.:
   - `https://chromewebstore.google.com/detail/<name>/<EXTENSION_ID>`

---
