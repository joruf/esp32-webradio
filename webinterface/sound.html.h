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

const char sound_html[] PROGMEM = R"=====(
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

		<title>Webradio - Sound Settings</title>
	</head>
	
	<body itemscope itemtype="http://schema.org/WebPage">	


		<nav class="navbar navbar-default navbar-static-top">
			<div>
				<span class="navbar-brand hidden-xs hidden-sm"><b><a href="https://www.loresoft.de/" title="Loresoft" lang="de-DE"><img src="data:image/png;base64,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" alt="Loresoft" /></a></b></span>
			</div>
			<div class="navbarDistance">
					<a href="index.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABUAAAAZCAYAAADe1WXtAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOjBFMkE3QzdFMDY3NTExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOjBFMkE3QzdEMDY3NTExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PhttG6QAAADoSURBVHja7JTdEYMgEIS9jAWkg5QQO4glWIKl2Eo6sATTASWQDkwFZJkBhzDeIYS85Wa+8YFjZflZMsY03xQRnfEZQO+4tIUifcA17mkPCoUit+QEa38P1Ahm25ILcXuK1RVv9qmpU09w3xZUuNIXWBwzNHQ4J+f0H4GIKj0ov/ED18PNSdrHOGXcYVPzoH5y+n/RCqI2rcAEFrB+PJbUnWPGbPwpNlQKReX0Cv48uWe4hg2J18YhW2FElSA4Jq0woh3QUa/2OUF+uULgkHADOudU+/jzJVo5mlCRE9lKCVv0SVZy6y3AAEtqmkTk57dyAAAAAElFTkSuQmCC" /> WEBRADIO</span></a>
					<a href="config.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOjZDOUYzNEJDMDZCNjExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOjZDOUYzNEJCMDZCNjExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PpudlXcAAAEkSURBVHjarFaLDYQgDJWbgBEYwREcwREYxRFuBEZwBEdgBEfwJuDA9AypLbZ6JI0JPN7rVzUppe7uMsYM+eGzOdiK2d6Zcz1ARUBredlsc7lO2IKwOzjC4SQUiARx8XogsCdwAfYNck/gfQO/A7A3s8L76SLabiAEtpI65gKFdawAXArExUiJCAvrgHOpuyIQ4Z9EwGNKZKyIf3tBUsRDhDjDqcJ7XtIpESJskVMWjxo0RILA89Sqn2SoNGlZcds+Je8vHXxCDl1j1QIK8o2bF1ZASG7RzLAi/yBvviTVBWVeK2zh1d1S5V6K38NWtSLc2S6GbSzYF+SUWh/4QkV8AHuD9NtNFU02RHxqPa5BLSIib4h4rosshO1u/GVMFHmxrwADAD5Dl19EesQ5AAAAAElFTkSuQmCC" /> CONFIGURATION</span></a>
					<a href="sound.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOkI0N0JDRjhEMDY5MTExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOkI0N0JDRjhDMDY5MTExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PgFdWUIAAAExSURBVHjaxFUBEcIwDFxRMAmVMAnBQSVMAhImAQlIQAJzUBzgAByU9C7jspCsZdc7cvfHyNbPLcn/upRSVwOMiEiEW+05R4c3wznX48+T5/Cc6yrioJBNCBBpUJ4D+R8xflUQbbiwNuRrjzghHiy/IOcy4YC4svy44mTkk0KyF2Hh5S26de0i1rRoLya1RVTANyjQc065RcF45TvimFeT1vNIOS3C1xblqnRD25ZoCK8X4vs8T1z9orFYeGXYUHconI2OLswoKRbFlX5S8i9BFlK0irlAAhu3Q4F/5v0Ea8hy9diQrefB0sFoDWuRPxGPBvnKJrQCQ2uhyQLXBgXOlptCQzcdNKvwDd3UW27KhzyxgW5aQm5L8YMj5O8rLAGUBQn//+gbfvQS9jzXDuMtwAATXCNvATubRQAAAABJRU5ErkJggg==" /> SOUND SETTINGS</span></a>
					<a href="firmware.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABsAAAAWCAYAAAAxSueLAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOjg5RUQ2MzQwMDZDQjExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOjg5RUQ2MzNGMDZDQjExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PnMZ5UQAAAEpSURBVHjavFbbEYIwEAyM/6YD7UBKoAPTgZRgCZRgCZRgCdiBJWAHdBAX5zLimdfhY2cWZyB3S47LnspaqyQEGrAH7YxXsAV1NFYgosEzE7Ee0eobYn1CyHEM7VBSOitgJxIDtmANGiqPFVJniQGnBck5Dc+7UgxFUXT4OSg/buDA7k0V2HjW1tRQT7AdGemHpy4dIjEnF8cDY63d05qX+5mdep3WlWzre/Ub7PB52lL9D00JRQ2eQftjsU1Jnrb/x9ZW5A4pXOhgh+CeVeA6lmhMtPuWdexbNzLXCR2D6fw+LsG2pSRDjgH7Xmb+0u5QxryvofKkKlAFDHtwY0cyYozEBz+aZ5FRc8yOX/C3oEvNrRALSiACDKAnE69FcQvFNImNkri7AAMAebfFpPog2YAAAAAASUVORK5CYII=" /> FIRMWARE UPGRADE</span></a>
					<a href="about.html"><span class="btn btn-primary"><img class="icon" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABoAAAAaCAYAAACpSkzOAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+5pVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMy1jMDExIDY2LjE0NTY2MSwgMjAxMi8wMi8wNi0xNDo1NjoyNyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvMS4xLyIgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ1dWlkOjY1RTYzOTA2ODZDRjExREJBNkUyRDg4N0NFQUNCNDA3IiB4bXBNTTpEb2N1bWVudElEPSJ4bXAuZGlkOkEzRUY0MTQ5MDZBMDExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOkEzRUY0MTQ4MDZBMDExRTI5OUZEQTZGODg4RDc1ODdCIiB4bXA6Q3JlYXRvclRvb2w9IkFkb2JlIFBob3Rvc2hvcCBDUzYgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDowMTgwMTE3NDA3MjA2ODExODA4M0ZFMkJBM0M1RUU2NSIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDowNjgwMTE3NDA3MjA2ODExODA4M0U3NkRBMDNEMDVDMSIvPiA8ZGM6dGl0bGU+IDxyZGY6QWx0PiA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPmdseXBoaWNvbnM8L3JkZjpsaT4gPC9yZGY6QWx0PiA8L2RjOnRpdGxlPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/PlHOIcUAAAD8SURBVHjavFaBDYQgDAQmcISOgBs4kqP8KG7yjsAIbsC3pn548kiLSpML0dAe0CvFxhiNxKy1gANkvwP6B1EAIiqBA78oIE0tIPAcOI1VIBjYOSpBPoOICM0j1gaSA+TrT4mYZLtAcmDLyfLjWm8gSXc2/COS5sQnu6/m7IeI1SVaaXbUEp9djY5VPhuhYT35dBTY/K2jSp1cRTguBdDkJlOolGy/USZtbpQ5IkxOm5tWc4q5bySLjTzgTB8LjhX3OJFa3g1iCGnBLoJVjehgs15G32PFb0kLFlrkLdwZaC9VXyDx4ku1a5vo1vi6tvKuj5Onnlu21wPyI8AANwDIP2ZbcJQAAAAASUVORK5CYII=" /> ABOUT</span></a>
			</div>			
		</nav>
		
	
		<div id="body" class="container">
			<h1>Sound Settings</h1>


			<div class="panel panel-default">
				<div class="panel-heading">Webradio Control<br /><small>Click to adapt the sound output</small></div>
				<div class="panel-body">

					<div class="row">
						<div class="col-xs-12 col-sm-6 col-md-3">
							<label for="HA">Treble Gain</label><br />
							<select id="toneha" size="16">
								<option value="8">-12 dB</option>
								<option value="9">-10.5 dB</option>
								<option value="10">-9 dB</option>
								<option value="11">-7.5 dB</option>
								<option value="12">-6 dB</option>
								<option value="13">-4.5 dB</option>
								<option value="14">-3 dB</option>
								<option value="15">-1.5 dB</option>
								<option value="0" selected>Off</option>
								<option value="1">+1.5 dB</option>
								<option value="2">+3 dB</option>
								<option value="3">+4.5 dB</option>
								<option value="4">+6 dB</option>
								<option value="5">+7.5 dB</option>
								<option value="6">+9 dB</option>
								<option value="7">+10.5 dB</option>
							</select>
						</div>
						<div class="col-xs-12 col-sm-6 col-md-3">
							<label for="HF">Treble Freq</label><br />
							<select id="tonehf" size="16">
								<option value="1">1 kHz</option>
								<option value="2">2 kHz</option>
								<option value="3">3 kHz</option>
								<option value="4">4 kHz</option>
								<option value="5">5 kHz</option>
								<option value="6">6 kHz</option>
								<option value="7">7 kHz</option>
								<option value="8">8 kHz</option>
								<option value="9">9 kHz</option>
								<option value="10">10 kHz</option>
								<option value="11">11 kHz</option>
								<option value="12">12 kHz</option>
								<option value="13">13 kHz</option>
								<option value="14">14 kHz</option>
								<option value="15">15 kHz</option>
							</select>
						</div>
						<div class="col-xs-12 col-sm-6 col-md-3">
							<label for="LA">Bass Gain</label><br />
							<select id="tonela" size="16">
								<option value="0" selected>Off</option>
								<option value="1">+1 dB</option>
								<option value="2">+2 dB</option>
								<option value="3">+3 dB</option>
								<option value="4">+4 dB</option>
								<option value="5">+5 dB</option>
								<option value="6">+6 dB</option>
								<option value="7">+7 dB</option>
								<option value="8">+8 dB</option>
								<option value="9">+9 dB</option>
								<option value="10">+10 dB</option>
								<option value="11">+11 dB</option>
								<option value="12">+12 dB</option>
								<option value="13">+13 dB</option>
								<option value="14">+14 dB</option>
								<option value="15">+15 dB</option>
							</select>
						</div>
						<div class="col-xs-12 col-sm-6 col-md-3">
							<label for="LF">Bass Freq</label><br />
							<select id="tonelf" size="16">
								<option value="2">10 Hz</option>
								<option value="3">20 Hz</option>
								<option value="4">30 Hz</option>
								<option value="5">40 Hz</option>
								<option value="6">50 Hz</option>
								<option value="7">60 Hz</option>
								<option value="8">70 Hz</option>
								<option value="9">80 Hz</option>
								<option value="10">90 Hz</option>
								<option value="11">100 Hz</option>
								<option value="12">110 Hz</option>
								<option value="13">120 Hz</option>
								<option value="14">130 Hz</option>
								<option value="15">140 Hz</option>
							</select>
						</div>
					</div>


				</div>
			</div>


		</div>	

	
		<footer class="navbar navbar-default navbar-fixed-bottom">
			<input id="command_log" type="text" style="width:100%" placeholder="Waiting for a command...." disabled><br>
		</footer>

	
	
		<script src="interface.js"></script>
	</body>

</html>

)=====";
