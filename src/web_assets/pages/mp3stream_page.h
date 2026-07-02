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
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
		<!--
		<link rel="icon" href="favicon.ico" type="image/x-icon">
		<link rel="icon" href="favicon.ico" type="image/png">
		-->

		<link rel="stylesheet" type="text/css" href="app.css" />

		<title>Webradio - MP3 Stream</title>
	</head>

	<body itemscope itemtype="http://schema.org/WebPage">
@include_nav@

<div id="body" class="container sound-page mp3-page">
			<header class="sound-page-header">
				<h1>MP3 Stream</h1>
				<p class="sound-page-lead">Stream MP3 files from your smartphone or PC to the webradio over your local network.</p>
			</header>
			<div class="sound-studio">

			<div class="row panel panel-default sound-card">
				<div class="panel-heading sound-card-heading">
					<span class="sound-card-icon" aria-hidden="true">&#127760;</span>
					<div class="sound-card-heading-text">
						<strong><span class="mp3-step-badge">1</span> Media server</strong>
						<span class="sound-card-sub">Run a small HTTP server on your smartphone/PC and scan the folder for MP3 files.</span>
					</div>
				</div>
				<div class="panel-body">
					<label class="mp3-field-label" for="mp3-base-url">Base URL of your media server</label>
					<input id="mp3-base-url" class="form-control mp3-input" placeholder="http://192.168.178.35:8000/">
					<div class="mp3-action-row">
						<button id="mp3-server-test-btn" class="btn btn-default mp3-server-test-btn" type="button">Check Server</button>
						<button id="mp3-scan-btn" class="btn btn-primary mp3-scan-btn" type="button">Scan folder</button>
						<button id="mp3-play-all-btn" class="btn mp3-play-btn mp3-play-all-btn" type="button" disabled><span class="btn-icon" aria-hidden="true">&#9654;</span> Play all</button>
					</div>
					<p class="mp3-help">Example: open a terminal in your music folder, then run <button type="button" class="mp3-copy-cmd" data-copy-cmd="python3 -m http.server 8000" title="Click to copy command">python3 -m http.server 8000</button> and paste the Base URL above. It is filled in automatically.</p>
				</div>
			</div>

			<div class="row panel panel-default sound-card">
				<div class="panel-heading sound-card-heading">
					<span class="sound-card-icon" aria-hidden="true">&#127925;</span>
					<div class="sound-card-heading-text">
						<strong><span class="mp3-step-badge">2</span> Available tracks</strong>
						<span class="sound-card-sub">Click a track to start streaming. The currently playing file is highlighted.</span>
					</div>
				</div>
				<div class="panel-body">
					<div class="webradio-control-toolbar mp3-volume-toolbar">
						<label for="mp3-volume" class="webradio-control-label">Volume</label>
						<span class="webradio-volume-value"><span id="mp3-volume-value">0</span>%</span>
						<input type="range" class="form-control webradio-control-slider" id="mp3-volume" min="0" max="100" step="5">
					</div>
					<p id="mp3-picker-status" class="mp3-now-status">Enter a Base URL and scan the folder to list MP3 files.</p>
					<div id="mp3-track-list" class="mp3-track-list" role="list">
						<div class="mp3-track-empty">No tracks loaded yet.</div>
					</div>
				</div>
			</div>

			</div>
		</div>

<input id="station" type="hidden" value="">

		<script src="app.js"></script>
	<script>
	(function () {
		var baseEl = document.getElementById("mp3-base-url");
		var serverTestBtn = document.getElementById("mp3-server-test-btn");
		var serverLinkEl = document.getElementById("mp3-server-link");
		var scanBtn = document.getElementById("mp3-scan-btn");
		var playAllBtn = document.getElementById("mp3-play-all-btn");
		var trackListEl = document.getElementById("mp3-track-list");
		var MEDIA_SERVER_PORT = 8000;
		var scanInFlight = false;

		function setStatus(message, isError) {
			if (typeof setMp3PickerStatus === "function") {
				setMp3PickerStatus(message);
				return;
			}
			var statusEl = document.getElementById("mp3-picker-status");
			if (statusEl) {
				statusEl.textContent = String(message || "");
				statusEl.classList.toggle("is-error", !!isError);
			}
		}

		function normalizeBase(baseUrl) {
			if (typeof normalizeBaseUrl === "function") {
				return normalizeBaseUrl(baseUrl);
			}
			var normalized = String(baseUrl || "").trim();
			if (!normalized) {
				return "";
			}
			if (!/^https?:\/\//i.test(normalized)) {
				normalized = "http://" + normalized;
			}
			if (normalized.charAt(normalized.length - 1) !== "/") {
				normalized += "/";
			}
			return normalized;
		}

		function updateServerTestLink() {
			var baseUrl = normalizeBase(baseEl ? baseEl.value : "");
			if (!serverLinkEl) {
				return baseUrl;
			}
			if (!baseUrl) {
				serverLinkEl.href = "#";
				serverLinkEl.textContent = "enter Base URL above";
				serverLinkEl.classList.add("is-disabled");
				serverLinkEl.setAttribute("aria-disabled", "true");
				serverLinkEl.setAttribute("tabindex", "-1");
				return "";
			}
			serverLinkEl.href = baseUrl;
			serverLinkEl.textContent = baseUrl;
			serverLinkEl.classList.remove("is-disabled");
			serverLinkEl.setAttribute("aria-disabled", "false");
			serverLinkEl.removeAttribute("tabindex");
			return baseUrl;
		}

		function openServerInNewTab() {
			var baseUrl = updateServerTestLink();
			if (!baseUrl) {
				setStatus("Enter the Base URL of your media server first.", true);
				return;
			}
			window.open(baseUrl, "_blank", "noopener,noreferrer");
		}

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

		if (baseEl && !String(baseEl.value || "").trim()) {
			var session = typeof getMp3StreamSession === "function" ? getMp3StreamSession() : null;
			if (session && session.baseUrl) {
				baseEl.value = session.baseUrl;
			} else {
				fetch("/?clientip&_t=" + Date.now())
					.then(function (response) { return response.json(); })
					.then(function (data) {
						if (data && data.ip && !String(baseEl.value || "").trim()) {
							baseEl.value = "http://" + data.ip + ":" + MEDIA_SERVER_PORT + "/";
							updateServerTestLink();
						}
					})
					.catch(function () {});
			}
		}
		updateServerTestLink();

		function updateTrackHighlight(session, isPlaying) {
			if (!trackListEl || !session) {
				return;
			}
			var items = trackListEl.querySelectorAll(".mp3-track-item");
			var i;
			for (i = 0; i < items.length; i++) {
				var item = items[i];
				var index = parseInt(item.getAttribute("data-index"), 10);
				var isActive = index === session.currentIndex;
				item.classList.toggle("is-active", isActive);
				item.classList.toggle("is-playing", isActive && !!isPlaying);
			}
		}

		function renderTrackList(session) {
			if (!trackListEl) {
				return;
			}
			session = session || (typeof getMp3StreamSession === "function" ? getMp3StreamSession() : { tracks: [], currentIndex: -1 });
			trackListEl.innerHTML = "";
			if (!session.tracks.length) {
				trackListEl.innerHTML = '<div class="mp3-track-empty">No tracks loaded yet.</div>';
				if (playAllBtn) {
					playAllBtn.disabled = true;
				}
				updateTrackHighlight(session, false);
				return;
			}

			session.tracks.forEach(function (track, index) {
				var button = document.createElement("button");
				button.type = "button";
				button.className = "mp3-track-item";
				button.setAttribute("role", "listitem");
				button.setAttribute("data-index", String(index));
				button.setAttribute("data-url", track.url);
				button.innerHTML = '<span class="mp3-track-play btn-icon" aria-hidden="true">&#9654;</span><span class="mp3-track-name"></span>';
				button.querySelector(".mp3-track-name").textContent = track.name;
				button.addEventListener("click", function () {
					if (typeof mp3StreamPlayTrackAtIndex === "function") {
						mp3StreamPlayTrackAtIndex(index, false);
					}
				});
				trackListEl.appendChild(button);
			});

			if (playAllBtn) {
				playAllBtn.disabled = false;
			}
			updateTrackHighlight(session, session.lastPlaying);
		}

		window.onMp3StreamPagePlaybackUpdate = function (session, playing) {
			updateTrackHighlight(session, playing);
		};

		function restorePageFromSession() {
			var session = typeof getMp3StreamSession === "function" ? getMp3StreamSession() : null;
			if (!session) {
				renderTrackList({ tracks: [], currentIndex: -1 });
				return;
			}
			if (baseEl && session.baseUrl && !String(baseEl.value || "").trim()) {
				baseEl.value = session.baseUrl;
			}
			updateServerTestLink();
			renderTrackList(session);
			if (session.tracks.length) {
				setStatus(session.tracks.length + " MP3 file(s) loaded. Playback continues in the background.");
			}
			if (typeof ensureMp3StreamLiveUpdates === "function") {
				ensureMp3StreamLiveUpdates();
			}
		}

		function scanMediaServer() {
			var baseUrl;
			if (scanInFlight) {
				return;
			}
			baseUrl = normalizeBase(baseEl ? baseEl.value : "");
			if (!baseUrl) {
				setStatus("Enter the Base URL of your media server first.", true);
				return;
			}

			scanInFlight = true;
			if (scanBtn) {
				scanBtn.disabled = true;
				scanBtn.textContent = "Scanning...";
			}
			setStatus("Checking connection and scanning folder...");

			fetch("/?mediadir=" + encodeURIComponent(baseUrl) + "&_t=" + Date.now())
				.then(function (response) { return response.json(); })
				.then(function (data) {
					if (!data || data.ok !== true) {
						setStatus((data && data.error) || "Could not scan folder.", true);
						return;
					}
					if (!data.files || !data.files.length) {
						setStatus((data && data.message) || "No MP3 files found in this folder.", true);
						return;
					}
					if (typeof mp3StreamSetTracksFromScan === "function") {
						mp3StreamSetTracksFromScan(baseUrl, data.files);
					}
					renderTrackList(typeof getMp3StreamSession === "function" ? getMp3StreamSession() : null);
					setStatus(data.files.length + " MP3 file(s) found. Click a track to play.");
					if (typeof showNotification === "function") {
						showNotification("Found " + data.files.length + " MP3 file(s).", { type: "success" });
					}
				})
				.catch(function () {
					setStatus("Scan failed. Check the Base URL and make sure the server is running.", true);
				})
				.finally(function () {
					scanInFlight = false;
					if (scanBtn) {
						scanBtn.disabled = false;
						scanBtn.textContent = "Scan folder";
					}
				});
		}

		if (scanBtn) {
			scanBtn.addEventListener("click", scanMediaServer);
		}
		if (serverTestBtn) {
			serverTestBtn.addEventListener("click", openServerInNewTab);
		}
		if (playAllBtn) {
			playAllBtn.addEventListener("click", function () {
				var session = typeof getMp3StreamSession === "function" ? getMp3StreamSession() : null;
				if (!session || !session.tracks.length) {
					setStatus("No tracks available. Scan the folder first.", true);
					return;
				}
				if (typeof mp3StreamStartPlayAll === "function") {
					mp3StreamStartPlayAll();
				}
			});
		}
		if (baseEl) {
			baseEl.addEventListener("input", updateServerTestLink);
			baseEl.addEventListener("change", updateServerTestLink);
			baseEl.addEventListener("keydown", function (event) {
				if (event.key === "Enter") {
					event.preventDefault();
					scanMediaServer();
				}
			});
		}

		restorePageFromSession();
	})();
	</script>
	</body>
</html>
)=====";
