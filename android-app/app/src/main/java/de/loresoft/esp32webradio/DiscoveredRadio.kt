package de.loresoft.esp32webradio

/**
 * One ESP32 Webradio found on the local network.
 *
 * @param host Reachable host used for the base URL (prefer IPv4)
 * @param ip Canonical IPv4 address for display and deduplication
 * @param baseUrl Normalized http base URL
 * @param firmware Firmware label from getJSON, if any
 * @param station Station name from getJSON, if any
 * @param hardwareProfile Active hardware profile id, if any
 */
data class DiscoveredRadio(
    val host: String,
    val ip: String,
    val baseUrl: String,
    val firmware: String = "",
    val station: String = "",
    val hardwareProfile: String = ""
) {
    /** Short line for the selection list (station + firmware). */
    fun listTitle(): String {
        val name = station.ifBlank { "Webradio" }
        return if (firmware.isNotBlank()) {
            "$name  ·  $firmware"
        } else {
            name
        }
    }

    /** Secondary line: IPv4 must be visible; optional hardware profile. */
    fun listSubtitle(): String {
        return if (hardwareProfile.isNotBlank()) {
            "$ip  ·  $hardwareProfile"
        } else {
            ip
        }
    }
}
