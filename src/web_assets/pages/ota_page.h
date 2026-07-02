// ota.html file in raw data format for PROGMEM
// "=====( CONTENT )====="; Equal chars required as terminating char

const char ota_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html prefix="og: http://purl.org/dc/elements/1.1/" lang="en-EN">
	<head>
		<meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">

		<link rel="stylesheet" type="text/css" href="app.css" />

		<title>Webradio - OTA Update</title>
	</head>

	<body itemscope itemtype="http://schema.org/WebPage">	
@include_nav@



<div id="body" class="container sound-page">
			<header class="sound-page-header">
				<h1>OTA Update</h1>
				<p class="sound-page-lead">Check firmware status, review release history and install updates over the air.</p>
			</header>
			<div class="sound-studio">

			<div id="ota-settings" class="row panel panel-default sound-card">
				<div class="panel-heading">Firmware status</div>
				<div class="panel-body">
					<p class="config-help">Check online for a newer firmware build, then install it over the air. Do not unplug power during the update.</p>
					<table class="table table-condensed ota-status-table">
						<tbody>
							<tr><th scope="row">Installed version</th><td id="ota-current-version">...</td></tr>
							<tr><th scope="row">Installed release</th><td id="ota-current-date">...</td></tr>
							<tr><th scope="row">Available version</th><td id="ota-latest-version">-</td></tr>
							<tr><th scope="row">Available release</th><td id="ota-latest-date">-</td></tr>
							<tr><th scope="row">Status</th><td id="ota-status-text">Not checked yet</td></tr>
							<tr><th scope="row">Firmware URL</th><td id="ota-bin-url" class="config-row-meta">-</td></tr>
						</tbody>
					</table>
					<div class="maintenance-actions">
						<button type="button" id="ota-check-btn" class="btn btn-default"><span class="btn-icon" aria-hidden="true">&#8635;</span> Check for updates</button>
						<button type="button" id="ota-install-btn" class="btn btn-warning" disabled><span class="btn-icon" aria-hidden="true">&#11015;</span> Install update</button>
					</div>
				</div>
			</div>

			<div id="ota-version-picker" class="row panel panel-default sound-card">
				<div class="panel-heading">Install a specific release</div>
				<div class="panel-body">
					<p class="config-help">The list below is detected online from the repository folder <code>ota/releases/</code> and only contains firmware images that actually exist. Selecting an older version downgrades the device. Do not unplug power during the update.</p>
					<div class="form-group">
						<label for="ota-version-select">Available firmware</label>
						<select class="form-control" id="ota-version-select" disabled>
							<option value="">Loading available releases...</option>
						</select>
					</div>
					<div class="maintenance-actions">
						<button type="button" id="ota-version-install-btn" class="btn btn-warning" disabled><span class="btn-icon" aria-hidden="true">&#11015;</span> Install selected version</button>
					</div>
					<p id="ota-version-picker-status" class="config-row-meta">-</p>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Local firmware upload (.bin)</div>
				<div class="panel-body">
					<p class="config-help">Upload a local firmware <code>.bin</code> file directly from this device. The web radio restarts automatically after a successful upload.</p>
					<p class="ota-upload-warning"><strong>&#9888; Warning:</strong> Only upload a firmware image that was built for <strong>this device (ESP32 web radio)</strong>. Uploading a wrong version, a file for a different chip, or any unsuitable/corrupted <code>.bin</code> can make the device unbootable and may require a USB reflash to recover. The file is validated before flashing (ESP32 image signature, chip type and size), but these checks cannot guarantee full compatibility - proceed only if you are sure.</p>
					<div class="form-group">
						<label for="ota-local-bin">Firmware file</label>
						<input type="file" class="form-control" id="ota-local-bin" accept=".bin,application/octet-stream">
					</div>
					<div class="maintenance-actions">
						<button type="button" id="ota-upload-btn" class="btn btn-primary"><span class="btn-icon" aria-hidden="true">&#128228;</span> Upload and install</button>
					</div>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Release history</div>
				<div class="panel-body">
					<p class="config-help">Changelog from the remote version manifest. Refreshed when you open this page or run <strong>Check for updates</strong>.</p>
					<ul id="ota-version-history" class="ota-history-list">
						<li class="ota-history-empty">No release history loaded yet.</li>
					</ul>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading">Update source</div>
				<div class="panel-body">
					<p class="config-help">OTA metadata and firmware releases are hosted on GitHub:
						<a href="https://github.com/joruf/esp32-webradio" target="_blank" rel="noopener">github.com/joruf/esp32-webradio</a>
						(folder <code>ota/</code>).</p>
					<p class="config-help">JSON manifest URL below (<code>raw.githubusercontent.com</code>). Saved to NVS automatically when you leave the field. If GitHub is unreachable, the radio uses its built-in manifest (<code>/ota/version.json</code> on this device).</p>
					<div class="form-group">
						<label for="feat-ota-url">OTA version check URL</label>
						<input type="text" class="form-control" id="feat-ota-url" value="https://raw.githubusercontent.com/joruf/esp32-webradio/main/ota/version.json">
					</div>
				</div>
			</div>
			</div>
		</div>
		<div id="ota-progress-modal" class="ota-modal-backdrop" style="display:none;" aria-live="assertive">
			<div class="ota-modal-card">
				<div class="ota-modal-icon" id="ota-modal-icon" aria-hidden="true">&#128339;</div>
				<h3 id="ota-modal-title">Please wait</h3>
				<p id="ota-modal-message">Firmware update is being installed...</p>
				<div class="maintenance-actions ota-modal-actions">
					<button type="button" id="ota-modal-close" class="btn btn-primary">OK</button>
				</div>
			</div>
		</div>

		<script src="app.js"></script>
	</body>
</html>

)=====";
