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
<div id="body" class="container">
			<h1>Scheduler</h1>

			<div class="row panel panel-default">
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

			<div class="row panel panel-default">
				<div class="panel-heading">Alarm Schedules</div>
				<div class="panel-body">
					<p class="alarm-help">Configure alarm schedules below. Each alarm needs at least one weekday. Enter start and shut-off as <strong>hh:mm</strong> (click hours or minutes to edit each pair). Use <strong>Save Alarm</strong> or <strong>Remove Alarm</strong> to store changes; the result appears in the command log above.</p>
					<div id="alarm-list"></div>
					<button type="button" id="add-alarm" class="btn btn-primary"><span class="btn-icon" aria-hidden="true">&#10133;</span> Add Alarm</button>
				</div>
			</div>
		</div>

		<script src="app.js"></script>
	</body>
</html>

)=====";
