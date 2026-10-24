// about.html file in raw data format for PROGMEM
// "=====( CONTENT )====="; Equal chars required as terminating char

const char about_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html prefix="og: http://purl.org/dc/elements/1.1/" lang="en-EN">
	<head>
		<meta http-equiv="content-type" content="text/html; charset=UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">

		<link rel="stylesheet" type="text/css" href="app.css" />

		<title>Webradio - About</title>
	</head>

	<body itemscope itemtype="http://schema.org/WebPage">
@include_nav@

<div id="body" class="container sound-page">
			<header class="sound-page-header">
				<h1>About</h1>
				<p class="sound-page-lead">ESP32 Webradio by Joachim Ruf, Loresoft: streaming, alarms, web control, display and OTA updates.</p>
			</header>
			<div class="sound-studio">

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Author</div>
				<div class="panel-body text-center">
					<p>
						<strong>Joachim Ruf</strong><br>
						<strong>Loresoft</strong>
					</p>
					<p class="config-help">Hardware and firmware for home internet radio.</p>
					<p>
						<a href="https://www.loresoft.de" target="_blank" rel="noopener" class="btn btn-default"><span class="btn-icon" aria-hidden="true">&#127760;</span> www.loresoft.de</a>
						<a href="https://github.com/joruf/esp32-webradio" target="_blank" rel="noopener" class="btn btn-default"><span class="btn-icon" aria-hidden="true">&#128279;</span> GitHub</a>
					</p>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Hardware</div>
				<div class="panel-body">
					<p class="config-help">Active hardware profile: <strong id="hw-profile-about">-</strong></p>
					<p class="config-help">SD card: <strong id="hw-sd-about">-</strong></p>
					<p class="config-help">Profiles: <code>wroom_tft</code> (ESP32-WROOM + VS1053 + rotary + ST7735 TFT), <code>cam_headless</code> (ESP32-CAM without camera + VS1053 + rotary + SD_MMC), <code>s3_touch_28b</code> (Waveshare ESP32-S3 Touch LCD 2.8B). Switch under Configuration. Use the matching firmware image for your board.</p>
					<p class="config-help">Firmware: <strong id="firmware-version">-</strong> <span id="firmware-date"></span></p>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Source code</div>
				<div class="panel-body">
					<p class="config-help">Firmware, documentation, OTA manifests and release binaries are on GitHub. Built with PlatformIO; the web UI is embedded in PROGMEM.</p>
					<p>
						<a href="https://github.com/joruf/esp32-webradio" target="_blank" rel="noopener" class="btn btn-default"><span class="btn-icon" aria-hidden="true">&#128279;</span> github.com/joruf/esp32-webradio</a>
					</p>
					<p class="config-help">Clone:</p>
					<p><code>git clone https://github.com/joruf/esp32-webradio.git</code></p>
				</div>
			</div>

			<hr>

			<p class="lead">
				ESP32 Webradio is a FreeRTOS internet radio with VS1053 hardware decoding, colour TFT (or touch LCD),
				rotary-encoder control and a full browser UI. It plays internet streams and local MP3 over Wi-Fi,
				wakes you with scheduled alarms, and keeps a persistent log of boots, shutdowns and runtime issues.
			</p>

			<div class="row">
				<div class="col-md-6">
					<h3>Playback &amp; Audio</h3>
					<ul>
						<li><strong>Internet radio:</strong> MP3, Ogg and AAC streams with ICY metadata (station and song title) on the display and in the web UI.</li>
						<li><strong>VS1053 decoder:</strong> Hardware decoding offloads the ESP32; a dedicated stream task keeps the buffer full for gap-free playback.</li>
						<li><strong>Stream watchdog:</strong> Detects stalled streams and reconnects after short network dropouts.</li>
						<li><strong>MP3 Stream page:</strong> Play MP3 files from a PC or phone over the local Wi-Fi.</li>
						<li><strong>Presets:</strong> Many station presets; supports <code>.m3u</code> playlist URLs.</li>
						<li><strong>Volume:</strong> 0–100 in <strong>5-step</strong> increments (encoder and web UI).</li>
						<li><strong>Equalizer:</strong> Treble and bass via Sound Settings.</li>
						<li><strong>YouTube links:</strong> Clicking the current title or a title-history entry copies the YouTube search link to the clipboard.</li>
					</ul>
				</div>
				<div class="col-md-6">
					<h3>Control &amp; Display</h3>
					<ul>
						<li><strong>Rotary encoder:</strong> Volume (5-step), single click play/mute, double click to change station.</li>
						<li><strong>Web interface:</strong> Playback, volume, presets, alarms and configuration from any browser on the LAN.</li>
						<li><strong>Live updates:</strong> Station and title via Server-Sent Events, with polling fallback.</li>
						<li><strong>TFT dashboard:</strong> Date, time, station, title, volume, VU meter and Wi-Fi status.</li>
						<li><strong>Display rotation:</strong> Optional 180&deg; landscape flip in Display Settings (for upside-down mounts).</li>
						<li><strong>Auto backlight:</strong> Turns on with interaction and dims after inactivity.</li>
						<li><strong>Dark mode:</strong> Scheduled dark theme for the web UI.</li>
					</ul>
				</div>
			</div>

			<div class="row">
				<div class="col-md-6">
					<h3>Scheduler &amp; Timers</h3>
					<ul>
						<li><strong>Alarm clock:</strong> Multiple alarms with weekdays, start/stop times and a gradual volume fade. If the stream cannot be restored, a selectable offline wake tone plays.</li>
						<li><strong>Sleep / snooze:</strong> Stop playback after a chosen number of minutes; volume fades out gently.</li>
						<li><strong>NTP time:</strong> Configurable server, UTC offset and daylight saving for reliable alarms.</li>
					</ul>
				</div>
				<div class="col-md-6">
					<h3>Network &amp; Configuration</h3>
					<ul>
						<li><strong>Multi-WiFi:</strong> Several networks with automatic fallback; SoftAP / captive portal when none are found.</li>
						<li><strong>JSON configuration:</strong> WiFi, presets, GPIO, clock, volume, tone, features and alarms in NVS, editable in the browser.</li>
						<li><strong>Partial saves:</strong> Changing one setting does not overwrite unrelated values.</li>
						<li><strong>Diagnostics:</strong> Chip, memory, flash, network and partition details.</li>
					</ul>
				</div>
			</div>

			<div class="row">
				<div class="col-md-6">
					<h3>Power Log</h3>
					<ul>
						<li><strong>Boot &amp; shutdown history:</strong> Last events in NVS for planned and unexpected restarts.</li>
						<li><strong>Errors &amp; warnings:</strong> WiFi loss, stream failures, OTA and config save problems.</li>
						<li><strong>Timestamps:</strong> Date and time when NTP is available.</li>
					</ul>
				</div>
				<div class="col-md-6">
					<h3>Firmware Updates (OTA)</h3>
					<ul>
						<li><strong>Online update:</strong> Check and install the latest release from the project manifest.</li>
						<li><strong>Specific version:</strong> Install any listed release, including older ones for downgrade.</li>
						<li><strong>Local upload:</strong> Flash a <code>.bin</code> from your PC with size and image checks.</li>
						<li><strong>Safe flashing:</strong> Invalid images are rejected before writing; progress is shown on the display.</li>
					</ul>
				</div>
			</div>

			</div>
		</div>

		<script src="app.js"></script>
	</body>
</html>

)=====";
