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
					<li><a href="mp3play.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABo...............==" /> MP3 Player</span></a></li>
 */

const char index_html[] PROGMEM = R"=====(
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

		<title>Webradio</title>
	</head>
	
	<body itemscope itemtype="http://schema.org/WebPage">	
@include_nav@
<div id="body" class="container">
			<h1>Webradio</h1>
	
			<div class="row panel panel-default">
				<div class="panel-heading">
					Metadata
					<small>
						<span id="datelabel">DD.MM.YYYY</span>
					</small>
				</div>
				<div class="panel-body">
					<span id="statlabel" style="color: red;">Lade Sender...</span> |
					<a id="youtubelink" href="#" target="_blank" style="color: blue; text-decoration: none;">
							<span id="songlabel">Lade Titel...</span>
					</a>
					<div id="title-history" class="title-history-table" aria-live="polite">
						<div class="title-history-empty">No songs recognized yet.</div>
					</div>
				</div>
			</div>

			<div class="row panel panel-default">
				<div class="panel-heading">Webradio Control</div>

				<div class="panel-body webradio-control-body">
					<div class="webradio-control-toolbar">
						<label for="index-volume" class="webradio-control-label">Volume</label>
						<span class="webradio-volume-value"><span id="index-volume-value">0</span>%</span>
						<input type="range" class="form-control webradio-control-slider" id="index-volume" min="0" max="100" step="1">
						<div class="webradio-playback-actions">
							<button id="play" type="button" class="btn btn-success"><span class="btn-icon" aria-hidden="true">&#9654;</span> Play</button>
							<button id="stop" type="button" class="btn btn-danger"><span class="btn-icon" aria-hidden="true">&#9209;</span> Stop</button>
						</div>
					</div>
				</div>
			</div>

			<div class="row panel panel-default">
				<div class="panel-heading">Radio Channel List<br /><small>Click on a radio station to change the channel or insert a new stream</small></div>
				<div class="panel-body">

					<div class="input-group">
						<span class="input-group-btn">
							<button id="setstat" type="button" class="btn btn-regular"><span class="btn-icon" aria-hidden="true">&#9654;</span> Play Stream</button>
						</span>
						<input id="station" class="form-control" placeholder="Insert a radio stream">
					</div>
					<br /><br />
					
					<select id="preset" class="form-control webradio-preset-list" size="10">
						<option value="-1">-- Select --</option>
					</select>
				</div>
			</div>

			<div class="row panel panel-default">
				<div class="panel-heading">Display</div>
				<div class="panel-body webradio-display-body">
					<button id="backlight_on" type="button" class="btn btn-warning"><span class="btn-icon" aria-hidden="true">&#9728;</span> Display On</button>
					<button id="backlight_off" type="button" class="btn btn-regular"><span class="btn-icon" aria-hidden="true">&#9790;</span> Display Off</button>
				</div>
			</div>

		</div>
	
		<script src="app.js"></script>
	</body>
</html>

)=====";
