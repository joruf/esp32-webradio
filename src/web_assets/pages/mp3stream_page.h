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

const char mp3stream_html[] PROGMEM = R"=====(
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

<div id="body" class="container sound-page mp3-page">
	<header class="sound-page-header">
		<h1>MP3 Player</h1>
		<p class="sound-page-lead">Stream MP3 files from your smartphone or PC to the webradio over your local network.</p>
	</header>

	<div class="sound-studio">

		<section class="panel panel-default sound-card mp3-now-card">
			<div class="panel-heading sound-card-heading">
				<span class="sound-card-icon" aria-hidden="true">&#9835;</span>
				<div class="sound-card-heading-text">
					<strong>Current stream</strong>
					<span class="sound-card-sub">The URL currently sent to the radio</span>
				</div>
			</div>
			<div class="panel-body">
				<div id="mp3-now-url" class="mp3-now-url">No stream selected yet</div>
				<p id="mp3-picker-status" class="mp3-now-status">Choose a file and set a Base URL to begin.</p>
			</div>
		</section>

		<section class="panel panel-default sound-card">
			<div class="panel-heading sound-card-heading">
				<span class="sound-card-icon" aria-hidden="true">&#127760;</span>
				<div class="sound-card-heading-text">
					<strong><span class="mp3-step-badge">1</span> Media server Base URL</strong>
					<span class="sound-card-sub">Run a small HTTP server on your smartphone/PC and enter its folder root once.</span>
				</div>
			</div>
			<div class="panel-body">
				<label class="mp3-field-label" for="mp3-base-url">Base URL of your media server</label>
				<input id="mp3-base-url" class="form-control mp3-input" placeholder="http://192.168.1.20:8000/music/">
				<p class="mp3-help">Example: open a terminal in your music folder, then run <button type="button" class="mp3-copy-cmd" data-copy-cmd="python3 -m http.server 8000" title="Click to copy command">python3 -m http.server 8000</button> and paste it there. The Base URL above is filled in automatically.</p>
			</div>
		</section>

		<section class="panel panel-default sound-card">
			<div class="panel-heading sound-card-heading">
				<span class="sound-card-icon" aria-hidden="true">&#127925;</span>
				<div class="sound-card-heading-text">
					<strong><span class="mp3-step-badge">2</span> Choose your music</strong>
					<span class="sound-card-sub">Pick a single MP3 or a whole folder, then play any track.</span>
				</div>
			</div>
			<div class="panel-body">
				<div class="mp3-mode">
					<input type="radio" name="mp3-mode" id="mp3-mode-file" class="mp3-mode-radio" checked>
					<input type="radio" name="mp3-mode" id="mp3-mode-folder" class="mp3-mode-radio">

					<div class="mp3-mode-tabs" role="tablist">
						<label class="mp3-mode-tab" for="mp3-mode-file"><span aria-hidden="true">&#127925;</span> Single file</label>
						<label class="mp3-mode-tab" for="mp3-mode-folder"><span aria-hidden="true">&#128193;</span> Folder</label>
					</div>

					<div class="mp3-mode-panels">
						<div class="mp3-mode-panel mp3-mode-panel-file">
							<div class="mp3-picker">
								<span class="mp3-picker-icon" aria-hidden="true">&#127925;</span>
								<div class="mp3-picker-title">Choose one MP3 or M3U file</div>
								<div class="mp3-picker-sub">The file name appears on the radio display and playback starts immediately.</div>
								<input id="mp3-file-picker" class="mp3-file-input" type="file" accept=".mp3,.m3u,audio/mpeg,audio/mp3,audio/x-mpegurl,application/vnd.apple.mpegurl">
							</div>
						</div>

						<div class="mp3-mode-panel mp3-mode-panel-folder">
							<div class="mp3-picker">
								<span class="mp3-picker-icon" aria-hidden="true">&#128193;</span>
								<div class="mp3-picker-title">Choose a complete folder</div>
								<div class="mp3-picker-sub">Every MP3/M3U file inside is listed below. Click one to start the stream.</div>
								<input id="mp3-folder-picker" class="mp3-file-input" type="file" webkitdirectory directory multiple accept=".mp3,.m3u,audio/mpeg,audio/mp3,audio/x-mpegurl,application/vnd.apple.mpegurl">
							</div>
							<label class="mp3-field-label" for="mp3-folder-files">Files in selected folder</label>
							<select id="mp3-folder-files" class="form-control mp3-filelist" size="8">
								<option value="">Select a file from folder...</option>
							</select>
						</div>
					</div>
				</div>
			</div>
		</section>

		<section class="panel panel-default sound-card">
			<div class="panel-heading sound-card-heading">
				<span class="sound-card-icon" aria-hidden="true">&#9654;</span>
				<div class="sound-card-heading-text">
					<strong><span class="mp3-step-badge">3</span> Stream URL &amp; playback</strong>
					<span class="sound-card-sub">Verify the generated URL, then start the stream on the radio.</span>
				</div>
			</div>
			<div class="panel-body">
				<label class="mp3-field-label" for="station">MP3 or M3U URL</label>
				<div class="mp3-stream-row">
					<input id="station" class="form-control mp3-input mp3-stream-input" placeholder="http://192.168.1.20:8000/music/song.mp3">
					<button id="setstat" class="btn mp3-play-btn" type="button"><span class="btn-icon" aria-hidden="true">&#9654;</span> START STREAM</button>
				</div>
				<p class="mp3-help">Local paths such as <code>C:\Music\song.mp3</code> cannot be played directly. The file or playlist must be reachable over HTTP in your local network.</p>
			</div>
		</section>

	</div>
</div>

	<script src="app.js"></script>
	<script>
	(function () {
		var stationEl = document.getElementById("station");
		var nowUrlEl = document.getElementById("mp3-now-url");
		var baseEl = document.getElementById("mp3-base-url");
		var pickers = [document.getElementById("mp3-file-picker"), document.getElementById("mp3-folder-picker")];
		var MEDIA_SERVER_PORT = 8000;

		function copyTextToClipboard(text) {
			if (navigator.clipboard && navigator.clipboard.writeText) {
				return navigator.clipboard.writeText(text);
			}
			return new Promise(function (resolve, reject) {
				var textarea = document.createElement("textarea");
				textarea.value = text;
				textarea.setAttribute("readonly", "");
				textarea.style.position = "fixed";
				textarea.style.left = "-9999px";
				document.body.appendChild(textarea);
				textarea.select();
				try {
					if (document.execCommand("copy")) {
						resolve();
					} else {
						reject(new Error("copy failed"));
					}
				} catch (err) {
					reject(err);
				} finally {
					document.body.removeChild(textarea);
				}
			});
		}

		document.querySelectorAll(".mp3-copy-cmd").forEach(function (button) {
			button.addEventListener("click", function () {
				var command = String(button.getAttribute("data-copy-cmd") || button.textContent || "").trim();
				if (!command) {
					return;
				}
				copyTextToClipboard(command).then(function () {
					button.classList.add("is-copied");
					if (typeof showNotification === "function") {
						showNotification("Command copied: " + command, { type: "success" });
					}
					window.setTimeout(function () {
						button.classList.remove("is-copied");
					}, 1500);
				}).catch(function () {
					if (typeof showNotification === "function") {
						showNotification("Could not copy command. Select and copy manually.", { type: "error" });
					}
				});
			});
		});

		// Prefill the base URL with the requesting device's own IP so the media server URL is ready to use.
		if (baseEl && !String(baseEl.value || "").trim()) {
			fetch("/?clientip&_t=" + Date.now())
				.then(function (response) { return response.json(); })
				.then(function (data) {
					if (data && data.ip && !String(baseEl.value || "").trim()) {
						baseEl.value = "http://" + data.ip + ":" + MEDIA_SERVER_PORT + "/";
					}
				})
				.catch(function () {});
		}

		function refreshNowUrl() {
			if (!nowUrlEl) {
				return;
			}
			var value = stationEl ? String(stationEl.value || "").trim() : "";
			if (value) {
				nowUrlEl.textContent = value;
				nowUrlEl.classList.add("is-set");
			} else {
				nowUrlEl.textContent = "No stream selected yet";
				nowUrlEl.classList.remove("is-set");
			}
		}

		if (stationEl) {
			stationEl.addEventListener("input", refreshNowUrl);
			stationEl.addEventListener("change", refreshNowUrl);
		}
		// app.js writes the station value on its own change handlers; read it afterwards.
		pickers.forEach(function (picker) {
			if (picker) {
				picker.addEventListener("change", function () { window.setTimeout(refreshNowUrl, 0); });
			}
		});
		document.addEventListener("change", function (e) {
			if (e.target && e.target.id === "mp3-folder-files") {
				window.setTimeout(refreshNowUrl, 0);
			}
		});
		window.setInterval(refreshNowUrl, 1000);
		refreshNowUrl();
	})();
	</script>
	</body>
</html>
)=====";
