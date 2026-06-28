// display.html file in raw data format for PROGMEM
// "=====( CONTENT )====="; Equal chars required as terminating char

const char display_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html prefix="og: http://purl.org/dc/elements/1.1/" lang="en-EN">
	<head>
		<meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">

		<link rel="stylesheet" type="text/css" href="app.css" />

		<title>Webradio - Display and behaviour</title>
	</head>

	<body itemscope itemtype="http://schema.org/WebPage">	
@include_nav@



<div id="body" class="container sound-page">
			<header class="sound-page-header">
				<h1>Display and behaviour</h1>
				<p class="sound-page-lead">Tune dark mode and visual runtime features for display and stream handling.</p>
			</header>
			<div class="sound-studio">

			<div id="display-settings" class="row panel panel-default sound-card">
				<div class="panel-heading">Options</div>
				<div class="panel-body">
					<p class="config-help">Dark mode switches on automatically between the configured hours. Changes are saved immediately after each interaction.</p>
					<div class="display-settings-grid">
					<div class="form-group display-setting-card">
						<label for="feat-dark-start">Dark mode from</label>
						<select class="form-control sound-select display-hour-select" id="feat-dark-start"></select>
					</div>
					<div class="form-group display-setting-card">
						<label for="feat-dark-end">Dark mode until</label>
						<select class="form-control sound-select display-hour-select" id="feat-dark-end"></select>
					</div>
					</div>
					<div class="checkbox display-toggle">
						<label><input type="checkbox" id="feat-vu-meter" checked> Enable VU meter on TFT</label>
					</div>
				</div>
			</div>
			</div>
		</div>

		<script src="app.js"></script>
	</body>
</html>

)=====";
