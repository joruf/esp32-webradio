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
					<div class="checkbox display-toggle">
						<label><input type="checkbox" id="feat-display-rotate-180"> Rotate display 180&deg;</label>
					</div>
					<p class="config-help">Use 180&deg; rotation when the panel is mounted upside down. Takes effect immediately after saving.</p>
					<div class="display-settings-grid">
					<div class="form-group display-setting-card">
						<label for="feat-display-colstart" title="ST7735 GRAM column start. In landscape this shifts the vertical origin. Leave 0 for aligned BLACKTAB panels.">TFT colstart</label>
						<select class="form-control sound-select" id="feat-display-colstart" title="ST7735 GRAM column start. In landscape this shifts the vertical origin. Leave 0 for aligned BLACKTAB panels."></select>
					</div>
					<div class="form-group display-setting-card">
						<label for="feat-display-rowstart" title="ST7735 GRAM row start. In landscape this is the horizontal X offset. Try 1 if the left column is missing and junk appears on the right.">TFT rowstart (landscape X)</label>
						<select class="form-control sound-select" id="feat-display-rowstart" title="ST7735 GRAM row start. In landscape this is the horizontal X offset. Try 1 if the left column is missing and junk appears on the right."></select>
					</div>
					</div>
					<p class="config-help">Per-panel GRAM offset for ST7735 clones. Default 0 keeps correctly aligned panels. If one radio shows a 1-column horizontal shift, set rowstart to 1 on that device only.</p>
				</div>
			</div>
			</div>
		</div>

		<script src="app.js"></script>
	</body>
</html>

)=====";
