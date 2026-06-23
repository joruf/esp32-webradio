package de.loresoft.esp32webradio

import android.os.Bundle
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import de.loresoft.esp32webradio.databinding.ActivitySettingsBinding

/**
 * Configure the ESP32 host (IP or hostname) for the embedded web UI.
 */
class SettingsActivity : AppCompatActivity() {

    private lateinit var binding: ActivitySettingsBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivitySettingsBinding.inflate(layoutInflater)
        setContentView(binding.root)

        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        title = getString(R.string.settings_title)

        binding.hostInput.setText(RadioPrefs.displayHost(RadioPrefs.getBaseUrl(this)))
        binding.saveButton.setOnClickListener {
            saveAndOpen()
        }
    }

    private fun saveAndOpen() {
        val raw = binding.hostInput.text?.toString()?.trim().orEmpty()
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
