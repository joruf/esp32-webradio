// scheduler.html file in raw data format for PROGMEM
// "=====( CONTENT )====="; Equal chars required as terminating char

const char scheduler_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html prefix="og: http://purl.org/dc/elements/1.1/" lang="en-EN">
	<head>
		<meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">

		<link rel="stylesheet" type="text/css" href="app.css" />

		<title>Webradio - Scheduler</title>
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

<div id="body" class="container sound-page">
			<header class="sound-page-header">
				<h1>Scheduler</h1>
				<p class="sound-page-lead">Control sleep timer and alarm schedules with persistent settings stored on the device.</p>
			</header>
			<div class="sound-studio">

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Snooze Timer</div>
				<div class="panel-body">
					<p class="config-help">Snooze stops playback after the selected duration.</p>
					<div class="row">
						<div class="form-group col-xs-12 col-sm-6 col-md-3 col-lg-3">
							<select class="form-control" id="snooze">
								<option value="0">Off</option>
								<option value="1">1 minute</option>
								<option value="5">5 minutes</option>
								<option value="10">10 minutes</option>
								<option value="15">15 minutes</option>
								<option value="20">20 minutes</option>
								<option value="25">25 minutes</option>
								<option value="30">30 minutes</option>
								<option value="45">45 minutes</option>
								<option value="60">60 minutes</option>
							</select>
						</div>
					</div>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Alarm Schedules</div>
				<div class="panel-body">
					<p class="alarm-help">Configure alarm schedules below. Each alarm needs at least one weekday. Enter start and shut-off as <strong>hh:mm</strong> (click hours or minutes to edit each pair). Use <strong>Save Alarm</strong> or <strong>Remove Alarm</strong> to store changes; the result appears in the command log above.</p>
					<div id="alarm-list"></div>
					<button type="button" id="add-alarm" class="btn btn-primary"><span class="btn-icon" aria-hidden="true">&#10133;</span> Add Alarm</button>
				</div>
			</div>
			</div>
		</div>

		<script src="app.js"></script>
	</body>
</html>

)=====";
