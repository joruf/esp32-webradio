// sound.html – PROGMEM embedded Sound / EQ page
// Terminator must match opening delimiter length exactly: )=====

const char sound_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html prefix="og: http://purl.org/dc/elements/1.1/" lang="en-EN">
	<head>
		<meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<link rel="stylesheet" type="text/css" href="app.css" />
		<title>Webradio - Sound</title>
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
				<h1>Sound</h1>
				<p class="sound-page-lead">Master level and VS1053 tone shaping. All adjustments apply to the device immediately. Equalizer changes are saved to NVS automatically.</p>
			</header>

			<div id="sound-settings" class="sound-studio">
				<section class="panel panel-default sound-card sound-master-card">
					<div class="panel-heading sound-card-heading">
						<span class="sound-card-icon" aria-hidden="true">&#128266;</span>
						<div class="sound-card-heading-text">
							<strong>Master output</strong>
							<span class="sound-card-sub">Digital volume before the amplifier stage</span>
						</div>
					</div>
					<div class="panel-body">
						<div class="sound-master-toolbar">
							<label for="sound-volume" class="sound-master-label">Level</label>
							<div class="sound-master-meter" aria-hidden="true">
								<span class="sound-master-meter-fill" id="sound-volume-meter"></span>
							</div>
							<span class="sound-master-value"><span id="sound-volume-value">0</span><small>%</small></span>
							<input type="range" class="sound-master-slider" id="sound-volume" min="0" max="100" step="1" aria-valuemin="0" aria-valuemax="100">
						</div>
					</div>
				</section>

				<section class="panel panel-default sound-card sound-eq-card">
					<div class="panel-heading sound-card-heading">
						<span class="sound-card-icon" aria-hidden="true">&#127932;</span>
						<div class="sound-card-heading-text">
							<strong>Equalizer</strong>
							<span class="sound-card-sub">Bass shelf and treble shelf (VS1053 tone registers)</span>
						</div>
					</div>
					<div class="panel-body">
						<div class="sound-eq-layout">
							<div class="sound-eq-visual" aria-hidden="true">
								<svg class="sound-eq-svg" viewBox="0 0 360 100" preserveAspectRatio="none">
									<defs>
										<linearGradient id="sound-eq-fill" x1="0%" y1="0%" x2="0%" y2="100%">
											<stop offset="0%" stop-color="#5b9bd5" stop-opacity="0.45"/>
											<stop offset="100%" stop-color="#5b9bd5" stop-opacity="0.05"/>
										</linearGradient>
									</defs>
									<line x1="0" y1="50" x2="360" y2="50" class="sound-eq-grid-line"/>
									<path id="sound-eq-curve" class="sound-eq-curve" d="M0,50 L360,50"/>
									<path id="sound-eq-curve-fill" class="sound-eq-curve-fill" d="M0,50 L360,50 L360,100 L0,100 Z"/>
								</svg>
								<div class="sound-eq-axis">
									<span>Low</span>
									<span>Mid</span>
									<span>High</span>
								</div>
							</div>

							<div class="sound-band-grid">
								<article class="sound-band sound-band-treble">
									<header class="sound-band-header">
										<span class="sound-band-tag">High shelf</span>
										<h2 class="sound-band-title">Treble</h2>
										<p class="sound-band-summary" id="sound-treble-summary">Off @ 1 kHz</p>
									</header>
									<div class="sound-param">
										<label for="tone-ha">Gain</label>
										<select class="form-control sound-select" id="tone-ha"></select>
									</div>
									<div class="sound-param">
										<label for="tone-hf">Corner frequency</label>
										<select class="form-control sound-select" id="tone-hf"></select>
									</div>
								</article>

								<article class="sound-band sound-band-bass">
									<header class="sound-band-header">
										<span class="sound-band-tag">Low shelf</span>
										<h2 class="sound-band-title">Bass</h2>
										<p class="sound-band-summary" id="sound-bass-summary">Off from 10 Hz</p>
									</header>
									<div class="sound-param">
										<label for="tone-la">Gain</label>
										<select class="form-control sound-select" id="tone-la"></select>
									</div>
									<div class="sound-param">
										<label for="tone-lf">Corner frequency</label>
										<select class="form-control sound-select" id="tone-lf"></select>
									</div>
								</article>
							</div>
						</div>

						<footer class="sound-eq-footer">
							<p class="sound-eq-hint">EQ changes are applied live and stored in your radio configuration.</p>
							<button type="button" class="btn btn-default" id="sound-eq-reset"><span class="btn-icon" aria-hidden="true">&#8634;</span> Reset EQ</button>
						</footer>
					</div>
				</section>
			</div>
		</div>

		<script src="app.js"></script>
	</body>
</html>

)=====";
