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

const char mp3play_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html prefix="og: http://purl.org/dc/elements/1.1/" lang="en-EN">
	<head>
		<meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
		<!--
		<link rel="icon" href="favicon.ico" type="image/x-icon">
		<link rel="icon" href="favicon.ico" type="image/png">
		-->

		<link rel="stylesheet" type="text/css" href="app.css" />

		<title>Webradio - MP3 Player</title>
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
			<h1>MP3 Player</h1>	
	
			<div class="panel panel-default">
				<div class="panel-heading">MP3 File List<br /><small>Click on a MP3 files to play it</small></div>
				<div class="panel-body">

					<div class="row">
						<div class="col-xs-12 col-sm-2">
							<div>
								<button id="downpreset" class="btn btn-regular" onclick="httpGet('downpreset=1', true)"><span class="btn-icon" aria-hidden="true">&#9664;</span> PREVIOUS FILE</button>
							</div>
							<div>
								<button id="uppreset" class="btn btn-regular" onclick="httpGet('uppreset=1', true)"><span class="btn-icon" aria-hidden="true">&#9654;</span> NEXT FILE</button>
							</div>
							<div>
								<button id="mp3track" class="btn btn-regular" onclick="httpGet('mp3track=0', true)"><span class="btn-icon" aria-hidden="true">&#128256;</span> RANDOM FILE</button>
							</div>
						</div>
						<div class="col-xs-12 col-sm-10">
							<label for="seltrack">MP3 files on SD card</label><br />
							<select id="seltrack" class="select selectw" size="16">
								<option value="-1">Select a track here</option>
							</select>
						</div>
					</div>
					
				</div>
			</div>
		</div>	
	
		<script src="app.js"></script>
	</body>
</html>
)=====";
