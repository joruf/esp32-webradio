// index.html file in raw data format for PROGMEM
// "=====( CONTENT )====="; Equal chars required as terminating char
// BASE64 Glyphicons: https://andrew.hedges.name/experiments/glyphicons/
/*
<div class="panel panel-default">
	<div class="panel-heading">Webradio Control</div>
	<div class="panel-body">
	</div>
</div>
					<li><a href="mp3play.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABoAAAAaCAYAAACpSkzOAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOjQ5MUFGOTFBMDZBNzExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOjQ5MUFGOTE5MDZBNzExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PkFCGFQAAAEZSURBVHjaxFaLDYIwEKUuIBvYDWSEuoEjMAKjMAIjsIGOgBvUDXCCejVHUq9fIZyXvJCUu/fS1+tHGGOqkhBCSPhIMqyhXhcRWKEYkLi3hDY1Ao05MskVEaix2PwIW1MXCUE0gGmFyAJb21Be4a4RrIMVuQOOxOEnYES4cUWcyPgLoIB78tYI7aIzmQFdynus7TCXzqz2rAusyRyyICHWBMT6LyHsrtUiGTHpCtHZdAGSsUQcbfRmtfx094mOEGRb2Mn1+EK29Rmhxdo2IUQd+pwoigyqAiG3s1Qg1+M8VNviDLjB/hsyeXKrUGnorUIPwAXsakuSeZqBrb25NyzPEcR2qHJcE/wX395X+f8eJ3s9twTXA/ItwACdK7DO9WgFFwAAAABJRU5ErkJggg==" /> MP3 Player</span></a></li>
 */

const char config_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html prefix="og: http://purl.org/dc/elements/1.1/" lang="en-EN">
	<head>
		<meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
		<!--
		<link rel="icon" href="favicon.ico" type="image/x-icon">
		<link rel="icon" href="favicon.ico" type="image/png">
		-->

		<link rel="stylesheet" type="text/css" href="index.css" />

		<title>Webradio - Configuration</title>
	</head>
	
	<body itemscope itemtype="http://schema.org/WebPage">	


		<nav class="navbar navbar-default navbar-static-top">
			<div id="body" class="container">
				<div class="navbarDistance">
						<a href="index.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABUAAAAZCAYAAADe1WXtAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOjBFMkE3QzdFMDY3NTExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOjBFMkE3QzdEMDY3NTExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PhttG6QAAADoSURBVHja7JTdEYMgEIS9jAWkg5QQO4glWIKl2Eo6sATTASWQDkwFZJkBhzDeIYS85Wa+8YFjZflZMsY03xQRnfEZQO+4tIUifcA17mkPCoUit+QEa38P1Ahm25ILcXuK1RVv9qmpU09w3xZUuNIXWBwzNHQ4J+f0H4GIKj0ov/ED18PNSdrHOGXcYVPzoH5y+n/RCqI2rcAEFrB+PJbUnWPGbPwpNlQKReX0Cv48uWe4hg2J18YhW2FElSA4Jq0woh3QUa/2OUF+uULgkHADOudU+/jzJVo5mlCRE9lKCVv0SVZy6y3AAEtqmkTk57dyAAAAAElFTkSuQmCC" /> WEBRADIO</span></a>
						<a href="config.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOjZDOUYzNEJDMDZCNjExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOjZDOUYzNEJCMDZCNjExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PpudlXcAAAEkSURBVHjarFaLDYQgDJWbgBEYwREcwREYxRFuBEZwBEdgBEfwJuDA9AypLbZ6JI0JPN7rVzUppe7uMsYM+eGzOdiK2d6Zcz1ARUBredlsc7lO2IKwOzjC4SQUiARx8XogsCdwAfYNck/gfQO/A7A3s8L76SLabiAEtpI65gKFdawAXArExUiJCAvrgHOpuyIQ4Z9EwGNKZKyIf3tBUsRDhDjDqcJ7XtIpESJskVMWjxo0RILA89Sqn2SoNGlZcds+Je8vHXxCDl1j1QIK8o2bF1ZASG7RzLAi/yBvviTVBWVeK2zh1d1S5V6K38NWtSLc2S6GbSzYF+SUWh/4QkV8AHuD9NtNFU02RHxqPa5BLSIib4h4rosshO1u/GVMFHmxrwADAD5Dl19EesQ5AAAAAElFTkSuQmCC" /> CONFIGURATION</span></a>
						<a href="sound.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOkI0N0JDRjhEMDY5MTExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOkI0N0JDRjhDMDY5MTExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PgFdWUIAAAExSURBVHjaxFUBEcIwDFxRMAmVMAnBQSVMAhImAQlIQAJzUBzgAByU9C7jspCsZdc7cvfHyNbPLcn/upRSVwOMiEiEW+05R4c3wznX48+T5/Cc6yrioJBNCBBpUJ4D+R8xflUQbbiwNuRrjzghHiy/IOcy4YC4svy44mTkk0KyF2Hh5S26de0i1rRoLya1RVTANyjQc065RcF45TvimFeT1vNIOS3C1xblqnRD25ZoCK8X4vs8T1z9orFYeGXYUHconI2OLswoKRbFlX5S8i9BFlK0irlAAhu3Q4F/5v0Ea8hy9diQrefB0sFoDWuRPxGPBvnKJrQCQ2uhyQLXBgXOlptCQzcdNKvwDd3UW27KhzyxgW5aQm5L8YMj5O8rLAGUBQn//+gbfvQS9jzXDuMtwAATXCNvATubRQAAAABJRU5ErkJggg==" /> SOUND SETTINGS</span></a>
						<a href="firmware.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABsAAAAWCAYAAAAxSueLAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOjg5RUQ2MzQwMDZDQjExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOjg5RUQ2MzNGMDZDQjExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PnMZ5UQAAAEpSURBVHjavFbbEYIwEAyM/6YD7UBKoAPTgZRgCZRgCZRgCdiBJWAHdBAX5zLimdfhY2cWZyB3S47LnspaqyQEGrAH7YxXsAV1NFYgosEzE7Ee0eobYn1CyHEM7VBSOitgJxIDtmANGiqPFVJniQGnBck5Dc+7UgxFUXT4OSg/buDA7k0V2HjW1tRQT7AdGemHpy4dIjEnF8cDY63d05qX+5mdep3WlWzre/Ub7PB52lL9D00JRQ2eQftjsU1Jnrb/x9ZW5A4pXOhgh+CeVeA6lmhMtPuWdexbNzLXCR2D6fw+LsG2pSRDjgH7Xmb+0u5QxryvofKkKlAFDHtwY0cyYozEBz+aZ5FRc8yOX/C3oEvNrRALSiACDKAnE69FcQvFNImNkri7AAMAebfFpPog2YAAAAAASUVORK5CYII=" /> FIRMWARE UPGRADE</span></a>
						<a href="about.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABoAAAAaCAYAAACpSkzOAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOkEzRUY0MTQ5MDZBMDExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOkEzRUY0MTQ4MDZBMDExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PlHOIcUAAAD8SURBVHjavFaBDYQgDAQmcISOgBs4kqP8KG7yjsAIbsC3pn548kiLSpML0dAe0CvFxhiNxKy1gANkvwP6B1EAIiqBA78oIE0tIPAcOI1VIBjYOSpBPoOICM0j1gaSA+TrT4mYZLtAcmDLyfLjWm8gSXc2/COS5sQnu6/m7IeI1SVaaXbUEp9djY5VPhuhYT35dBTY/K2jSp1cRTguBdDkJlOolGy/USZtbpQ5IkxOm5tWc4q5bySLjTzgTB8LjhX3OJFa3g1iCGnBLoJVjehgs15G32PFb0kLFlrkLdwZaC9VXyDx4ku1a5vo1vi6tvKuj5Onnlu21wPyI8AANwDIP2ZbcJQAAAAASUVORK5CYII=" /> ABOUT</span></a>
				</div>
				
				<input id="command_log" type="text" style="width:100%" placeholder="Waiting for a command...." disabled><br>
			</div>
		</nav>
		

		<div id="body" class="container">
			<h1>Configuration</h1>


			<div class="row panel panel-default">
				<div class="panel-heading">Sleep-Timer</div>
				<div class="panel-body">
					<div class="row">
						<div class="form-group col-xs-12 col-sm-6 col-md-3 col-lg-3">
							<select class="form-control" id="sleeptime">
								<option value="0">Aus</option>
								<option value="1">1 Minute</option>
								<option value="5">5 Minuten</option>
								<option value="10">10 Minuten</option>
								<option value="15">15 Minuten</option>
								<option value="20">20 Minuten</option>
								<option value="25">25 Minuten</option>
								<option value="30">30 Minuten</option>
								<option value="45">45 Minuten</option>
								<option value="60">60 Minuten</option>
							</select>
						</div>
					</div>
				</div>
			</div>


			<div class="row panel panel-default">
				<div class="panel-heading">Alarm-Clock (with shut-off)</div>
				<div class="panel-body">
					<div class="row">
						<div class="form-group col-xs-12 col-sm-6 col-md-3 col-lg-3">
							<label>Start (hour)</label>
							<select class="form-control alarm-field" id="from_hour">
								<option name="0">0</option>
								<option name="1">1</option>
								<option name="2">2</option>
								<option name="3">3</option>
								<option name="4">4</option>
								<option name="5">5</option>
								<option name="6">6</option>
								<option name="7">7</option>
								<option name="8">8</option>
								<option name="9">9</option>
								<option name="10">10</option>
								<option name="11">11</option>
								<option name="12">12</option>
								<option name="13">13</option>
								<option name="14">14</option>
								<option name="15">15</option>
								<option name="16">16</option>
								<option name="17">17</option>
								<option name="18">18</option>
								<option name="19">19</option>
								<option name="20">20</option>
								<option name="21">21</option>
								<option name="22">22</option>
								<option name="23">23</option>
							</select>
						</div>
						<div class="form-group col-xs-12 col-sm-6 col-md-3 col-lg-3">
							<label>Start (minute)</label>
							<select class="form-control alarm-field" id="from_minute">
								<option name="00">00</option>
								<option name="01">01</option>
								<option name="02">02</option>
								<option name="03">03</option>
								<option name="04">04</option>
								<option name="05">05</option>
								<option name="06">06</option>
								<option name="07">07</option>
								<option name="08">08</option>
								<option name="09">09</option>
								<option name="10">10</option>
								<option name="11">11</option>
								<option name="12">12</option>
								<option name="13">13</option>
								<option name="14">14</option>
								<option name="15">15</option>
								<option name="16">16</option>
								<option name="17">17</option>
								<option name="18">18</option>
								<option name="19">19</option>
								<option name="20">20</option>
								<option name="21">21</option>
								<option name="22">22</option>
								<option name="23">23</option>
								<option name="24">24</option>
								<option name="25">25</option>
								<option name="26">26</option>
								<option name="27">27</option>
								<option name="28">28</option>
								<option name="29">29</option>
								<option name="30">30</option>
								<option name="31">31</option>
								<option name="32">32</option>
								<option name="33">33</option>
								<option name="34">34</option>
								<option name="35">35</option>
								<option name="36">36</option>
								<option name="37">37</option>
								<option name="38">38</option>
								<option name="39">39</option>
								<option name="40">40</option>
								<option name="41">41</option>
								<option name="42">42</option>
								<option name="43">43</option>
								<option name="44">44</option>
								<option name="45">45</option>
								<option name="46">46</option>
								<option name="47">47</option>
								<option name="48">48</option>
								<option name="49">49</option>
								<option name="50">50</option>
								<option name="51">51</option>
								<option name="52">52</option>
								<option name="53">53</option>
								<option name="54">54</option>
								<option name="55">55</option>
								<option name="56">56</option>
								<option name="57">57</option>
								<option name="58">58</option>
								<option name="59">59</option>
							</select>
						</div>
						
						<div class="form-group col-xs-12 col-sm-6 col-md-3 col-lg-3">
							<label>Shut-off (hour)</label>
							<select class="form-control alarm-field" id="to_hour">
								<option name="0">0</option>
								<option name="1">1</option>
								<option name="2">2</option>
								<option name="3">3</option>
								<option name="4">4</option>
								<option name="5">5</option>
								<option name="6">6</option>
								<option name="7">7</option>
								<option name="8">8</option>
								<option name="9">9</option>
								<option name="10">10</option>
								<option name="11">11</option>
								<option name="12">12</option>
								<option name="13">13</option>
								<option name="14">14</option>
								<option name="15">15</option>
								<option name="16">16</option>
								<option name="17">17</option>
								<option name="18">18</option>
								<option name="19">19</option>
								<option name="20">20</option>
								<option name="21">21</option>
								<option name="22">22</option>
								<option name="23">23</option>
							</select>
						</div>
						<div class="form-group col-xs-12 col-sm-6 col-md-3 col-lg-3">
							<label>Shut-off (minute)</label>
							<select class="form-control alarm-field" id="to_minute">
								<option name="00">00</option>
								<option name="01">01</option>
								<option name="02">02</option>
								<option name="03">03</option>
								<option name="04">04</option>
								<option name="05">05</option>
								<option name="06">06</option>
								<option name="07">07</option>
								<option name="08">08</option>
								<option name="09">09</option>
								<option name="10">10</option>
								<option name="11">11</option>
								<option name="12">12</option>
								<option name="13">13</option>
								<option name="14">14</option>
								<option name="15">15</option>
								<option name="16">16</option>
								<option name="17">17</option>
								<option name="18">18</option>
								<option name="19">19</option>
								<option name="20">20</option>
								<option name="21">21</option>
								<option name="22">22</option>
								<option name="23">23</option>
								<option name="24">24</option>
								<option name="25">25</option>
								<option name="26">26</option>
								<option name="27">27</option>
								<option name="28">28</option>
								<option name="29">29</option>
								<option name="30">30</option>
								<option name="31">31</option>
								<option name="32">32</option>
								<option name="33">33</option>
								<option name="34">34</option>
								<option name="35">35</option>
								<option name="36">36</option>
								<option name="37">37</option>
								<option name="38">38</option>
								<option name="39">39</option>
								<option name="40">40</option>
								<option name="41">41</option>
								<option name="42">42</option>
								<option name="43">43</option>
								<option name="44">44</option>
								<option name="45">45</option>
								<option name="46">46</option>
								<option name="47">47</option>
								<option name="48">48</option>
								<option name="49">49</option>
								<option name="50">50</option>
								<option name="51">51</option>
								<option name="52">52</option>
								<option name="53">53</option>
								<option name="54">54</option>
								<option name="55">55</option>
								<option name="56">56</option>
								<option name="57">57</option>
								<option name="58">58</option>
								<option name="59">59</option>
							</select>
						</div>
					</div>
				</div>
			</div>


			<div class="row panel panel-default">
				<div class="panel-heading">Configuration File</div>
				<div class="panel-body">
					<div class="row panel-body">
						<textarea id="prefs" rows="20" style="width: 100%;"></textarea> 

						<div class="btn_border col-xs-12 col-sm-6 col-md-3 col-lg-3">
							<button id="savePref" class="btn btn-lg btn-success">
								<img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAaCAYAAACtv5zzAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOkI0N0JDRjg1MDY5MTExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOkI0N0JDRjg0MDY5MTExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PlXYqDAAAAEjSURBVHja7FXREYIwDKUuYEdgBEZwBEboCI7gCIzACIzACIzACO0E2HqphhDaIPbP3r1Tm+S9a/Js1bIslWQppaz/uMJP5+u0qO6AwCrR1ylJ3aUqvcIJIvxqwxYFxLJ7ALPiJALDDwQmVsAvvVNwVCCgjrx4Bi36HlyiIhLtxTkOhW7ckLHAEK0Z3EMdFF0FsLhmw7XTnhb2Gw+baEVAs2MQ/Z5BmDwKWDJ4kyA3JNfSGOeeHhdB/MGQd0xej+JD/BOz7ZEUMzmbNtEWzFwhIpgAOpEz4zbR9nQZAZ0ih5wOn7TiHHEG4DzMmbSgERCaDEcyOAoExjMCq3uFIa9z9ZL34P5l7HNbZGATJ8hdI69bcCn5oBV/Mv8CIgFXkN89BRgAy9xaTz00S4MAAAAASUVORK5CYII=" />
								Save Configuration
							</button>
						</div>

						<div class="btn_border col-xs-12 col-sm-6 col-md-3 col-lg-3">
							<button id="getdefs" class="btn btn-lg btn-primary">
								<img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABkAAAAZCAYAAADE6YVjAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOjRBRTJCQzE1MDZDQzExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOjRBRTJCQzE0MDZDQzExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/Pv82KJ8AAAFhSURBVHjavJWBlYMgDIa1E7BB2aBucI7gCDcCIziCI3QERmAER6AbcBPkwr3fexGR2h4e7/2t5mE+SCBpiKh5Jh4jy7FI2OLPzDLPvr80O6Nt2451x6tmfWSm3Vge80dWn3VWWDlBA6tb3pOdeDwrVoBts7M9yCwgDjYTgWJOXHWP508RPlWE8JgwUeM/HIk5IG4BYGdmA8HKSKwogvSRg5EAlYhEn0K8gIR3APDj4CMemu4XguSSkHkHICKiNzkBleSJqSEAx1yopgrOOxG2IM88yWTV1IVvqW5OHrmyEqpDeDs+samzdvIlbMNZEPcfECtsVz4MpiqlUFa6g3fCJleANhU8qaT0KggXL+xAplyptxmQ+cNufhbZYtLSchUOwS2J6gOObGIfoGsmEw/2rbPtF/ekFOejmortV+TIv+DUoUWvQlWEJDC7k9wZbaJPWviqXaxyUmvEoivL1bcAAwA9bUObR3jgfgAAAABJRU5ErkJggg==" />
								Load Default Values
							</button>
						</div>
						
					</div>
				</div>
			</div>


		</div>


	
		<script src="interface.js"></script>
	</body>

</html>

)=====";
