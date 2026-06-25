// display.html file in raw data format for PROGMEM
// "=====( CONTENT )====="; Equal chars required as terminating char

const char display_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html prefix="og: http://purl.org/dc/elements/1.1/" lang="en-EN">
	<head>
		<meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">

		<link rel="stylesheet" type="text/css" href="app.css" />

		<title>Webradio - Display and behaviour</title>
	</head>

	<body itemscope itemtype="http://schema.org/WebPage">
		<nav class="navbar navbar-default navbar-static-top">
			<div id="body" class="container">
				<div class="navbarDistance">
					<a href="index.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#9835;</span><span class="nav-label">WEBRADIO</span></span></a>
					<a href="scheduler.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#9200;</span><span class="nav-label">SCHEDULER</span></span></a>
					<a href="config.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#9881;</span><span class="nav-label">CONFIGURATION</span></span></a>
					<a href="sound.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#128266;</span><span class="nav-label">SOUND SETTINGS</span></span></a>
					<a href="display.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#128187;</span><span class="nav-label">DISPLAY SETTINGS</span></span></a>
					<a href="maintenance.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#128295;</span><span class="nav-label">MAINTENANCE</span></span></a>
					<a href="diagnostics.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#128202;</span><span class="nav-label">DIAGNOSTICS</span></span></a>
					<a href="ota.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#11015;</span><span class="nav-label">OTA UPDATE</span></span></a>
					<a href="about.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#8505;</span><span class="nav-label">ABOUT</span></span></a>
				</div>
			</div>
		</nav>

<div id="body" class="container">
			<h1>Display and behaviour</h1>

			<div id="display-settings" class="row panel panel-default">
				<div class="panel-heading">Options</div>
				<div class="panel-body">
					<p class="config-help">Dark mode switches on automatically between the configured hours. VU meter and stream watchdog can be disabled here. Click <strong>Save settings</strong> to store changes in NVS.</p>
					<div class="form-group">
						<label for="feat-dark-start">Dark mode from (hour 0-23)</label>
						<input type="number" class="form-control" id="feat-dark-start" min="0" max="23" value="20">
					</div>
					<div class="form-group">
						<label for="feat-dark-end">Dark mode until (hour 0-23)</label>
						<input type="number" class="form-control" id="feat-dark-end" min="0" max="23" value="7">
					</div>
					<div class="form-group">
						<label for="feat-alarm-fade">Alarm volume fade (seconds)</label>
						<input type="number" class="form-control" id="feat-alarm-fade" min="5" max="600" value="15">
					</div>
					<div class="checkbox">
						<label><input type="checkbox" id="feat-vu-meter" checked> VU meter on TFT</label>
					</div>
					<div class="checkbox">
						<label><input type="checkbox" id="feat-watchdog" checked> Stream watchdog (auto-reconnect)</label>
					</div>
					<button type="button" id="display-settings-save" class="btn btn-success"><span class="btn-icon" aria-hidden="true">&#128190;</span> Save settings</button>
				</div>
			</div>
		</div>

		<script src="app.js"></script>
	</body>
</html>

)=====";
