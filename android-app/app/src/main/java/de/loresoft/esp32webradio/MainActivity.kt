package de.loresoft.esp32webradio

import android.annotation.SuppressLint
import android.content.Intent
import android.graphics.Bitmap
import android.os.Bundle
import android.view.Menu
import android.view.MenuItem
import android.view.View
import android.webkit.WebChromeClient
import android.webkit.WebResourceError
import android.webkit.WebResourceRequest
import android.webkit.WebSettings
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.swiperefreshlayout.widget.SwipeRefreshLayout
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.progressindicator.LinearProgressIndicator
import de.loresoft.esp32webradio.databinding.ActivityMainBinding

/**
 * Full-screen WebView for the ESP32 Webradio web interface.
 * Uses the same HTTP API and pages as the browser — no firmware changes required.
 */
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    private val settingsLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) {
        if (RadioPrefs.getBaseUrl(this) != null) {
            loadHomePage()
        } else {
            finish()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupToolbar(binding.toolbar)
        setupWebView(binding.webView)
        setupSwipeRefresh(binding.swipeRefresh, binding.webView)

        if (RadioPrefs.getBaseUrl(this) == null) {
            openSettings()
            return
        }

        loadHomePage()
    }

    private fun setupToolbar(toolbar: MaterialToolbar) {
        setSupportActionBar(toolbar)
        supportActionBar?.title = getString(R.string.app_name)
    }

    @SuppressLint("SetJavaScriptEnabled")
    private fun setupWebView(webView: WebView) {
        WebView.setWebContentsDebuggingEnabled(true)

        webView.settings.apply {
            javaScriptEnabled = true
            domStorageEnabled = true
            databaseEnabled = true
            setSupportZoom(true)
            builtInZoomControls = true
            displayZoomControls = false
            loadWithOverviewMode = true
            useWideViewPort = true
            mixedContentMode = WebSettings.MIXED_CONTENT_ALWAYS_ALLOW
            loadsImagesAutomatically = true
        }

        webView.webViewClient = object : WebViewClient() {
            override fun shouldOverrideUrlLoading(view: WebView, request: WebResourceRequest): Boolean {
                val base = RadioPrefs.getBaseUrl(this@MainActivity) ?: return false
                val url = request.url.toString()
                if (url.startsWith(base)) {
                    return false
                }
                return false
            }

            override fun onPageStarted(view: WebView?, url: String?, favicon: Bitmap?) {
                binding.progressBar.visibility = View.VISIBLE
                binding.progressBar.setProgressCompat(0, false)
            }

            override fun onPageFinished(view: WebView?, url: String?) {
                binding.progressBar.visibility = View.GONE
                binding.swipeRefresh.isRefreshing = false
            }

            override fun onReceivedError(
                view: WebView,
                request: WebResourceRequest,
                error: WebResourceError
            ) {
                if (request.isForMainFrame) {
                    binding.swipeRefresh.isRefreshing = false
                    binding.progressBar.visibility = View.GONE
                    val url = request.url?.toString().orEmpty()
                    Toast.makeText(
                        this@MainActivity,
                        getString(R.string.error_load_failed_detail, url, error.errorCode),
                        Toast.LENGTH_LONG
                    ).show()
                }
            }
        }

        webView.webChromeClient = object : WebChromeClient() {
            override fun onProgressChanged(view: WebView?, newProgress: Int) {
                binding.progressBar.setProgressCompat(newProgress, true)
                if (newProgress >= 100) {
                    binding.progressBar.visibility = View.GONE
                }
            }
        }
    }

    private fun setupSwipeRefresh(swipeRefresh: SwipeRefreshLayout, webView: WebView) {
        swipeRefresh.setOnRefreshListener {
            webView.reload()
        }
    }

    private fun loadHomePage() {
        val url = RadioPrefs.pageUrl(this, "index.html")
        if (url == null) {
            openSettings()
            return
        }
        binding.webView.loadUrl(url)
    }

    private fun ensurePageLoaded() {
        if (RadioPrefs.getBaseUrl(this) == null) {
            return
        }
        val current = binding.webView.url
        if (current.isNullOrBlank() || current == "about:blank") {
            loadHomePage()
        }
    }

    private fun openSettings() {
        settingsLauncher.launch(Intent(this, SettingsActivity::class.java))
    }

    override fun onResume() {
        super.onResume()
        binding.webView.onResume()
        ensurePageLoaded()
    }

    override fun onPause() {
        binding.webView.onPause()
        super.onPause()
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.menu_main, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.action_reload -> {
                loadHomePage()
                true
            }
            R.id.action_settings -> {
                openSettings()
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }

    @Deprecated("Deprecated in Java")
    override fun onBackPressed() {
        if (binding.webView.canGoBack()) {
            binding.webView.goBack()
        } else {
            @Suppress("DEPRECATION")
            super.onBackPressed()
        }
    }
}
