// eventlog.html file in raw data format for PROGMEM
const char eventlog_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en-EN">
	<head>
		<meta http-equiv="content-type" content="text/html; charset=UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<link rel="stylesheet" type="text/css" href="app.css" />
		<title>Webradio - Power Log</title>
	</head>
	<body>
@include_nav@

<div id="body" class="container sound-page">
			<header class="sound-page-header">
				<h1>Power Log</h1>
				<p class="sound-page-lead">The most recent shutdown and boot events. Times are shown from the NTP clock when it was available, otherwise as &quot;unknown&quot;.</p>
			</header>
			<div class="sound-studio">
			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Boot &amp; shutdown history</div>
				<div class="panel-body">
					<div class="eventlog-table-wrap">
						<table class="eventlog-table">
							<thead>
								<tr>
									<th>#</th>
									<th>Event</th>
									<th>Reason</th>
									<th>Time</th>
									<th>Firmware</th>
								</tr>
							</thead>
							<tbody id="eventlog-body">
								<tr><td colspan="5">Loading...</td></tr>
							</tbody>
						</table>
					</div>
					<button type="button" id="eventlog-refresh" class="btn btn-primary">Refresh</button>
				</div>
			</div>
			</div>
		</div>
		<script src="app.js"></script>
	</body>
</html>
)=====";
