// maintenance.html file in raw data format for PROGMEM
// "=====( CONTENT )====="; Equal chars required as terminating char

const char maintenance_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html prefix="og: http://purl.org/dc/elements/1.1/" lang="en-EN">
	<head>
		<meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<link rel="stylesheet" type="text/css" href="app.css" />
		<title>Webradio - Maintenance</title>
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
			<h1>Maintenance</h1>

			<div id="maintenance-page" class="row panel panel-default panel-danger-zone">
				<div class="panel-heading">Device Maintenance</div>
				<div class="panel-body">
					<p class="config-help">These actions affect the entire device. Use them only when you know what you are doing.</p>
					<div class="maintenance-actions">
						<button type="button" id="reset" class="btn btn-danger"><span class="btn-icon" aria-hidden="true">&#128260;</span> Restart Webradio</button>
					</div>
				</div>
			</div>

			<div class="row panel panel-default panel-danger-zone">
				<div class="panel-heading">Erase Storage</div>
				<div class="panel-body">
					<p class="config-help">Permanently delete all configuration stored in NVS (EEPROM). On the next boot, factory default settings will be written automatically.</p>
					<div class="maintenance-actions">
						<button type="button" id="erase-eeprom" class="btn btn-danger"><span class="btn-icon" aria-hidden="true">&#128465;</span> Erase EEPROM</button>
					</div>
				</div>
			</div>

			<div class="row panel panel-default">
				<div class="panel-heading">
					Raw Configuration
				</div>
				<div class="panel-body" id="raw-config-panel">
					<p class="config-help">View, edit, or paste the complete JSON configuration. Use Reload to refresh current values from NVS, then Save JSON to apply. Restore Defaults resets the device to factory settings.</p>
					<textarea id="raw-config-json" class="form-control config-raw-json" rows="12" spellcheck="false"></textarea>
					<div class="config-raw-actions">
						<button type="button" id="raw-config-load" class="btn btn-default"><span class="btn-icon" aria-hidden="true">&#8635;</span> Reload</button>
						<button type="button" id="raw-config-save" class="btn btn-success"><span class="btn-icon" aria-hidden="true">&#128190;</span> Save JSON</button>
						<button type="button" id="restore-config" class="btn btn-warning"><span class="btn-icon" aria-hidden="true">&#8634;</span> Restore Defaults</button>
					</div>
				</div>
			</div>
		</div>

		<div id="config-dialog" class="config-dialog hidden" aria-hidden="true">
			<div class="config-dialog-backdrop"></div>
			<div class="config-dialog-card panel panel-default">
				<div class="panel-heading" id="config-dialog-title">Confirm</div>
				<div class="panel-body" id="config-dialog-body"></div>
			</div>
		</div>

		<script src="app.js"></script>
	</body>
</html>

)=====";
