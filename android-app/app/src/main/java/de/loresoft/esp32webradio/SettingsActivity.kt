package de.loresoft.esp32webradio

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import de.loresoft.esp32webradio.databinding.ActivitySettingsBinding
import de.loresoft.esp32webradio.databinding.ItemDiscoveredRadioBinding
import java.util.concurrent.Executors
import java.util.concurrent.Future
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Configure the ESP32 host: discover radios on Wi-Fi or enter an address manually.
 *
 * Discovery is non-blocking: each radio appears in the list as soon as it is found.
 * Tapping an item applies the selection immediately and finishes Settings even while
 * the background scan continues (further UI updates are cancelled).
 */
class SettingsActivity : AppCompatActivity() {

    private lateinit var binding: ActivitySettingsBinding
    private val discoverExecutor = Executors.newSingleThreadExecutor()
    private var discoverFuture: Future<*>? = null
    private val discoverCancelled = AtomicBoolean(false)
    /** When true, ignore further onFound UI updates (user already selected). */
    private val uiUpdatesCancelled = AtomicBoolean(false)
    private val discoveredByIp = linkedMapOf<String, DiscoveredRadio>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivitySettingsBinding.inflate(layoutInflater)
        setContentView(binding.root)

        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        title = getString(R.string.settings_title)

        binding.hostInput.setText(RadioPrefs.displayHost(RadioPrefs.getBaseUrl(this)))
        binding.saveButton.setOnClickListener {
            saveAndOpen(binding.hostInput.text?.toString().orEmpty())
        }
        binding.discoverButton.setOnClickListener {
            startDiscovery()
        }

        startDiscovery()
    }

    override fun onDestroy() {
        discoverCancelled.set(true)
        uiUpdatesCancelled.set(true)
        discoverFuture?.cancel(true)
        discoverExecutor.shutdownNow()
        super.onDestroy()
    }

    private fun startDiscovery() {
        if (discoverFuture?.isDone == false) {
            return
        }

        discoverCancelled.set(false)
        uiUpdatesCancelled.set(false)
        discoveredByIp.clear()
        binding.discoverButton.isEnabled = false
        binding.discoverProgress.visibility = View.VISIBLE
        binding.discoverStatus.text = getString(R.string.settings_discover_scanning)
        binding.discoveredList.removeAllViews()

        val appContext = applicationContext
        discoverFuture = discoverExecutor.submit {
            try {
                RadioDiscovery.scan(appContext, discoverCancelled) { radio ->
                    onRadioFound(radio)
                }
            } catch (_: Exception) {
                // Scan failures are reflected as an empty/partial list below.
            }
            runOnUiThread {
                if (isFinishing || isDestroyed || uiUpdatesCancelled.get()) {
                    return@runOnUiThread
                }
                binding.discoverProgress.visibility = View.GONE
                binding.discoverButton.isEnabled = true
                if (discoveredByIp.isEmpty()) {
                    binding.discoverStatus.text = getString(R.string.settings_discover_none)
                } else {
                    updateFoundStatus(discoveredByIp.size)
                }
            }
        }
    }

    private fun onRadioFound(radio: DiscoveredRadio) {
        runOnUiThread {
            if (isFinishing || isDestroyed || uiUpdatesCancelled.get()) {
                return@runOnUiThread
            }
            if (discoveredByIp.containsKey(radio.ip)) {
                return@runOnUiThread
            }
            discoveredByIp[radio.ip] = radio
            appendRadioItem(radio)
            updateFoundStatus(discoveredByIp.size)
        }
    }

    private fun updateFoundStatus(count: Int) {
        binding.discoverStatus.text = resources.getQuantityString(
            R.plurals.settings_discover_found,
            count,
            count
        )
    }

    private fun appendRadioItem(radio: DiscoveredRadio) {
        val inflater = LayoutInflater.from(this)
        val item = ItemDiscoveredRadioBinding.inflate(inflater, binding.discoveredList, false)
        item.radioTitle.text = radio.listTitle()
        item.radioSubtitle.text = radio.listSubtitle()
        item.root.setOnClickListener {
            selectRadio(radio)
        }
        binding.discoveredList.addView(item.root)
    }

    private fun selectRadio(radio: DiscoveredRadio) {
        // Stop further UI updates and cancel the background scan; apply selection now.
        uiUpdatesCancelled.set(true)
        discoverCancelled.set(true)
        discoverFuture?.cancel(true)

        binding.hostInput.setText(radio.ip)
        RadioPrefs.setBaseUrl(this, radio.baseUrl)
        Toast.makeText(
            this,
            getString(R.string.settings_discover_selected, radio.ip),
            Toast.LENGTH_SHORT
        ).show()
        setResult(RESULT_OK)
        finish()
    }

    private fun saveAndOpen(rawInput: String) {
        val raw = rawInput.trim()
        if (raw.isEmpty()) {
            Toast.makeText(this, R.string.error_no_host, Toast.LENGTH_SHORT).show()
            return
        }

        uiUpdatesCancelled.set(true)
        discoverCancelled.set(true)
        discoverFuture?.cancel(true)

        RadioPrefs.setBaseUrl(this, raw)
        setResult(RESULT_OK)
        finish()
    }

    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }
}
