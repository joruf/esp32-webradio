// MQTT: Message Queuing Telemetry Transport
// offenes Nachrichtenprotokoll für Machine-to-Machine-Kommunikation


//**************************************************************************************************
//                                    M Q T T R E C O N N E C T                                    *
//**************************************************************************************************
// Reconnect to broker.                                                                            *
//**************************************************************************************************
bool mqttreconnect()
{
  static uint32_t retrytime = 0 ;                         // Limit reconnect interval
  bool            res = false ;                           // Connect result
  char            clientid[20] ;                          // Client ID
  char            subtopic[60] ;                          // Topic to subscribe

  if ( ( millis() - retrytime ) < 5000 )                  // Don't try to frequently
  {
    return res ;
  }
  retrytime = millis() ;                                  // Set time of last try
  if ( mqttcount > MAXMQTTCONNECTS )                      // Tried too much?
  {
    mqtt_on = false ;                                     // switch off forever
    return res ;                                          // and quit
  }
  mqttcount++ ;                                           // Count the retries
  dbgprint ( "(Re)connecting number %d to MQTT %s", mqttcount, ini_block.mqttbroker.c_str() ); // Show some debug info
  sprintf ( clientid, "%s-%04d", NAME, (int) random ( 10000 ) % 10000 ); // Generate client ID
  res = mqttclient.connect ( clientid, ini_block.mqttuser.c_str(), ini_block.mqttpasswd.c_str()); // Connect to broker
  if (res) {
    sprintf ( subtopic, "%s/%s", ini_block.mqttprefix.c_str(), MQTT_SUBTOPIC ); // Add prefix to subtopic
    res = mqttclient.subscribe ( subtopic ) ;             // Subscribe to MQTT
    if (!res) {
      dbgprint ( "MQTT subscribe failed!" ) ;             // Failure
    }
    mqttpub.trigger ( MQTT_IP ) ;                         // Publish own IP
  }
  else {
    dbgprint ( "MQTT connection failed, rc=%d", mqttclient.state() ) ;
  }
  return res ;
}


//**************************************************************************************************
//                                    O N M Q T T M E S S A G E                                    *
//**************************************************************************************************
// Executed when a subscribed message is received.                                                 *
// Note that message is not delimited by a '\0'.                                                   *
// Note that cmd buffer is shared with serial input.                                               *
//**************************************************************************************************
void onMqttMessage ( char* topic, byte* payload, unsigned int len )
{
  const char*  reply ;                                // Result from analyzeCmd

  if (strstr(topic, MQTT_SUBTOPIC) ) { // Check on topic, maybe unnecessary
    if (len >= sizeof(cmd)) { // Message may not be too long
      len = sizeof(cmd) - 1 ;
    }
    strncpy ( cmd, (char*)payload, len ) ;            // Make copy of message
    cmd[len] = '\0' ;                                 // Take care of delimiter
    dbgprint("MQTT message arrived [%s], lenght = %d, %s", topic, len, cmd) ;
    reply = analyzeCmd ( cmd ) ;                      // Analyze command and handle it
    dbgprint ( reply ) ;                              // Result for debugging
  }
}
