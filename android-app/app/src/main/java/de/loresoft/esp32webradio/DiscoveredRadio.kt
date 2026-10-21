package de.loresoft.esp32webradio

/**
 * One ESP32 Webradio found on the local network.
 *
 * @param host Reachable host or IP (no scheme)
 * @param baseUrl Normalized http base URL
 * @param firmware Firmware label from getJSON, if any
 * @param station Station name from getJSON, if any
 * @param hardwareProfile Active hardware profile id, if any
 */
data class DiscoveredRadio(
    val host: String,
    val baseUrl: String,
    val firmware: String = "",
    val station: String = "",
    val hardwareProfile: String = ""
) {
    /** Short line for the selection list. */
    fun listTitle(): String {
        val name = station.ifBlank { host }
        return if (firmware.isNotBlank()) {
            "$name  ·  $firmware"
        } else {
            name
        }
    }

    /** Secondary line with host and optional profile. */
    fun listSubtitle(): String {
        return if (hardwareProfile.isNotBlank()) {
            "$host  ·  $hardwareProfile"
        } else {
            host
        }
    }
}
