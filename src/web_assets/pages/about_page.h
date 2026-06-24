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
		<meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
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
<div id="body" class="container">
			<h1>About</h1>

			<div class="row panel panel-default">
				<div class="panel-heading">Firmware</div>
				<div class="panel-body">
					<p class="config-help">Installed version, release information, and over-the-air updates are on a dedicated page.</p>
					<a href="ota.html" class="btn btn-default"><span class="btn-icon" aria-hidden="true">&#11015;</span> Open OTA Update</a>
				</div>
			</div>

			<p class="lead">
				This is a high-performance ESP32-based web radio featuring an integrated alarm clock and sleep functionality.
			</p>

			<div class="row">
				<div class="col-md-6">
					<h3>Core Features</h3>
					<ul>
						<li><strong>Web Radio & Streaming:</strong> Plays high-quality audio streams with full metadata (Artist & Track) display.</li>
						<li><strong>Alarm & Sleep Timer:</strong> Integrated wake-up functionality and a customizable sleep timer for nightly use.</li>
						<li><strong>Smart Connectivity:</strong> Stores multiple WiFi profiles and automatically connects to the best available network.</li>
						<li><strong>Time Synchronization:</strong> Supports custom NTP servers (e.g., <code>pool.ntp.org</code>) for precise timekeeping.</li>
					</ul>
				</div>
				<div class="col-md-6">
					<h3>Smart Display & Audio</h3>
					<ul>
						<li><strong>Auto-Display Control:</strong> The screen activates instantly upon interaction (volume, station change) and features a 5-minute auto-dim/power-off function.</li>
						<li><strong>Advanced DSP:</strong> Full control over audio characteristics:
							<ul>
								<li>Treble Gain & Frequency tuning</li>
								<li>Bass Gain & Frequency enhancement</li>
							</ul>
						</li>
					</ul>
				</div>
			</div>

			<hr>

			<div class="row panel panel-default">
				<div class="panel-heading">Source code</div>
				<div class="panel-body">
					<p class="config-help">Firmware source, documentation, and OTA release files are available on GitHub.</p>
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

		<script src="app.js"></script>
	</body>
</html>

)=====";
