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
@include_nav@
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
