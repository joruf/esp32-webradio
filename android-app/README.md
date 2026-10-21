# ESP32 Webradio – Android App

Native Android shell around the existing ESP32 web interface. **No firmware changes** — the app loads the same pages as your browser (`index.html`, `scheduler.html`, `config.html`, etc.).

## Requirements

- Android 7.0+ (API 24)
- Phone and ESP32 on the **same Wi‑Fi**
- ESP32 IP address (from router or radio display), e.g. `192.168.1.42`

## Install on your Android device

You need the APK file: `app/build/outputs/apk/debug/app-debug.apk`  
(Build it yourself as described below, or copy it from someone who already built the project.)

### Option A — Install via USB (PC + cable)

Best if you build the app on your computer and your phone is connected by USB.

1. **Enable Developer options** on the phone  
   **Settings → About phone →** tap **Build number** 7 times.

2. **Enable USB debugging**  
   **Settings → Developer options → USB debugging** → On.

3. On **Xiaomi / MIUI / HyperOS** also enable **Install via USB** (or *USB debugging (Security settings)*) in Developer options — otherwise `adb install` may fail with *Install canceled by user*.

4. Connect the phone with a USB cable. On the phone, confirm **Allow USB debugging** if asked.

5. On the PC (from the `android-app` folder):

```bash
export PATH=~/Android/Sdk/platform-tools:$PATH   # or wherever adb is installed
adb devices                                    # phone should appear as "device"
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

6. Open **ESP32 Webradio** on the home screen.

### Option B — Install APK on the phone (no PC)

1. Copy `app-debug.apk` to the phone (USB file transfer, e‑mail, cloud, messenger, etc.) into **Downloads**.

2. Open the APK with the **Files** app (or tap the download notification).

3. If Android asks for permission: allow **Install unknown apps** for that app (Files, Chrome, …).

4. Tap **Install** and open **ESP32 Webradio**.

### First launch

1. Open the app.
2. The app **scans Wi‑Fi** for ESP32 Webradio devices and lists every radio it finds (station, firmware, IP).
3. Tap the radio you want, or enter an IP manually and tap **Save and open**.

The web interface appears inside the app. Use the menu (⋮) for **Change radio** or **Reload**.

**Tips**

- Phone and radios must be on the **same Wi‑Fi**. Multiple radios are listed so you can pick one.
- Manual fallback: IP from the radio display / router, or `http://radio.fritz.box` on a Fritzbox LAN.
- Close the **browser on your PC** while using the app — the ESP32 handles only one HTTP client well at a time.
- If the page stays empty: menu → **Reload**, or force‑close the app and open it again.

## Build

### Android Studio

1. **File → Open** → select the `android-app` folder
2. Wait for Gradle sync
3. **Run** on a device or emulator (emulator only works if it can reach your LAN IP)

### Command line (Linux, without Android Studio)

One-time setup on this machine (already done if you used the assistant):

- Android SDK: `~/Android/Sdk` (platform 34, build-tools 34.0.0, platform-tools/adb)
- JDK 17: `~/.local/jdk/jdk-17.0.19+10` (Gradle needs a **JDK**, not only a JRE)
- `local.properties` with `sdk.dir=...` (gitignored; created automatically by the script below)

Build:

```bash
cd android-app
./build-debug.sh
```

Or manually:

```bash
export ANDROID_HOME=~/Android/Sdk
export JAVA_HOME=~/.local/jdk/jdk-17.0.19+10
export PATH="$JAVA_HOME/bin:$ANDROID_HOME/platform-tools:$PATH"
./gradlew assembleDebug
```

APK output: `app/build/outputs/apk/debug/app-debug.apk`

Install on a connected phone (USB debugging enabled):

```bash
export PATH=~/Android/Sdk/platform-tools:$PATH
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

**Alternative (apt):** `sudo apt install openjdk-17-jdk adb google-android-platform-34-installer google-android-build-tools-34.0.0-installer` and set `sdk.dir=/usr/lib/android-sdk` in `local.properties`.

## First use

Same as **First launch** under [Install on your Android device](#install-on-your-android-device) above.

## Features

- Full web UI (Webradio, Scheduler, Configuration, Sound, Maintenance, About)
- **LAN discovery:** scan Wi‑Fi for all ESP32 Webradios and pick one from the list
- Pull down to refresh
- Back button navigates within the WebView
- HTTP cleartext allowed for local network (required by ESP32)

## Notes

- Discovery probes the local subnet with `/?getJSON`. Manual IP entry remains available.
- Only one HTTP client is served well by the ESP32 at a time — close the browser on PC while using the app if commands fail.

## License

Same as the ESP32 Webradio project (MIT).
