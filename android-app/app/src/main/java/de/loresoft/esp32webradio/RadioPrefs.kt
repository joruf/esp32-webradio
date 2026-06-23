package de.loresoft.esp32webradio

import android.content.Context
import android.content.SharedPreferences
import androidx.core.content.edit

/**
 * Stores the base URL of the ESP32 web interface.
 */
object RadioPrefs {
    private const val PREFS_NAME = "esp32_webradio"
    private const val KEY_BASE_URL = "base_url"

    private fun prefs(context: Context): SharedPreferences {
        return context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    }

    /**
     * @return Normalized base URL including scheme, without trailing slash, or null if unset
     */
    fun getBaseUrl(context: Context): String? {
        val stored = prefs(context).getString(KEY_BASE_URL, null)?.trim().orEmpty()
        if (stored.isEmpty()) {
            return null
        }
        return normalizeBaseUrl(stored)
    }

    /**
     * @param hostOrUrl IP, hostname, or full http URL
     */
    fun setBaseUrl(context: Context, hostOrUrl: String) {
        val normalized = normalizeBaseUrl(hostOrUrl.trim())
        prefs(context).edit {
            putString(KEY_BASE_URL, normalized)
        }
    }

    /**
     * Build a full page URL from a stored base URL and relative path.
     *
     * @param path e.g. "index.html" or "/scheduler.html"
     */
    fun pageUrl(context: Context, path: String): String? {
        val base = getBaseUrl(context) ?: return null
        val cleanPath = path.trim().removePrefix("/")
        return "$base/$cleanPath"
    }

    /**
     * Normalize user input to http://host form (no trailing slash).
     */
    fun normalizeBaseUrl(input: String): String {
        var value = input.trim()
        if (value.isEmpty()) {
            return value
        }

        if (!value.startsWith("http://", ignoreCase = true) &&
            !value.startsWith("https://", ignoreCase = true)
        ) {
            value = "http://$value"
        }

        while (value.endsWith("/")) {
            value = value.dropLast(1)
        }

        return value
    }

    /**
     * Extract host part for display in settings (without scheme).
     */
    fun displayHost(baseUrl: String?): String {
        if (baseUrl.isNullOrBlank()) {
            return ""
        }
        return baseUrl
            .removePrefix("http://")
            .removePrefix("https://")
            .trimEnd('/')
    }
}
