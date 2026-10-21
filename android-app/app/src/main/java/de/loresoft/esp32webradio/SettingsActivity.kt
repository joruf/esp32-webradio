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
 */
class SettingsActivity : AppCompatActivity() {

    private lateinit var binding: ActivitySettingsBinding
    private val discoverExecutor = Executors.newSingleThreadExecutor()
    private var discoverFuture: Future<*>? = null
    private val discoverCancelled = AtomicBoolean(false)

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
        discoverFuture?.cancel(true)
        discoverExecutor.shutdownNow()
        super.onDestroy()
    }

    private fun startDiscovery() {
        if (discoverFuture?.isDone == false) {
            return
        }

        discoverCancelled.set(false)
        binding.discoverButton.isEnabled = false
        binding.discoverProgress.visibility = View.VISIBLE
        binding.discoverStatus.text = getString(R.string.settings_discover_scanning)
        binding.discoveredList.removeAllViews()

        val appContext = applicationContext
        discoverFuture = discoverExecutor.submit {
            val radios = try {
                RadioDiscovery.scan(appContext, discoverCancelled)
            } catch (_: Exception) {
                emptyList()
            }
            runOnUiThread {
                if (isFinishing || isDestroyed) {
                    return@runOnUiThread
                }
                binding.discoverProgress.visibility = View.GONE
                binding.discoverButton.isEnabled = true
                showDiscoveredRadios(radios)
            }
        }
    }

    private fun showDiscoveredRadios(radios: List<DiscoveredRadio>) {
        binding.discoveredList.removeAllViews()

        if (radios.isEmpty()) {
            binding.discoverStatus.text = getString(R.string.settings_discover_none)
            return
        }

        binding.discoverStatus.text = resources.getQuantityString(
            R.plurals.settings_discover_found,
            radios.size,
            radios.size
        )

        val inflater = LayoutInflater.from(this)
        for (radio in radios) {
            val item = ItemDiscoveredRadioBinding.inflate(inflater, binding.discoveredList, false)
            item.radioTitle.text = radio.listTitle()
            item.radioSubtitle.text = radio.listSubtitle()
            item.root.setOnClickListener {
                selectRadio(radio)
            }
            binding.discoveredList.addView(item.root)
        }
    }

    private fun selectRadio(radio: DiscoveredRadio) {
        binding.hostInput.setText(radio.host)
        RadioPrefs.setBaseUrl(this, radio.baseUrl)
        Toast.makeText(
            this,
            getString(R.string.settings_discover_selected, radio.host),
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

        RadioPrefs.setBaseUrl(this, raw)
        setResult(RESULT_OK)
        finish()
    }

    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }
}
