// index.html file in raw data format for PROGMEM
// "=====( CONTENT )====="; Equal chars required as terminating char
// BASE64 Glyphicons: https://andrew.hedges.name/experiments/glyphicons/
/*
<div class="panel panel-default">
	<div class="panel-heading">Webradio Control</div>
	<div class="panel-body">
	</div>
</div>
					<li><a href="mp3play.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABoAAAAaCAYAAACpSkzOAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOjQ5MUFGOTFBMDZBNzExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOjQ5MUFGOTE5MDZBNzExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PkFCGFQAAAEZSURBVHjaxFaLDYIwEKUuIBvYDWSEuoEjMAKjMAIjsIGOgBvUDXCCejVHUq9fIZyXvJCUu/fS1+tHGGOqkhBCSPhIMqyhXhcRWKEYkLi3hDY1Ao05MskVEaix2PwIW1MXCUE0gGmFyAJb21Be4a4RrIMVuQOOxOEnYES4cUWcyPgLoIB78tYI7aIzmQFdynus7TCXzqz2rAusyRyyICHWBMT6LyHsrtUiGTHpCtHZdAGSsUQcbfRmtfx094mOEGRb2Mn1+EK29Rmhxdo2IUQd+pwoigyqAiG3s1Qg1+M8VNviDLjB/hsyeXKrUGnorUIPwAXsakuSeZqBrb25NyzPEcR2qHJcE/wX395X+f8eJ3s9twTXA/ItwACdK7DO9WgFFwAAAABJRU5ErkJggg==" /> MP3 Player</span></a></li>
 */

const char config_html[] PROGMEM = R"=====(
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

		<title>Webradio - Configuration</title>
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
					<a href="diagnostics.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#128202;</span><span class="nav-label">DIAGNOSTICS</span></span></a>
					<a href="maintenance.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#128295;</span><span class="nav-label">MAINTENANCE</span></span></a>
					<a href="ota.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#11015;</span><span class="nav-label">OTA UPDATE</span></span></a>
					<a href="about.html"><span class="btn btn-primary nav-btn"><span class="nav-icon" aria-hidden="true">&#8505;</span><span class="nav-label">ABOUT</span></span></a>
				</div>
			</div>
		</nav>

<div id="body" class="container">
			<h1>Configuration</h1>

			<div class="row panel panel-default">
				<div class="panel-heading">WiFi Networks</div>
				<div class="panel-body">
					<p class="config-help">Add WiFi networks in priority order. The radio tries them from top to bottom. Signal strength is refreshed automatically.</p>
					<div id="wifi-list" class="config-item-list"></div>
					<button type="button" id="add-wifi" class="btn btn-primary"><span class="btn-icon" aria-hidden="true">&#128246;</span> Add Network</button>
				</div>
			</div>

			<div class="row panel panel-default">
				<div class="panel-heading">Clock and NTP</div>
				<div class="panel-body">
					<div class="row">
						<div class="form-group col-xs-12 col-sm-6">
							<label for="clk-server">NTP server</label>
							<input type="text" class="form-control" id="clk-server" placeholder="pool.ntp.org">
						</div>
						<div class="form-group col-xs-6 col-sm-3">
							<label for="clk-offset" title="Hours added to UTC for your local standard (winter) time. Range UTC-11 to UTC+14. Example: UTC+1 for Central Europe.">UTC offset</label>
							<select class="form-control" id="clk-offset" title="Hours added to UTC for your local standard (winter) time. Range UTC-11 to UTC+14. Example: UTC+1 for Central Europe."></select>
						</div>
						<div class="form-group col-xs-6 col-sm-3">
							<label for="clk-dst" title="Extra hours added during daylight saving time (summer). Enable +1 hour if your region uses summer time.">Summer time (DST)</label>
							<select class="form-control" id="clk-dst" title="Extra hours added during daylight saving time (summer). Enable +1 hour if your region uses summer time."></select>
						</div>
					</div>
				</div>
			</div>

			<div class="row panel panel-default">
				<div class="panel-heading">Startup</div>
				<div class="panel-body">
					<div class="row">
						<div class="form-group col-xs-12 col-sm-4">
							<label for="cfg-preset">Active preset on boot</label>
							<select class="form-control" id="cfg-preset"></select>
						</div>
					</div>
					<p class="config-help">Equalizer settings are on the <a href="sound.html">Sound Settings</a> page. Volume is on the <a href="index.html">Webradio</a> page.</p>
				</div>
			</div>

			<div class="row panel panel-default">
				<div class="panel-heading">Radio Presets</div>
				<div class="panel-body">
					<p class="config-help">Define stream URLs for your preset buttons. Drag entries to change the display order on the Webradio page. Slot numbers identify each preset.</p>
					<div id="preset-list" class="config-item-list"></div>
					<button type="button" id="add-preset" class="btn btn-primary"><span class="btn-icon" aria-hidden="true">&#10133;</span> Add Preset</button>
				</div>
			</div>

			<div class="row panel panel-default">
				<div class="panel-heading">GPIO Button Actions</div>
				<div class="panel-body">
					<p class="config-help">Map physical GPIO pins to radio commands, for example <code>uppreset = 1</code> or <code>stop</code>.</p>
					<div id="gpio-list" class="config-item-list"></div>
					<button type="button" id="add-gpio" class="btn btn-primary"><span class="btn-icon" aria-hidden="true">&#128297;</span> Add GPIO Action</button>
				</div>
			</div>

			<div class="row panel panel-default">
				<div class="panel-heading config-collapse-heading" id="raw-config-toggle" role="button" tabindex="0" aria-expanded="false" aria-controls="raw-config-panel">
					Raw Configuration
					<span class="config-collapse-icon" aria-hidden="true">&#9654;</span>
				</div>
				<div class="panel-body config-collapse-panel hidden" id="raw-config-panel">
					<p class="config-help">View, edit, or paste the complete JSON configuration. Use Reload to refresh from the form fields, then Save JSON to apply. Restore Defaults resets the device to factory settings.</p>
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
				<div class="panel-heading" id="config-dialog-title">Edit Entry</div>
				<div class="panel-body" id="config-dialog-body"></div>
			</div>
		</div>
		
		<script src="app.js"></script>
	</body>
</html>

)=====";
