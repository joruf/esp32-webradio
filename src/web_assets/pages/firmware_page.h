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

const char firmware_html[] PROGMEM = R"=====(
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

		<title>Webradio - Firmware Upgrade</title>
	</head>
	
	<body itemscope itemtype="http://schema.org/WebPage">	
@include_nav@
<div id="body" class="container sound-page">
			<header class="sound-page-header">
				<h1>Firmware Upgrade</h1>
				<p class="sound-page-lead">Install the latest firmware directly on the device. Keep power connected during the whole process.</p>
			</header>
			<div class="sound-studio">
			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Legacy OTA Installer</div>
				<div class="panel-body">
					<p><strong>Current Firmware:</strong> <span id="firmware">...</span></p>
					<p>After pressing install, the firmware is downloaded from the configured update host.</p>
					<p><strong>Please note:</strong> Do not unplug your power supply.</p>
					<button class="btn btn-alert" onclick="httpGet('update')"><span class="btn-icon" aria-hidden="true">&#11015;</span> Install latest Firmware Update</button>
				</div>
			</div>
			</div>
		</div>

		<script src="app.js"></script>
	</body>
</html>

)=====";
