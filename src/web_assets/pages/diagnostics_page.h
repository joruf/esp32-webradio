// diagnostics.html file in raw data format for PROGMEM
const char diagnostics_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en-EN">
	<head>
		<meta http-equiv="content-type" content="text/html; charset=UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<link rel="stylesheet" type="text/css" href="app.css" />
		<title>Webradio - Diagnostics</title>
	</head>
	<body>
@include_nav@
<div id="body" class="container">
			<h1>Diagnostics</h1>
			<div class="row panel panel-default">
				<div class="panel-heading">System status</div>
				<div class="panel-body">
					<pre id="diag-output" class="diag-output">Loading...</pre>
					<button type="button" id="diag-refresh" class="btn btn-primary">Refresh</button>
				</div>
			</div>
		</div>
		<script src="app.js"></script>
	</body>
</html>
)=====";
