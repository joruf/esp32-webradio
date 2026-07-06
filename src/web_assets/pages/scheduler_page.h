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
@include_nav@



<div id="body" class="container sound-page">
			<header class="sound-page-header">
				<h1>Scheduler</h1>
				<p class="sound-page-lead">Control sleep timer and alarm schedules with persistent settings stored on the device.</p>
			</header>
			<div class="sound-studio">

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Snooze Timer</div>
				<div class="panel-body">
					<p class="config-help">Set 1 min or 5-minute steps up to 60 min. While running, the slider moves toward zero; drag to set a new duration.</p>
					<div id="snooze-status" class="snooze-status" hidden>
						<p class="snooze-countdown-line">Off <strong id="snooze-off-at">--:--</strong> | left <strong id="snooze-remaining">--</strong></p>
					</div>
					<div class="snooze-slider-toolbar">
						<div class="snooze-slider-header">
							<label for="snooze-slider" class="snooze-slider-label">Timer</label>
							<span class="snooze-slider-value" id="snooze-slider-value">Off</span>
						</div>
						<input type="range" class="snooze-slider-input range-slider-ticked" id="snooze-slider" min="0" max="13" step="1" value="0" list="snooze-slider-ticks" aria-valuemin="0" aria-valuemax="13">
					</div>
					<datalist id="snooze-slider-ticks">
						<option value="0" label="Off"></option>
						<option value="1" label="1"></option>
						<option value="2" label="5"></option>
						<option value="3" label="10"></option>
						<option value="4" label="15"></option>
						<option value="5" label="20"></option>
						<option value="6" label="25"></option>
						<option value="7" label="30"></option>
						<option value="8" label="35"></option>
						<option value="9" label="40"></option>
						<option value="10" label="45"></option>
						<option value="11" label="50"></option>
						<option value="12" label="55"></option>
						<option value="13" label="60"></option>
					</datalist>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Alarm Schedules</div>
				<div class="panel-body">
					<div class="form-group display-setting-card">
						<label for="feat-alarm-fade">Alarm volume fade</label>
						<input type="range" class="form-control range-slider-ticked" id="feat-alarm-fade" min="5" max="60" step="5" value="15" list="feat-alarm-fade-ticks">
						<datalist id="feat-alarm-fade-ticks">
							<option value="5" label="5"></option>
							<option value="10" label="10"></option>
							<option value="15" label="15"></option>
							<option value="20" label="20"></option>
							<option value="25" label="25"></option>
							<option value="30" label="30"></option>
							<option value="35" label="35"></option>
							<option value="40" label="40"></option>
							<option value="45" label="45"></option>
							<option value="50" label="50"></option>
							<option value="55" label="55"></option>
							<option value="60" label="60"></option>
						</datalist>
						<div class="config-help">Time to gradually change the volume: fade in from silence to your current volume when an alarm starts, and fade out to silence when the snooze timer ends. Duration: <strong id="feat-alarm-fade-value">15 sec</strong></div>
					</div>
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
