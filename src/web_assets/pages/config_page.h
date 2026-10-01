// index.html file in raw data format for PROGMEM
// "=====( CONTENT )====="; Equal chars required as terminating char
// BASE64 Glyphicons: https://andrew.hedges.name/experiments/glyphicons/
/*
<div class="panel panel-default">
	<div class="panel-heading">Webradio Control</div>
	<div class="panel-body">
	</div>
</div>
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
@include_nav@

<div id="body" class="container sound-page">
			<header class="sound-page-header">
				<h1>Configuration</h1>
				<p class="sound-page-lead">Manage WiFi, startup behavior, presets and GPIO mappings. Changes are saved directly to NVS.</p>
			</header>
			<div class="sound-studio">

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">WiFi Networks</div>
				<div class="panel-body">
					<p class="config-help">Add WiFi networks in priority order. The radio tries them from top to bottom. Signal strength is refreshed automatically.</p>
					<div id="wifi-list" class="config-item-list"></div>
					<button type="button" id="add-wifi" class="btn btn-primary"><span class="btn-icon" aria-hidden="true">&#128246;</span> Add Network</button>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
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

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Hardware profile</div>
				<div class="panel-body">
					<p class="config-help">Select a wiring template. This sets all IO pins, SD backend and whether the Display Settings page is shown. The device reboots after applying. Use the matching OTA firmware image for your board (WROOM vs CAM).</p>
					<div class="row">
						<div class="form-group col-xs-12 col-sm-8">
							<label for="hw-profile-select">Configuration template</label>
							<select class="form-control" id="hw-profile-select"></select>
						</div>
						<div class="form-group col-xs-12 col-sm-4">
							<label>&nbsp;</label>
							<button type="button" id="hw-profile-apply" class="btn btn-primary btn-block"><span class="btn-icon" aria-hidden="true">&#128190;</span> Apply and reboot</button>
						</div>
					</div>
					<p class="config-help" id="hw-profile-help"></p>
					<p class="config-help">Build default for this firmware: <strong id="hw-profile-build-default">-</strong>. Active: <strong id="hw-profile-active">-</strong>.</p>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Startup</div>
				<div class="panel-body">
					<div class="row">
						<div class="form-group col-xs-12 col-sm-4">
							<label for="cfg-preset">Active preset on boot</label>
							<select class="form-control" id="cfg-preset"></select>
						</div>
					</div>
					<p class="config-help">Equalizer settings are on the <a href="sound.html">Sound Settings</a> page. Volume is on the <a href="index.html">Webradio</a> page. Raw JSON editing is available on the <a href="maintenance.html">Maintenance</a> page.</p>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Playback Reliability</div>
				<div class="panel-body">
					<div class="checkbox display-toggle">
						<label><input type="checkbox" id="feat-watchdog" checked> Enable stream watchdog (auto-reconnect)</label>
					</div>
					<p class="config-help">When enabled, this monitors network streams only (not SD/local files). If no stream data arrives for 30 seconds, the player stops and automatically reconnects to the same stream (up to 5 retries per stall). When disabled, no automatic reconnect is triggered by this watchdog.</p>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Radio Presets</div>
				<div class="panel-body">
					<p class="config-help">Define stream URLs for your preset buttons. Drag entries to change the display order on the Webradio page. Slot numbers identify each preset.</p>
					<div id="preset-list" class="config-item-list"></div>
					<button type="button" id="add-preset" class="btn btn-primary"><span class="btn-icon" aria-hidden="true">&#10133;</span> Add Preset</button>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">GPIO Button Actions</div>
				<div class="panel-body">
					<p class="config-help">Map physical GPIO pins to radio commands, for example <code>uppreset = 1</code> or <code>stop</code>.</p>
					<div id="gpio-list" class="config-item-list"></div>
					<button type="button" id="add-gpio" class="btn btn-primary"><span class="btn-icon" aria-hidden="true">&#128297;</span> Add GPIO Action</button>
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
