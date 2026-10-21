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
 */
object RadioDiscovery {
    private const val TAG = "RadioDiscovery"
    private const val CONNECT_TIMEOUT_MS = 350
    private const val READ_TIMEOUT_MS = 500
    private const val MAX_PARALLEL = 32
    private const val SCAN_TIMEOUT_SEC = 25L

    /**
     * Scan the local /24 (or known hostnames) and return unique radios.
     *
     * @param context App context for Wi-Fi info
     * @param cancelled Optional flag to abort early
     */
    fun scan(context: Context, cancelled: AtomicBoolean = AtomicBoolean(false)): List<DiscoveredRadio> {
        val found = ConcurrentHashMap<String, DiscoveredRadio>()

        fun add(radio: DiscoveredRadio?) {
            if (radio == null) {
                return
            }
            val key = radio.host.lowercase()
            found.merge(key, radio) { existing, incoming ->
                if (existing.firmware.isBlank() && incoming.firmware.isNotBlank()) {
                    incoming
                } else {
                    existing
                }
            }
        }

        for (host in knownHostCandidates()) {
            if (cancelled.get()) {
                break
            }
            add(probeHost(host))
        }

        val prefix = ipv4SubnetPrefix(context)
        if (prefix != null && !cancelled.get()) {
            val pool = Executors.newFixedThreadPool(MAX_PARALLEL)
            for (last in 1..254) {
                val host = "$prefix.$last"
                pool.execute {
                    if (!cancelled.get()) {
                        add(probeHost(host))
                    }
                }
            }
            pool.shutdown()
            try {
                pool.awaitTermination(SCAN_TIMEOUT_SEC, TimeUnit.SECONDS)
            } catch (_: InterruptedException) {
                Thread.currentThread().interrupt()
            }
            if (!pool.isTerminated) {
                pool.shutdownNow()
            }
        } else {
            Log.w(TAG, "No Wi-Fi IPv4 address; hostname probes only")
        }

        return found.values.sortedWith(
            compareBy({ it.station.ifBlank { it.host }.lowercase() }, { it.host })
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

        val host = when {
            reportedIp.matches(Regex("""\d{1,3}(\.\d{1,3}){3}""")) -> reportedIp
            else -> requestedHost
        }
        val baseUrl = RadioPrefs.normalizeBaseUrl(host)
        return DiscoveredRadio(
            host = RadioPrefs.displayHost(baseUrl),
            baseUrl = baseUrl,
            firmware = firmware,
            station = station,
            hardwareProfile = profile
        )
    }

    /**
     * Resolve a hostname to an IPv4 string when possible (best-effort).
     */
    fun resolveIpv4(host: String): String? {
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
