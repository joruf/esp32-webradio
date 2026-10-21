package de.loresoft.esp32webradio

import android.content.Context
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.net.wifi.WifiManager
import android.util.Log
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.Inet4Address
import java.net.InetAddress
import java.net.URL
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Discover ESP32 Webradio devices on the current Wi-Fi subnet.
 *
 * Probes each IPv4 host with GET /?getJSON and accepts responses that look like
 * this firmware (firmware / hardware_profile / station fields).
 *
 * Discovery is incremental: [onFound] is invoked as soon as each unique IPv4
 * responds. The full /24 scan always runs to completion unless [cancelled].
 */
object RadioDiscovery {
    private const val TAG = "RadioDiscovery"
    private const val CONNECT_TIMEOUT_MS = 500
    private const val READ_TIMEOUT_MS = 700
    private const val MAX_PARALLEL = 32
    private const val SCAN_TIMEOUT_SEC = 45L

    private val IPV4_REGEX = Regex("""^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$""")

    /**
     * Scan known hostnames and the local /24. Invokes [onFound] for each new
     * radio (deduped by IPv4) while the scan continues in parallel.
     *
     * @param context App context for Wi-Fi info
     * @param cancelled Optional flag to abort early (e.g. after user selection)
     * @param onFound Called from a worker thread for each newly discovered radio
     * @return All unique radios found (may be empty if cancelled early)
     */
    fun scan(
        context: Context,
        cancelled: AtomicBoolean = AtomicBoolean(false),
        onFound: ((DiscoveredRadio) -> Unit)? = null
    ): List<DiscoveredRadio> {
        val found = ConcurrentHashMap<String, DiscoveredRadio>()

        fun tryAdd(radio: DiscoveredRadio?) {
            if (radio == null || cancelled.get()) {
                return
            }
            val key = radio.ip
            if (key.isBlank() || !isIpv4(key)) {
                Log.d(TAG, "Skipping radio without IPv4: host=${radio.host}")
                return
            }
            val previous = found.putIfAbsent(key, radio)
            if (previous == null) {
                Log.i(TAG, "Found Webradio at $key (${radio.station})")
                try {
                    onFound?.invoke(radio)
                } catch (e: Exception) {
                    Log.w(TAG, "onFound callback failed for $key", e)
                }
            } else if (previous.firmware.isBlank() && radio.firmware.isNotBlank()) {
                found[key] = radio
            }
        }

        // Known hostnames may resolve to only one device (e.g. radio.local).
        // Still probe them for a quick first hit, then always scan the full /24.
        for (host in knownHostCandidates()) {
            if (cancelled.get()) {
                break
            }
            tryAdd(probeHost(host))
        }

        val prefix = ipv4SubnetPrefix(context)
        if (prefix != null && !cancelled.get()) {
            Log.i(TAG, "Scanning subnet $prefix.0/24")
            val pool = Executors.newFixedThreadPool(MAX_PARALLEL)
            for (last in 1..254) {
                if (cancelled.get()) {
                    break
                }
                val host = "$prefix.$last"
                pool.execute {
                    if (!cancelled.get()) {
                        tryAdd(probeHost(host))
                    }
                }
            }
            pool.shutdown()
            try {
                // Wait for the full scan; do not stop after the first hit.
                val finished = pool.awaitTermination(SCAN_TIMEOUT_SEC, TimeUnit.SECONDS)
                if (!finished) {
                    Log.w(TAG, "Subnet scan timed out after ${SCAN_TIMEOUT_SEC}s; shutting down pool")
                }
            } catch (_: InterruptedException) {
                Thread.currentThread().interrupt()
            }
            if (!pool.isTerminated) {
                pool.shutdownNow()
            }
        } else if (prefix == null) {
            Log.w(TAG, "No Wi-Fi IPv4 address; hostname probes only")
        }

        return found.values.sortedWith(
            compareBy({ it.station.ifBlank { it.ip }.lowercase() }, { ipv4SortKey(it.ip) })
        )
    }

    private fun knownHostCandidates(): List<String> {
        return listOf(
            "radio.local",
            "radio.fritz.box",
            "ESP32Webradio.local",
            "esp32webradio.local"
        )
    }

    private fun ipv4SubnetPrefix(context: Context): String? {
        val cm = context.applicationContext.getSystemService(Context.CONNECTIVITY_SERVICE) as? ConnectivityManager
        val network = cm?.activeNetwork
        val link = cm?.getLinkProperties(network)
        val fromLink = link?.linkAddresses
            ?.mapNotNull { it.address as? Inet4Address }
            ?.firstOrNull { !it.isLoopbackAddress }
            ?.hostAddress
        if (fromLink != null) {
            val parts = fromLink.split('.')
            if (parts.size == 4) {
                return "${parts[0]}.${parts[1]}.${parts[2]}"
            }
        }

        val caps = cm?.getNetworkCapabilities(network)
        if (caps != null && !caps.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) &&
            !caps.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET)
        ) {
            Log.i(TAG, "Active network is not Wi-Fi/Ethernet; trying WifiManager")
        }

        val wifi = context.applicationContext.getSystemService(Context.WIFI_SERVICE) as? WifiManager
            ?: return null
        @Suppress("DEPRECATION")
        val ipInt = wifi.connectionInfo?.ipAddress ?: 0
        if (ipInt == 0) {
            return null
        }
        val a = ipInt and 0xff
        val b = ipInt shr 8 and 0xff
        val c = ipInt shr 16 and 0xff
        return "$a.$b.$c"
    }

    private fun probeHost(host: String): DiscoveredRadio? {
        var connection: HttpURLConnection? = null
        try {
            val url = URL("http://$host/?getJSON")
            connection = (url.openConnection() as HttpURLConnection).apply {
                connectTimeout = CONNECT_TIMEOUT_MS
                readTimeout = READ_TIMEOUT_MS
                requestMethod = "GET"
                instanceFollowRedirects = false
                useCaches = false
                setRequestProperty("Connection", "close")
                setRequestProperty("Accept", "application/json")
            }
            val code = connection.responseCode
            if (code !in 200..299) {
                return null
            }
            val body = connection.inputStream.bufferedReader().use { it.readText() }
            return parseWebradioJson(host, body)
        } catch (_: Exception) {
            return null
        } finally {
            connection?.disconnect()
        }
    }

    private fun parseWebradioJson(requestedHost: String, body: String): DiscoveredRadio? {
        val json = try {
            JSONObject(body)
        } catch (_: Exception) {
            return null
        }

        val firmware = json.optString("firmware", "").trim()
        val station = json.optString("station", "").trim()
        val profile = json.optString("hardware_profile", "").trim()
        val reportedIp = json.optString("ip", "").trim()

        val looksLikeWebradio =
            firmware.isNotBlank() ||
                profile.isNotBlank() ||
                json.has("stream_kbps") ||
                json.has("playback_state") ||
                json.has("hardware_profiles")

        if (!looksLikeWebradio) {
            return null
        }

        // Prefer getJSON "ip", then numeric probe host, then DNS resolve of hostname.
        val ip = when {
            isIpv4(reportedIp) -> reportedIp
            isIpv4(requestedHost) -> requestedHost
            else -> resolveIpv4(requestedHost)
        } ?: return null

        val baseUrl = RadioPrefs.normalizeBaseUrl(ip)
        return DiscoveredRadio(
            host = ip,
            ip = ip,
            baseUrl = baseUrl,
            firmware = firmware,
            station = station,
            hardwareProfile = profile
        )
    }

    fun isIpv4(value: String): Boolean {
        val match = IPV4_REGEX.matchEntire(value.trim()) ?: return false
        return match.groupValues.drop(1).all { part ->
            val n = part.toIntOrNull() ?: return false
            n in 0..255
        }
    }

    private fun ipv4SortKey(ip: String): Long {
        val parts = ip.split('.')
        if (parts.size != 4) {
            return Long.MAX_VALUE
        }
        var key = 0L
        for (part in parts) {
            key = (key shl 8) + (part.toIntOrNull() ?: 0)
        }
        return key
    }

    /**
     * Resolve a hostname to an IPv4 string when possible (best-effort).
     */
    fun resolveIpv4(host: String): String? {
        if (isIpv4(host)) {
            return host
        }
        return try {
            InetAddress.getAllByName(host)
                .filterIsInstance<Inet4Address>()
                .firstOrNull()
                ?.hostAddress
        } catch (_: Exception) {
            null
        }
    }
}
