// index.html file in raw data format for PROGMEM
// NOTE: We have to try to save load requests on startup, otherwise the CPU could hang
// "=====( CONTENT )====="; Equal chars required as terminating char
// BASE64 Glyphicons: https://andrew.hedges.name/experiments/glyphicons/
/*
<div class="panel panel-default">
  <div class="panel-heading">Webradio Control</div>
  <div class="panel-body">
  </div>
</div>
 */

const char about_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html prefix="og: http://purl.org/dc/elements/1.1/" lang="en-EN">
	<head>
		<meta http-equiv="content-type" content="text/html; charset=UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
		<!--
		<link rel="icon" href="favicon.ico" type="image/x-icon">
		<link rel="icon" href="favicon.ico" type="image/png">
		-->

		<link rel="stylesheet" type="text/css" href="app.css" />

		<title>Webradio - About</title>
	</head>
	
	<body itemscope itemtype="http://schema.org/WebPage">	
@include_nav@

<div id="body" class="container sound-page">
			<header class="sound-page-header">
				<h1>About</h1>
				<p class="sound-page-lead">What this device does: streaming, alarms, web control, diagnostics and firmware updates.</p>
			</header>
			<div class="sound-studio">

			<p class="lead">
				ESP32 Webradio is a dual-core Internet radio with VS1053 hardware decoding, a colour TFT display,
				rotary-encoder control and a full web interface. It streams internet radio and local MP3 files,
				wakes you with scheduled alarms, and keeps a persistent log of boots, shutdowns and runtime issues.
			</p>

			<div class="row">
				<div class="col-md-6">
					<h3>Playback &amp; Audio</h3>
					<ul>
						<li><strong>Internet radio:</strong> MP3, Ogg and AAC streams with ICY metadata (station name and song title) on the display and in the web UI.</li>
						<li><strong>VS1053 decoder:</strong> Hardware MP3 decoding offloads the ESP32; a dedicated stream task keeps the audio buffer full for gap-free playback.</li>
						<li><strong>Stream watchdog:</strong> Detects stalled streams and reconnects automatically after short network dropouts.</li>
						<li><strong>MP3 Stream page:</strong> Play MP3 files from your PC or phone over the local Wi-Fi (folder served via a simple HTTP server).</li>
						<li><strong>Presets &amp; playlists:</strong> Multiple station presets; supports <code>.m3u</code> playlist URLs.</li>
						<li><strong>Volume:</strong> 0–100 scale mapped to an audible range; all changes in <strong>5-step increments</strong> (encoder and web UI).</li>
						<li><strong>Equalizer:</strong> Treble and bass gain and frequency control via the Sound Settings page.</li>
					</ul>
				</div>
				<div class="col-md-6">
					<h3>Control &amp; Display</h3>
					<ul>
						<li><strong>Rotary encoder:</strong> Turn for volume (5-step), single click for play/mute, double click to change station.</li>
						<li><strong>Web interface:</strong> Control playback, volume, presets, alarms and configuration from any browser on your network.</li>
						<li><strong>Live updates:</strong> Station and title refresh via Server-Sent Events with polling fallback.</li>
						<li><strong>TFT dashboard:</strong> Date, time, station, song title, volume bar, VU meter and Wi-Fi signal on the built-in display.</li>
						<li><strong>Auto backlight:</strong> Display turns on with any interaction and dims after inactivity.</li>
						<li><strong>Dark mode:</strong> Optional scheduled dark theme for the web UI (Display Settings).</li>
					</ul>
				</div>
			</div>

			<div class="row">
				<div class="col-md-6">
					<h3>Scheduler &amp; Timers</h3>
					<ul>
						<li><strong>Alarm clock:</strong> Multiple alarms with weekday selection, start/stop times and a gradual <strong>Alarm volume fade</strong> (5–60&nbsp;s in 5&nbsp;s steps) that fades the volume in when an alarm starts. If the stream cannot be restored after watchdog reconnects, a soft offline wake tone plays (beep, rising chirp, bird peeps, water bubble, descending cascade, or arcade jingle; selectable on the Scheduler page).</li>
						<li><strong>Sleep / snooze timer:</strong> Stop playback automatically after a chosen number of minutes (index page or <code>/?snooze=XX</code>); the volume fades out gently over the same Alarm volume fade duration.</li>
						<li><strong>NTP time:</strong> Configurable time server, UTC offset and daylight-saving for reliable alarm triggering.</li>
					</ul>
				</div>
				<div class="col-md-6">
					<h3>Network &amp; Configuration</h3>
					<ul>
						<li><strong>Multi-WiFi:</strong> Several stored networks with automatic fallback; captive portal when no known network is found.</li>
						<li><strong>JSON configuration:</strong> WiFi, presets, GPIO buttons, clock, volume, tone and alarms stored in NVS and editable in the browser.</li>
						<li><strong>Partial saves:</strong> Changing one setting updates only that value — volume, station and other options stay untouched.</li>
						<li><strong>Diagnostics page:</strong> Chip, memory, flash, network and partition details for troubleshooting.</li>
					</ul>
				</div>
			</div>

			<div class="row">
				<div class="col-md-6">
					<h3>Power Log</h3>
					<ul>
						<li><strong>Boot &amp; shutdown history:</strong> Last 20 events in NVS — planned restarts (user, firmware update, factory reset) and unexpected ones (power loss, brownout, crash).</li>
						<li><strong>Errors &amp; warnings:</strong> Runtime issues such as WiFi loss, stream failures, OTA problems and config save errors.</li>
						<li><strong>Timestamps:</strong> Date and time when NTP is available; otherwise marked as unknown until the clock syncs.</li>
					</ul>
				</div>
				<div class="col-md-6">
					<h3>Firmware Updates (OTA)</h3>
					<ul>
						<li><strong>Online update:</strong> Check and install the latest release from the project manifest.</li>
						<li><strong>Specific version:</strong> Install any release listed in the repository (including older versions for downgrade).</li>
						<li><strong>Local upload:</strong> Flash a <code>.bin</code> file from your PC with browser and device validation (size, ESP32 image signature, chip type).</li>
						<li><strong>Safe flashing:</strong> Invalid images are rejected before writing to flash; the OTA screen shows progress without display artefacts.</li>
					</ul>
				</div>
			</div>

			<hr>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Hardware</div>
				<div class="panel-body">
					<p class="config-help">Active hardware profile: <strong id="hw-profile-about">-</strong></p>
					<p class="config-help">SD card: <strong id="hw-sd-about">-</strong></p>
					<p class="config-help">Supported templates: <code>wroom_tft</code> (ESP32-WROOM + VS1053 + rotary + TFT) and <code>cam_headless</code> (ESP32-CAM without camera + VS1053 + rotary + onboard SD). Switch under Configuration. Use the matching OTA firmware image for your board.</p>
					<p class="config-help">Firmware: <strong id="firmware-version">-</strong> <span id="firmware-date"></span></p>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Source code</div>
				<div class="panel-body">
					<p class="config-help">Firmware source, documentation, OTA manifests and release binaries are on GitHub. Built with PlatformIO; the web UI is embedded in PROGMEM.</p>
					<p>
						<a href="https://github.com/joruf/esp32-webradio" target="_blank" rel="noopener" class="btn btn-default"><span class="btn-icon" aria-hidden="true">&#128279;</span> github.com/joruf/esp32-webradio</a>
					</p>
					<p class="config-help">Download or clone the repository:</p>
					<p><code>git clone https://github.com/joruf/esp32-webradio.git</code></p>
				</div>
			</div>

			<hr>
			
			<div class="row">
				<div class="col-xs-12 text-center">
					<p>
						Developed by <strong>Loresoft</strong><br>
						<a href="https://www.loresoft.de" target="_blank" class="btn btn-default"><span class="btn-icon" aria-hidden="true">&#127760;</span> Visit Loresoft</a>
					</p>
				</div>
			</div>
			</div>
		</div>

		<script src="app.js"></script>
	</body>
</html>

)=====";
