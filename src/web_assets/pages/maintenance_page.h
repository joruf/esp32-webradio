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
						<a href="ota.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#11015;</span><span class="nav-label">OTA UPDATE</span></span></a>
						<!--<a href="firmware.html"><span class="btn btn-primary nav-btn"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABsAAAAWCAYAAAAxSueLAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOjg5RUQ2MzQwMDZDQjExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOjg5RUQ2MzNGMDZDQjExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PnMZ5UQAAAEpSURBVHjavFbbEYIwEAyM/6YD7UBKoAPTgZRgCZRgCZRgCdiBJWAHdBAX5zLimdfhY2cWZyB3S47LnspaqyQEGrAH7YxXsAV1NFYgosEzE7Ee0eobYn1CyHEM7VBSOitgJxIDtmANGiqPFVJniQGnBck5Dc+7UgxFUXT4OSg/buDA7k0V2HjW1tRQT7AdGemHpy4dIjEnF8cDY63d05qX+5mdep3WlWzre/Ub7PB52lL9D00JRQ2eQftjsU1Jnrb/x9ZW5A4pXOhgh+CeVeA6lmhMtPuWdexbNzLXCR2D6fw+LsG2pSRDjgH7Xmb+0u5QxryvofKkKlAFDHtwY0cyYozEBz+aZ5FRc8yOX/C3oEvNrRALSiACDKAnE69FcQvFNImNkri7AAMAebfFpPog2YAAAAAASUVORK5CYII=" /> FIRMWARE UPGRADE</span></a>-->
						<a href="maintenance.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#128295;</span><span class="nav-label">MAINTENANCE</span></span></a>
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
